
/*
 * test_menu_navigation.c
 *
 * Unity unit tests for the menu navigation logic in
 * CM4/Core/Src/MenuFunctions.c.
 *
 * Functions under test:
 *  
 *
 * Test ID mapping:
 *   
 */

#include "unity.h"
#include <stdbool.h>
#include "MenuFunctions.h"   /* struct menu_var, incvar, decvar, … */
#include <string.h>
#include <stdio.h>

/* ---- Externals provided by globals_stub.c ----------------------------- */
extern char     String_Var_ptr;
extern uint32_t MenuTimer;
extern uint8_t  fast_inc;

/* ---- Externals provided by menu_navigation_stub.c ----------------------------- */
extern MenuStack MenuStackc[MenuStackSize];


/* ---- Unity fixtures --------------------------------------------------- */

void setUp(void)
{
    String_Var_ptr = 1;   /* select first character position */
    MenuTimer      = 0;
    fast_inc       = 0;
    Stackpointer   = 1; // TODO is this one when get get into the node menu (which we'll be testing) or is it 0 at the start of the test?
    MenuStackc = {{0}};
}

void tearDown(void) {}

/* ---- Test helpers ----------------------------------------------------- */

/*
 * Build a numeric-type menu_var (no enum list).
 * getvalue() calls atof(str_value) for numeric vars, so str_value is
 * pre-populated to match value to keep the two fields consistent.
 */
static struct menu_var make_num_var(float val, float inc,
                                    float min_val, float max_val)
{
    struct menu_var v;
    memset(&v, 0, sizeof(v));
    v.value    = val;
    v.inc      = inc;
    v.min      = min_val;
    v.max      = max_val;
    v.dec_pos  = 0;
    v.len_str  = 4;
    v.str_enum = "";        /* empty string → numeric type */
    v.next_var = NULL;
    snprintf(v.str_value, sizeof(v.str_value), "%4.0f", val);
    return v;
}

/*
 * Build an enum-type menu_var.
 * len_str >= 0 selects the "regular enum" path in getvalue / getstrval.
 * value is 1-based (first enum entry = 1).
 * str_value must contain the text of the current enum entry (without padding)
 * so that getvalue() can match it against the enum list.
 */
static struct menu_var make_enum_var(float val, float max_val,
                                     const char *enum_list,
                                     const char *current_str)
{
    struct menu_var v;
    memset(&v, 0, sizeof(v));
    v.value    = val;
    v.inc      = 1.0f;
    v.min      = 1.0f;
    v.max      = max_val;
    v.dec_pos  = 0;
    v.len_str  = 4;           /* right-aligned into 4 chars */
    v.str_enum = enum_list;
    v.next_var = NULL;
    strncpy(v.str_value, current_str, STR_VALUE_LEN - 1);
    return v;
}

/* =================================================================
 * TC-MN-01
 * 
 * ================================================================= */




/* ================================================================= */
int main(void)
{
    UNITY_BEGIN();

    return UNITY_END();
}
