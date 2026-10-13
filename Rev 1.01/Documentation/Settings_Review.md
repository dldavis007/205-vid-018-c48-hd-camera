# VID-18 settings and shared-stack review

The four firmware `mco.c`, `mcohw.c`, `mco.h` and `mcohw.h` files match
pre-migration commit `cb80077`. This review changes none of them. VID-19 was
not modified.

The original saved-variable block remains contiguous and in declaration order:
`NullVar`, `CamTag`, `disp_add`, `MicOnOff`, `TriggerMode`, `NullVar2`. Save/load continue walking
from the beginning variable to the ending variable. Although `disp_add` displays
the separately stored camera address, it keeps its legacy CSV position to avoid
shifting existing settings. New legacy settings belong before `NullVar2`.

`TriggerMode` now uses the same legacy save/load loop as the other menu settings.
Its separate write function and startup load call have been removed. Older CSV
records are recognized by their trailing blank marker; the old virtual-index-9
record is read only to import an existing trigger selection. Once the new CSV
is saved, the CSV selection takes precedence and index 9 is no longer written.
The five choices and enum values are unchanged.
PC flash persistence remains in `flash_<receive-port>.bin` in the host's working folder.

## Fixes

- Save always writes the terminating null byte, including an even-length CSV.
  Previously an even-length CSV could leave stale data beyond its end.
- Save validates strings and capacity before writing and does not skip empty
  fields in a way that would shift subsequent variables.
- Load checks its buffer bounds before unpacking additional EEPROM words.
- The menu label is now `TRIGGER`, consistently uppercase and short enough to
  stay clear of the six-character setting value.

The EEPROM addresses, token order and word packing remain compatible. Tests were
added for the even-length terminator, shorter replacement data and unterminated
EEPROM. Together with the existing settings and trigger migration tests, all nine pass against
the production menu code with a native GCC EEPROM dictionary stand-in.

The environment lacks 32-bit C libraries, so the normal `-m32` test build and
full Windows PC host were not run here. Run the normal settings tests on Windows:

```powershell
cd "Rev 1.01\tests"
mingw32-make test_subroutines
```

Rebuild CM4 in the STM32 environment and verify settings after restart on the
PC bench and hardware.
