# =============================================================================
# Makefile  —  VID-018 host-side unit tests (Unity framework)
#
# Prerequisites
# -------------
#   1. MinGW-w64 gcc in PATH (MSYS2: pacman -S mingw-w64-x86_64-gcc)
#   2. Unity framework downloaded into tests/unity/
#      Run:  powershell -ExecutionPolicy Bypass -File get_unity.ps1
#
# Usage
# -----
#   make all          build and run both test suites
#   make test_eeprom  build and run EEPROM tests only
#   make test_menu    build and run menu-variable tests only
#  	make test_subroutines  build and run Save/Load_Variables and
#   make clean        remove built executables
#
# Include-path strategy
# ----------------------
# -Istubs is listed FIRST so the compiler finds our fake stm32h7xx_hal.h
# and fake eeprom.h before the real ones in the project driver directories.
# No production source file is modified.
# =============================================================================

CC     := gcc
CFLAGS := -Wall -Wextra -g -std=c11 -m32 \
          -ffunction-sections -fdata-sections \
          -Wno-unused-variable -Wno-unused-parameter

# test_subroutines needs globals in the standard .data segment (no per-symbol
# sections) so that five consecutive struct menu_var definitions in
# menu_globals_stub.c are exactly sizeof(struct menu_var) apart.  With
# -fdata-sections each global gets a PE sub-section padded to 32-byte
# alignment, creating 64-byte gaps instead of the required 40-byte steps.
SUBS_CFLAGS := -Wall -Wextra -g -std=c11 -m32 \
               -ffunction-sections \
               -Wno-unused-variable -Wno-unused-parameter

# Path from this Makefile's directory to the firmware project root
ROOT := ..

# Include search order: stubs first, then real project headers
INCS := -Istubs \
        -Iunity \
        -I$(ROOT)/CM4/Core/Inc \
        -I$(ROOT)/Drivers/EEPROM/Inc

# On Windows the linker flag --gc-sections removes functions and their
# symbol requirements that are unreachable from main(), greatly reducing
# the number of stubs needed for the menu_var target.
LDFLAGS := -m32 -Wl,--gc-sections

# Unity source (downloaded by get_unity.ps1)
UNITY := unity/unity.c

# =============================================================================
# Target: test_eeprom
#
# Compiles the real eeprom.c against the stub headers and fake flash memory.
# No CAN or CANopen code is involved.
# =============================================================================
EEPROM_PROD := $(ROOT)/Drivers/EEPROM/Src/eeprom.c
EEPROM_STUBS := stubs/hal_flash_stub.c

test_eeprom.exe: $(UNITY) $(EEPROM_STUBS) $(EEPROM_PROD) test_eeprom.c
	$(CC) $(CFLAGS) $(INCS) $(LDFLAGS) -o $@ $^

test_eeprom: test_eeprom.exe
	./test_eeprom.exe

# =============================================================================
# Target: test_menu_var
#
# Compiles MenuFunctions.c against stub headers, stub CAN implementations,
# and stub global variables.  eeprom.c is NOT compiled; EE_* functions are
# provided by mco_stub.c with safe no-op implementations.
# =============================================================================
MENU_PROD  := $(ROOT)/CM4/Core/Src/MenuFunctions.c
MENU_STUBS := stubs/mco_stub.c stubs/globals_stub.c

test_menu_var.exe: $(UNITY) $(MENU_STUBS) $(MENU_PROD) test_menu_var.c
	$(CC) $(CFLAGS) $(INCS) $(LDFLAGS) -o $@ $^

test_menu: test_menu_var.exe
	./test_menu_var.exe



# =============================================================================
# Target: test_menu_navigation
#
# Compiles MenuFunctions.c against stub headers, stub CAN implementations and
# menu_navigation_stub.c, which supplies the globals plus a fixture Menuc[]
# table built for the navigation paths (see the notes in that file).
# eeprom.c is NOT compiled; EE_* functions come from mco_stub.c.
# =============================================================================
NAV_PROD  := $(ROOT)/CM4/Core/Src/MenuFunctions.c
NAV_STUBS := stubs/mco_stub.c stubs/menu_navigation_stub.c

test_menu_navigation.exe: $(UNITY) $(NAV_STUBS) $(NAV_PROD) test_menu_navigation.c
	$(CC) $(CFLAGS) $(INCS) $(LDFLAGS) -o $@ $^

test_menu_navigation: test_menu_navigation.exe
	./test_menu_navigation.exe

# =============================================================================
# Target: test_subroutines
#
# Tests Save_Variables, Load_Variables, Save_Serial_Num, Load_Serial_Num.
# Links the real eeprom.c (for genuine EEPROM round-trips) against
# hal_flash_stub.c (fake flash), can_stubs.c (CAN no-ops, no EE stubs),
# and menu_globals_stub.c (menu_var globals with .menu_data section order).
# =============================================================================
SUBS_PROD  := $(ROOT)/CM4/Core/Src/MenuFunctions.c
SUBS_EEPROM := $(ROOT)/Drivers/EEPROM/Src/eeprom.c
SUBS_STUBS := stubs/hal_flash_stub.c stubs/can_stubs.c stubs/menu_globals_stub.c

test_subroutines.exe: $(UNITY) $(SUBS_STUBS) $(SUBS_PROD) $(SUBS_EEPROM) test_subroutines.c
	$(CC) $(SUBS_CFLAGS) $(INCS) $(LDFLAGS) -o $@ $^

test_subroutines: test_subroutines.exe
	./test_subroutines.exe

# =============================================================================
# Convenience targets
# =============================================================================
.PHONY: all test_eeprom test_menu test_subroutines test_menu_navigation clean

all: test_eeprom test_menu test_subroutines test_menu_navigation

clean:
	-del /f /q test_eeprom.exe test_menu_var.exe test_subroutines.exe test_menu_navigation.exe 2>nul

# Production trigger pairing and nonblocking scan tests (hardware stand-ins).
test_trigger_pairing.exe: ../CM4/Core/Src/Subroutines.c ../CM4/Core/Src/canNodeIds.c test_trigger_pairing.c
	$(CC) $(CFLAGS) -funsigned-char -I../pc_side/stubs -I../CM4/Core/Inc -I../Drivers/EEPROM/Inc $(LDFLAGS) -o $@ $^

test_trigger_pairing: test_trigger_pairing.exe
	./test_trigger_pairing.exe

all: test_trigger_pairing
.PHONY: test_trigger_pairing
