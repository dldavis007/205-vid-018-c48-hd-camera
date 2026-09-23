/*
 * menu_navigation_stub.c  —  HOST-SIDE STUB (test_menu_navigation target)
 *
 * Provides what MenuFunctions.c needs at link time, plus a purpose-built
 * Menuc[] fixture for the navigation tests.
 *
 * Why a fixture menu table instead of the production one from Subroutines.c:
 *   the real table's longest menu has five entries, so the scrolling paths in
 *   LoadMenu (FirstLine, the "cursor reached line 9" rule, wrapping to the
 *   last entry of a long menu) could never be reached.  The fixture keeps the
 *   production shape - entry 0 is the title, a blank entry is 20 spaces, a
 *   submenu's Index is the parent's Index shifted left with the cursor line
 *   appended - and adds a ten-entry menu that does exercise them.
 *
 *   ROOT {0,0,0,0}  4 entries; entry 1 -> SUB, entry 2 -> TALL,
 *                   entry 3 has no submenu so Select runs its function
 *   SUB  {0,0,0,1}  2 entries; entry 2 carries a menu_var for the
 *                   Variable_flag (edit-in-place) paths
 *   TALL {0,0,0,2}  10 entries, for the scrolling paths
 *
 * The timer thread is load-bearing, not a convenience: CursorUp, CursorDown,
 * Select and DeSelect all end in DisplayTitler(), which does
 * `Timer1 = n; while (Timer1);`.  On the target the SysTick interrupt counts
 * Timer1 down; here this thread does, as fast as it can, so the display waits
 * finish immediately.  Nothing else is driven by it: MenuTimer stays under the
 * tests' control, because menu_function() gates on it.
 */

/* Lean Win32: winuser.h's A/W macros (LoadMenu -> LoadMenuA) collide with the
 * firmware's own LoadMenu. */
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nodecfg.h"       /* UNSIGNED8                                */
#include "mco.h"           /* CAN_MSG                                  */
#include "procimg.h"       /* PROCIMG_SIZE                             */
#include "eeprom.h"        /* NB_OF_VAR                                */
#include "Subroutines.h"   /* Gen_Flags bits, RTI_One_Sec              */
#include "MenuFunctions.h" /* struct menu_var, MenuStack, MenuStruct   */
#include "menu_navigation_stub.h"

/* ---- Menu variable used by the SUB menu's "VALUE" line ----------------- */
/* Numeric type: an empty str_enum selects the numeric path in getvalue(). */
struct menu_var NavValue = { 5, 1, 1, 9, 0, 4, "   5", "", NULL };

/* ---- Menu variables MenuFunctions.c refers to by name ------------------ */
static const char s_enum_NULL[]   = "";
static const char s_enum_off_on[] = "OFF, ON";
static const char s_enum_alpha[]  =
    " ,0,1,2,3,4,5,6,7,8,9,-,(,),@,:,;,<,>,.,A,B,C,D,E,F,G,H,I,J,K,L,M,N,"
    "O,P,Q,R,S,T,U,V,W,X,Y,Z,-";
static const char s_enum_number[] = "0,1,2,3,4,5,6,7,8,9,-";

struct menu_var NullVar   = { 0,0,0,0,0,0,  " ",           s_enum_NULL,   NULL };
struct menu_var CamTag    = { 1,1,1,46,0,-11,"     HD CAM", s_enum_alpha,  NULL };
struct menu_var disp_add  = { 1,1,1,46,0,-4, "1234",        s_enum_alpha,  NULL };
struct menu_var MicOnOff  = { 2,1,1,2, 0,3,  " ON",         s_enum_off_on, NULL };
struct menu_var NullVar2  = { 0,0,0,0,0,0,  " ",           s_enum_NULL,   NULL };
struct menu_var SerialNum = { 1,1,1,10,0,-7,"-------",      s_enum_number, NULL };

/* ---- Spy: the function Select() runs for a line with no submenu -------- */
int nav_action_calls = 0;

static int nav_action(void)
{
    nav_action_calls++;
    return 0;
}

/* ---- The fixture menu table ------------------------------------------- */
/* Entry 0 is the title; a blank line must be 20 spaces, because LoadMenu
 * tests Entry[i][1] == ' ' to find the end of a menu. */
#define BLANK "                    "

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
MenuStruct const Menuc[MenuSize] = {
    /* ---- [0] ROOT ---------------------------------------------------- */
    {
        {0, 0, 0, 0},
        {
            "        ROOT        ",
            " SUB MENU           ",
            " TALL MENU          ",
            " ACTION             ",
            " EXIT               ",
            BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {&NullVar, &NullVar, &NullVar, &NullVar, &NullVar, &NullVar,
         &NullVar, &NullVar, &NullVar, &NullVar, &NullVar},
        {&NullFunction, &NullFunction, &nav_action, &ExitMenu, &NullFunction,
         &NullFunction, &NullFunction, &NullFunction, &NullFunction,
         &NullFunction, &NullFunction},
    },
    /* ---- [1] SUB, reached from ROOT entry 1 -------------------------- */
    {
        {0, 0, 0, 1},
        {
            "        SUB         ",
            " BACK               ",
            " VALUE              ",
            BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
        },
        {0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0},      /* VALUE prints at column 9 */
        {&NullVar, &NavValue, &NullVar, &NullVar, &NullVar, &NullVar,
         &NullVar, &NullVar, &NullVar, &NullVar, &NullVar},
        {&ExitMenu, &StdVarFunction, &NullFunction, &NullFunction,
         &NullFunction, &NullFunction, &NullFunction, &NullFunction,
         &NullFunction, &NullFunction, &NullFunction},
    },
    /* ---- [2] TALL, reached from ROOT entry 2: 10 entries ------------- */
    {
        {0, 0, 0, 2},
        {
            "        TALL        ",
            " LINE 01            ",
            " LINE 02            ",
            " LINE 03            ",
            " LINE 04            ",
            " LINE 05            ",
            " LINE 06            ",
            " LINE 07            ",
            " LINE 08            ",
            " LINE 09            ",
            " LINE 10            ",
            BLANK,
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {&NullVar, &NullVar, &NullVar, &NullVar, &NullVar, &NullVar,
         &NullVar, &NullVar, &NullVar, &NullVar, &NullVar},
        {&NullFunction, &NullFunction, &NullFunction, &NullFunction,
         &NullFunction, &NullFunction, &NullFunction, &NullFunction,
         &NullFunction, &NullFunction, &NullFunction},
    },
    /* ---- [3..5] unused: an index no test navigates to ----------------- */
    {
        {9, 9, 9, 3},
        {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
         BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {0}, {0}, {0},
    },
    {
        {9, 9, 9, 4},
        {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
         BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {0}, {0}, {0},
    },
    {
        {9, 9, 9, 5},
        {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
         BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {0}, {0}, {0},
    },
};

MenuStack MenuStackc[MenuStackSize];
#pragma GCC diagnostic pop

/* ---- Globals from main.c ---------------------------------------------- */

uint32_t Timer1          = 0;
uint32_t MenuTimer       = 0;
uint32_t IncSpeedUpTimer = 0;
uint8_t  fast_inc        = 0;
bool     CAN_Operating   = false;

uint16_t VirtAddVarTab[NB_OF_VAR];
uint16_t EEDataTable[NB_OF_VAR];

/* ---- Globals from Subroutines.c --------------------------------------- */

char      String_Var_ptr = 0;
char      Variable_flag  = 0;
char      Multi_Var_ptr  = 0;
uint8_t   StackPointer   = 0;
char      Gen_Flags      = 0;
uint16_t  cam_add        = 0;
uint8_t   TC0_RCVD_Data  = 0;
char      F1_Rcvd_Data   = 0;

char      Menu[12][21];
UNSIGNED8 gProcImg[PROCIMG_SIZE];
CAN_MSG   gTxMsg;

/* ---- Hardware-touching functions MenuFunctions.c calls ----------------- */

int  nav_camera_on_calls = 0;
bool nav_camera_on       = false;
bool nav_mic_on          = false;

void camera_ON(bool val)       { nav_camera_on = val; nav_camera_on_calls++; }
void mic_setEnabled(bool val)  { nav_mic_on = val; }
void COP_Trig(void)            {}

HAL_StatusTypeDef HAL_FLASH_Unlock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Lock(void)   { return HAL_OK; }

/* ---- Timer thread: stands in for SysTick counting Timer1 down ---------- */

static HANDLE       timer_thread  = NULL;
static volatile int timer_running = 0;

static DWORD WINAPI timer_fn(LPVOID arg)
{
    (void)arg;
    while (timer_running) {
        if (Timer1)
            Timer1--;
        else
            Sleep(0);        /* nothing waiting: yield instead of spinning hot */
    }
    return 0;
}

void nav_timer_start(void)
{
    if (timer_thread) return;
    timer_running = 1;
    timer_thread  = CreateThread(NULL, 0, timer_fn, NULL, 0, NULL);
}

void nav_timer_stop(void)
{
    if (!timer_thread) return;
    timer_running = 0;
    WaitForSingleObject(timer_thread, 2000);
    CloseHandle(timer_thread);
    timer_thread = NULL;
}

/* ---- Per-test reset ---------------------------------------------------- */

/* menu_function's key state, defined in MenuFunctions.c. Reset here so each
 * test starts from power-on values instead of inheriting the previous one's. */
extern int8_t CursorDownFlag, CursorUpFlag, SelectFlag;
extern char   UpdateMenu, UpdateArrayVar, AcceptKeys, StoreFlag;

void nav_reset(void)
{
    CursorDownFlag = CursorUpFlag = SelectFlag = 0;
    UpdateMenu = UpdateArrayVar = AcceptKeys = StoreFlag = 0;

    memset(MenuStackc, 0, sizeof MenuStackc);
    memset(Menu, 0, sizeof Menu);
    memset(gProcImg, 0, sizeof gProcImg);

    StackPointer   = 0;
    Variable_flag  = 0;
    String_Var_ptr = 0;
    Multi_Var_ptr  = 0;
    Gen_Flags      = 0;
    TC0_RCVD_Data  = 0;
    F1_Rcvd_Data   = 0;
    MenuTimer      = 0;
    fast_inc       = 0;

    NavValue.value = 5;
    strcpy(NavValue.str_value, "   5");

    nav_action_calls    = 0;
    nav_camera_on_calls = 0;
    nav_camera_on       = false;
    nav_mic_on          = false;
}
