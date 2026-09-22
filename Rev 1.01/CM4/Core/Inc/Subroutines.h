#ifndef __SUBROUTINES_H
#define __SUBROUTINES_H


#include <stdint.h>
#include <stdbool.h>
#include "eeprom.h"

#define Revision "1.01"

#define NODE_ID 0x21
#define WTX_ID 0x310  

#define TeleData_Rev     0x8000            
#define TeleData_Fwd     0x4000            
#define TeleData_ExSlow  0x2000            
#define TeleData_Slow    0x1000            
#define TeleData_Fast    0x0800            
#define TeleData_Data    0x0400            
#define TeleData_Reset   0x0200            
#define TeleData_Trig    0x0100            
#define TeleData_F1      0x0080            
#define TeleData_F2      0x0040            
#define TeleData_F3      0x0020            
#define TeleData_RotCCW  0x0010            
#define TeleData_RotCW   0x0008            
#define TeleData_Select  0x0004            
#define TeleData_Up      0x0002            
#define TeleData_Down    0x0001   


#define F1_Up         0x01
#define F1_Down       0x02
#define F1_Left       0x04
#define F1_Right      0x08
#define F1_Select     0x10



#define Gen_Flags_SCI1Xmtng   0x01
#define Gen_Flags_SIN0Rcvd    0x02
#define Gen_Flags_SIN1Rcvd    0x04
#define Gen_Flags_Timer1      0x08
#define Gen_Flags_Timer2      0x10
#define Gen_Flags_No2Wire     0x20
#define Gen_Flags_Menu_Active 0x40

#define RTI_One_Sec 1000  //milliseconds


#define SerNum1VirtIndex  0
#define CamVirtIndex      8
#define MenuVar1Index     20


enum OSDdir{NONE, UP, DOWN, LEFT, RIGHT, MID};  

void doevents (void);
void mic_setEnabled(bool val);
void Save_Camera_Add (void);
void Load_Camera_Add (void);
int ResetProc (void);
void camera_ON(bool val);
void setOSD(enum OSDdir button);






#endif /* __SUBROUTINES_H */
