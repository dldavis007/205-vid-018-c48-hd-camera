# Rev 1.01 — open items

Findings that are not being acted on yet. Nothing here has been changed in the
firmware; each entry records what was observed, how it was found, and what a
fix would involve, so the decision can be made later.

## TC0_RCVD_Data truncates the button word to 8 bits

**Status:** open, no code change made (deliberate — see *Why it is not fixed*).
**Found:** 2026-09-23, while writing `tests/test_menu_navigation.c`.

`TC0_RCVD_Data` holds the button-box word that arrives in the process image.
It is declared 8 bits wide:

    CM4/Core/Src/Subroutines.c:15      uint8_t TC0_RCVD_Data;

but `menu_function()` assigns it a 16-bit value:

    CM4/Core/Src/MenuFunctions.c:900   TC0_RCVD_Data = gProcImg[OUT_digi_2]<<8 | gProcImg[OUT_digi_1];

so the high byte is discarded on every pass. The `TeleData_*` bits are defined
across both bytes in `CM4/Core/Inc/Subroutines.h`, which splits them into two
groups:

| Reachable (low byte) | Never set (high byte) |
|---|---|
| `TeleData_F1` 0x0080, `TeleData_F2` 0x0040, `TeleData_F3` 0x0020, `TeleData_RotCCW` 0x0010, `TeleData_RotCW` 0x0008, `TeleData_Select` 0x0004, `TeleData_Up` 0x0002, `TeleData_Down` 0x0001 | `TeleData_Rev` 0x8000, `TeleData_Fwd` 0x4000, `TeleData_ExSlow` 0x2000, `TeleData_Slow` 0x1000, `TeleData_Fast` 0x0800, `TeleData_Data` 0x0400, `TeleData_Reset` 0x0200, `TeleData_Trig` 0x0100 |

**Consequence.** Any test against a high-byte bit is always false. The one the
current code depends on is the first statement of the main loop:

    CM4/Core/Src/Subroutines.c:295-296
        if (TC0_RCVD_Data & TeleData_Reset)     /* 0x0200 */
            ResetProc();

That branch can never be taken, so the button box's **Reset key does not reset
this unit**. The menu keys (Up, Down, Select, F1) are all in the low byte and
work normally, which is why this has not shown up in use.

**What a fix looks like.** Widen the variable to `uint16_t` in
`Subroutines.c:15`. Everything that reads it already uses the 16-bit
`TeleData_*` constants, and the assignment already produces a 16-bit value, so
no other line needs to change. Worth checking afterwards: the reset path now
becomes live, so a held Reset key would repeat `ResetProc()` on every pass of
`doevents()` unless it is edge-detected the way `menu_function()` handles
`SelectFlag`.

**Why it is not fixed.** Making the Reset key live changes how the unit
behaves on the bus, which needs a deliberate decision and a retest, not a
drive-by edit.

**Note for the tests.** `tests/stubs/globals_stub.c` declares this global as
`unsigned int`, which does not match production and hides the truncation in the
`test_menu_var` target. `tests/stubs/menu_navigation_stub.c` matches production
at `uint8_t`. If the firmware is widened, both stubs should follow.
