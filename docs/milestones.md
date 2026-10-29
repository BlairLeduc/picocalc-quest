# Milestone log

What each milestone verified, on which board, on what date, and what was
not checked, newest first. `design.md` §16 holds each milestone's scope
and done-when criteria; this file keeps the full record.

**M1 (board bring-up) is run on the board**, on 2026-10-29, on a Pimoroni
Pico Plus 2 (RP2350B rev 2, 16 MB flash; radio not probed) in a PicoCalc,
flashed over SWD, with Arm GCC 15.2.Rel1 and SDK 2.3.1, development build.
**The owner's look at the panel is outstanding.**

- **Clocks** (`picocalc/board.c`, from pico-atom's 300 MHz path, §11.1):
  150 MHz, rail to 1.20 V, the flash's QMI divider and RX delay doubled
  from SRAM, PLL to 300 MHz, `clk_peri` onto `clk_sys`.
- **Log** (`picocalc/log.c`, §14.2): a `stdio_driver_t` into a 4 KB ring
  under a hardware spin lock, so upstream's `printf` goes through it from
  either core; it writes the UART directly until core 1 starts, then core 1
  drains it. UART0 on GP2/GP3 through UART_AUX (D12). Writes that do not
  fit are dropped whole and counted.
- **Core 1** (`picocalc/core1.c`): the southbridge, then the LCD (from
  pico-oric, with its sleeps made busy waits, HW §9.7), the corner-coded
  test pattern, then the log pump, a battery and temperature read each
  second, and the heartbeat's skeleton.
- **PSRAM** (`picocalc/psram_check.c`, §11.3): upstream's `psram_init()`,
  then a round trip before the heap: a word at every power-of-two offset,
  and 64 blocks of 1 KB spread over the 8 MB, plain and inverted, with the
  XIP cache cleaned and invalidated between write and read. On failure,
  core 0 stops before the heap and core 1 paints the picture's area red.
- **Clock** (`picocalc/clock.c`, §13.2): the shell's time, read on each side
  of the clock change, and the calendar arithmetic, with its host test.
- **Scripts**: `flash.sh` (from pico-oric: `reset halt` then `resume`) and
  `capture.sh` (refuses a port in use; timestamped log under `out/`).

**Checked on the board** (UART logs `out/m1-first.log`, `out/m1-second.log`,
not committed):

- The banner arrives, then **118 consecutive heartbeats** (1 to 118, none
  missing) with **0 I²C errors and 0 log writes dropped**.
- clk_sys and clk_peri 300,000,000 Hz, core rail 1,200 mV; flash clkdiv 6,
  RX delay 4 (**50 MHz**); PSRAM clkdiv 3, RX delay 3 (**100 MHz**).
- **PSRAM verifies at 300 MHz**: 32,790 words in 16.3–16.5 ms (two boots).
- Southbridge: I²C 10,000 Hz, firmware version 0, a register read
  4,840–4,853 µs; battery 100 %.
- LCD: SPI **75,000,000 Hz as configured** (not measured on SCK); init,
  reset to DISPON, 424 ms; the test pattern 19.4 ms.
- **The shell's clock**: set (the POWMAN record survived the SWD reset),
  read as 2026-10-29 14:18:09 UTC, offset +0:00, matching the workstation's
  UTC to the minute; across the clock change it moved 1–2 ms in 1.3–1.6 ms of
  uptime.
- Die temperature 21–23 °C over two minutes at 300 MHz, idle core 1, core 0
  in upstream's welcome and then its empty selector.
- Upstream's start-up continues: dlmalloc heap 7,680 KB, the SD card mounts
  on `spi0` at the configured 25 MHz (no `/quest` on the card used), and the
  selector reports 0 games.

**Host:** `test_clock` passes: every day from 1900 to 2199 against a
day-by-day model, offsets either side of UTC and across midnight and the
year, 2000 and 2100, before 1970, and the fallback.

**Measured** (development build): RAM used to 120,636 bytes, leaving
**338,116 of 448 KB** (the log ring is 4,096 of the 4,804 bytes added);
text 3,606,988 bytes. Core 1's loop ran 42,600–42,950 times a second while
core 0 ran the welcome screen and 49,750 once it was in the selector, a
sign that core 0's work slows core 1's fetches through the shared XIP
cache; M2 measures it properly.

**Found:**
- `flash_do_cmd` (here, reading the JEDEC id for the banner) puts back the
  bootrom's flash timing, so the first boot ran the flash at 100 MHz.
  The id is now read before the clock moves (HW §3, §11.4).
- Upstream's welcome screen, set for 10 s, ran about 85 s before the
  selector, with nothing presenting its frames; M2 looks at its timing.
- Upstream's `cabal_system_init()` still starts PS/2 on `pio0`/`pio1`
  and says the mouse failed; the M3 patch removes it.
- The crash record's magic reads as garbage at a cold boot, as expected of
  `.uninitialized_data`; upstream prints it and goes on.

**Not checked:** the test pattern on the panel (the owner's look: corners,
colours, edges); started from the shell, the time against the shell's own
display (the clock was read after an SWD reset, not a shell launch); the
release build on the board (built clean, not flashed); the temperature
under load.

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
`test_patch_roundtrip_repo` pass. The second applies the committed patch,
regenerates it, and checks that the new patch rebuilds the same files. It
compares files, not patch text: on CI's first run, GNU `diff` split
`board_config.h`'s hunks differently from macOS's BSD `diff`, for the same
edit. `scripts/build.sh`
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

CI's first firmware build was killed by the runner when an unbounded
`make -j` ran some 600 compilers at once; CI now runs `-j $(nproc)`.

**Checked on CI** (Ubuntu, GNU `patch` and `diff`, Arm GCC 13.2.Rel1, SDK
2.3.1), 2026-10-29, commit `354f6e7`: both host tests, `check_sources.sh`,
and both firmware builds pass. GCC 13.2's text is 3,566,940 bytes
(development) and 3,565,256 (release), about 28 KB less than 15.2's.

**Not checked:** `_unlink` deleting a save (M7); anything on the device.
