/*
 * test_subroutines.c
 *
 * Unity unit tests for the EEPROM save/load functions in
 * CM4/Core/Src/MenuFunctions.c.
 *
 * Functions under test:
 *   Save_Variables()    — packs menu_var str_values and writes to EEPROM
 *   Load_Variables()    — reads EEPROM and restores menu_var str_values
 *   Save_Serial_Num()   — writes SerialNum.str_value chars to EEPROM
 *   Load_Serial_Num()   — reads EEPROM chars back into SerialNum.str_value
 *
 * Why these are testable without hardware:
 *   The flash HAL (HAL_FLASH_Program / HAL_FLASHEx_Erase) is replaced by
 *   hal_flash_stub.c, which backs a 256 KB VirtualAlloc'd region at a
 *   low-4-GB address so the uint32_t address arithmetic in eeprom.c works.
 *   The real eeprom.c is compiled; only its flash layer is stubbed.
 *
 * Memory layout assumption:
 *   Save/Load_Variables iterate from &NullVar to &NullVar2 by
 *   sizeof(struct menu_var).  menu_globals_stub.c defines all five structs
 *   with __attribute__((section(".menu_data"))) in the correct order so
 *   they are contiguous in the host binary.
 *
 * Test ID mapping:
 *   TC-SB-01  test_save_load_variables_micOnOff
 *   TC-SB-02  test_save_load_variables_nullvar
 *   TC-SB-03  test_save_load_serial_num_round_trip
 *   TC-SB-04  test_save_serial_num_skips_when_already_set
 */

#include "unity.h"
#include <stdbool.h>
#include <string.h>
#include "MenuFunctions.h"  /* Save_Variables, Load_Variables, etc. */
#include "Subroutines.h"    /* SerNum1VirtIndex                     */
#include "eeprom.h"         /* NB_OF_VAR, EE_Init                   */

/* ---- Externs provided by menu_globals_stub.c and hal_flash_stub.c ----- */

extern struct menu_var NullVar;
extern struct menu_var NullVar2;
extern struct menu_var CamTag;
extern struct menu_var disp_add;
extern struct menu_var MicOnOff;
extern struct menu_var SerialNum;

extern uint16_t VirtAddVarTab[NB_OF_VAR];
extern uint16_t EEDataTable[NB_OF_VAR];
extern char     String_Var_ptr;

void fake_flash_reset(void);
void InitVirtAddVarTab(void);

/* ---- Unity fixtures --------------------------------------------------- */

void setUp(void)
{
    fake_flash_reset();
    EE_Init();
    InitVirtAddVarTab();
    memset(EEDataTable, 0, sizeof(EEDataTable));

    /* Restore menu_var str_values to production defaults */
    strncpy(NullVar.str_value,  " ",           STR_VALUE_LEN - 1);
    strncpy(CamTag.str_value,   "     HD CAM", STR_VALUE_LEN);
    strncpy(disp_add.str_value, "1234",         STR_VALUE_LEN - 1);
    strncpy(MicOnOff.str_value, " ON",          STR_VALUE_LEN - 1);
    MicOnOff.value = 2.0f;
    strncpy(NullVar2.str_value, " ",           STR_VALUE_LEN - 1);

    memset(SerialNum.str_value, '-', 7);
    SerialNum.str_value[7] = '\0';

    String_Var_ptr = 1;
}

void tearDown(void) {}

/* =================================================================
 * TC-SB-01
 * Save_Variables packs all menu str_values into EEPROM.
 * Load_Variables unpacks them and restores each struct in order.
 * Verify MicOnOff.str_value survives a full save → corrupt → load cycle.
 * ================================================================= */
void test_save_load_variables_micOnOff(void)
{
    strncpy(MicOnOff.str_value, " ON", STR_VALUE_LEN - 1);
    Save_Variables();

    strncpy(MicOnOff.str_value, "XXX", STR_VALUE_LEN - 1); /* corrupt */

    Load_Variables();

    TEST_ASSERT_EQUAL_STRING(" ON", MicOnOff.str_value);
}

/* =================================================================
 * TC-SB-02
 * Load_Variables restores the first struct in the .menu_data section
 * (NullVar) as well as interior structs.
 * ================================================================= */
void test_save_load_variables_nullvar(void)
{
    strncpy(NullVar.str_value, " ", STR_VALUE_LEN - 1);
    Save_Variables();

    strncpy(NullVar.str_value, "XYZ", STR_VALUE_LEN - 1); /* corrupt */

    Load_Variables();

    TEST_ASSERT_EQUAL_STRING(" ", NullVar.str_value);
}

/* =================================================================
 * TC-SB-03
 * Save_Serial_Num writes each character of SerialNum.str_value to a
 * dedicated virtual address.  Load_Serial_Num reads them back.
 * ================================================================= */
void test_save_load_serial_num_round_trip(void)
{
    strncpy(SerialNum.str_value, "1234567", 7);
    SerialNum.str_value[7] = '\0';

    Save_Serial_Num();

    memset(SerialNum.str_value, 0, STR_VALUE_LEN); /* wipe */

    Load_Serial_Num();

    TEST_ASSERT_EQUAL_CHAR('1', SerialNum.str_value[0]);
    TEST_ASSERT_EQUAL_CHAR('2', SerialNum.str_value[1]);
    TEST_ASSERT_EQUAL_CHAR('3', SerialNum.str_value[2]);
    TEST_ASSERT_EQUAL_CHAR('4', SerialNum.str_value[3]);
    TEST_ASSERT_EQUAL_CHAR('5', SerialNum.str_value[4]);
    TEST_ASSERT_EQUAL_CHAR('6', SerialNum.str_value[5]);
    TEST_ASSERT_EQUAL_CHAR('7', SerialNum.str_value[6]);
}

/* =================================================================
 * TC-SB-04
 * Save_Serial_Num is a no-op when EEDataTable already holds a numeric
 * serial number AND SerialNum.str_value[0] is also numeric.  Both
 * guards must be true for the early-return to fire.
 * ================================================================= */
void test_save_serial_num_skips_when_already_set(void)
{
    EEDataTable[SerNum1VirtIndex] = '5'; /* simulate previously saved num */

    strncpy(SerialNum.str_value, "9999999", 7);
    SerialNum.str_value[7] = '\0';

    Save_Serial_Num(); /* should return early — both are digits */

    /* EEDataTable[0] must remain '5', not be overwritten with '9' */
    TEST_ASSERT_EQUAL_INT('5', EEDataTable[SerNum1VirtIndex]);
}

/* A 24-character default CSV must still write its null terminator. */
void test_even_length_csv_has_terminator(void)
{
    uint16_t value;
    Save_Variables();
    TEST_ASSERT_EQUAL_INT(0, EE_ReadVariable(VirtAddVarTab[32], &value));
    TEST_ASSERT_EQUAL_HEX16(0, value);
}

void test_shorter_csv_does_not_load_stale_tail(void)
{
    Save_Variables();
    strcpy(CamTag.str_value, "A");
    Save_Variables();
    strcpy(CamTag.str_value, "CORRUPT");
    Load_Variables();
    TEST_ASSERT_EQUAL_STRING("A", CamTag.str_value);
    TEST_ASSERT_EQUAL_STRING(" ON", MicOnOff.str_value);
    TEST_ASSERT_EQUAL_STRING(" ", NullVar2.str_value);
}

void test_unterminated_eeprom_is_bounded(void)
{
    HAL_FLASH_Unlock();
    for (unsigned i = 20; i < NB_OF_VAR; ++i)
        TEST_ASSERT_EQUAL_INT(0, EE_WriteVariable(VirtAddVarTab[i], 0x4141));
    HAL_FLASH_Lock();
    Load_Variables();
    TEST_ASSERT_EQUAL_STRING("     HD CAM", CamTag.str_value);
}

/* ================================================================= */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_save_load_variables_micOnOff);
    RUN_TEST(test_save_load_variables_nullvar);
    RUN_TEST(test_save_load_serial_num_round_trip);
    RUN_TEST(test_save_serial_num_skips_when_already_set);
    RUN_TEST(test_even_length_csv_has_terminator);
    RUN_TEST(test_shorter_csv_does_not_load_stale_tail);
    RUN_TEST(test_unterminated_eeprom_is_bounded);
    return UNITY_END();
}
