/*
 * test_eeprom.c
 *
 * Unity unit tests for the EEPROM emulation layer (Drivers/EEPROM/Src/eeprom.c).
 *
 * What is being tested:
 *   The three public functions — EE_Init, EE_ReadVariable, EE_WriteVariable —
 *   are exercised against a 256 KB in-memory byte array (fake_flash_storage)
 *   that is set to the erased state (all 0xFF) before every test case.
 *
 * How the hardware abstraction is removed:
 *   tests/stubs/eeprom.h redefines EEPROM_START_ADDRESS to point at
 *   fake_flash_storage rather than the real STM32 flash address 0x081C0000.
 *   HAL_FLASH_Program / HAL_FLASHEx_Erase in hal_flash_stub.c then operate
 *   on that same array with memcpy / memset, so eeprom.c logic runs
 *   unmodified on any host machine.
 *
 * Test ID mapping (for traceability to manual test plan):
 *   TC-EE-01  test_init_formats_blank_flash
 *   TC-EE-02  test_read_with_no_valid_page_returns_no_valid_page
 *   TC-EE-03  test_read_absent_variable_returns_not_found
 *   TC-EE-04  test_write_read_roundtrip
 *   TC-EE-05  test_multiple_variables_roundtrip
 *   TC-EE-06  test_overwrite_returns_latest_value
 */

#include "unity.h"
#include "eeprom.h"    /* picks up tests/stubs/eeprom.h */
#include <string.h>
#include <stdint.h>

/* ---- Provided by hal_flash_stub.c ------------------------------------- */
extern void fake_flash_reset(void);

/*
 * VirtAddVarTab is declared 'extern' in eeprom.c.
 * In production it lives in user.c; here we own it.
 * Populate with sequential addresses 1..NB_OF_VAR — the exact values do
 * not matter for the basic read/write tests, but they must be non-zero and
 * unique (0xFFFF is explicitly prohibited by the EEPROM emulation spec).
 */
uint16_t VirtAddVarTab[NB_OF_VAR];

/* ---- Unity fixtures --------------------------------------------------- */

void setUp(void)
{
    for (int i = 0; i < NB_OF_VAR; i++)
        VirtAddVarTab[i] = (uint16_t)(i + 1);

    fake_flash_reset();   /* all 0xFF — matches physically erased flash */
}

void tearDown(void) {}

/* ---- Internal helper -------------------------------------------------- */

static void init_eeprom_ok(void)
{
    uint16_t result = EE_Init();
    TEST_ASSERT_EQUAL_HEX16(EE_OK, result);
}

/* ================================================================
 * TC-EE-01
 * EE_Init on completely blank flash (both pages erased):
 *   - must return EE_OK
 *   - must mark PAGE0 as VALID (first 16-bit word at PAGE0_BASE = 0x0000)
 * ================================================================ */
void test_init_formats_blank_flash(void)
{
    uint16_t result = EE_Init();

    TEST_ASSERT_EQUAL_HEX16(EE_OK, result);

    /* PAGE0 base address should now hold VALID_PAGE marker (0x0000).
     * EE_Format writes a uint32_t[8] zero array via HAL_FLASH_Program;
     * the first two bytes of that write become the page-status word. */
    uint16_t page0_status = *(volatile uint16_t *)(uintptr_t)PAGE0_BASE_ADDRESS;
    TEST_ASSERT_EQUAL_HEX16(VALID_PAGE, page0_status);
}

/* ================================================================
 * TC-EE-02
 * Reading before EE_Init (no valid page exists) must return NO_VALID_PAGE.
 * PAGE0 and PAGE1 are both 0xFF (ERASED), so EE_FindValidPage sees no
 * VALID_PAGE marker and returns NO_VALID_PAGE without touching *Data.
 * ================================================================ */
void test_read_with_no_valid_page_returns_no_valid_page(void)
{
    uint16_t data   = 0xBEEF;
    uint16_t result = EE_ReadVariable(0x0001, &data);

    TEST_ASSERT_EQUAL_HEX16(NO_VALID_PAGE, result);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, data);   /* data must be untouched */
}

/* ================================================================
 * TC-EE-03
 * Reading a virtual address that was never written returns 1 (not found).
 * The valid page is set up by EE_Init but contains no data records.
 * ================================================================ */
void test_read_absent_variable_returns_not_found(void)
{
    init_eeprom_ok();

    uint16_t data   = 0xDEAD;
    uint16_t result = EE_ReadVariable(0x0001, &data);

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL_HEX16(0xDEAD, data);   /* data must be untouched */
}

/* ================================================================
 * TC-EE-04
 * Writing a variable then reading it back must return the same value.
 * This exercises EE_VerifyPageFullWriteVariable (write path) and the
 * backward-scan loop in EE_ReadVariable (read path).
 * ================================================================ */
void test_write_read_roundtrip(void)
{
    init_eeprom_ok();

    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0001, 0xABCD));

    uint16_t data   = 0;
    uint16_t result = EE_ReadVariable(0x0001, &data);

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_HEX16(0xABCD, data);
}

/* ================================================================
 * TC-EE-05
 * Three distinct virtual addresses can coexist in the same page.
 * Each must be read back independently with the correct value.
 * ================================================================ */
void test_multiple_variables_roundtrip(void)
{
    init_eeprom_ok();

    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0001, 0x1111));
    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0002, 0x2222));
    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0003, 0x3333));

    uint16_t d1 = 0, d2 = 0, d3 = 0;
    TEST_ASSERT_EQUAL(0, EE_ReadVariable(0x0001, &d1));
    TEST_ASSERT_EQUAL(0, EE_ReadVariable(0x0002, &d2));
    TEST_ASSERT_EQUAL(0, EE_ReadVariable(0x0003, &d3));

    TEST_ASSERT_EQUAL_HEX16(0x1111, d1);
    TEST_ASSERT_EQUAL_HEX16(0x2222, d2);
    TEST_ASSERT_EQUAL_HEX16(0x3333, d3);
}

/* ================================================================
 * TC-EE-06
 * Writing the same virtual address twice must return the most recent value.
 * EE_ReadVariable scans from the END of the page backwards, so the newest
 * write (at a higher offset) is found before the earlier one.
 * ================================================================ */
void test_overwrite_returns_latest_value(void)
{
    init_eeprom_ok();

    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0001, 0x1234));
    TEST_ASSERT_EQUAL_HEX16(EE_OK, EE_WriteVariable(0x0001, 0x5678));

    uint16_t data   = 0;
    uint16_t result = EE_ReadVariable(0x0001, &data);

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_HEX16(0x5678, data);
}

/* ================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_formats_blank_flash);
    RUN_TEST(test_read_with_no_valid_page_returns_no_valid_page);
    RUN_TEST(test_read_absent_variable_returns_not_found);
    RUN_TEST(test_write_read_roundtrip);
    RUN_TEST(test_multiple_variables_roundtrip);
    RUN_TEST(test_overwrite_returns_latest_value);
    return UNITY_END();
}
