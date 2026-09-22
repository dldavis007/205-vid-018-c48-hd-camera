/**************************************************************************
MODULE:    MCOHW
CONTAINS:  Preliminary, limited hardware driver implementation for
           the 2-Wire Controller
           This version re-uses functions provided by
           www.esacademy.com/faq/progs
           If you need a full-featured 8XC591 MicroCANopen driver, contact
           support@esacademy.com
COPYRIGHT: Embedded Systems Academy, Inc. 2002-2005.
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   THIS IS THE EDUCATIONAL VERSION OF MICROCANOPEN
           See file license_educational.txt or
           www.microcanopen.com/license_educational.txt
           A commercial MicroCANopen license is available at
           www.CANopenStore.com
VERSION:   2.10, ESA 12-JAN-05
           $LastChangedDate: 2005-01-12 13:53:59 -0700 (Wed, 12 Jan 2005) $
           $LastChangedRevision: 48 $
---------------------------------------------------------------------------
*/
#include "mcohw.h"
#include "Subroutines.h"
#include "main.h"


uint8_t dlc_to_len[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

FDCAN_TxHeaderTypeDef TxHeader = {
  .IdType = FDCAN_STANDARD_ID,
  .TxFrameType = FDCAN_DATA_FRAME,
  .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
  .BitRateSwitch = FDCAN_BRS_OFF,
  .FDFormat = FDCAN_CLASSIC_CAN,
  .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
  .MessageMarker = 0
};

extern FDCAN_HandleTypeDef hfdcan1;
extern uint32_t Timer1;


/**************************************************************************
DOES:    Gets the value of the current 1 millisecond system timer 
RETURNS: The current timer tick                                   
**************************************************************************/
UNSIGNED32 MCOHW_GetTime
  (
  void
  )
{
   return HAL_GetTick();
}

/**************************************************************************
DOES:    Checks if a moment in time has passed (a timestamp has expired)
RETURNS: 0 if timestamp has not yet expired, 1 if the             
         timestamp has expired                                    
**************************************************************************/
UNSIGNED8 MCOHW_IsTimeExpired
  (
  UNSIGNED32 timestamp  // timestamp to check for expiration
  )
{
  return ((UNSIGNED32)(HAL_GetTick() - timestamp) < 0x80000000) ? 1 : 0;
}

/**************************************************************************
DOES:    Timer interrupt service routine                          
         Increments the global millisecond counter tick           
         This function needs to be called once every millisecond  
RETURNS: nothing
NOTE:    See TC3_Int_Handler                                                     
**************************************************************************/
/*void MCOHW_TimerISR   //using HAL_GetTick() for STM32
  (
  void
  ) 
{
  gTimCnt++;
}*/

/**************************************************************************
DOES:    Transmits a CAN message                                  
RETURNS: 0 if the message could not be transmitted, 1 if the      
         message was transmitted                                  
**************************************************************************/
UNSIGNED8 MCOHW_PushMessage
  (
  CAN_MSG *pTransmitBuf  // pointer to buffer containing CAN message to transmit                         
  )
  {
    uint8_t len = pTransmitBuf->LEN;

    if (len > 8)
      return 0;

    TxHeader.Identifier = pTransmitBuf->ID;   // 11-bit standard ID
    TxHeader.DataLength = Length_to_DLC(len); // HAL expects DLC length

    Timer1 = RTI_One_Sec * 0.02;
    while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0 && !Timer1);

    if (!Timer1)
      return 0; //fifo full too long

    // Attempt transmission
    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, pTransmitBuf->BUF) != HAL_OK)
      return 0; // Failed to queue

    return 1;  // Queued successfully
}

/**************************************************************************
DOES:    Gets the next received CAN message and places it in
         a receive buffer                                         
RETURNS: 0 if no message received, 1 if message received and      
         copied to the buffer                                     
**************************************************************************/
UNSIGNED8 MCOHW_PullMessage(CAN_MSG *pReceiveBuf)
{
    FDCAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];         

    // Check if a message is available in Rx FIFO0
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) == 0)
      return 0; 
        
    // Try to read the message
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
      return 0;

    pReceiveBuf->ID = (UNSIGNED16)(RxHeader.Identifier & 0x7FF);  // 11-bit mask
    pReceiveBuf->LEN = dlc_to_len[RxHeader.DataLength];

    for (int i = 0; i < pReceiveBuf->LEN; i++)
        pReceiveBuf->BUF[i] = RxData[i];

    //gRxCAN_Rcvd++;
    return 1;  // Message successfully received
}



uint32_t Length_to_DLC(uint8_t len)
{
    switch (len)
    {
        case 0: return FDCAN_DLC_BYTES_0;
        case 1: return FDCAN_DLC_BYTES_1;
        case 2: return FDCAN_DLC_BYTES_2;
        case 3: return FDCAN_DLC_BYTES_3;
        case 4: return FDCAN_DLC_BYTES_4;
        case 5: return FDCAN_DLC_BYTES_5;
        case 6: return FDCAN_DLC_BYTES_6;
        case 7: return FDCAN_DLC_BYTES_7;
        default: return FDCAN_DLC_BYTES_8;  
    }
}
