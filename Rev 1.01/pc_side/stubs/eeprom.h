/* eeprom.h - HOST STAND-IN for the PC-side host build.
 *
 * Same contents as Drivers/EEPROM/Inc/eeprom.h, except that the flash
 * addresses point at pc_fake_flash (pc_hal.c) instead of bank 2 sector 6.
 * The real eeprom.c compiles against this unchanged, so the host runs the
 * production EEPROM emulation on a file-backed buffer.
 */

#ifndef __EEPROM_H
#define __EEPROM_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* EEPROM emulation firmware error codes */
#define EE_OK      (uint32_t)HAL_OK
#define EE_ERROR   (uint32_t)HAL_ERROR
#define EE_BUSY    (uint32_t)HAL_BUSY
#define EE_TIMEOUT (uint32_t)HAL_TIMEOUT

#define PAGE_SIZE     (uint32_t)0x20000   /* Page size = 128KByte */
#define VOLTAGE_RANGE (uint8_t)VOLTAGE_RANGE_3

/* Host flash: 2 x PAGE_SIZE, allocated below 4 GB (eeprom.c keeps every
 * address in a uint32_t). Allocated by pc_flash_init(). */
extern uint8_t *pc_fake_flash;

#define EEPROM_START_ADDRESS ((uint32_t)(uintptr_t)pc_fake_flash)

#define PAGE0_BASE_ADDRESS ((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS  ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))
#define PAGE0_ID           FLASH_SECTOR_6

#define PAGE1_BASE_ADDRESS ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS  ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))
#define PAGE1_ID           FLASH_SECTOR_7

#define PAGE0 ((uint16_t)0x0000)
#define PAGE1 ((uint16_t)0x0001)

#define NO_VALID_PAGE ((uint16_t)0x00AB)

#define ERASED       ((uint16_t)0xFFFF)
#define RECEIVE_DATA ((uint16_t)0xEEEE)
#define VALID_PAGE   ((uint16_t)0x0000)

#define READ_FROM_VALID_PAGE ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE  ((uint8_t)0x01)

#define PAGE_FULL ((uint8_t)0x80)

#define NB_OF_VAR 161

uint16_t EE_Init(void);
uint16_t EE_ReadVariable(uint16_t VirtAddress, uint16_t *Data);
uint16_t EE_WriteVariable(uint16_t VirtAddress, uint16_t Data);

#endif /* __EEPROM_H */
