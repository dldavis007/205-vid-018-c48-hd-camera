/*
 * stm32h7xx_hal.h  —  HOST-SIDE STUB
 *
 * Replaces the real STM32 HAL header when compiling firmware modules on a
 * Windows host for unit testing.  The include-path trick puts tests/stubs/
 * BEFORE the real driver directories, so the compiler finds this file first.
 *
 * Only the types and macros actually consumed by the files under test are
 * defined here.
 */

#ifndef STUB_STM32H7XX_HAL_H
#define STUB_STM32H7XX_HAL_H

#include <stdint.h>
#include <stddef.h>

/* STM32 uses __IO as a volatile qualifier on memory-mapped registers */
#define __IO volatile

/*
 * nodecfg.h defines MEM_NEAR/MEM_FAR as the 8051 keywords 'data'/'xdata'.
 * Neutralise the macros directly so 'data' remains usable as a variable name.
 * #undef first to suppress redefinition warnings regardless of include order.
 */
// #undef  MEM_NEAR
// #define MEM_NEAR
// #undef  MEM_FAR
// #define MEM_FAR

/* ---- HAL status -------------------------------------------------------- */
typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

/* ---- Flash programming / erase types ----------------------------------- */
/*
 * eeprom.c uses TYPEERASE_SECTORS (EE_Init, EE_PageTransfer) and
 * FLASH_TYPEERASE_SECTORS (EE_Format) as aliases for the same operation;
 * both are defined here so both spellings compile without errors.
 */
#define FLASH_TYPEPROGRAM_FLASHWORD   0x03U
#define FLASH_TYPEERASE_SECTORS       0x01U
#define TYPEERASE_SECTORS             0x01U

/* ---- Flash bank / sector IDs ------------------------------------------ */
#define FLASH_BANK_2                  0x02U
#define FLASH_SECTOR_6                6U
#define FLASH_SECTOR_7                7U

/* ---- Voltage range ---------------------------------------------------- */
#define VOLTAGE_RANGE_3               0x03U

/* ---- Flash status flags (used in __HAL_FLASH_CLEAR_FLAG) -------------- */
#define FLASH_FLAG_EOP                (1U << 0)
#define FLASH_FLAG_OPERR              (1U << 1)
#define FLASH_FLAG_WRPERR             (1U << 2)
#define FLASH_FLAG_PGSERR             (1U << 3)

/* The stub flash never sets real hardware flags; clear is a no-op */
#define __HAL_FLASH_CLEAR_FLAG(flags) ((void)(flags))

/* ---- Flash erase parameter struct ------------------------------------- */
typedef struct {
    uint32_t TypeErase;
    uint32_t Banks;
    uint32_t Sector;
    uint32_t NbSectors;
    uint32_t VoltageRange;
} FLASH_EraseInitTypeDef;

/* ---- HAL function declarations (implemented in hal_flash_stub.c) ------ */
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address,
                                    uint32_t DataAddress);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit,
                                     uint32_t *SectorError);
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);

#endif /* STUB_STM32H7XX_HAL_H */
