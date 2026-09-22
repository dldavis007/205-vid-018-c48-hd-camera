/*
 * hal_flash_stub.c  —  HOST-SIDE STUB
 *
 * Provides:
 *   1. fake_flash_storage   — pointer to a 256 KB region allocated in the
 *      low 4 GB of virtual address space.  It stands in for the two STM32H745
 *      flash sectors used by the EEPROM emulation layer.
 *   2. fake_flash_reset()   — helper called from test setUp() to put the
 *      region into the erased state (all 0xFF), exactly like blank flash.
 *   3. HAL_FLASH_Program()  — copies 32 bytes (one FLASHWORD) from a source
 *      buffer into the fake region at Address.
 *   4. HAL_FLASHEx_Erase()  — fills the targeted sector with 0xFF.
 *   5. HAL_FLASH_Unlock/Lock — no-ops; the host has no flash lock hardware.
 *
 * Why low 4 GB?
 *   eeprom.c stores all flash addresses as uint32_t (it targets a 32-bit MCU).
 *   On a 64-bit Windows host the default PE image base is 0x140000000, so BSS
 *   globals land above the 4 GB boundary and uint32_t truncation loses the
 *   upper bits.  VirtualAlloc with a low-address hint reliably places the
 *   buffer below 0xFFFFFFFF so the cast (uint32_t)(uintptr_t)ptr is lossless.
 *
 * Why 32 bytes per program operation?
 *   The STM32H745 flash word is 256 bits (32 bytes).  eeprom.c always
 *   passes arrays of 8 x uint32_t to HAL_FLASH_Program() and relies on
 *   both the data value and the virtual address being written as full 32-byte
 *   words separated by 32 bytes within the page.  The stub must honour that
 *   width so address-scanning loops in eeprom.c find data at the expected
 *   offsets.
 */

#include "stm32h7xx_hal.h"
#include "eeprom.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <windows.h>

/* ---- Fake flash storage ----------------------------------------------- */

/* Pointer to a VirtualAlloc'd 256 KB region; declared extern in eeprom.h */
uint8_t *fake_flash_storage = NULL;

/* ---- Public helpers ---------------------------------------------------- */

void fake_flash_reset(void)
{
    if (fake_flash_storage == NULL) {
        /*
         * Scan the low 4 GB in 16 MB steps until we get a block whose address
         * fits in a uint32_t.  eeprom.c stores all addresses as uint32_t, so
         * the buffer must live below the 4 GB boundary.
         */
        for (uintptr_t hint = 0x10000000u;
             hint + 0x40000U < 0xFF000000u;
             hint += 0x01000000u)
        {
            void *p = VirtualAlloc((LPVOID)hint, 0x40000U,
                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (p == NULL)
                continue;
            if ((uintptr_t)p <= 0xFFFFFFFFu) {
                fake_flash_storage = (uint8_t *)p;
                break;
            }
            VirtualFree(p, 0, MEM_RELEASE);
        }
        assert(fake_flash_storage != NULL);
        assert((uintptr_t)fake_flash_storage <= 0xFFFFFFFFu);
    }

    memset(fake_flash_storage, 0xFF, 0x40000U);
}

/* ---- HAL stub implementations ----------------------------------------- */

/*
 * Simulates FLASH_TYPEPROGRAM_FLASHWORD: writes exactly 32 bytes from the
 * buffer pointed to by DataAddress into the fake region at Address.
 *
 * eeprom.c passes a local uint32_t[8] array cast to uint32_t as DataAddress.
 * The stack is allocated in the same low virtual address range as the heap on
 * 64-bit Windows, so the cast to uintptr_t recovers the full pointer safely.
 */
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address,
                                    uint32_t DataAddress)
{
    (void)TypeProgram;
    memcpy((void *)(uintptr_t)Address, (void *)(uintptr_t)DataAddress, 32U);
    return HAL_OK;
}

/*
 * Simulates a sector erase: fills the relevant 128 KB page with 0xFF.
 * The sector is identified by pEraseInit->Sector matching PAGE0_ID or
 * PAGE1_ID (6 or 7 respectively, as defined in the stub eeprom.h).
 */
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit,
                                     uint32_t *SectorError)
{
    uint32_t base = (pEraseInit->Sector == PAGE0_ID) ? PAGE0_BASE_ADDRESS
                                                      : PAGE1_BASE_ADDRESS;
    memset((void *)(uintptr_t)base, 0xFF, PAGE_SIZE);
    if (SectorError) *SectorError = 0U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Lock(void)   { return HAL_OK; }
