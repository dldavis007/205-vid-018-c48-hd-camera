#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Subroutines.h"
#include "MenuFunctions.h"
#include "mco.h"
#include "procimg.h"
#include "eeprom.h"
#include "main.h"
uint16_t VirtAddVarTab[NB_OF_VAR],EEDataTable[NB_OF_VAR];
UNSIGNED8 gProcImg[PROCIMG_SIZE];
bool CAN_Operating=true;
GPIO_TypeDef pc_gpio_ports[8];
uint32_t BootUpTimer,Timer1;
extern uint16_t cam_add;
static uint32_t now;static uint16_t stored;static int present,writes,count;
static CAN_MSG frames[8];
char *getstrval(struct menu_var *v){const char *s[]={"  INSP","   FBE","LIQUID"," CLEAN","  NONE"};strcpy(v->str_value,s[(int)v->value-1]);return v->str_value;}
int menu_function(void){return 0;}
void Display(char *s){(void)s;}
void CAN_Operational(void){}
UNSIGNED8 MCO_ProcessStack(void){return 0;}
UNSIGNED32 MCOHW_GetTime(void){return now;}
UNSIGNED8 MCOHW_IsTimeExpired(UNSIGNED32 t){return (int32_t)(now-t)>=0;}
UNSIGNED8 MCOHW_PushMessage(CAN_MSG *m){frames[count++]=*m;return 1;}
void MCOUSER_FatalError(UNSIGNED16 e){(void)e;assert(0);}
HAL_StatusTypeDef HAL_FLASH_Unlock(void){return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Lock(void){return HAL_OK;}
uint16_t EE_ReadVariable(uint16_t a,uint16_t *v){(void)a;*v=stored;return present?0:1;}
uint16_t EE_WriteVariable(uint16_t a,uint16_t v){(void)a;stored=v;present=1;writes++;return 0;}
void pc_side_reset(void){assert(0);}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p,uint16_t n){(void)p;(void)n;return GPIO_PIN_RESET;}
void HAL_GPIO_WritePin(GPIO_TypeDef *p,uint16_t n,GPIO_PinState s){(void)p;(void)n;(void)s;}
int main(void){
 const unsigned char nodes[]={0x68,0x6e,0x6e,0x48,0};const char *tags[]={"INSP","FBE\0","LIQU","CLEA","NONE"};cam_add=0x1234;
 for(int i=0;i<5;i++){
 count=0;TriggerMode.value=i+1;assert(update_paired_triggerable_unit());
 assert(count==1&&frames[0].ID==0x321&&frames[0].LEN==8);
 assert(frames[0].BUF[0]==nodes[i]&&frames[0].BUF[1]==0x34&&frames[0].BUF[2]==0x12&&frames[0].BUF[3]==0x18);
 assert(!memcmp(frames[0].BUF+4,tags[i],4));

 }
 count=0;now=100;gProcImg[OUT_digi_6]=2;doevents();assert(count==0);
 now=145;doevents();assert(count==0);now=146;doevents();assert(count==3);
 assert(frames[0].ID==0x500&&frames[1].ID==0x500&&frames[1].BUF[7]==3&&frames[2].ID==0x321);
 doevents();assert(count==3);
 puts("PASS: five pairing payloads, nonblocking scan and legacy tag frames");return 0;
}
