/* stm32h7xx_hal.h - HOST STAND-IN for the PC-side host build.
 *
 * -Ipc_side/stubs comes first, so every #include "stm32h7xx_hal.h" in the
 * firmware lands here instead of the real HAL. Only what the compiled TUs
 * touch is declared; the functions live in pc_hal.c.
 */

#ifndef PC_SIDE_STM32H7XX_HAL_H
#define PC_SIDE_STM32H7XX_HAL_H

#include <stdint.h>
#include <stddef.h>

#define __IO volatile
#define __weak
#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

/* ---- CMSIS intrinsics -------------------------------------------------- */
/* Interrupt masking is a flag, as on the target's PRIMASK: the RTI thread
 * defers ticks while it is set. NVIC_SystemReset relaunches the process. */
void pc_intr_set_masked(int masked);
void pc_side_reset(void);

#define __disable_irq()     pc_intr_set_masked(1)
#define __enable_irq()      pc_intr_set_masked(0)
#define __BKPT(...)         ((void)0)
#define __NOP()             ((void)0)
#define NVIC_SystemReset()  pc_side_reset()

/* ---- Common types ------------------------------------------------------ */
typedef enum { HAL_OK = 0x00U, HAL_ERROR = 0x01U,
               HAL_BUSY = 0x02U, HAL_TIMEOUT = 0x03U } HAL_StatusTypeDef;

typedef enum { DISABLE = 0U, ENABLE = 1U } FunctionalState;

#define HAL_MAX_DELAY 0xFFFFFFFFU

/* ---- GPIO -------------------------------------------------------------- */
/* Each port is a host struct; the pin state a WritePin sets is what a
 * ReadPin sees, which is all doevents() needs from RELAY_Pin. */
typedef struct {
    const char *pc_name;
    uint16_t    pc_state;
} GPIO_TypeDef;

extern GPIO_TypeDef pc_gpio_ports[8];

#define GPIOA (&pc_gpio_ports[0])
#define GPIOB (&pc_gpio_ports[1])
#define GPIOC (&pc_gpio_ports[2])
#define GPIOD (&pc_gpio_ports[3])
#define GPIOE (&pc_gpio_ports[4])
#define GPIOF (&pc_gpio_ports[5])
#define GPIOG (&pc_gpio_ports[6])
#define GPIOH (&pc_gpio_ports[7])

typedef enum { GPIO_PIN_RESET = 0U, GPIO_PIN_SET = 1U } GPIO_PinState;

#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)
#define GPIO_PIN_4  ((uint16_t)0x0010)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_6  ((uint16_t)0x0040)
#define GPIO_PIN_7  ((uint16_t)0x0080)
#define GPIO_PIN_8  ((uint16_t)0x0100)
#define GPIO_PIN_9  ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)

#define GPIO_MODE_INPUT      0x00000000U
#define GPIO_MODE_OUTPUT_PP  0x00000001U
#define GPIO_MODE_OUTPUT_OD  0x00000011U
#define GPIO_NOPULL          0x00000000U
#define GPIO_PULLUP          0x00000001U
#define GPIO_PULLDOWN        0x00000002U
#define GPIO_SPEED_FREQ_LOW  0x00000000U

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
} GPIO_InitTypeDef;

void        HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void        HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- FDCAN ------------------------------------------------------------- */
typedef struct { uint32_t pc_dummy; } FDCAN_GlobalTypeDef;
extern FDCAN_GlobalTypeDef pc_fdcan1_instance;
#define FDCAN1 (&pc_fdcan1_instance)

#define FDCAN_FRAME_CLASSIC       0x00000000U
#define FDCAN_MODE_NORMAL         0x00000000U
#define FDCAN_STANDARD_ID         0x00000000U
#define FDCAN_EXTENDED_ID         0x40000000U
#define FDCAN_DATA_FRAME          0x00000000U
#define FDCAN_REMOTE_FRAME        0x20000000U
#define FDCAN_ESI_ACTIVE          0x00000000U
#define FDCAN_BRS_OFF             0x00000000U
#define FDCAN_CLASSIC_CAN         0x00000000U
#define FDCAN_NO_TX_EVENTS        0x00000000U
#define FDCAN_DATA_BYTES_8        0x00000008U
#define FDCAN_TX_FIFO_OPERATION   0x00000000U
#define FDCAN_RX_FIFO0            0x00000040U
#define FDCAN_RX_FIFO1            0x00000041U

/* Values matter: mcohw.c indexes dlc_to_len[] with RxHeader.DataLength. */
#define FDCAN_DLC_BYTES_0 ((uint32_t)0x00000000U)
#define FDCAN_DLC_BYTES_1 ((uint32_t)0x00000001U)
#define FDCAN_DLC_BYTES_2 ((uint32_t)0x00000002U)
#define FDCAN_DLC_BYTES_3 ((uint32_t)0x00000003U)
#define FDCAN_DLC_BYTES_4 ((uint32_t)0x00000004U)
#define FDCAN_DLC_BYTES_5 ((uint32_t)0x00000005U)
#define FDCAN_DLC_BYTES_6 ((uint32_t)0x00000006U)
#define FDCAN_DLC_BYTES_7 ((uint32_t)0x00000007U)
#define FDCAN_DLC_BYTES_8 ((uint32_t)0x00000008U)

#define FDCAN_ACCEPT_IN_RX_FIFO0  0x00000001U
#define FDCAN_FILTER_TO_RXFIFO0   0x00000001U
#define FDCAN_FILTER_RANGE        0x00000000U
#define FDCAN_FILTER_DUAL         0x00000001U
#define FDCAN_FILTER_MASK         0x00000002U
#define FDCAN_ACCEPT_NON_MATCHING 0x00000000U
#define FDCAN_REJECT              0x00000002U
#define FDCAN_FILTER_REMOTE       0x00000000U
#define FDCAN_REJECT_REMOTE       0x00000001U

typedef struct {
    uint32_t FrameFormat;
    uint32_t Mode;
    FunctionalState AutoRetransmission;
    FunctionalState TransmitPause;
    FunctionalState ProtocolException;
    uint32_t NominalPrescaler;
    uint32_t NominalSyncJumpWidth;
    uint32_t NominalTimeSeg1;
    uint32_t NominalTimeSeg2;
    uint32_t DataPrescaler;
    uint32_t DataSyncJumpWidth;
    uint32_t DataTimeSeg1;
    uint32_t DataTimeSeg2;
    uint32_t MessageRAMOffset;
    uint32_t StdFiltersNbr;
    uint32_t ExtFiltersNbr;
    uint32_t RxFifo0ElmtsNbr;
    uint32_t RxFifo0ElmtSize;
    uint32_t RxFifo1ElmtsNbr;
    uint32_t RxFifo1ElmtSize;
    uint32_t RxBuffersNbr;
    uint32_t RxBufferSize;
    uint32_t TxEventsNbr;
    uint32_t TxBuffersNbr;
    uint32_t TxFifoQueueElmtsNbr;
    uint32_t TxFifoQueueMode;
    uint32_t TxElmtSize;
} FDCAN_InitTypeDef;

typedef struct {
    FDCAN_GlobalTypeDef *Instance;
    FDCAN_InitTypeDef    Init;
} FDCAN_HandleTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t RxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t RxTimestamp;
    uint32_t FilterIndex;
    uint32_t IsFilterMatchingFrame;
} FDCAN_RxHeaderTypeDef;

typedef struct {
    uint32_t IdType;
    uint32_t FilterIndex;
    uint32_t FilterType;
    uint32_t FilterConfig;
    uint32_t FilterID1;
    uint32_t FilterID2;
    uint32_t IsCalibrationMsg;
    uint32_t RxBufferIndex;
} FDCAN_FilterTypeDef;

typedef struct {
    uint32_t LastErrorCode;
    uint32_t DataLastErrorCode;
    uint32_t Activity;
    uint32_t ErrorPassive;
    uint32_t Warning;
    uint32_t BusOff;
} FDCAN_ProtocolStatusTypeDef;

HAL_StatusTypeDef HAL_FDCAN_Init(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan,
                                         FDCAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan,
                                               uint32_t NonMatchingStd,
                                               uint32_t NonMatchingExt,
                                               uint32_t RejectRemoteStd,
                                               uint32_t RejectRemoteExt);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                FDCAN_TxHeaderTypeDef *pTxHeader,
                                                uint8_t *pTxData);
uint32_t          HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *hfdcan);
uint32_t          HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                         FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData);

/* ---- TIM --------------------------------------------------------------- */
typedef struct { uint32_t pc_dummy; } TIM_TypeDef;
extern TIM_TypeDef pc_tim1_instance;
#define TIM1 (&pc_tim1_instance)

#define TIM_COUNTERMODE_UP               0x00000000U
#define TIM_CLOCKDIVISION_DIV1           0x00000000U
#define TIM_AUTORELOAD_PRELOAD_DISABLE   0x00000000U
#define TIM_CLOCKSOURCE_INTERNAL         0x00000000U
#define TIM_TRGO_RESET                   0x00000000U
#define TIM_TRGO2_RESET                  0x00000000U
#define TIM_MASTERSLAVEMODE_DISABLE      0x00000000U

typedef struct {
    uint32_t Prescaler;
    uint32_t CounterMode;
    uint32_t Period;
    uint32_t ClockDivision;
    uint32_t RepetitionCounter;
    uint32_t AutoReloadPreload;
} TIM_Base_InitTypeDef;

typedef struct {
    TIM_TypeDef         *Instance;
    TIM_Base_InitTypeDef Init;
} TIM_HandleTypeDef;

typedef struct {
    uint32_t ClockSource;
    uint32_t ClockPolarity;
    uint32_t ClockPrescaler;
    uint32_t ClockFilter;
} TIM_ClockConfigTypeDef;

typedef struct {
    uint32_t MasterOutputTrigger;
    uint32_t MasterOutputTrigger2;
    uint32_t MasterSlaveMode;
} TIM_MasterConfigTypeDef;

HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_ConfigClockSource(TIM_HandleTypeDef *htim,
                                            TIM_ClockConfigTypeDef *sClockSourceConfig);
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
                                                        TIM_MasterConfigTypeDef *sMasterConfig);
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

/* ---- UART -------------------------------------------------------------- */
typedef struct { uint32_t pc_dummy; } USART_TypeDef;
extern USART_TypeDef pc_uart7_instance;
#define UART7 (&pc_uart7_instance)

#define UART_WORDLENGTH_8B            0x00000000U
#define UART_STOPBITS_1               0x00000000U
#define UART_PARITY_NONE              0x00000000U
#define UART_MODE_TX_RX               0x0000000CU
#define UART_HWCONTROL_NONE           0x00000000U
#define UART_OVERSAMPLING_16          0x00000000U
#define UART_ONE_BIT_SAMPLE_DISABLE   0x00000000U
#define UART_PRESCALER_DIV1           0x00000000U
#define UART_ADVFEATURE_NO_INIT       0x00000000U
#define UART_TXFIFO_THRESHOLD_1_8     0x00000000U
#define UART_RXFIFO_THRESHOLD_1_8     0x00000000U

typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
    uint32_t OneBitSampling;
    uint32_t ClockPrescaler;
} UART_InitTypeDef;

typedef struct {
    uint32_t AdvFeatureInit;
} UART_AdvFeatureInitTypeDef;

typedef struct {
    USART_TypeDef             *Instance;
    UART_InitTypeDef           Init;
    UART_AdvFeatureInitTypeDef AdvancedInit;
} UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UARTEx_SetTxFifoThreshold(UART_HandleTypeDef *huart, uint32_t Threshold);
HAL_StatusTypeDef HAL_UARTEx_SetRxFifoThreshold(UART_HandleTypeDef *huart, uint32_t Threshold);
HAL_StatusTypeDef HAL_UARTEx_DisableFifoMode(UART_HandleTypeDef *huart);

/* ---- WWDG -------------------------------------------------------------- */
typedef struct { uint32_t pc_dummy; } WWDG_TypeDef;
extern WWDG_TypeDef pc_wwdg2_instance;
#define WWDG2 (&pc_wwdg2_instance)

#define WWDG_PRESCALER_128  0x00000700U
#define WWDG_EWI_ENABLE     0x00000200U

typedef struct {
    uint32_t Prescaler;
    uint32_t Window;
    uint32_t Counter;
    uint32_t EWIMode;
} WWDG_InitTypeDef;

typedef struct {
    WWDG_TypeDef    *Instance;
    WWDG_InitTypeDef Init;
} WWDG_HandleTypeDef;

HAL_StatusTypeDef HAL_WWDG_Init(WWDG_HandleTypeDef *hwwdg);
HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg);
void HAL_WWDG_IRQHandler(WWDG_HandleTypeDef *hwwdg);
void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg);

/* ---- RCC / PWR --------------------------------------------------------- */
#define RCC_OSCILLATORTYPE_HSE   0x00000001U
#define RCC_HSE_ON               0x00010000U
#define RCC_PLL_ON               0x00000002U
#define RCC_PLLSOURCE_HSE        0x00000002U
#define RCC_PLL1VCIRANGE_3       0x0000000CU
#define RCC_PLL1VCOWIDE          0x00000000U
#define RCC_CLOCKTYPE_SYSCLK     0x00000001U
#define RCC_CLOCKTYPE_HCLK       0x00000002U
#define RCC_CLOCKTYPE_D1PCLK1    0x00000004U
#define RCC_CLOCKTYPE_PCLK1      0x00000008U
#define RCC_CLOCKTYPE_PCLK2      0x00000010U
#define RCC_CLOCKTYPE_D3PCLK1    0x00000020U
#define RCC_SYSCLKSOURCE_PLLCLK  0x00000003U
#define RCC_SYSCLK_DIV1          0x00000000U
#define RCC_HCLK_DIV2            0x00000008U
#define RCC_APB1_DIV4            0x00000005U
#define RCC_APB2_DIV2            0x00000004U
#define RCC_APB3_DIV2            0x00000004U
#define RCC_APB4_DIV2            0x00000004U
#define RCC_MCO2                 0x00000001U
#define RCC_MCO2SOURCE_SYSCLK    0x00000000U
#define RCC_MCODIV_4             0x04000000U

#define PWR_DIRECT_SMPS_SUPPLY        0x00000004U
#define PWR_REGULATOR_VOLTAGE_SCALE3  0x00000000U
#define PWR_FLAG_VOSRDY               0x00000001U

#define FLASH_LATENCY_2  0x00000002U

typedef struct {
    uint32_t PLLState;
    uint32_t PLLSource;
    uint32_t PLLM;
    uint32_t PLLN;
    uint32_t PLLP;
    uint32_t PLLQ;
    uint32_t PLLR;
    uint32_t PLLRGE;
    uint32_t PLLVCOSEL;
    uint32_t PLLFRACN;
} RCC_PLLInitTypeDef;

typedef struct {
    uint32_t           OscillatorType;
    uint32_t           HSEState;
    uint32_t           LSEState;
    uint32_t           HSIState;
    RCC_PLLInitTypeDef PLL;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;
    uint32_t SYSCLKDivider;
    uint32_t AHBCLKDivider;
    uint32_t APB3CLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
    uint32_t APB4CLKDivider;
} RCC_ClkInitTypeDef;

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency);
void HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv);
void HAL_RCC_EnableCSS(void);
HAL_StatusTypeDef HAL_PWREx_ConfigSupply(uint32_t SupplySource);

/* Clock-enable and voltage-scaling macros poke registers on the target. */
#define __HAL_RCC_GPIOA_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOD_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOE_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOF_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOG_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_GPIOH_CLK_ENABLE()  ((void)0)
#define __HAL_RCC_HSEM_CLK_ENABLE()   ((void)0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(x)  ((void)(x))
#define __HAL_PWR_GET_FLAG(x)               (1)   /* VOSRDY: never spin */

/* ---- FLASH ------------------------------------------------------------- */
#define FLASH_TYPEPROGRAM_FLASHWORD  0x03U
#define FLASH_TYPEERASE_SECTORS      0x01U
#define TYPEERASE_SECTORS            0x01U
#define FLASH_BANK_2                 0x02U
#define FLASH_SECTOR_6               6U
#define FLASH_SECTOR_7               7U
#define VOLTAGE_RANGE_3              0x03U

#define FLASH_FLAG_EOP    (1U << 0)
#define FLASH_FLAG_OPERR  (1U << 1)
#define FLASH_FLAG_WRPERR (1U << 2)
#define FLASH_FLAG_PGSERR (1U << 3)

#define __HAL_FLASH_CLEAR_FLAG(flags) ((void)(flags))

typedef struct {
    uint32_t TypeErase;
    uint32_t Banks;
    uint32_t Sector;
    uint32_t NbSectors;
    uint32_t VoltageRange;
} FLASH_EraseInitTypeDef;

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t DataAddress);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError);
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);

/* ---- Core / tick ------------------------------------------------------- */
HAL_StatusTypeDef HAL_Init(void);
void     HAL_IncTick(void);
uint32_t HAL_GetTick(void);
void     HAL_Delay(uint32_t Delay);
void     HAL_SYSTICK_IRQHandler(void);
void     HAL_SYSTICK_Callback(void);
void     HAL_NVIC_SystemReset(void);

#endif /* PC_SIDE_STM32H7XX_HAL_H */
