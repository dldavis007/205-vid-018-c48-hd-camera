/* main.c - PC-side host entry point for the VID-018 HD camera (Rev 1.01, CM4).
 *
 * Brings up the host services, then calls the firmware's own main(), which is
 * compiled as fw_main() (-Dmain=fw_main for that one TU). Everything the
 * firmware does from there on is production code.
 *
 * Usage:  pc_side_host.exe [recv_port] [send_port]   (prompts if omitted)
 *         Defaults 20018 / 20100: bind :20018, send to the can_udp_hub on :20100.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "Subroutines.h"
#include "MenuFunctions.h"
#include "pc_side.h"
#include "pc_log.h"

int fw_main(void);          /* CM4/Core/Src/main.c, built with -Dmain=fw_main */

/* Firmware globals no header declares. */
extern char     Gen_Flags;
extern uint8_t  StackPointer;
extern uint16_t cam_add;
extern bool     camActive;
extern bool     CAN_Operating;

#define WWDG_TIMEOUT_MS 1020    /* hwwdg2: 1.02 s */

static volatile int           g_running   = 1;
static volatile int           g_init_done = 0;
static unsigned short         g_recv_port, g_send_port;

static BOOL WINAPI on_ctrl(DWORD type)
{
    (void)type;
    g_running = 0;
    return TRUE;
}

static const char *menu_state(void)
{
    if (!g_init_done)                      return "init";
    if (Gen_Flags & Gen_Flags_Menu_Active) return "menu";
    return "idle";
}

/* Heartbeat via the console TITLE (kernel32, not stdout), so it keeps updating
 * when the terminal blocks our output: 'loop=' climbing with a frozen log
 * means the terminal is stuck; 'loop=' frozen too means the loop is hung. */
static DWORD WINAPI watchdog_fn(void *arg)
{
    unsigned long last = 0;
    int           stalled_secs = 0;
    unsigned int  last_ticks = 0, last_skips = 0;
    unsigned      secs = 0;
    DWORD         last_ms = GetTickCount();
    char          title[240];

    (void)arg;
    while (g_running) {
        unsigned long loops;
        unsigned int  ticks, skips, gap;
        DWORD         now;
        double        real_hz, rate;

        Sleep(1000);

        loops        = pc_loop_count();
        stalled_secs = (g_init_done && loops == last && !pc_is_resetting())
                     ? stalled_secs + 1 : 0;

        ticks   = pc_rti_ticks();
        skips   = pc_rti_skipped();
        gap     = pc_loop_max_gap_ms();
        now     = GetTickCount();
        real_hz = (now > last_ms)
                ? (double)(ticks - last_ticks) * 1000.0 / (double)(now - last_ms)
                : 0.0;
        rate    = real_hz / 1000.0;

        snprintf(title, sizeof title,
                 "VID-018 HD camera host Rev %s | :%u->:%u | loop=%lu | %s | "
                 "RTI %.0fHz (%.2fx) skip=%u | gap %ums%s",
                 Revision, (unsigned)g_recv_port, (unsigned)g_send_port,
                 loops, menu_state(), real_hz, rate, skips - last_skips, gap,
                 stalled_secs ? " | *** STALLED ***" : "");
        SetConsoleTitleA(title);

        if (stalled_secs == 1)
            LOG_PRINTF(("[STALL] the main loop has not completed a pass for 1s (%s)\n",
                        menu_state()));

        /* Periodic heartbeat, so a redirected log shows the loop and RTI rate
         * even when nothing else changes. */
        if (++secs % 10 == 0)
            LOG_PRINTF(("[host] loop=%lu (%.0f/s) | %s | RTI %.0fHz (%.2fx) skip=%u\n",
                        loops, (double)(loops - last), menu_state(),
                        real_hz, rate, skips - last_skips));

        /* On the target a pass this long would have tripped the window watchdog. */
        if (gap > WWDG_TIMEOUT_MS)
            LOG_PRINTF(("[wdg ] loop pass took %u ms: the target's WWDG (%u ms) "
                        "would have reset the unit\n", gap, WWDG_TIMEOUT_MS));
        pc_loop_reset_max_gap();

        last       = loops;
        last_ticks = ticks;
        last_skips = skips;
        last_ms    = now;
    }
    return 0;
}

void pc_log_changes(void)
{
    LOG_IF_CHANGED("[menu] Gen_Flags = 0x%02lX", (unsigned char)Gen_Flags);
    LOG_IF_CHANGED("[menu] StackPointer = %ld", StackPointer);
    LOG_IF_CHANGED("[cam ] cam_add = 0x%04lX", cam_add);
    LOG_IF_CHANGED("[cam ] camera active = %ld", camActive);
    LOG_IF_CHANGED("[can ] CAN_Operating = %ld", CAN_Operating);
}

static DWORD WINAPI firmware_fn(void *arg)
{
    (void)arg;
    g_init_done = 1;
    fw_main();                  /* never returns: while(1) { doevents(); COP_Trig(); } */
    return 0;
}

static unsigned short ask_port(const char *label, unsigned short def)
{
    char line[32];
    printf("%s port [%u]: ", label, (unsigned)def);
    if (fgets(line, sizeof line, stdin)) {
        int v = atoi(line);                /* blank -> 0 -> keep default */
        if (v > 0 && v < 65536) return (unsigned short)v;
    }
    return def;
}

/* QuickEdit: one click in the console pauses stdout and freezes every logger. */
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif
static void disable_console_quickedit(void)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD  mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        mode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(h, mode);
    }
}

int main(int argc, char **argv)
{
    HANDLE fw_thread;

    SetConsoleCtrlHandler(on_ctrl, TRUE);
    setvbuf(stdout, NULL, _IONBF, 0);
    disable_console_quickedit();

    printf("VID-018 HD camera Rev %s - PC-side host  (Ctrl+C to quit)\n", Revision);

    if (argc > 2) {
        g_recv_port = (unsigned short)atoi(argv[1]);
        g_send_port = (unsigned short)atoi(argv[2]);
    } else {
        g_recv_port = ask_port("recv (this host listens on)", 20018);
        g_send_port = ask_port("send (hub/peer listens on)", 20100);
    }

    pc_log_init();                   /* after the prompts: logging is async from here */

    if (pc_menu_data_check() != 0) {
        pc_log_shutdown();
        return 1;
    }

    if (pc_flash_init(g_recv_port) != 0) {
        LOG_PRINTF(("[fatal] could not allocate the fake flash below 4 GB\n"));
        pc_log_shutdown();
        return 1;
    }

    if (pc_can_init(g_recv_port, g_send_port) != 0) {
        LOG_PRINTF(("[fatal] CAN/UDP init failed\n"));
        pc_log_shutdown();
        return 1;
    }

    rti_thread_start();
    CreateThread(NULL, 0, watchdog_fn, NULL, 0, NULL);

    LOG_PRINTF(("[host] bringing up the unit...\n"));
    fw_thread = CreateThread(NULL, 0, firmware_fn, NULL, 0, NULL);

    while (g_running) Sleep(50);

    LOG_PRINTF(("\n[host] shutting down\n"));
    rti_thread_stop();
    pc_flash_sync();
    pc_can_shutdown();
    pc_log_shutdown();
    if (fw_thread) CloseHandle(fw_thread);
    return 0;
}
