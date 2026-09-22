
/*
 * test_menu_var.c
 *
 * Unity unit tests for the menu-variable logic in
 * CM4/Core/Src/MenuFunctions.c.
 *
 * Functions under test:
 *   incvar()   — increments a menu variable, wraps at max
 *   decvar()   — decrements a menu variable, wraps at min
 *   getvalue() — resolves the current numeric or enum value from str_value
 *   getstrval()— formats value back into str_value string
 *
 * Why these functions are testable without hardware:
 *   incvar / decvar modify struct menu_var fields using ordinary arithmetic
 *   and call getvalue / getstrval, which use only standard C string functions
 *   (strlen, strtok, strcmp, sprintf, strncpy, atof).  No HAL, CAN, or EEPROM
 *   calls appear in any of these four functions' call graphs.
 *
 * External globals required by incvar / decvar (defined in globals_stub.c):
 *   String_Var_ptr  — which character position is being edited (1-based)
 *   MenuTimer       — timestamp used to compute IncSpeedUpTimer
 *   IncSpeedUpTimer — written by incvar/decvar; not read here
 *   fast_inc        — enables 10× speed mode when non-zero
 *
 * Test ID mapping:
 *   TC-MV-01  test_incvar_increments_numeric_value
 *   TC-MV-02  test_incvar_wraps_numeric_at_max
 *   TC-MV-03  test_decvar_decrements_numeric_value
 *   TC-MV-04  test_decvar_wraps_numeric_at_min
 *   TC-MV-05  test_fast_inc_jumps_by_ten
 *   TC-MV-06  test_getvalue_finds_first_enum
 *   TC-MV-07  test_getvalue_finds_middle_enum
 *   TC-MV-08  test_getvalue_finds_end_enum
 *   TC-MV-09  test_incvar_advances_enum
 *   TC-MV-10  test_incvar_wraps_enum_at_last_entry
 *   TC-MV-11  test_decvar_wraps_enum_at_first_entry
 *   TC-MV-12  test_getstrval_formats_integer
 *   TC-MV-13  test_getstrval_formats_enum
 *   TC-MV-14  test_UpdateVarByValue_sets_value_and_str
 *   TC-MV-15  test_UpdateVarByString_numeric_updates_value
 *   TC-MV-16  test_UpdateVarByString_enum_updates_value
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

/* ---- Unity fixtures --------------------------------------------------- */

void setUp(void)
{
    String_Var_ptr = 1;   /* select first character position */
    MenuTimer      = 0;
    fast_inc       = 0;
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
 * TC-MV-01
 * incvar on a numeric variable increments value by inc.
 * ================================================================= */
void test_incvar_increments_numeric_value(void)
{
    struct menu_var v = make_num_var(5.0f, 1.0f, 0.0f, 10.0f);
    incvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, v.value);
}

/* =================================================================
 * TC-MV-02
 * incvar at max wraps around to min.
 * value = max + inc = 11 > max(10) → value = min(0).
 * ================================================================= */
void test_incvar_wraps_numeric_at_max(void)
{
    struct menu_var v = make_num_var(10.0f, 1.0f, 0.0f, 10.0f);
    incvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, v.value);
}

/* =================================================================
 * TC-MV-03
 * decvar on a numeric variable decrements value by inc.
 * ================================================================= */
void test_decvar_decrements_numeric_value(void)
{
    struct menu_var v = make_num_var(5.0f, 1.0f, 0.0f, 10.0f);
    decvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, v.value);
}

/* =================================================================
 * TC-MV-04
 * decvar at min wraps around to max.
 * value = min - inc = -1 < min(0) → value = max(10).
 * ================================================================= */
void test_decvar_wraps_numeric_at_min(void)
{
    struct menu_var v = make_num_var(0.0f, 1.0f, 0.0f, 10.0f);
    decvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, v.value);
}

/* =================================================================
 * TC-MV-05
 * When fast_inc is set, incvar jumps by inc × 10.
 * ================================================================= */
void test_fast_inc_jumps_by_ten(void)
{
    fast_inc = 1;
    struct menu_var v = make_num_var(0.0f, 1.0f, 0.0f, 100.0f);
    incvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, v.value);
}

/* =================================================================
 * TC-MV-06
 * getvalue() with an enum list and str_value matching the FIRST entry
 * must set value to 1.
 * ================================================================= */
void test_getvalue_finds_first_enum(void)
{
    static const char enums[] = "LOW,MED,HIGH";
    struct menu_var v = make_enum_var(1.0f, 3.0f, enums, "LOW");
    float result = getvalue(&v, 0);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, result);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, v.value);
}

/* =================================================================
 * TC-MV-07
 * getvalue() with str_value matching the MIDDLE entry sets value to 2.
 * ================================================================= */
void test_getvalue_finds_middle_enum(void)
{
    static const char enums[] = "LOW,MED,HIGH";
    struct menu_var v = make_enum_var(1.0f, 3.0f, enums, "MED");
    float result = getvalue(&v, 0);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, result);
}
/* =================================================================
 * TC-MV-08
 * getvalue() with str_value matching the END entry sets value to 3.
 * ================================================================= */
void test_getvalue_finds_end_enum(void)
{
    static const char enums[] = "LOW,MED,HIGH";
    struct menu_var v = make_enum_var(1.0f, 3.0f, enums, "HIGH");
    float result = getvalue(&v, 0);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, result);

}

/* =================================================================
 * TC-MV-09
 * incvar on an enum variable at entry 1 ("OFF") advances to entry 2 ("ON").
 * getvalue() is called internally by incvar to sync value from str_value
 * before the increment, then getstrval() updates str_value after.
 * ================================================================= */
void test_incvar_advances_enum(void)
{
    static const char enums[] = "OFF,ON";
    struct menu_var v = make_enum_var(1.0f, 2.0f, enums, "OFF");
    incvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, v.value);
}

/* =================================================================
 * TC-MV-10
 * incvar at the last enum entry wraps back to entry 1 (min = 1).
 * ================================================================= */
void test_incvar_wraps_enum_at_last_entry(void)
{
    static const char enums[] = "OFF,ON";
    struct menu_var v = make_enum_var(2.0f, 2.0f, enums, "ON");
    incvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, v.value);
}

/* =================================================================
 * TC-MV-11
 * decvar at the first enum entry (value = min = 1) wraps to the last
 * entry (value = max = 3).
 * ================================================================= */
void test_decvar_wraps_enum_at_first_entry(void)
{
    static const char enums[] = "A,B,C";
    struct menu_var v = make_enum_var(1.0f, 3.0f, enums, "A");
    decvar(&v);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, v.value);
}

/* =================================================================
 * TC-MV-12
 * getstrval on a numeric integer variable right-aligns value within
 * len_str characters.  value=7, dec_pos=0, len_str=4 → "   7"
 * ================================================================= */
void test_getstrval_formats_integer(void)
{
    struct menu_var v = make_num_var(7.0f, 1.0f, 0.0f, 10.0f);
    char *result = getstrval(&v);
    TEST_ASSERT_EQUAL_STRING("   7", result);
    TEST_ASSERT_EQUAL_STRING("   7", v.str_value);
}

/* =================================================================
 * TC-MV-13
 * getstrval on an enum variable selects the token at value (1-based)
 * and right-aligns it within len_str.
 * value=2, "LOW,MED,HIGH", len_str=4 → " MED"
 * ================================================================= */
void test_getstrval_formats_enum(void)
{
    static const char enums[] = "LOW,MED,HIGH";
    struct menu_var v = make_enum_var(2.0f, 3.0f, enums, "LOW");
    char *result = getstrval(&v);
    TEST_ASSERT_EQUAL_STRING(" MED", result);
}

/* =================================================================
 * TC-MV-14
 * UpdateVarByValue sets value and refreshes str_value via getstrval.
 * value=8, dec_pos=0, len_str=4 → str_value="   8"
 * ================================================================= */
void test_UpdateVarByValue_sets_value_and_str(void)
{
    struct menu_var v = make_num_var(0.0f, 1.0f, 0.0f, 10.0f);
    UpdateVarByValue(&v, 8.0f);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, v.value);
    TEST_ASSERT_EQUAL_STRING("   8", v.str_value);
}

/* =================================================================
 * TC-MV-15
 * UpdateVarByString on a numeric variable parses the string to a
 * float via getvalue.  "   6" → value=6.0  (atof ignores spaces)
 * ================================================================= */
void test_UpdateVarByString_numeric_updates_value(void)
{
    struct menu_var v = make_num_var(0.0f, 1.0f, 0.0f, 10.0f);
    UpdateVarByString(&v, "   6");
    TEST_ASSERT_EQUAL_FLOAT(6.0f, v.value);
}

/* =================================================================
 * TC-MV-16
 * UpdateVarByString on an enum variable scans the token list and sets
 * value to the matching 1-based position.  "MED" → value=2.
 * ================================================================= */
void test_UpdateVarByString_enum_updates_value(void)
{
    static const char enums[] = "LOW,MED,HIGH";
    struct menu_var v = make_enum_var(1.0f, 3.0f, enums, "LOW");
    UpdateVarByString(&v, "MED");
    TEST_ASSERT_EQUAL_FLOAT(2.0f, v.value);
}




/* ================================================================= */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_incvar_increments_numeric_value);
    RUN_TEST(test_incvar_wraps_numeric_at_max);
    RUN_TEST(test_decvar_decrements_numeric_value);
    RUN_TEST(test_decvar_wraps_numeric_at_min);
    RUN_TEST(test_fast_inc_jumps_by_ten);
    RUN_TEST(test_getvalue_finds_first_enum);
    RUN_TEST(test_getvalue_finds_middle_enum);
    RUN_TEST(test_getvalue_finds_end_enum);
    RUN_TEST(test_incvar_advances_enum);
    RUN_TEST(test_incvar_wraps_enum_at_last_entry);
    RUN_TEST(test_decvar_wraps_enum_at_first_entry);
    RUN_TEST(test_getstrval_formats_integer);
    RUN_TEST(test_getstrval_formats_enum);
    RUN_TEST(test_UpdateVarByValue_sets_value_and_str);
    RUN_TEST(test_UpdateVarByString_numeric_updates_value);
    RUN_TEST(test_UpdateVarByString_enum_updates_value);
    return UNITY_END();
}
