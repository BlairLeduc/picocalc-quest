# Milestone log

What each milestone verified, on which board, on what date, and what was
not checked, newest first. `design.md` §16 holds each milestone's scope
and done-when criteria; this file keeps the full record.

**M0 (skeleton, patch pipeline, CI) is built**, on 2026-10-29, on macOS
with Arm GCC 15.2.Rel1, Pico SDK 2.3.1 and BSD `patch` and `diff`. Not on
a device.

- **The submodule** `modules/frank-quest` at `949301b` (release 1.00).
- **The patch pipeline** (design.md §5.2). `scripts/apply_patch.sh` makes
  the build's copy (`frank-quest/`) or, with `--working`, the files the
  patch edits (`working/`). `scripts/create_patch.sh` walks `working/` in
  sorted order, without timestamps, and refuses new files and unresolved
  rejects. `cmake/upstream.cmake` copies and patches at configure time when
  a stamp of the submodule commit, the patch's hash and the script's hash
  changes; the patch and the submodule's HEAD are configure dependencies.
- **The source lists** (§5.4). `cmake/upstream-sources.cmake` holds
  upstream's lists, copied from its `CMakeLists.txt`, and seven excluded
  files, each with its reason: `src/main.c`, `drivers/HDMI.c`,
  `drivers/audio/audio.c`, the three PS/2 sources and
  `drivers/sdcard/pio_spi.c`. `scripts/check_sources.sh` reads it through
  CMake and reports 533 compiled and 7 excluded, matching upstream.
- **The first patch**: `src/board_config.h` (one PicoCalc block in place of
  M1/M2) and `drivers/sdcard/sdcard.c` (`SDCARD_CLK_FAST`, built at 25 MHz;
  `SDCARD_EXTERNAL_FATTIME`, not yet defined).
- **Shims** in `picocalc/`: `main.c` (upstream's start-up minus clocks, HDMI
  and USB, so the engine set is linked), `video_shim.c`
  (`graphics_set_palette` into a pending RGB565 table), `audio_shim.c`
  (silent; init reports no audio), `input_shim.c` (no keys, no mouse) and
  `syscalls.c`.
- **CI**: host tests, `check_sources.sh`, and the firmware with and without
  the UART, size printed, UF2s uploaded. `scripts/build.sh` builds with one
  command.

**Checked on the host:** `test_patch_roundtrip_synthetic` and
`test_patch_roundtrip_repo` pass. The second regenerates the committed
patch from `working/` and compares it byte for byte. `scripts/build.sh`
in a copy of the tree with no `build/`, `frank-quest/` or `working/` built
the UF2 in about two minutes. The stamp re-patches
when the patch changes and not otherwise. `check_sources.sh` reported a
planted missing engine file and a planted missing excluded file, which were
then removed. Both firmware builds link clean, with `picocalc/` under
`-Wall -Wextra -Werror`.

**Measured** (development build, UART on): flash `.text` 2,654,964 bytes,
`.rodata` 915,120, about 3.59 MB in all. SRAM: `.data` 21,124, `.bss`
92,280, `.uninitialized_data` 2,148. RAM is used to 115,832 bytes, leaving
**342,920 of 448 KB**. The release build is 1,700 bytes of text smaller.
The same source built from a copy in a longer directory path was 2,304
bytes larger, most likely from `__FILE__` strings, so compare sizes only
between builds from the same path.

**Found:**
- Upstream's `initBackend()` creates the mixer at a literal 44,100 Hz, so
  `rp2350-system.cpp` needs an audio patch in M5 (design.md §5.3, §8.2).
- Deleting a save reaches newlib's `_unlink`, which neither the SDK nor
  upstream provides; Arm GCC 15.2's newlib does not link without it.
  `picocalc/syscalls.c` sends it to upstream's `cabal_remove()`.
- `src/frank_quest_memory.cpp` is in upstream's tree but not built.
- `sdcard.c` does not include `board_config.h`, so the card's pins are
  compile definitions, as upstream's were.

**Not checked:** the scripts under GNU `patch` and `diff` (CI's Ubuntu
runner); CI itself, which has not yet run; the firmware with CI's Arm GCC
13.2; `_unlink` deleting a save (M7); anything on the device.
