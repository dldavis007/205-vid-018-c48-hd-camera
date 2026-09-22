/*
 * nodecfg.h  —  HOST-SIDE STUB
 *
 * Shadows CM4/Core/Inc/nodecfg.h so that the 8051 memory-class keywords
 * 'data' and 'xdata' (used by MEM_NEAR / MEM_FAR) are always neutralised
 * to empty strings after this header is processed.
 *
 * Strategy: include the real nodecfg.h first (to get UNSIGNED8/16/32,
 * NR_OF_TPDOS, etc.), then unconditionally undefine and redefine MEM_NEAR
 * and MEM_FAR as empty.  Because -Istubs precedes -I../CM4/Core/Inc, every
 * #include "nodecfg.h" in the test build lands here first.
 */

#ifndef STUB_NODECFG_H
#define STUB_NODECFG_H

#include "../../CM4/Core/Inc/nodecfg.h"   /* real definitions */

#undef  MEM_NEAR
#define MEM_NEAR
#undef  MEM_FAR
#define MEM_FAR

#endif /* STUB_NODECFG_H */
