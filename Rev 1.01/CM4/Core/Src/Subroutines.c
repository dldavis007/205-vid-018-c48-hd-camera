#include <string.h>
#include <stdlib.h>
#include "Subroutines.h"
#include "main.h"
#include "nodecfg.h"
#include "mco.h"
#include "procimg.h"
#include "MenuFunctions.h"
#include "eeprom.h"
#include "mcohw.h"


uint16_t ran_num = 0;  //used to create unique camera address  
uint16_t cam_add;
uint8_t TC0_RCVD_Data;
char Gen_Flags = 0;
char F1_Rcvd_Data = 0;
char F1_Data_prev = 0;
char lastMicSetting[5];
CAN_MSG gTxMsg;
bool micChanged = false;
bool camActive = false;
static bool addr_tx_pending;
static UNSIGNED32 addr_tx_time;

extern uint32_t BootUpTimer;
extern uint32_t Timer1;

extern uint16_t VirtAddVarTab[NB_OF_VAR];
extern uint16_t EEDataTable[NB_OF_VAR];
extern UNSIGNED8 gProcImg[];
extern bool CAN_Operating;


//struct menu_var{
//float value;            //actual value of the variable, or pointer to enum list if an enum type variable, In "String type" var this points to the char that String_Var_ptr points to
//float inc;              //increment/decrement value, may be int or float, must be 1 for enum type variable
//float min;              //minimum value of variable, may be int or float, must point to first enum in list for enum type variable
//float max;              //maximum value of variable, may be int or float, must point to last enum in list for enum type variable
//int dec_pos;            //decimal position, zero if variable is an int or an enum
//int len_str;			  //length of str_value, pads left with spaces, Make this negative for "String type" variables (serial num, camera tag, etc.)
//char str_value[20];     //string equivalent of value, or current enum pointed to by value, or current "String type" variable
//char *str_enum;         //pointer to comma separated enum list, must be NULL for non-enum variable types
//struct menu_var *next_var;
//};


//Enum strings have a maximum length of 100 chars including the Null
const char enum_NULL_str[] = "";
const char enum_off_on_str[] = "OFF, ON";
const char enum_polarity_str[] = "POS,NEG";
const char enum_angle_dir_str[] = " CW,CCW";
const char enum_size_str[] = " 8,12,24";
const char enum_encoder_str[] = "25,32,50,64,100,128,256,512";
const char enum_alpha_str[] = " ,0,1,2,3,4,5,6,7,8,9,-,(,),@,:,;,<,>,.,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,-"; //last char is the cursor char, do not count for max
const char enum_number_str[] = "0,1,2,3,4,5,6,7,8,9,-";  //last char is the cursor char, do not count for max


struct menu_var SerialNum = {
    1,1,1,10,0,-7,"-------",enum_number_str, NULL
};

//saved seperately at Virtual Address 0x9999  
uint16_t cam_addx = 0x0000;          //unique camera address from ran_num


//The following Menu Variables are NOT saved via EEPROM emulation
struct menu_var Rev = {
	   1,1,1,46,0,-4,Revision,enum_alpha_str, NULL   //display only
}; 



/* Stored separately at virtual index 9: legacy menu token order stays intact. */
const char enum_trig_str[] = "  INSP,   FBE,LIQUID, CLEAN,  NONE";
struct menu_var TriggerMode = {5,1,1,5,0,6,"  NONE",enum_trig_str,NULL};

//The following Menu Variables ARE saved via EEPROM emulation
__attribute__((section(".menu_data"))) struct menu_var NullVar = {
    0,0,0,0,0,0," ",enum_NULL_str, NULL
};

__attribute__((section(".menu_data"))) struct menu_var CamTag = {
	   1,1,1,46,0,-11,"     HD CAM",enum_alpha_str, NULL   //display only
}; 

__attribute__((section(".menu_data"))) struct menu_var disp_add = {
	   1,1,1,46,0,-4,"1234",enum_alpha_str, NULL 
}; 

__attribute__((section(".menu_data"))) struct menu_var MicOnOff = {
	   2,1,1,2,0,3," ON",enum_off_on_str, NULL 
}; 
					   
__attribute__((section(".menu_data"))) struct menu_var NullVar2 = {
	   0,0,0,0,0,0," ",enum_NULL_str, NULL
}; 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"

struct MenuStruct const Menuc[MenuSize] = {     
                                        0,0,0,0,
                                        "      HD CAMERA     ",
                                        " SETTINGS           ",
                                        " DIAGNOSTICS        ",
                                        " STATUS             ",
                                        " DEFAULTS           ",
                                        " EXIT               ",
                                        "                    ",
                                        "                    ",
                                        "                    ",
                                        "                    ",
                                        "                    ",
                                        "                    ",
                                        0,0,0,0,0,0,0,0,0,0,0,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &ExitMenu,
                                        &NullFunction,
										&NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        
                                            0,0,0,1,
                                            "   CAMERA SETTINGS  ",
											" TAG                ",
											" MICROPHONE         ",
											" TRIGGER            ",
											" EXIT               ",
											"                    ",
											"                    ",
											"                    ",
											"                    ",						
											"                    ",
											"                    ",
											"                    ",
											9,17,13,0,0,0,0,0,0,0,0,
                                            &CamTag,
                                            &MicOnOff,
											&TriggerMode,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &StdVarFunction,
											&StdVarFunction,
                                            &update_paired_triggerable_unit_menu_wrapper,
											&ExitMenu,
											&NullFunction,
											&NullFunction,
                                            &NullFunction,
                                            &NullFunction,
											&NullFunction,
                                            &NullFunction,
                                            &NullFunction,
																				                                                      
                                            0,0,0,2,
                                            " CAMERA DIAGNOSTICS ",
											" EXIT               ",
											"                    ",
											"                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            0,0,0,0,0,0,0,0,0,0,0,
                                            &NullVar,
                                            &NullVar,
											&NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &ExitMenu,
                                            &NullFunction,
											&NullFunction,
                                            &NullFunction,
                                            &NullFunction,
											&NullFunction,
											&NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                                                                         
                                            0,0,0,3,
                                            "     STATUS MENU    ",
                                            " SOFTWARE REV       ",
											" CAMERA ID          ",
                                            " SERIAL NUM         ",
                                            " EXIT               ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            16,16,13,0,0,0,0,0,0,0,0,
                                            &Rev,
                                            &disp_add,
                                            &SerialNum,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullFunction,
											&NullFunction,
                                            &SerVarFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            
                                            0,0,0,4,
                                            "     DEFAULTS       ",
                                            " EXIT               ",
                                            " RESTORE DEFAULTS   ",
                                            "                    ",
                                            "      WARNING       ",
                                            "YOU WILL LOSE ALL   ",
                                            "OF THE CURRENT      ",
                                            "SETTINGS WHEN       ",
                                            "RESTORING DEFAULTS  ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            0,0,0,0,0,0,0,0,0,0,0,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &ExitMenu,
                                            &RestoreDefaults,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction
 
};

MenuStack MenuStackc[MenuStackSize] = {0,0,0,0,1,0};
#pragma GCC diagnostic pop
uint8_t StackPointer = 0;
char Menu[12][21];
char Variable_flag = 0;
char String_Var_ptr = 0;
char Multi_Var_ptr = 0;


void doevents(void)
{
    if (TC0_RCVD_Data & TeleData_Reset)
        ResetProc();

    menu_function();

    if ((TC0_RCVD_Data & TeleData_F1) && HAL_GPIO_ReadPin(GPIOH, RELAY_Pin) == GPIO_PIN_SET) // BBox Camera Mode
    {
        if (F1_Rcvd_Data != F1_Data_prev)
        {
            if (F1_Rcvd_Data & F1_Up) // up
                setOSD(UP);
            else if (F1_Rcvd_Data & F1_Down) // down
                setOSD(DOWN);
            else if (F1_Rcvd_Data & F1_Left) // left
                setOSD(LEFT);
            else if (F1_Rcvd_Data & F1_Right) // right
                setOSD(RIGHT);
            else if (F1_Rcvd_Data & F1_Select) // select
                setOSD(MID);
            else
                setOSD(NONE); // no button
        }

        F1_Data_prev = F1_Rcvd_Data;
    }

    if (TC0_RCVD_Data & TeleData_F2) // F2 Button
    {
    }
    if (TC0_RCVD_Data & TeleData_F3) // F3 Button
    {
    }

    ran_num++;

    sprintf(disp_add.str_value, "%04X", cam_add); // for status diplay of camera address

    // MicOnOff setting changed
    if (MenuStackc[StackPointer].Index[3] == 1 && strcmp(MicOnOff.str_value, lastMicSetting))
    {
        micChanged = true;
    }

    strcpy(lastMicSetting, MicOnOff.str_value);

    if (micChanged && (TC0_RCVD_Data & TeleData_Select))
    {
        if (MicOnOff.value == 1)
            mic_setEnabled(false);
        else
            mic_setEnabled(true);

        micChanged = false;
    }

    if (gProcImg[OUT_digi_4] == (cam_add & 0x00FF) && gProcImg[OUT_digi_5] << 8 == (cam_add & 0xFF00)) // compare
    {
        char tempstr[27];
        int i = 0;
        gProcImg[OUT_digi_4] = gProcImg[OUT_digi_5] = 0;
        camera_ON(true);
        if (MicOnOff.value == 2)
            mic_setEnabled(true);
        while (CamTag.str_value[i++] == 0x20);
        sprintf(tempstr, "Proc:%s", &CamTag.str_value[i - 1]);
        Display(tempstr);
    }
    else if (gProcImg[OUT_digi_4] || gProcImg[OUT_digi_5])
    {
        camera_ON(false);
        mic_setEnabled(false);
    }
    // command to generate random address
    if (gProcImg[OUT_digi_6] & 0x01) // triggered by findCamera on controller
    {
        gProcImg[OUT_digi_6] = 0x00;
        srand(ran_num); // seed the random number
        cam_add = rand();
        cam_addx = (uint16_t)(cam_add & 0xFFFF);
        Save_Camera_Add();
    }

    // command to transmit address + tag
    if (gProcImg[OUT_digi_6] & 0x02) // triggered by findCamera and scanCamera on controller
    {
        gProcImg[OUT_digi_6] = 0x00;

        addr_tx_time = MCOHW_GetTime() + cam_add / 100;
        addr_tx_pending = true;
    }

    if (addr_tx_pending && MCOHW_IsTimeExpired(addr_tx_time))
    {
        addr_tx_pending = false;
        // transmit camera tag
        gTxMsg.ID = 0x500;
        gTxMsg.LEN = 8;

        gTxMsg.BUF[0] = 0x18; // used to signal camera tag for VID-018
        gTxMsg.BUF[1] = cam_add;
        gTxMsg.BUF[2] = cam_add >> 8;
        gTxMsg.BUF[3] = CamTag.str_value[0];
        gTxMsg.BUF[4] = CamTag.str_value[1];
        gTxMsg.BUF[5] = CamTag.str_value[2];
        gTxMsg.BUF[6] = CamTag.str_value[3];
        gTxMsg.BUF[7] = CamTag.str_value[4];

        if (!MCOHW_PushMessage(&gTxMsg))
        {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
        }

        gTxMsg.BUF[0] = 0x18;
        gTxMsg.BUF[1] = CamTag.str_value[5];
        gTxMsg.BUF[2] = CamTag.str_value[6];
        gTxMsg.BUF[3] = CamTag.str_value[7];
        gTxMsg.BUF[4] = CamTag.str_value[8];
        gTxMsg.BUF[5] = CamTag.str_value[9];
        gTxMsg.BUF[6] = CamTag.str_value[10];
        gTxMsg.BUF[7] = 0x03;

        if (!MCOHW_PushMessage(&gTxMsg))
        {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
        }
        update_paired_triggerable_unit();
    }

    if ((gProcImg[OUT_digi_6] & 0x04) &&                     // command to store address
        (gProcImg[OUT_digi_7] == (cam_add & 0x00FF)) &&      // lsb - old address
        ((gProcImg[OUT_digi_8] << 8) == (cam_add & 0xFF00))) // msb - old address
    {                                                        // change to new address
        gProcImg[OUT_digi_6] = 0x00;
        cam_add = gProcImg[OUT_digi_9] + (gProcImg[OUT_digi_10] << 8);
        // send new address
        cam_addx = (uint16_t)(cam_add & 0xFFFF);
        Save_Camera_Add();
    }

    MCO_ProcessStack();   // maintain CAN bus

    // allows CAN to work when no controller exists and AUTOSTART not defined (eg. just using a CAN analyzer).
    if (!BootUpTimer && !CAN_Operating) 
    {
        CAN_Operational(); 
    }
}

void mic_setEnabled(bool val)
{
    if (val == true)
	    HAL_GPIO_WritePin(GPIOC, MIC_SHDN_Pin, GPIO_PIN_SET);     //turn mic on
	else	
	    HAL_GPIO_WritePin(GPIOC, MIC_SHDN_Pin, GPIO_PIN_RESET);   //turn mic off  
}

void camera_ON(bool val)
{
    if (val == true)
    {
        HAL_GPIO_WritePin(GPIOH, RELAY_Pin, GPIO_PIN_SET);       //turn camera on
        camActive = true;
    }
	    
	else	
    {
        HAL_GPIO_WritePin(GPIOH, RELAY_Pin, GPIO_PIN_RESET);     //turn camera off 
        camActive = false;
    }
            
}

void Save_Camera_Add (void)
{
    HAL_FLASH_Unlock();
    if (EE_WriteVariable(VirtAddVarTab[CamVirtIndex], cam_addx) == 0)
        EEDataTable[CamVirtIndex] = cam_addx;
    HAL_FLASH_Lock();
}

//retreive camera address from Emulated EEProm
void Load_Camera_Add (void)
{
    EE_ReadVariable(VirtAddVarTab[CamVirtIndex], &EEDataTable[CamVirtIndex]);  

    if (EEDataTable[CamVirtIndex] != 0)
        cam_add = EEDataTable[CamVirtIndex];  
    else 
        cam_add = 0xFFFF;       
}

int ResetProc (void)
{
    NVIC_SystemReset();					
	return 0;
}

void setOSD(enum OSDdir button)
{
    switch(button)
    {
    case NONE:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_SET);
        break;
    case UP:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_RESET);
        break;
    case DOWN:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_RESET);
        break;
    case LEFT:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_SET);
        break;
    case RIGHT:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_RESET);
        break;
    case MID:
        HAL_GPIO_WritePin(GPIOB, MUX_S2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, MUX_S1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, MUX_S0_Pin, GPIO_PIN_SET);
        break;
    }
}
can_unit_t trigger_menu_to_unit(void)
{
    switch ((int)TriggerMode.value) {
    case 1: return CAN_UNIT_INSPECTION;
    case 2: return CAN_UNIT_FBE_COATER;
    case 3: return CAN_UNIT_LIQUID_COATER;
    case 4: return CAN_UNIT_CLEANER;
    default: return CAN_UNIT_NONE;
    }
}

int update_paired_triggerable_unit(void)
{
    CAN_MSG message = {0};
    getstrval(&TriggerMode);
    message.ID = 0x321;
    message.LEN = 8;
    message.BUF[0] = can_node_id_for_unit(trigger_menu_to_unit());
    message.BUF[1] = (uint8_t)cam_add;
    message.BUF[2] = (uint8_t)(cam_add >> 8);
    message.BUF[3] = 0x18;
    strncpy((char *)&message.BUF[4],
            TriggerMode.str_value + strspn(TriggerMode.str_value, " "), 4);
    if (!MCOHW_PushMessage(&message)) {
        MCOUSER_FatalError(0x8801);
        return 0;
    }
    return 1;
}

void Save_Trigger_Mode(void)
{
    uint16_t value = 0xA500 | (uint16_t)TriggerMode.value;
    uint16_t previous;
    if (EE_ReadVariable(VirtAddVarTab[9], &previous) == 0 && previous == value)
        return;
    HAL_FLASH_Unlock();
    if (EE_WriteVariable(VirtAddVarTab[9], value) == 0)
        EEDataTable[9] = value;
    HAL_FLASH_Lock();
}

void Load_Trigger_Mode(void)
{
    uint16_t value;
    TriggerMode.value = 5;
    if (EE_ReadVariable(VirtAddVarTab[9], &value) == 0 &&
        (value & 0xFF00) == 0xA500 && (value & 0xFF) >= 1 && (value & 0xFF) <= 5)
        TriggerMode.value = value & 0xFF;
    getstrval(&TriggerMode);
}
