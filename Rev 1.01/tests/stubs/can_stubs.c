/*
 * can_stubs.c  —  HOST-SIDE STUB
 *
 * CAN hardware and CANopen stack stubs for the test_subroutines target.
 *
 * Unlike mco_stub.c (used by test_menu_var), this file does NOT provide
 * EE_* stubs.  The test_subroutines target links against the real eeprom.c,
 * which supplies those symbols.  Including EE stubs here would cause
 * duplicate-symbol link errors.
 */

#include <stdint.h>
#include "nodecfg.h"   /* UNSIGNED8, UNSIGNED16, UNSIGNED32 */
#include "mco.h"       /* CAN_MSG, MCO_* prototypes         */
#include "mcohw.h"     /* MCOHW_* prototypes                */

/* ---- CAN hardware stubs ----------------------------------------------- */

UNSIGNED8  MCOHW_GetStatus(void)                          { return 0; }
UNSIGNED8  MCOHW_Init(UNSIGNED16 baud)                    { (void)baud; return 1; }
UNSIGNED8  MCOHW_SetCANFilter(UNSIGNED16 id)              { (void)id; return 1; }
UNSIGNED8  MCOHW_PushMessage(CAN_MSG *p)                  { (void)p; return 1; }
UNSIGNED8  MCOHW_PullMessage(CAN_MSG *p)                  { (void)p; return 0; }
UNSIGNED32 MCOHW_GetTime(void)                            { return 0; }
UNSIGNED8  MCOHW_IsTimeExpired(UNSIGNED32 t)              { (void)t; return 0; }
uint32_t   Length_to_DLC(uint8_t len)                     { return len; }

/* ---- CANopen stack stubs ---------------------------------------------- */

void MCO_Init(UNSIGNED16 b, UNSIGNED8 n, UNSIGNED16 h)
{
    (void)b; (void)n; (void)h;
}

void MCO_InitTPDO(UNSIGNED8 n, UNSIGNED16 id, UNSIGNED16 et,
                  UNSIGNED16 it, UNSIGNED8 len, UNSIGNED8 off)
{
    (void)n; (void)id; (void)et; (void)it; (void)len; (void)off;
}

void MCO_InitRPDO(UNSIGNED8 n, UNSIGNED16 id, UNSIGNED8 len, UNSIGNED8 off)
{
    (void)n; (void)id; (void)len; (void)off;
}

UNSIGNED8 MCO_ProcessStack(void)          { return 0; }
void MCOUSER_ResetApplication(void)       {}
void MCOUSER_ResetCommunication(void)     {}
void MCOUSER_FatalError(UNSIGNED16 e)     { (void)e; }
void CAN_Operational(void)                {}
