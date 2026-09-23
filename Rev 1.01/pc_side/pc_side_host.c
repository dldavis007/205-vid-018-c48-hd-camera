/* pc_side_host.c - PC-only support layer for the VID-018 HD camera host build:
 * a UDP CAN bus in place of FDCAN1, async logging, the RTI simulation thread
 * that raises SysTick, loop instrumentation and reset.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "pc_side.h"
#include "pc_log.h"

/* Core/Src/stm32h7xx_it.c: the real interrupt handlers. */
void SysTick_Handler(void);
void TIM1_UP_IRQHandler(void);

/* ==========================================================================
 * Async, drop-on-full logging. Producers only format and enqueue; one writer
 * thread owns stdout, so a stalled terminal can never block the firmware.
 * ========================================================================== */
#define LOGQ_N    1024
#define LOGQ_LINE 200

static char              logq[LOGQ_N][LOGQ_LINE];
static volatile unsigned logq_head = 0, logq_tail = 0;
static volatile unsigned logq_dropped = 0;
static CRITICAL_SECTION  logq_lock;
static HANDLE            log_thread  = NULL;
static volatile int      log_running = 0;

static DWORD WINAPI log_thread_fn(LPVOID arg)
{
    (void)arg;
    for (;;) {
        char     line[LOGQ_LINE];
        unsigned dropped = 0;
        int      have = 0;

        EnterCriticalSection(&logq_lock);
        if (logq_tail != logq_head) {
            strcpy(line, logq[logq_tail]);
            logq_tail = (logq_tail + 1) % LOGQ_N;
            have = 1;
            if (logq_dropped) { dropped = logq_dropped; logq_dropped = 0; }
        }
        LeaveCriticalSection(&logq_lock);      /* write outside the lock */

        if (have) {
            fputs(line, stdout);
            if (dropped)
                fprintf(stdout, "[log ] dropped %u line(s) - output too slow\n", dropped);
            if (ferror(stdout)) clearerr(stdout);
        } else {
            if (!log_running) break;           /* stop only once drained */
            Sleep(2);
        }
    }
    fflush(stdout);
    return 0;
}

void pc_log_init(void)
{
    if (log_thread) return;
    InitializeCriticalSection(&logq_lock);
    log_running = 1;
    log_thread  = CreateThread(NULL, 0, log_thread_fn, NULL, 0, NULL);
}

void pc_log_shutdown(void)
{
    if (!log_thread) return;
    log_running = 0;
    WaitForSingleObject(log_thread, 2000);
    CloseHandle(log_thread);
    log_thread = NULL;
    DeleteCriticalSection(&logq_lock);
}

void pc_log_printf(const char *fmt, ...)
{
    char     line[LOGQ_LINE];
    va_list  ap;
    unsigned nxt;

    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    line[sizeof line - 1] = '\0';

    if (!log_thread) { fputs(line, stdout); return; }   /* before pc_log_init */

    EnterCriticalSection(&logq_lock);
    nxt = (logq_head + 1) % LOGQ_N;
    if (nxt == logq_tail) {
        logq_dropped++;
    } else {
        strcpy(logq[logq_head], line);
        logq_head = nxt;
    }
    LeaveCriticalSection(&logq_lock);
}

/* ==========================================================================
 * CAN transport over UDP. One datagram per frame, as can_udp.py has it:
 *   [ID lo, ID hi, LEN, LEN data bytes]
 * Binds recv_port and sends to send_port on 127.0.0.1 (the hub).
 * ========================================================================== */
#define RX_RING_N 128

#ifndef SIO_UDP_CONNRESET               /* not always in the mingw headers */
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

typedef struct {
    uint16_t id;
    uint8_t  len;
    uint8_t  buf[8];
} pc_frame;

static SOCKET             can_sock = INVALID_SOCKET;
static struct sockaddr_in can_peer;
static unsigned short     can_recv_port = 0, can_send_port = 0;   /* for a reset relaunch */
static HANDLE             can_rx_thread = NULL;
static volatile int       can_running   = 0;
static volatile unsigned  can_filtered  = 0;

static pc_frame          rx_ring[RX_RING_N];
static volatile unsigned rx_head = 0, rx_tail = 0;
static CRITICAL_SECTION  rx_lock;

static void can_log(const char *dir, uint16_t id, const uint8_t *buf, int len)
{
    char hex[3 * 8 + 1];
    int  i, n = 0;
    SYSTEMTIME t;

    for (i = 0; i < len && i < 8; i++)
        n += sprintf(hex + n, "%02X ", buf[i]);
    hex[n] = '\0';

    GetLocalTime(&t);
    LOG_PRINTF(("[%02d:%02d:%02d.%03d] [CAN %s] 0x%03X [%d] %s\n",
                t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, dir, id, len, hex));
}

static DWORD WINAPI can_rx_fn(LPVOID arg)
{
    uint8_t pkt[16];
    (void)arg;

    while (can_running) {
        int n = recvfrom(can_sock, (char *)pkt, sizeof pkt, 0, NULL, NULL);
        if (n < 3) { Sleep(1); continue; }          /* nothing waiting, or a runt */
        {
            pc_frame m;
            unsigned nxt;

            m.id  = (uint16_t)(pkt[0] | (pkt[1] << 8));
            m.len = pkt[2];
            if (m.len > 8) m.len = 8;
            if (n < 3 + (int)m.len) continue;
            memcpy(m.buf, pkt + 3, m.len);

            /* The hub forwards every frame; the FDCAN filters decide. */
            if (!pc_can_filter_accept(m.id)) { can_filtered++; continue; }

            can_log("RX", m.id, m.buf, m.len);

            EnterCriticalSection(&rx_lock);
            nxt = (rx_head + 1) % RX_RING_N;
            if (nxt != rx_tail) {                   /* drop if full */
                rx_ring[rx_head] = m;
                rx_head = nxt;
            }
            LeaveCriticalSection(&rx_lock);
        }
    }
    return 0;
}

/* Returns 0 on success. */
int pc_can_init(unsigned short recv_port, unsigned short send_port)
{
    WSADATA wsa;
    struct sockaddr_in me;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    can_recv_port = recv_port;
    can_send_port = send_port;

    can_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (can_sock == INVALID_SOCKET) return 2;

    memset(&me, 0, sizeof me);
    me.sin_family      = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    me.sin_port        = htons(recv_port);
    if (bind(can_sock, (struct sockaddr *)&me, sizeof me) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        LOG_PRINTF(("[can ] bind 127.0.0.1:%u failed, WSA error %d%s\n", recv_port, err,
                    err == WSAEADDRINUSE ? " (port in use - another host running?)" : ""));
        return 3;
    }

    /* A send to a port nobody has bound yet would otherwise fail the next
     * recv with WSAECONNRESET. */
    {
        BOOL  off = 0;
        DWORD ret = 0;
        WSAIoctl(can_sock, SIO_UDP_CONNRESET, &off, sizeof off, NULL, 0, &ret, NULL, NULL);
    }

    /* Non-blocking: a full peer drops frames instead of stalling the firmware. */
    {
        u_long nb = 1;
        ioctlsocket(can_sock, FIONBIO, &nb);
    }

    memset(&can_peer, 0, sizeof can_peer);
    can_peer.sin_family      = AF_INET;
    can_peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    can_peer.sin_port        = htons(send_port);

    InitializeCriticalSection(&rx_lock);
    can_running   = 1;
    can_rx_thread = CreateThread(NULL, 0, can_rx_fn, NULL, 0, NULL);

    LOG_PRINTF(("[can ] UDP bus up: recv :%u  send :%u\n", recv_port, send_port));
    return 0;
}

void pc_can_shutdown(void)
{
    can_running = 0;
    if (can_rx_thread) {
        WaitForSingleObject(can_rx_thread, 1000);
        CloseHandle(can_rx_thread);
        can_rx_thread = NULL;
    }
    if (can_sock != INVALID_SOCKET) {
        closesocket(can_sock);
        can_sock = INVALID_SOCKET;
    }
    DeleteCriticalSection(&rx_lock);
    WSACleanup();
}

int pc_can_send(uint16_t id, const uint8_t *data, int len)
{
    uint8_t pkt[11];

    if (can_sock == INVALID_SOCKET) return 0;
    if (len > 8) len = 8;

    pkt[0] = (uint8_t)(id & 0xFF);
    pkt[1] = (uint8_t)((id >> 8) & 0xFF);
    pkt[2] = (uint8_t)len;
    memcpy(pkt + 3, data, len);

    sendto(can_sock, (const char *)pkt, 3 + len, 0,
           (struct sockaddr *)&can_peer, sizeof can_peer);
    can_log("TX", id, data, len);
    return 1;
}

int pc_can_rx_level(void)
{
    int n;
    EnterCriticalSection(&rx_lock);
    n = (int)((rx_head - rx_tail + RX_RING_N) % RX_RING_N);
    LeaveCriticalSection(&rx_lock);
    return n;
}

int pc_can_rx_pop(uint16_t *id, uint8_t *data, int *len)
{
    int got = 0;
    EnterCriticalSection(&rx_lock);
    if (rx_tail != rx_head) {
        pc_frame *m = &rx_ring[rx_tail];
        *id  = m->id;
        *len = m->len;
        memcpy(data, m->buf, m->len);
        rx_tail = (rx_tail + 1) % RX_RING_N;
        got = 1;
    }
    LeaveCriticalSection(&rx_lock);
    return got;
}

/* ==========================================================================
 * RTI simulation thread: raises the real SysTick_Handler at 1 kHz, which is
 * what HAL_GetTick() counts and what decrements Timer1, BootUpTimer,
 * MenuTimer and IncSpeedUpTimer. Load-bearing, not a convenience: every
 * `Timer1 = n; while (Timer1);` wait in the firmware ends only because of it.
 * ========================================================================== */
#define RTI_HZ          1000.0
#define TIM1_PERIOD_MS  500      /* TPS3850 kick, as htim1 is configured */

/* Cap per wakeup, so resuming from a breakpoint does not expire every timer at once. */
#define RTI_MAX_CATCHUP 100

static volatile int          rti_running       = 0;
static HANDLE                rti_handle        = NULL;
static volatile unsigned int rti_tick_count    = 0;
static volatile unsigned int rti_skipped_count = 0;

static DWORD WINAPI rti_thread_fn(LPVOID arg)
{
    LARGE_INTEGER freq, now, prev;
    double        owed = 0.0;        /* ticks the clock says are due */
    unsigned      since_tim1 = 0;

    (void)arg;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    while (rti_running) {
        Sleep(1);
        if (!rti_running) break;

        /* Count ticks from the clock, not from wakeups: Sleep(1) often takes
         * ~15 ms on Windows, and firmware timeouts would run long by that. */
        QueryPerformanceCounter(&now);
        owed += (double)(now.QuadPart - prev.QuadPart) / (double)freq.QuadPart * RTI_HZ;
        prev  = now;

        if (owed > (double)RTI_MAX_CATCHUP) owed = (double)RTI_MAX_CATCHUP;

        while (owed >= 1.0) {
            if (pc_intr_masked()) {
                /* Keep the debt: masking delays ticks, it does not lose them. */
                rti_skipped_count++;
                break;
            }
            SysTick_Handler();
            rti_tick_count++;
            owed -= 1.0;

            if (++since_tim1 >= TIM1_PERIOD_MS) {
                since_tim1 = 0;
                if (pc_tim1_it_enabled()) TIM1_UP_IRQHandler();
            }
        }
    }
    return 0;
}

unsigned int pc_rti_ticks(void)   { return rti_tick_count; }
unsigned int pc_rti_skipped(void) { return rti_skipped_count; }

void rti_thread_start(void)
{
    rti_tick_count = 0;
    rti_running    = 1;
    rti_handle     = CreateThread(NULL, 0, rti_thread_fn, NULL, 0, NULL);
}

void rti_thread_stop(void)
{
    rti_running = 0;
    if (rti_handle) {
        WaitForSingleObject(rti_handle, 2000);
        CloseHandle(rti_handle);
        rti_handle = NULL;
    }
}

/* ==========================================================================
 * Loop instrumentation. COP_Trig() calls HAL_WWDG_Refresh() once per pass of
 * the firmware's main loop, and its stand-in calls this.
 * ========================================================================== */
static volatile unsigned long loop_count   = 0;
static volatile unsigned int  loop_max_gap = 0;

void pc_loop_tick(void)
{
    static DWORD last_ms = 0;
    DWORD now = GetTickCount();
    DWORD gap;

    if (last_ms) {
        gap = now - last_ms;
        if (gap > loop_max_gap) loop_max_gap = (unsigned int)gap;
    }
    last_ms = now;

    loop_count++;
    pc_log_changes();
    Sleep(1);                 /* pacing: the target's loop is free-running */
}

unsigned long pc_loop_count(void)       { return loop_count; }
unsigned int  pc_loop_max_gap_ms(void)  { return loop_max_gap; }
void          pc_loop_reset_max_gap(void) { loop_max_gap = 0; }

/* ==========================================================================
 * Reset. NVIC_SystemReset() and HAL_NVIC_SystemReset() land here: the
 * firmware resets from ResetProc (button-box Reset key) and from
 * MCOUSER_ResetApplication (NMT reset node). Relaunch on the same ports; the
 * socket has to close first, because the child binds the same recv port.
 * ========================================================================== */
static volatile int resetting = 0;

int pc_is_resetting(void) { return resetting; }

void pc_side_reset(void)
{
    char exe[MAX_PATH];
    char cmd[MAX_PATH + 32];
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;

    resetting = 1;              /* the loop stops here: not a stall */
    LOG_PRINTF(("[host] *** RESET: relaunching on :%u -> :%u ***\n",
                can_recv_port, can_send_port));

    rti_thread_stop();
    pc_flash_sync();
    pc_can_shutdown();

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);

    if (GetModuleFileNameA(NULL, exe, sizeof exe)) {
        snprintf(cmd, sizeof cmd, "\"%s\" %u %u", exe, can_recv_port, can_send_port);
        /* Ports as args skip the prompts; inherited handles keep a redirected
         * run's output. */
        if (CreateProcessA(exe, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        } else {
            LOG_PRINTF(("[host] relaunch failed (%lu)\n", GetLastError()));
        }
    }

    pc_log_shutdown();
    ExitProcess(0);
}
