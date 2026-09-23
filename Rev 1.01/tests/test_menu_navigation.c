/*
 * test_menu_navigation.c
 *
 * Unity unit tests for the menu navigation logic in
 * CM4/Core/Src/MenuFunctions.c, against the fixture menu table in
 * stubs/menu_navigation_stub.c (ROOT, SUB and TALL - see the notes there).
 *
 * Functions under test:
 *   LoadMenu, InsertCursor, CursorUp, CursorDown, Select, DeSelect,
 *   menu_function
 *
 * Test ID mapping:
 *   TC-MN-01  cursor down moves to the next entry
 *   TC-MN-02  cursor up moves to the previous entry
 *   TC-MN-03  cursor up on the first entry wraps to the last
 *   TC-MN-04  cursor down past the last entry wraps to the first
 *   TC-MN-05  LoadMenu copies the title and the entries into Menu[][]
 *   TC-MN-06  InsertCursor marks the cursor line
 *   TC-MN-07  cursor down on line 8 of a long menu scrolls instead of moving
 *   TC-MN-08  cursor up with the menu scrolled moves the window back up
 *   TC-MN-09  cursor up at the top of a long menu wraps to its last entry
 *   TC-MN-10  Select enters the submenu for that line
 *   TC-MN-11  Select runs the line's function when it has no submenu
 *   TC-MN-12  DeSelect returns to the parent menu
 *   TC-MN-13  cursor up in variable mode increments the variable
 *   TC-MN-14  cursor down in variable mode decrements the variable
 *   TC-MN-15  the activate command opens the root menu and turns the camera on
 *   TC-MN-16  a key is ignored until a pass has enabled key acceptance
 *   TC-MN-17  a held Down key repeats, so the menu scrolls
 *   TC-MN-18  the menu timer sets the repeat rate
 *   TC-MN-19  Select does not repeat while held
 *
 * Not covered here: DeSelect from the root menu, which leaves the menu and
 * ends in `Timer1 = ...; while (Timer1);` inside Send_Menu_Status/ClearTitler
 * after MCO_ProcessStack, and Save_Variables, which test_subroutines owns.
 */

#include "unity.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Subroutines.h"          /* Gen_Flags bits, TeleData bits      */
#include "MenuFunctions.h"
#include "nodecfg.h"              /* UNSIGNED8                          */
#include "procimg.h"              /* OUT_digi_*                         */
#include "menu_navigation_stub.h"

/* ---- Externals provided by menu_navigation_stub.c --------------------- */
extern MenuStack MenuStackc[MenuStackSize];
extern uint8_t   StackPointer;
extern char      Menu[12][21];
extern char      Variable_flag;
extern char      String_Var_ptr;
extern char      Gen_Flags;
extern uint16_t  cam_add;
extern uint32_t  MenuTimer;
extern UNSIGNED8 gProcImg[];

/* ---- Unity fixtures --------------------------------------------------- */

/*
 * The menu always opens on the root with the cursor on entry 1: menu_function
 * sets StackPointer = 0, CursorPos = 1, FirstLine = 0 when the activate
 * command arrives (MenuFunctions.c, "command to activate menu").  Tests that
 * need a different menu call open_menu() themselves.
 */
void setUp(void)
{
    nav_reset();
    MenuStackc[0].CursorPos = 1;
    Gen_Flags |= Gen_Flags_Menu_Active;
    LoadMenu(MenuStackc[0].Index);
}

void tearDown(void) {}

/* ---- Test helpers ----------------------------------------------------- */

/* Put the unit on fixture menu {0,0,0,index} at the bottom of the stack. */
static void open_menu(char index, int cursor, char firstline)
{
    StackPointer = 0;
    memset(MenuStackc, 0, sizeof(MenuStack) * MenuStackSize);
    MenuStackc[0].Index[3] = index;
    MenuStackc[0].CursorPos = cursor;
    MenuStackc[0].FirstLine = firstline;
    LoadMenu(MenuStackc[0].Index);
}

static int cursor(void)    { return MenuStackc[StackPointer].CursorPos; }
static int firstline(void) { return MenuStackc[StackPointer].FirstLine; }

/* The displayed line, with the cursor column masked out. */
static const char *line(int i)
{
    static char buf[21];
    memcpy(buf, Menu[i], 21);
    buf[0] = ' ';
    buf[20] = '\0';
    return buf;
}

/* =================================================================
 * TC-MN-01 / TC-MN-02: the cursor moves one entry at a time
 * ================================================================= */
static void test_cursor_down_moves_to_next_entry(void)
{
    TEST_ASSERT_EQUAL_INT(1, cursor());

    CursorDown();

    TEST_ASSERT_EQUAL_INT(2, cursor());
    TEST_ASSERT_EQUAL_INT(0, firstline());
    TEST_ASSERT_EQUAL_CHAR('>', Menu[2][0]);
}

static void test_cursor_up_moves_to_previous_entry(void)
{
    CursorDown();
    TEST_ASSERT_EQUAL_INT(2, cursor());

    CursorUp();

    TEST_ASSERT_EQUAL_INT(1, cursor());
    TEST_ASSERT_EQUAL_CHAR('>', Menu[1][0]);
}

/* =================================================================
 * TC-MN-03 / TC-MN-04: the cursor wraps at both ends of a short menu
 * ================================================================= */
static void test_cursor_up_at_first_entry_wraps_to_last(void)
{
    CursorUp();

    TEST_ASSERT_EQUAL_INT(NAV_ROOT_ENTRIES, cursor());   /* " EXIT" */
    TEST_ASSERT_EQUAL_INT(0, firstline());
    TEST_ASSERT_EQUAL_STRING(" EXIT               ", line(NAV_ROOT_ENTRIES));
}

static void test_cursor_down_past_last_entry_wraps_to_first(void)
{
    MenuStackc[0].CursorPos = NAV_ROOT_ENTRIES;
    LoadMenu(MenuStackc[0].Index);

    CursorDown();

    TEST_ASSERT_EQUAL_INT(1, cursor());
    TEST_ASSERT_EQUAL_INT(0, firstline());
}

/* =================================================================
 * TC-MN-05 / TC-MN-06: what LoadMenu and InsertCursor put on screen
 * ================================================================= */
static void test_load_menu_copies_title_and_entries(void)
{
    TEST_ASSERT_EQUAL_STRING("        ROOT        ", Menu[0]);
    TEST_ASSERT_EQUAL_STRING(" SUB MENU           ", line(1));
    TEST_ASSERT_EQUAL_STRING(" TALL MENU          ", line(2));
    TEST_ASSERT_EQUAL_STRING(" ACTION             ", line(3));
    TEST_ASSERT_EQUAL_STRING(" EXIT               ", line(4));
    TEST_ASSERT_EQUAL_STRING("                    ", line(5));
}

static void test_insert_cursor_marks_current_line(void)
{
    MenuStackc[0].CursorPos = 3;
    LoadMenu(MenuStackc[0].Index);
    TEST_ASSERT_EQUAL_CHAR(' ', Menu[3][0]);

    InsertCursor();

    TEST_ASSERT_EQUAL_CHAR('>', Menu[3][0]);
    TEST_ASSERT_EQUAL_CHAR(' ', Menu[2][0]);
}

/* =================================================================
 * TC-MN-07 .. TC-MN-09: a menu longer than the eight visible lines
 * ================================================================= */
static void test_cursor_down_at_line8_scrolls_the_window(void)
{
    open_menu(NAV_TALL, 8, 0);
    TEST_ASSERT_EQUAL_STRING(" LINE 08            ", line(8));

    CursorDown();

    /* The cursor stays on the bottom line; the window moves down one. */
    TEST_ASSERT_EQUAL_INT(8, cursor());
    TEST_ASSERT_EQUAL_INT(1, firstline());
    TEST_ASSERT_EQUAL_STRING(" LINE 09            ", line(8));
    TEST_ASSERT_EQUAL_STRING(" LINE 02            ", line(1));
}

static void test_cursor_up_with_menu_scrolled_moves_window_back(void)
{
    open_menu(NAV_TALL, 1, 1);
    TEST_ASSERT_EQUAL_STRING(" LINE 02            ", line(1));

    CursorUp();

    TEST_ASSERT_EQUAL_INT(1, cursor());
    TEST_ASSERT_EQUAL_INT(0, firstline());
    TEST_ASSERT_EQUAL_STRING(" LINE 01            ", line(1));
}

static void test_cursor_up_at_top_of_long_menu_wraps_to_last_entry(void)
{
    open_menu(NAV_TALL, 1, 0);

    CursorUp();

    /* Ten entries, eight lines: the last entry shows on the bottom line. */
    TEST_ASSERT_EQUAL_INT(8, cursor());
    TEST_ASSERT_EQUAL_INT(NAV_TALL_ENTRIES - 8, firstline());
    TEST_ASSERT_EQUAL_STRING(" LINE 10            ", line(8));
}

/* =================================================================
 * TC-MN-10 .. TC-MN-12: the menu stack
 * ================================================================= */
static void test_select_enters_submenu(void)
{
    TEST_ASSERT_EQUAL_INT(1, cursor());          /* " SUB MENU" */

    Select();

    TEST_ASSERT_EQUAL_INT(1, StackPointer);
    TEST_ASSERT_EQUAL_INT(NAV_SUB, MenuStackc[1].Index[3]);
    TEST_ASSERT_EQUAL_INT(1, cursor());          /* cursor starts at the top */
    TEST_ASSERT_EQUAL_INT(0, firstline());
    TEST_ASSERT_EQUAL_STRING("        SUB         ", Menu[0]);
}

static void test_select_runs_entry_function_when_no_submenu(void)
{
    MenuStackc[0].CursorPos = 3;                 /* " ACTION": no submenu */
    LoadMenu(MenuStackc[0].Index);

    Select();

    TEST_ASSERT_EQUAL_INT(1, nav_action_calls);
    TEST_ASSERT_EQUAL_INT(0, StackPointer);
    TEST_ASSERT_EQUAL_STRING("        ROOT        ", Menu[0]);
}

static void test_deselect_returns_to_parent_menu(void)
{
    Select();
    TEST_ASSERT_EQUAL_INT(1, StackPointer);
    Variable_flag  = 1;
    String_Var_ptr = 3;

    DeSelect();

    TEST_ASSERT_EQUAL_INT(0, StackPointer);
    TEST_ASSERT_EQUAL_STRING("        ROOT        ", Menu[0]);
    TEST_ASSERT_EQUAL_INT(0, Variable_flag);     /* editing is cancelled */
    TEST_ASSERT_EQUAL_INT(0, String_Var_ptr);
}

/* =================================================================
 * TC-MN-13 / TC-MN-14: in variable mode the keys edit, not navigate
 * ================================================================= */
static void test_cursor_up_in_variable_mode_increments_variable(void)
{
    Select();                                    /* into SUB */
    MenuStackc[StackPointer].CursorPos = 2;      /* " VALUE" */
    LoadMenu(MenuStackc[StackPointer].Index);
    Variable_flag = 1;
    String_Var_ptr = 1;

    CursorUp();

    TEST_ASSERT_EQUAL_FLOAT(6.0f, NavValue.value);
    TEST_ASSERT_EQUAL_INT(2, cursor());          /* the cursor did not move */
}

static void test_cursor_down_in_variable_mode_decrements_variable(void)
{
    Select();
    MenuStackc[StackPointer].CursorPos = 2;
    LoadMenu(MenuStackc[StackPointer].Index);
    Variable_flag = 1;
    String_Var_ptr = 1;

    CursorDown();

    TEST_ASSERT_EQUAL_FLOAT(4.0f, NavValue.value);
    TEST_ASSERT_EQUAL_INT(2, cursor());
}

/* =================================================================
 * TC-MN-15 .. TC-MN-17: menu_function, the button-to-cursor path
 * ================================================================= */

/* The controller's "activate menu" command for this camera: OUT_digi_6 bit 3
 * with the camera's own address in OUT_digi_7/8. */
static void send_activate_command(void)
{
    gProcImg[OUT_digi_6] = 0x08;
    gProcImg[OUT_digi_7] = cam_add & 0x00FF;
    gProcImg[OUT_digi_8] = (cam_add & 0xFF00) >> 8;
}

static void test_activate_command_opens_root_menu(void)
{
    nav_reset();                                 /* menu not active yet */
    cam_add = 0x1234;
    send_activate_command();

    TEST_ASSERT_EQUAL_INT(1, menu_function());   /* returns "menu is active" */

    TEST_ASSERT_TRUE(Gen_Flags & Gen_Flags_Menu_Active);
    TEST_ASSERT_EQUAL_INT(0, StackPointer);
    TEST_ASSERT_EQUAL_INT(1, cursor());
    TEST_ASSERT_EQUAL_INT(0, firstline());
    TEST_ASSERT_EQUAL_STRING("        ROOT        ", Menu[0]);
    TEST_ASSERT_TRUE(nav_camera_on);             /* the camera is switched on */
    TEST_ASSERT_EQUAL_INT(0, gProcImg[OUT_digi_6]);   /* command consumed */
}

/* One pass of the loop with the button word held down. MenuTimer gates the
 * key handling, and on the target it counts down between passes. */
static void press_keys(uint16_t buttons)
{
    gProcImg[OUT_digi_1] = buttons & 0xFF;
    gProcImg[OUT_digi_2] = buttons >> 8;
    MenuTimer = 0;
    menu_function();
}

/* The same, but with the menu timer still counting, which is what gates the
 * key handling between passes on the target. */
static void press_keys_timer_running(uint16_t buttons)
{
    gProcImg[OUT_digi_1] = buttons & 0xFF;
    gProcImg[OUT_digi_2] = buttons >> 8;
    MenuTimer = 5;
    menu_function();
}

/* The pass that turns AcceptKeys on, which the firmware does once the menu
 * timer has expired (see TC-MN-16). */
static void enable_keys(void)
{
    press_keys(0);
}

/*
 * A key pressed before the firmware has enabled key acceptance is ignored:
 * menu_function only sets AcceptKeys in the `if (!MenuTimer)` block, after the
 * block that reads the buttons, so the first pass can never act on one. This
 * is what stops the button press that opens the menu from also moving the
 * cursor.
 */
static void test_keys_are_ignored_until_enabled(void)
{
    press_keys(TeleData_Down);

    TEST_ASSERT_EQUAL_INT(1, cursor());
}

/*
 * Holding Down scrolls: the handler runs again on every pass where the menu
 * timer has expired, because it tests `if (CursorDownFlag)`, which is true for
 * the -1 it leaves behind as well as for a fresh 1.
 */
static void test_held_down_key_repeats(void)
{
    enable_keys();

    press_keys(TeleData_Down);
    TEST_ASSERT_EQUAL_INT(2, cursor());

    press_keys(TeleData_Down);                   /* still held */

    TEST_ASSERT_EQUAL_INT(3, cursor());
}

/* MenuTimer is what sets the repeat rate: while it is still counting, the
 * pass does nothing. The firmware reloads it with MenuTime after each move. */
static void test_menu_timer_gates_the_repeat(void)
{
    enable_keys();
    press_keys(TeleData_Down);
    TEST_ASSERT_EQUAL_INT(2, cursor());

    press_keys_timer_running(TeleData_Down);

    TEST_ASSERT_EQUAL_INT(2, cursor());
}

/*
 * Select is deliberately not a repeating key: the handler tests
 * `if (SelectFlag == 1)`, so the -1 it leaves behind does nothing until the
 * key is released and pressed again. Holding it must not walk down the menu
 * tree by itself.
 */
static void test_select_does_not_repeat_while_held(void)
{
    enable_keys();

    press_keys(TeleData_Select);                 /* into SUB */
    TEST_ASSERT_EQUAL_INT(1, StackPointer);

    press_keys(TeleData_Select);                 /* still held */
    TEST_ASSERT_EQUAL_INT(1, StackPointer);

    press_keys(0);                               /* released */
    press_keys(TeleData_Select);                 /* SUB's " BACK" line */

    TEST_ASSERT_EQUAL_INT(0, StackPointer);
}

/* ================================================================= */
int main(void)
{
    nav_timer_start();          /* counts Timer1 down, as SysTick does */
    UNITY_BEGIN();

    RUN_TEST(test_cursor_down_moves_to_next_entry);
    RUN_TEST(test_cursor_up_moves_to_previous_entry);
    RUN_TEST(test_cursor_up_at_first_entry_wraps_to_last);
    RUN_TEST(test_cursor_down_past_last_entry_wraps_to_first);
    RUN_TEST(test_load_menu_copies_title_and_entries);
    RUN_TEST(test_insert_cursor_marks_current_line);

    RUN_TEST(test_cursor_down_at_line8_scrolls_the_window);
    RUN_TEST(test_cursor_up_with_menu_scrolled_moves_window_back);
    RUN_TEST(test_cursor_up_at_top_of_long_menu_wraps_to_last_entry);

    RUN_TEST(test_select_enters_submenu);
    RUN_TEST(test_select_runs_entry_function_when_no_submenu);
    RUN_TEST(test_deselect_returns_to_parent_menu);

    RUN_TEST(test_cursor_up_in_variable_mode_increments_variable);
    RUN_TEST(test_cursor_down_in_variable_mode_decrements_variable);

    RUN_TEST(test_activate_command_opens_root_menu);
    RUN_TEST(test_keys_are_ignored_until_enabled);
    RUN_TEST(test_held_down_key_repeats);
    RUN_TEST(test_menu_timer_gates_the_repeat);
    RUN_TEST(test_select_does_not_repeat_while_held);

    {
        int failures = UNITY_END();
        nav_timer_stop();
        return failures;
    }
}
