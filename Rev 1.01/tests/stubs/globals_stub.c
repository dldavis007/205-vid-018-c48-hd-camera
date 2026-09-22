/*
 * globals_stub.c  —  HOST-SIDE STUB
 *
 * Defines every global variable that MenuFunctions.c declares as 'extern'.
 * Without these definitions the linker fails even when dead-code elimination
 * (-ffunction-sections + --gc-sections) strips functions that reference them,
 * because not all toolchain versions strip aggressively enough.
 *
 * Variables are zero-initialised unless a specific value is needed for the
 * functions under test.  Test setUp() functions may overwrite them directly
 * before each test case.
 *
 * Relationship to the real firmware:
 *   These are normally defined in Subroutines.c, main.c, or user.c.  We do
 *   not compile those files for host tests because they contain hardware init
 *   code (CAN, timer, EEPROM) that cannot run on a PC.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nodecfg.h"       /* UNSIGNED8 etc.          */
#include "mco.h"           /* CAN_MSG                 */
#include "procimg.h"       /* PROCIMG_SIZE            */
#include "eeprom.h"        /* NB_OF_VAR               */
#include "MenuFunctions.h" /* struct menu_var, MenuStack, MenuStruct */

/* ---- Variables used directly by incvar / decvar / getstrval ----------- */
/*
 * String_Var_ptr selects which character within a "String-type" menu variable
 * is being edited.  1-based index; incvar/decvar pass (String_Var_ptr - 1)
 * to getvalue().  Initialised to 1 so the first character is selected.
 */
char String_Var_ptr = 1;

/* Timer used to accelerate repeated button presses; written by incvar/decvar */
uint32_t MenuTimer       = 0;
uint32_t IncSpeedUpTimer = 0;

/* Non-zero enables 10× increment/decrement speed */
uint8_t fast_inc = 0;

/* ---- Other globals referenced elsewhere in MenuFunctions.c ------------ */
char     Variable_flag   = 0;
char     Multi_Var_ptr   = 0;
char     StackPointer    = 0;
char     Gen_Flags       = 0;
uint16_t cam_add         = 0;
uint32_t Timer1          = 0;
char     F1_Rcvd_Data    = 0;
unsigned int TC0_RCVD_Data = 0;
bool     micChanged      = false;

/* Menu display buffer */
char Menu[12][21];

/* CANopen process image and transmit message buffer */
UNSIGNED8 gProcImg[PROCIMG_SIZE];
CAN_MSG   gTxMsg;

/* EEPROM virtual-address and data tables */
uint16_t VirtAddVarTab[NB_OF_VAR];
uint16_t EEDataTable[NB_OF_VAR];

/* Menu variables defined in Subroutines.c in production code */
struct menu_var SerialNum = { 0 };
struct menu_var NullVar   = { 0 };
struct menu_var NullVar2  = { 0 };
struct menu_var MicOnOff  = { 0 };
struct menu_var CamTag    = { 0 };

/*
 * Menu structure tables.  Size must be at least MenuSize (6) for MenuStruct
 * and MenuStackSize (5) for MenuStack so code that indexes them does not go
 * out of bounds.  Values are zero so all function pointers are NULL and all
 * index strings are ""; functions that call into Menuc[] are not tested here.
 */
MenuStack       MenuStackc[5];
MenuStruct const Menuc[6] = {{ {0}, {""}, {0}, {NULL}, {NULL} }};
// MenuStruct Menuc[6] = {{ {0}, {""}, {0}, {NULL}, {NULL} }};
