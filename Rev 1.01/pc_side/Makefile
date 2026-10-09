# =============================================================================
# Makefile - VID-018 HD camera (Rev 1.01, CM4) PC-side host (GCC, -DPC_SIDE)
#
# Runs the real firmware main() on the PC over a UDP CAN bus. Needs TDM-GCC /
# MinGW-w64 gcc and mingw32-make on PATH.
#
#   mingw32-make        build pc_side_host.exe
#   mingw32-make run    build and run it (Ctrl+C to quit)
#   mingw32-make clean
#
# Not compiled - pc_hal.c stands in for what the firmware needs from them:
#   Drivers/STM32H7xx_HAL_Driver   register-level HAL
#   Common/Src/system_stm32h7xx_*  clock tree and vector table
#   Core/Src/stm32h7xx_hal_msp.c   peripheral pin/clock init
#   Core/Src/syscalls.c, sysmem.c  newlib retargeting
#   Core/Startup/*.s               reset handler
# =============================================================================

CC    := gcc
SHELL := cmd.exe

.DEFAULT_GOAL := all

FW  := ../CM4/Core/Src
EE  := ../Drivers/EEPROM/Src

# stubs/ first: it shadows stm32h7xx_hal.h and eeprom.h for every firmware TU.
INCS := -Istubs -I. -I../CM4/Core/Inc -I../Drivers/EEPROM/Inc

# -funsigned-char: ARM GCC's default char signedness, which the firmware's
# char flags and menu strings rely on.
# -malign-data=abi and no -fdata-sections keep the .menu_data globals
# contiguous and in declaration order (Save_Variables / Load_Variables walk
# NullVar..NullVar2 in sizeof(struct menu_var) steps); main.c checks this at
# startup.
# No -O: `while (Timer1);` spins on a non-volatile timer that the RTI thread
# decrements, and an optimizer would hoist the load out of the wait.
COMMON := -g -std=c11 -m32 -funsigned-char -malign-data=abi -ffunction-sections \
          -fno-toplevel-reorder -MMD -MP -DPC_SIDE

PROD_CFLAGS := -w $(COMMON)
HOST_CFLAGS := -Wall -Wno-unused-parameter $(COMMON)

LDFLAGS := -m32 -Wl,--gc-sections
LIBS    := -lws2_32

BUILD := build

FW_OBJS   := $(BUILD)/fw_main.o $(BUILD)/stm32h7xx_it.o $(BUILD)/Subroutines.o \
             $(BUILD)/canNodeIds.o $(BUILD)/MenuFunctions.o $(BUILD)/mco.o $(BUILD)/mcohw.o \
             $(BUILD)/user.o $(BUILD)/eeprom.o
HOST_OBJS := $(BUILD)/main.o $(BUILD)/pc_hal.o $(BUILD)/pc_side_host.o
OBJS      := $(FW_OBJS) $(HOST_OBJS)
EXE       := pc_side_host.exe

# Objects depend on this file too: the -D flags live here.
MKFILE := $(firstword $(MAKEFILE_LIST))

$(BUILD):
	-mkdir $(BUILD)

# The firmware's main() becomes fw_main(); pc_side/main.c owns main().
$(BUILD)/fw_main.o: $(FW)/main.c $(MKFILE) | $(BUILD)
	$(CC) $(PROD_CFLAGS) -Dmain=fw_main $(INCS) -c -o $@ $<

$(BUILD)/eeprom.o: $(EE)/eeprom.c $(MKFILE) | $(BUILD)
	$(CC) $(PROD_CFLAGS) $(INCS) -c -o $@ $<

$(BUILD)/%.o: $(FW)/%.c $(MKFILE) | $(BUILD)
	$(CC) $(PROD_CFLAGS) $(INCS) -c -o $@ $<

$(HOST_OBJS): $(BUILD)/%.o: %.c $(MKFILE) | $(BUILD)
	$(CC) $(HOST_CFLAGS) $(INCS) -c -o $@ $<

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.PHONY: all run clean
all: $(EXE)

# Kill a running host only if the relink needs the locked .exe.
run:
	@$(MAKE) --no-print-directory $(EXE) || ( \
	  echo *** relink blocked: a pc_side_host.exe is already running. && \
	  echo *** Stopping it - that terminal will report "Error 1". && \
	  taskkill /F /IM $(EXE) && $(MAKE) --no-print-directory $(EXE) )
	.\$(EXE)

clean:
	-rmdir /s /q $(BUILD) 2>nul
	-del /f /q $(EXE) 2>nul

-include $(wildcard $(BUILD)/*.d)
