/*
 * menu_globals_stub.c  —  HOST-SIDE STUB
 *
 * Defines every global variable and function that MenuFunctions.c requires
 * when compiled for the test_subroutines target.  Unlike globals_stub.c
 * (used for test_menu_var), the five EEPROM-backed menu_var structs are
 * placed in a ".menu_data" section so they are contiguous in memory — the
 * same layout assumption made by Save_Variables / Load_Variables pointer
 * arithmetic.
 *
 * Relationship to production code:
 *   Production:  NullVar … NullVar2 defined in Subroutines.c with
 *                __attribute__((section(".menu_data"))) and placed
 *                consecutively by the linker script.
 *   Test host:   Defined here in the same order with the same attribute.
 *                All five live in one translation unit so GCC preserves
 *                declaration order within the ".menu_data" section.
 *
 *   Timer1, VirtAddVarTab, EEDataTable etc. are defined here instead of
 *   main.c (which is not compiled for host tests).
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nodecfg.h"       /* UNSIGNED8, NB_OF_VAR via eeprom.h chain */
#include "mco.h"           /* CAN_MSG                                  */
#include "procimg.h"       /* PROCIMG_SIZE                             */
#include "eeprom.h"        /* NB_OF_VAR                                */
#include "MenuFunctions.h" /* struct menu_var, MenuStack, MenuStruct   */

/* ---- Enum string literals (mirror Subroutines.c) ---------------------- */

static const char s_enum_NULL[]   = "";
static const char s_enum_off_on[] = "OFF, ON";
static const char s_enum_alpha[]  =
    " ,0,1,2,3,4,5,6,7,8,9,-,(,),@,:,;,<,>,.,A,B,C,D,E,F,G,H,I,J,K,L,M,N,"
    "O,P,Q,R,S,T,U,V,W,X,Y,Z,-";
static const char s_enum_number[] = "0,1,2,3,4,5,6,7,8,9,-";

/* ---- EEPROM-backed menu variables (must stay in this exact order) ------ */
/*
 * Save_Variables iterates from &NullVar.str_value to &NullVar2.str_value
 * stepping by sizeof(struct menu_var), so all five structs MUST be exactly
 * sizeof(struct menu_var) bytes apart in memory.
 *
 * The production firmware achieves this with __attribute__((section(".menu_data")))
 * plus a linker script.  On a Windows host the PE section alignment (32 bytes)
 * would pad each struct to 64 bytes, breaking the 40-byte step.  Avoiding the
 * explicit section attribute keeps them in the normal .data segment where GCC
 * emits consecutive definitions adjacently (no extra padding).
 *
 * The test_subroutines target is compiled without -fdata-sections (see Makefile
 * SUBS_CFLAGS) so the five structs all land in a single combined .data section,
 * preserving their declaration order and 40-byte spacing.
 */
struct menu_var NullVar  __attribute__((aligned(4))) = { 0,0,0,0,0,0," ",  s_enum_NULL,  NULL };
struct menu_var CamTag   __attribute__((aligned(4))) = { 1,1,1,46,0,-11,"     HD CAM", s_enum_alpha, NULL };
struct menu_var disp_add __attribute__((aligned(4))) = { 1,1,1,46,0,-4, "1234",        s_enum_alpha, NULL };
struct menu_var MicOnOff __attribute__((aligned(4))) = { 2,1,1,2, 0,3,  " ON",         s_enum_off_on, NULL };
struct menu_var NullVar2 __attribute__((aligned(4))) = { 0,0,0,0,0,0," ",  s_enum_NULL,  NULL };

/* ---- Other menu variables (not EEPROM-backed) ------------------------- */

struct menu_var SerialNum = { 1,1,1,10,0,-7,"-------", s_enum_number, NULL };

/* ---- Globals from main.c ---------------------------------------------- */

uint32_t Timer1          = 0;
uint32_t MenuTimer       = 0;
uint32_t IncSpeedUpTimer = 0;
uint8_t  fast_inc        = 0;
bool     CAN_Operating   = false;

uint16_t VirtAddVarTab[NB_OF_VAR];
uint16_t EEDataTable[NB_OF_VAR];

/* ---- Globals from Subroutines.c --------------------------------------- */

char      String_Var_ptr  = 1;
char      Variable_flag   = 0;
char      Multi_Var_ptr   = 0;
uint8_t   StackPointer    = 0;
char      Gen_Flags       = 0;
uint16_t  cam_add         = 0;
uint8_t   TC0_RCVD_Data   = 0;
char      F1_Rcvd_Data    = 0;

char      Menu[12][21];
UNSIGNED8 gProcImg[PROCIMG_SIZE];
CAN_MSG   gTxMsg;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
MenuStack       MenuStackc[MenuStackSize];
MenuStruct const Menuc[MenuSize] = {0};
#pragma GCC diagnostic pop

/* ---- Functions declared in main.h ------------------------------------- */

void InitVirtAddVarTab(void)
{
    VirtAddVarTab[0] = 0xABC1;
    VirtAddVarTab[1] = 0xABC2;
    VirtAddVarTab[2] = 0xABC3;
    VirtAddVarTab[3] = 0xABC4;
    VirtAddVarTab[4] = 0xABC5;
    VirtAddVarTab[5] = 0xABC6;
    VirtAddVarTab[6] = 0xABC7;
    VirtAddVarTab[7] = 0xABC8;
    VirtAddVarTab[8] = 0x1234;
    for (uint16_t i = 20; i < NB_OF_VAR; i++)
        VirtAddVarTab[i] = i;
}

void Error_Handler(void)       {}
void MX_GPIO_Init(void)        {}
void COP_Trig(void)            {}
void copy_menu_data_to_ram(void) {}
void InitCANstack(void)        {}
