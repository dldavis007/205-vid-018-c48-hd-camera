/*
 * eeprom.h  —  HOST-SIDE STUB
 *
 * Replaces Drivers/EEPROM/Inc/eeprom.h during host-side unit testing.
 *
 * The critical difference from the real header: EEPROM_START_ADDRESS (and the
 * page addresses derived from it) are redirected to a plain C array called
 * fake_flash_storage[] that lives in hal_flash_stub.c.  All of the address
 * arithmetic inside eeprom.c then operates on real host memory instead of
 * trying to dereference STM32 flash addresses, which would segfault on a PC.
 *
 * 64-bit caveat: eeprom.c stores flash addresses in uint32_t variables.  On a
 * 64-bit host the cast (uint32_t)(uintptr_t)fake_flash_storage is safe only
 * when the array's address fits in 32 bits.  On Windows with MinGW, BSS/data
 * allocations are placed below 2 GB by the PE loader, so this holds in
 * practice.  hal_flash_stub.c verifies it at runtime with an assertion.
 */

#ifndef STUB_EEPROM_H
#define STUB_EEPROM_H

#include "stm32h7xx_hal.h"   /* picks up tests/stubs/stm32h7xx_hal.h */
#include <stdint.h>

/* ---- EE status codes (identical to real header) ----------------------- */
#define EE_OK      (uint32_t)HAL_OK
#define EE_ERROR   (uint32_t)HAL_ERROR
#define EE_BUSY    (uint32_t)HAL_BUSY
#define EE_TIMEOUT (uint32_t)HAL_TIMEOUT

/* ---- Flash page geometry (identical to real header) ------------------- */
#define PAGE_SIZE             ((uint32_t)0x20000U)   /* 128 KB per page */

#define VOLTAGE_RANGE         ((uint8_t)VOLTAGE_RANGE_3)

/*
 * Host-side fake flash: two contiguous pages totalling 256 KB.
 * Declared here so the PAGE*_BASE_ADDRESS macros can reference it.
 * Defined (and initialised) in hal_flash_stub.c.
 */
extern uint8_t *fake_flash_storage;   /* 2 × PAGE_SIZE, allocated in low 4 GB by fake_flash_reset() */

/*
 * Redirect flash addresses to the host array.
 * The cast chain uint8_t* → uintptr_t → uint32_t is necessary because
 * eeprom.c treats all addresses as uint32_t internally.
 */
#define EEPROM_START_ADDRESS  ((uint32_t)(uintptr_t)fake_flash_storage)

#define PAGE0_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + 0x0000U))
#define PAGE0_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1U)))
#define PAGE0_ID              FLASH_SECTOR_6   /* == 6U */

#define PAGE1_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (2U * PAGE_SIZE - 1U)))
#define PAGE1_ID              FLASH_SECTOR_7   /* == 7U */

/* ---- Page / variable status values (identical to real header) --------- */
#define PAGE0                 ((uint16_t)0x0000)
#define PAGE1                 ((uint16_t)0x0001)
#define NO_VALID_PAGE         ((uint16_t)0x00AB)

#define ERASED                ((uint16_t)0xFFFF)
#define RECEIVE_DATA          ((uint16_t)0xEEEE)
#define VALID_PAGE            ((uint16_t)0x0000)

#define READ_FROM_VALID_PAGE  ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE   ((uint8_t)0x01)

#define PAGE_FULL             ((uint8_t)0x80)

/* ---- Variable count (identical to real header) ------------------------ */
#define NB_OF_VAR             161

/* ---- Public API (identical signatures to real header) ----------------- */
uint16_t EE_Init(void);
uint16_t EE_ReadVariable(uint16_t VirtAddress, uint16_t *Data);
uint16_t EE_WriteVariable(uint16_t VirtAddress, uint16_t Data);

#endif /* STUB_EEPROM_H */
