/* pc_hal.c - the hardware layer for the PC-side host build.
 *
 * Stands in for the STM32 HAL functions the compiled firmware calls, plus the
 * two linker-script symbols it expects. GPIO is a host bit per pin, FDCAN is
 * the UDP bus in pc_side_host.c, and flash is a file-backed buffer that the
 * real eeprom.c programs and erases.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>        /* MenuFunctions.h uses bool */

#include "stm32h7xx_hal.h"
#include "eeprom.h"
#include "MenuFunctions.h"
#include "pc_side.h"
#include "pc_log.h"

/* ==========================================================================
 * Peripheral instances
 * ========================================================================== */
GPIO_TypeDef pc_gpio_ports[8] = {
    {"A", 0}, {"B", 0}, {"C", 0}, {"D", 0},
    {"E", 0}, {"F", 0}, {"G", 0}, {"H", 0},
};

FDCAN_GlobalTypeDef pc_fdcan1_instance;
TIM_TypeDef         pc_tim1_instance;
USART_TypeDef       pc_uart7_instance;
WWDG_TypeDef        pc_wwdg2_instance;

/* ==========================================================================
 * Linker-script symbols. On the target .menu_data is copied from its load
 * address in flash to RAM; in the host image it is already there, so
 * copy_menu_data_to_ram() must copy nothing: end == start.
 * ========================================================================== */
uint8_t _menu_data_load = 0;
uint8_t _smenu_data     = 0;
extern uint8_t _emenu_data __attribute__((alias("_smenu_data")));

/* ==========================================================================
 * Interrupt masking (PRIMASK) and the system tick
 * ========================================================================== */
static volatile int      intr_masked = 0;   /* enabled out of reset, as on Cortex-M */
static volatile uint32_t uwTick      = 0;

void pc_intr_set_masked(int masked) { intr_masked = masked ? 1 : 0; }
int  pc_intr_masked(void)           { return intr_masked; }

HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

void     HAL_IncTick(void) { uwTick++; }
uint32_t HAL_GetTick(void) { return uwTick; }

void HAL_Delay(uint32_t Delay) { Sleep(Delay); }

/* Real HAL_SYSTICK_IRQHandler; the callback lives in main.c and counts the
 * firmware's Timer1 / BootUpTimer / MenuTimer / IncSpeedUpTimer down. */
void HAL_SYSTICK_IRQHandler(void) { HAL_SYSTICK_Callback(); }

void HAL_NVIC_SystemReset(void) { pc_side_reset(); }

/* ==========================================================================
 * GPIO. Writes are logged by pin name, so camera, mic and OSD activity shows
 * up in the log without touching the firmware.
 * ========================================================================== */
static const struct {
    int         port;       /* index into pc_gpio_ports */
    uint16_t    pin;
    const char *name;
    int         quiet;      /* 1: state changes are not logged */
} pin_names[] = {
    {1, GPIO_PIN_6,  "MUX_S2",   1},   /* the S0..S2 trio is logged as an OSD key */
    {1, GPIO_PIN_3,  "MUX_S1",   1},
    {6, GPIO_PIN_11, "MUX_S0",   1},
    {4, GPIO_PIN_4,  "MUX_EN",   0},
    {2, GPIO_PIN_13, "MIC_SHDN", 0},
    {0, GPIO_PIN_3,  "MIC_GAIN", 0},
    {0, GPIO_PIN_2,  "WDI",      1},   /* toggles twice a second */
    {7, GPIO_PIN_5,  "RELAY",    0},
};

static int port_index(GPIO_TypeDef *GPIOx)
{
    int i;
    for (i = 0; i < 8; i++)
        if (GPIOx == &pc_gpio_ports[i]) return i;
    return -1;
}

/* setOSD() drives one key onto S2:S1:S0; decode it back for the log. */
static void log_osd_key(void)
{
    static int last = -1;
    int s2 = (pc_gpio_ports[1].pc_state & GPIO_PIN_6)  ? 1 : 0;
    int s1 = (pc_gpio_ports[1].pc_state & GPIO_PIN_3)  ? 1 : 0;
    int s0 = (pc_gpio_ports[6].pc_state & GPIO_PIN_11) ? 1 : 0;
    int code = (s2 << 2) | (s1 << 1) | s0;
    const char *key;

    if (code == last) return;
    last = code;

    switch (code) {
    case 7:  key = "none";   break;
    case 2:  key = "UP";     break;
    case 4:  key = "DOWN";   break;
    case 3:  key = "LEFT";   break;
    case 0:  key = "RIGHT";  break;
    case 1:  key = "SELECT"; break;
    default: return;         /* half-written trio, as during MX_GPIO_Init */
    }
    LOG_PRINTF(("[osd ] camera key: %s\n", key));
}

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    (void)GPIOx; (void)GPIO_Init;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    int      idx = port_index(GPIOx);
    uint16_t before;
    size_t   i;

    if (idx < 0) return;

    before = GPIOx->pc_state;
    if (PinState == GPIO_PIN_SET) GPIOx->pc_state |= GPIO_Pin;
    else                          GPIOx->pc_state &= (uint16_t)~GPIO_Pin;

    if (((before ^ GPIOx->pc_state) & GPIO_Pin) == 0) return;

    for (i = 0; i < sizeof pin_names / sizeof pin_names[0]; i++) {
        if (pin_names[i].port != idx || pin_names[i].pin != GPIO_Pin) continue;
        if (!pin_names[i].quiet)
            LOG_PRINTF(("[gpio] %-8s -> %s\n", pin_names[i].name,
                        PinState == GPIO_PIN_SET ? "SET" : "RESET"));
        break;
    }
    log_osd_key();
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    int idx = port_index(GPIOx);
    if (idx < 0) return GPIO_PIN_RESET;
    return (GPIOx->pc_state & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/* ==========================================================================
 * FDCAN. The UDP bus carries every frame on the hub, so the receive filters
 * mco.c installs are applied here, as the controller's hardware would.
 * ========================================================================== */
#define PC_MAX_FILTERS 16

static int      fdcan_started   = 0;
static int      reject_non_matching = 0;
static int      filter_count    = 0;
static struct { uint32_t id, mask; } filters[PC_MAX_FILTERS];

HAL_StatusTypeDef HAL_FDCAN_Init(FDCAN_HandleTypeDef *hfdcan)
{
    (void)hfdcan;
    fdcan_started       = 0;
    filter_count        = 0;
    reject_non_matching = 0;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan)
{
    (void)hfdcan;
    fdcan_started = 1;
    LOG_PRINTF(("[can ] FDCAN started, %d receive filter(s)\n", filter_count));
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan,
                                               uint32_t NonMatchingStd,
                                               uint32_t NonMatchingExt,
                                               uint32_t RejectRemoteStd,
                                               uint32_t RejectRemoteExt)
{
    (void)hfdcan; (void)NonMatchingExt; (void)RejectRemoteStd; (void)RejectRemoteExt;
    reject_non_matching = (NonMatchingStd == FDCAN_REJECT);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan,
                                         FDCAN_FilterTypeDef *sFilterConfig)
{
    (void)hfdcan;
    if (filter_count >= PC_MAX_FILTERS) return HAL_ERROR;
    /* FilterType is always FDCAN_FILTER_MASK here: ID1 is the value, ID2 the mask. */
    filters[filter_count].id   = sFilterConfig->FilterID1;
    filters[filter_count].mask = sFilterConfig->FilterID2;
    filter_count++;
    return HAL_OK;
}

/* Called by the receive thread for every datagram off the bus. */
int pc_can_filter_accept(uint16_t id)
{
    int i;
    if (!fdcan_started)       return 0;
    if (!reject_non_matching) return 1;
    for (i = 0; i < filter_count; i++)
        if ((id & filters[i].mask) == (filters[i].id & filters[i].mask)) return 1;
    return 0;
}

uint32_t HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *hfdcan)
{
    (void)hfdcan;
    return fdcan_started ? 8U : 0U;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                FDCAN_TxHeaderTypeDef *pTxHeader,
                                                uint8_t *pTxData)
{
    (void)hfdcan;
    if (!fdcan_started) return HAL_ERROR;
    /* DataLength is the DLC, which is the byte count for classic CAN frames. */
    if (!pc_can_send((uint16_t)pTxHeader->Identifier, pTxData, (int)pTxHeader->DataLength))
        return HAL_ERROR;
    return HAL_OK;
}

uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo)
{
    (void)hfdcan;
    if (!fdcan_started || RxFifo != FDCAN_RX_FIFO0) return 0U;
    return (uint32_t)pc_can_rx_level();
}

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                         FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData)
{
    uint16_t id;
    int      len;

    (void)hfdcan;
    if (RxLocation != FDCAN_RX_FIFO0)          return HAL_ERROR;
    if (!pc_can_rx_pop(&id, pRxData, &len))    return HAL_ERROR;

    memset(pRxHeader, 0, sizeof *pRxHeader);
    pRxHeader->Identifier = id;
    pRxHeader->IdType     = FDCAN_STANDARD_ID;
    pRxHeader->DataLength = (uint32_t)len;     /* DLC == length for 0..8 bytes */
    return HAL_OK;
}

/* ==========================================================================
 * TIM1: the 0.5 s hardware-watchdog tick. The RTI thread raises the interrupt
 * once HAL_TIM_Base_Start_IT has run.
 * ========================================================================== */
static int tim1_it_started = 0;

HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim) { (void)htim; return HAL_OK; }

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)
{
    (void)htim;
    tim1_it_started = 1;
    return HAL_OK;
}

int pc_tim1_it_enabled(void) { return tim1_it_started; }

HAL_StatusTypeDef HAL_TIM_ConfigClockSource(TIM_HandleTypeDef *htim,
                                            TIM_ClockConfigTypeDef *sClockSourceConfig)
{
    (void)htim; (void)sClockSourceConfig;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
                                                        TIM_MasterConfigTypeDef *sMasterConfig)
{
    (void)htim; (void)sMasterConfig;
    return HAL_OK;
}

void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim) { HAL_TIM_PeriodElapsedCallback(htim); }

/* ==========================================================================
 * UART7: the firmware's printf target. Buffered to whole lines.
 * ========================================================================== */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart) { (void)huart; return HAL_OK; }

HAL_StatusTypeDef HAL_UARTEx_SetTxFifoThreshold(UART_HandleTypeDef *huart, uint32_t t)
{ (void)huart; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_UARTEx_SetRxFifoThreshold(UART_HandleTypeDef *huart, uint32_t t)
{ (void)huart; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_UARTEx_DisableFifoMode(UART_HandleTypeDef *huart)
{ (void)huart; return HAL_OK; }

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout)
{
    static char line[200];
    static int  n = 0;
    uint16_t    i;

    (void)huart; (void)Timeout;
    for (i = 0; i < Size; i++) {
        char c = (char)pData[i];
        if (c == '\n' || n == (int)sizeof line - 1) {
            line[n] = '\0';
            LOG_PRINTF(("[uart] %s\n", line));
            n = 0;
        } else if (c != '\r') {
            line[n++] = c;
        }
    }
    return HAL_OK;
}

/* ==========================================================================
 * WWDG. Refresh is the one call the firmware makes once per main-loop pass
 * (COP_Trig), so it is where the host counts loops.
 * ========================================================================== */
HAL_StatusTypeDef HAL_WWDG_Init(WWDG_HandleTypeDef *hwwdg) { (void)hwwdg; return HAL_OK; }

HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg)
{
    (void)hwwdg;
    pc_loop_tick();
    return HAL_OK;
}

void HAL_WWDG_IRQHandler(WWDG_HandleTypeDef *hwwdg) { HAL_WWDG_EarlyWakeupCallback(hwwdg); }

/* ==========================================================================
 * RCC / PWR: clock tree bring-up has nothing to do on a PC.
 * ========================================================================== */
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *s)   { (void)s; return HAL_OK; }
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *s, uint32_t l)
{ (void)s; (void)l; return HAL_OK; }
void HAL_RCC_MCOConfig(uint32_t x, uint32_t s, uint32_t d) { (void)x; (void)s; (void)d; }
void HAL_RCC_EnableCSS(void) { }
HAL_StatusTypeDef HAL_PWREx_ConfigSupply(uint32_t s) { (void)s; return HAL_OK; }

/* ==========================================================================
 * Flash. eeprom.c keeps every address in a uint32_t, which is lossless here
 * because the host build is -m32. Backed by flash_<recv port>.bin, so each
 * instance on the bus keeps its own camera address and settings across a reset.
 * ========================================================================== */
#define PC_FLASH_SIZE (2U * PAGE_SIZE)

static uint8_t flash_storage[PC_FLASH_SIZE];
uint8_t *pc_fake_flash = NULL;

static char flash_path[MAX_PATH];
static int  flash_dirty = 0;

int pc_flash_init(unsigned short recv_port)
{
    FILE *f;

    pc_fake_flash = flash_storage;
    memset(pc_fake_flash, 0xFF, PC_FLASH_SIZE);   /* erased flash */

    snprintf(flash_path, sizeof flash_path, "flash_%u.bin", (unsigned)recv_port);

    f = fopen(flash_path, "rb");
    if (f) {
        size_t n = fread(pc_fake_flash, 1, PC_FLASH_SIZE, f);
        fclose(f);
        LOG_PRINTF(("[ee  ] %s loaded (%u bytes)\n", flash_path, (unsigned)n));
    } else {
        LOG_PRINTF(("[ee  ] %s not found: starting on blank flash\n", flash_path));
    }
    return 0;
}

void pc_flash_sync(void)
{
    FILE *f;
    if (!flash_dirty || !pc_fake_flash) return;
    f = fopen(flash_path, "wb");
    if (!f) {
        LOG_PRINTF(("[ee  ] cannot write %s\n", flash_path));
        return;
    }
    fwrite(pc_fake_flash, 1, PC_FLASH_SIZE, f);
    fclose(f);
    flash_dirty = 0;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t DataAddress)
{
    (void)TypeProgram;
    /* One STM32H7 flash word is 256 bits; eeprom.c always passes uint32_t[8]. */
    memcpy((void *)(uintptr_t)Address, (const void *)(uintptr_t)DataAddress, 32U);
    flash_dirty = 1;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError)
{
    uint32_t base = (pEraseInit->Sector == PAGE0_ID) ? PAGE0_BASE_ADDRESS
                                                     : PAGE1_BASE_ADDRESS;
    memset((void *)(uintptr_t)base, 0xFF, PAGE_SIZE);
    flash_dirty = 1;
    if (SectorError) *SectorError = 0U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void) { return HAL_OK; }

/* The firmware brackets every write with Unlock/Lock, so this is where the
 * backing file is refreshed. */
HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
    pc_flash_sync();
    return HAL_OK;
}

/* ==========================================================================
 * Menu-variable block check. Save_Variables / Load_Variables walk from
 * NullVar to NullVar2 in sizeof(struct menu_var) steps, which only works if
 * the .menu_data globals stay contiguous and in declaration order.
 * Also makes the section writable: PE may map an unknown section read-only.
 * ========================================================================== */
extern struct menu_var NullVar, CamTag, disp_add, MicOnOff, TriggerMode, NullVar2;

int pc_menu_data_check(void)
{
    struct menu_var *const expect[] = {&NullVar, &CamTag, &disp_add, &MicOnOff, &TriggerMode, &NullVar2};
    size_t i;
    DWORD  old;
    SIZE_T span = (SIZE_T)((char *)&NullVar2 - (char *)&NullVar) + sizeof(struct menu_var);

    for (i = 1; i < sizeof expect / sizeof expect[0]; i++) {
        if (expect[i] != expect[i - 1] + 1) {
            LOG_PRINTF(("[fatal] .menu_data is not contiguous: var %u is %+ld bytes "
                        "from the previous one, expected %u\n",
                        (unsigned)i, (long)((char *)expect[i] - (char *)expect[i - 1]),
                        (unsigned)sizeof(struct menu_var)));
            return 1;
        }
    }

    VirtualProtect(&NullVar, span, PAGE_READWRITE, &old);
    return 0;
}
