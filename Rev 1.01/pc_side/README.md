# Rev 1.01 PC-side host

Runs the **real** VID-018 HD camera firmware (CM4) as a Windows executable,
with live logging and a UDP CAN bus. Built by GCC with `-DPC_SIDE`; CMake and
the ARM toolchain never see `pc_side/`.

Nothing under `CM4/`, `Drivers/` or `Common/` is modified. The host build
swaps the hardware out one level lower, by putting `pc_side/stubs` first on
the include path so every `#include "stm32h7xx_hal.h"` lands on a stand-in.

## Running

```
mingw32-make run          # build + run (Ctrl+C to quit)
mingw32-make              # build only
mingw32-make clean
```

Or in VS Code: **PC_SIDE: Build & Run**, or **PC_SIDE: Tests, Build & Run** to
start the host only after every test suite passes.

Defaults: binds `:20018`, sends to the hub on `:20100`. Pass both as args to
skip the prompts (`pc_side_host.exe 20018 20100`).

The hub and emulators live in `C:\Working_Projects\can_emulators`:

```
python can_hub_gui.py            # or can_udp_hub.py, headless
```

Several cameras can share the bus: give each instance its own recv port. The
EEPROM file is keyed by that port, so each one keeps its own camera address.

## What runs

Compiled unchanged: `main.c`, `stm32h7xx_it.c`, `Subroutines.c`,
`MenuFunctions.c`, `mco.c`, `mcohw.c`, `user.c` and `Drivers/EEPROM/Src/eeprom.c`.

Not compiled — `pc_hal.c` stands in for what they need from these: the
`STM32H7xx_HAL_Driver` sources, `system_stm32h7xx_dualcore_boot_cm4_cm7.c`,
`stm32h7xx_hal_msp.c`, `syscalls.c`, `sysmem.c` and the startup assembly.

The firmware's `main()` is compiled as `fw_main()` (`-Dmain=fw_main`, that TU
only) and called from `pc_side/main.c` on its own thread, so the host keeps a
thread free for Ctrl+C. Everything from `HAL_Init()` onward is production code:
`SystemClock_Config`, the `MX_*_Init` functions, `InitCANstack`, `EE_Init`,
`Load_Camera_Add`, `Load_Serial_Num`, `Load_Variables`, then
`while (1) { doevents(); COP_Trig(); }`.

## The RTI simulation thread

`pc_side_host.c` raises the production `SysTick_Handler()` at 1 kHz. It derives
the tick count from `QueryPerformanceCounter`, because a 1 ms `Sleep` does not
take 1 ms on Windows. It also raises `TIM1_UP_IRQHandler()` every 500 ms, the
TPS3850 kick, once `HAL_TIM_Base_Start_IT` has run.

The thread is **load-bearing**. `SysTick_Handler` reaches
`HAL_SYSTICK_Callback()` in `main.c`, which counts down `Timer1`,
`BootUpTimer`, `MenuTimer` and `IncSpeedUpTimer`. Every `Timer1 = n; while
(Timer1);` wait in the firmware ends only because of it — there are about a
dozen, in `MenuFunctions.c`, `mcohw.c` and `Subroutines.c`. This is also why
the build has **no `-O`**: those timers are not `volatile`, and an optimizer
would hoist the load out of the wait.

Ticks are deferred while interrupts are masked, so `__disable_irq()` still
means something (`Error_Handler` is the only caller).

## The CAN bus

One datagram per frame, matching `can_udp.py`:

```
[ID lo, ID hi, LEN, LEN data bytes]
```

`mcohw.c` is compiled as-is, so the DLC conversion, the 20 ms TX-FIFO wait and
the 11-bit ID mask are all production code. Underneath it, the `HAL_FDCAN_*`
stand-ins send and receive datagrams.

The receive filters are honoured. `mco.c` sets the global filter to
`FDCAN_REJECT` and installs a mask filter per accepted ID, so the host drops
everything else off the hub, as the hardware would. `[can ] FDCAN started, N
receive filter(s)` reports how many were installed.

## EEPROM

The real `eeprom.c` runs against a 256 KB array, backed by
`flash_<recv port>.bin`. `eeprom.c` keeps every flash address in a `uint32_t`,
which stays lossless because the host build is `-m32`. The file is rewritten on `HAL_FLASH_Lock()`, which the
firmware calls after every write, so a camera address or a menu variable
survives a reset or a restart. Delete the file to boot on blank flash.

## Diagnostics

Logging is async and drop-on-full: the firmware and CAN threads only enqueue
lines, so a slow terminal can't stall them. Windows QuickEdit is disabled at
startup, because one stray click in the console would otherwise pause stdout
and freeze every thread that logs.

The GPIO stand-in logs writes by pin name, so `RELAY` (camera on/off),
`MIC_SHDN` and `MUX_EN` show up without any firmware edits. The `MUX_S0..S2`
trio is decoded back into the OSD key `setOSD()` drove (`[osd ] camera key:
UP`). `WDI` and the mux select lines are logged only through that decode,
because they toggle constantly.

A watchdog thread reports once a second through the console **title bar**
(`loop=`, menu state, RTI rate), and writes the same summary to the log every
ten seconds. Title updates go through kernel32, not stdout, so they keep going
when the terminal blocks output:

- `loop=` climbing with a frozen log: only the terminal is stuck.
- `loop=` frozen: the main loop is hung. `[STALL]` is logged once.

It also measures the longest gap between `COP_Trig()` calls and warns when a
pass took longer than the target's 1.02 s window watchdog would have allowed.

**Resets.** `ResetProc` (the button-box Reset key) and
`MCOUSER_ResetApplication` (NMT reset node) both call `NVIC_SystemReset`. The
host has no NVIC, so the stand-in flushes the EEPROM file and relaunches the
executable on the same ports.

## Host-build adaptations (no firmware changes)

- **`stubs/stm32h7xx_hal.h`** declares only the types, constants and functions
  the compiled TUs use. Values that the firmware computes with are copied from
  the real HAL, notably `FDCAN_DLC_BYTES_x`, which `mcohw.c` uses to index
  `dlc_to_len[]`. Register macros (`__HAL_RCC_*_CLK_ENABLE`,
  `__HAL_PWR_VOLTAGESCALING_CONFIG`) become no-ops, and `__HAL_PWR_GET_FLAG`
  returns 1, or `SystemClock_Config` would spin on VOSRDY forever.
- **`stubs/eeprom.h`** is the real header with the flash addresses pointed at
  the host buffer.
- **`-funsigned-char`**, because ARM GCC's `char` is unsigned by default and
  x86 GCC's is signed. The firmware's `char` flags and menu strings depend on
  it.
- **The linker-script symbols.** `copy_menu_data_to_ram()` copies `.menu_data`
  from its flash load address to RAM. In the host image it is already there, so
  `pc_hal.c` defines `_emenu_data` as an alias of `_smenu_data` and the copy
  loop runs zero times.
- **`.menu_data` layout.** `Save_Variables` / `Load_Variables` walk from
  `NullVar` to `NullVar2` in `sizeof(struct menu_var)` steps, so the build uses
  `-malign-data=abi`, `-fno-toplevel-reorder` and no `-fdata-sections`.
  `pc_menu_data_check()` verifies the layout at startup and refuses to run if
  it ever breaks, and makes the section writable, since PE can map an unknown
  section read-only.

## Debugging

**Run and Debug → PC_SIDE: Debug host** (F5) builds the host and starts it
under gdb in its own console window. Breakpoints go in the firmware sources
themselves — `doevents()`, `menu_function()`, `Save_Camera_Add()` — and step,
watch and call-stack all work, because this is ordinary x86 code with `-g`.

Two things to know while stopped on a breakpoint:

- The RTI thread keeps ticking, but catches up at most 100 ticks per wakeup, so
  resuming does not expire every firmware timer at once.
- The host's own threads (CAN receive, logging, watchdog) keep running, so
  frames still arrive and the title bar keeps updating. The watchdog will log
  `[STALL]` after a second, which is expected while you are stopped.

## IntelliSense

The workspace has a second C/C++ configuration, **PC_SIDE host**. Select it
(`C/C++: Select a Configuration...`) when editing files under `pc_side/`, or
the editor resolves `stm32h7xx_hal.h` to the real HAL and flags the stand-in
types. The build itself is unaffected either way.

## Trigger-unit pairing

CAMERA SETTINGS now includes Trigger MODE: INSP, FBE, LIQUID, CLEAN, NONE
(default NONE). FBE and LIQUID both address node 0x6E; LIQUID covers either
liquid-coater variant. Confirming an edit saves it and broadcasts 0x321.
A scan still sends the two existing 0x500 tag frames, then broadcasts 0x321.
Scan delays no longer block the firmware main loop.

Pairing payload: unit node ID, camera-address low/high bytes, camera type
0x18, then four bytes of the selected unit label (zero padded as needed).
The new setting uses versioned virtual address 0x1235 (table index 9),
separate from the legacy menu stream, camera address and serial number.
Absent/invalid records and Restore Defaults select NONE.

Run `make test_trigger_pairing` in tests/ for the pairing/persistence/scan
checks. Full Windows bench and STM32 target validation remain required.
