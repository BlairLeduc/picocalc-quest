# Milestone log

What each milestone verified, on which board, on what date, and what was
not checked, newest first. `design.md` §16 holds each milestone's scope
and done-when criteria; this file keeps the full record.

**M4 (first games, and the gate) is done**, on 2026-10-29, on the same
Pimoroni Pico Plus 2 W in a PicoCalc, flashed over SWD, with Arm GCC
15.2.Rel1 and SDK 2.3.1, development build. An AGI game played for 11
minutes, every engine's intro to a screen it could act in, and the
colours, checked by the owner. **The gate passes**, after a lever the
design did not have (below).

- **The gate failed first, on every title, 84–98 %** at the busiest 5 s.
  PC samples over SWD (`scripts/profile.sh`) put 64–83 % of core 0 in
  `memcpy` and `memset`, and the engines at 2–5 %: upstream's
  `updateScreen()` copied the game's 64 KB screen twice per call, PSRAM to
  PSRAM, 9.6 ms a copy, 35–45 calls a second, changed or not. None of
  M8's levers reach that. With the owner's agreement the first lever
  became cutting the copies (§7.2): a frame only when something shown has
  changed, and one copy instead of two. `memcpy` fell to 4 %.
- **Upstream launched each Gob, SCI and Kyra game as one fixed version**,
  whatever the files. Five of the six such titles on the card are CD
  releases.
  Gobliiins ran as the EGA floppy (dark, colours missing), Gobliins 2 as
  the VGA floppy (a still screen), Kyrandia as the floppy (a fault in its
  animation decoder), SQ4 without `ADGF_CD`. The launchers now match the
  folder's files against the engine's ScummVM table, as ScummVM's detector
  does, and launch with that entry (§10.2): `gob1cd` v1.02, `gob2cd`
  v2.01, `gob3cd` v1.02, `kyra1` CD, `sq3`, `sq4` CD, each named in the log.
  Kyra's table leaves language and platform to the entry, as
  `createInstance()` fills them; without that, `KYRA.DAT` was asked for an
  unknown platform.
- **Upstream knows one AGI folder**, `agi`; the card's `kq1` (the AGI
  original) launched as SCI's remake and stopped at `vocab 996`. The
  selector now takes `kq1`–`kq3`, `sq1`, `sq2`, `lsl1` and `pq1` as AGI
  when they hold `LOGDIR`, ahead of SCI; each AGI game saves under its own
  domain (§10.2).
- **`error()` froze the panel** (`exit()` ends in a breakpoint). It now
  shows a page with the message, and a key restarts (§13.1, `fatal.c`).
- **The mixer had never run** (the M0 shim kept its callback and never
  called it), so a sound never ended: Kyrandia's and SQ4's CD intros
  waited for their first voice line in loops that poll no input, deaf to
  Ctrl+Alt+Del. The shim now mixes silently into scratch at 44,100 Hz
  until M5 (§8.2).
- The heartbeat adds core 0's share in upstream's screen update and its
  calls; the selector logs both heaps at each entry; the M1 test pattern
  is drawn only with `PICOCALC_QUEST_TEST_PATTERN=ON`.
- **Scripts**: `gate.sh` (each title's busy share from a log, and the heap
  at each selector entry), `roundtrip.sh`, `profile.sh` (PC samples, HW
  §2.7). `PICOCALC_QUEST_SCREEN_CHECK=ON` builds the check of §7.2.

**Checked on the board** (UART logs `out/m4-*.log`, profiles
`out/m4-profile*.report`, not committed):

- **By the owner, on the keyboard**: King's Quest III played for **11
  minutes**, 2,035 key events, none lost or unknown, 0 I²C errors.
  Gobliiins, Gobliins 2 and Goblins 3 to their first interactive screens,
  colours right (after the table lookup); Kyrandia's intro through to its
  first scene; SQ4's menu and pointer, Enter, and its intro. With M3's
  Fate of Atlantis and SQ3, **every engine reaches an interactive
  screen.**
- **Ten selector round trips** (`roundtrip.sh`, King's Quest III and II
  in turn, 20 s each): all launched and quit cleanly. Over the 16 selector
  entries of that session, and 13 in the final pass, the heap was the same
  every time: **PSRAM 263,728 B in use, SRAM heap 0 B**.
- **The gate**, the busiest 5 s after the first 5 s of loading, core 0
  busy share (§14.3):

  | Title | Upstream's copies | Copies cut | Plus the silent mixer |
  |---|---:|---:|---:|
  | AGI: SQ2 / KQ1 / KQ2 / KQ3 | 84 (SQ2) | 15 / 27 / 15 / 41 | 22 / 28 / 18 / 44 |
  | Gobliiins / 2 / 3 | 85 / 87 / 98 | 20 / — / 21 | 22 / 20 / 28 |
  | Fate of Atlantis (intro) | 94 | 54 | **98** |
  | Kyrandia | — | — | 66 |
  | Space Quest III / IV | — | 36 / 14 | 62 / 19 |

  "Copies cut" is the gate as the design states it, audio not yet
  present: **every title under 75 %**. Gobliins 2 sent no frames then
  (its version was wrong); Kyrandia had not yet run. With the mixer
  running silently Fate of Atlantis is over, at a median of 70 %: MAME's
  OPL2 emulator alone is **48 %** of core 0 at 44,100 Hz (M5).
- **The error page**: SCI's `vocab 996` and Kyra's missing `KYRA.DAT`,
  each shown with its message, and a key restarted the program.
- **The update skip's check**: Gobliins 2 and SQ4 (then on the wrong
  versions) checksummed at every skipped update for 40 s each, 0 changes
  missed.
- 0 I²C errors and 0 ring full throughout; 813 log lines dropped, all
  upstream's Kyra heap walks while loading.

**Measured** (development build):

- Upstream's `cabal_update_screen()`: **9.6 ms a call** (a 64,000-byte
  copy PSRAM to PSRAM, 12,800 bytes cleared, the cache clean, the
  publish); the welcome screen spends 27–37 % of core 0 in it.
- `profile.sh`: about **4,000 samples a second** at 5 MHz SWD, the
  program running on.
- King's Quest III's help screen: **100 % of core 0** until a key, in
  AGI's "is a key pressed?" test, which polls without sleeping (§19).
- Core 1's stack 748 bytes at most (M3: 724).
- RAM used to 130,688 bytes, leaving **328,064 of 448 KB** (M3: 330,480);
  `.bss` 106,336, `.data` 21,924, text 2,673,212 plus `.rodata` 1,045,824
  (+129 KB, most of it the SCI and Kyra tables).

**Host:** the five tests pass, the patch round trip with the patch now
nine files; `check_sources.sh` 533 compiled, 7 excluded.

**Found:**
- The copies, the fixed versions, the one AGI folder, the frozen error
  and the mixer never run (above).
- With the probe attached, the halted core stops the timer, and core 1's
  `busy_wait_us` with it: no log, no display (HW §2.7). The log ring was
  read over SWD to find the error.
- Gob and the other point-and-click engines take only the arrows' codes
  from the keyboard; moving the pointer is M6.
- SCI games share one config domain, so their saves can collide (§19, M7).

**Not checked:** a full play of any title but King's Quest III; the
release build on the board; audio of any kind (M5); the pointer (M6);
saves (M7); the AGI help screen's busy-wait fixed (deferred, §19).

**M3 (card and keyboard) is done**, on 2026-10-29, on the same Pimoroni
Pico Plus 2 W in a PicoCalc, flashed over SWD, with Arm GCC 15.2.Rel1 and
SDK 2.3.1, development build. The selector, the games launching, every key
and Ctrl+Alt+Del checked on the keyboard by the owner; the no-card page
checked by the owner.

- **The keyboard** (§9.1): core 1 drains the MCU's FIFO every 25 ms into a
  64-entry ring of the MCU's own pairs, each with the time it was read
  (`picocalc/kbd.c`); core 0 owns the key map (`picocalc/keymap.c`), the
  held set and the translation, which the design had put on core 1 (§9.1
  says why it moved). Everything about a held key is fixed at its press,
  so a retranslated release (press `A`, release `a`; press Home, release
  Tab; Insert under Alt, released as `i`) gives the up that matches the
  down. ASCII as ScummVM's SDL backend gives it (§9.2).
- **The patch** to `rp2350-minimal.cpp` (§5.3): `cabal_poll_event()` takes
  keys from `picocalc_input_key()` and passes them to upstream's own
  Ctrl+Alt+Del detector unchanged; `cabal_system_init()` no longer starts
  the PS/2 mouse. The mouse block stays, fed nothing, for M6.
- **The card** (§10.1, §13.1): card detect, then the mount, timed, then
  `/quest`, all in `main.c` before upstream's `cabal_init()`. A failure
  shows a page core 1 draws in the 8x10 font (`picocalc/page.c`): no
  card, a card that did not mount, no `/quest`; and the PSRAM's, in place
  of M1's red fill. On the card's pages the program restarts itself once
  a card goes in.
- **The restart parks core 1** between southbridge transactions first
  (`picocalc_restart()`, `core1_park()`): see Found.
- **UART keys** (§14.1): printable bytes, `0x01 state code` for any MCU
  event, `0x02` to restart; `scripts/keys.sh` names them.
- The heartbeat adds keys taken, their mean and longest wait from the
  FIFO to the engine, ring-full, unknown and stray counts. Upstream's scan
  of `/quest` is timed by a link wrap of its C++ name.

**Checked on the board** (UART logs `out/m3-*.log`, not committed):

- **By the owner, on the keyboard**: moved through the selector with the
  arrows, launched Fate of Atlantis and Space Quest III, used SQ3's menus,
  and quit with **Ctrl+Alt+Del three times**, each back to the selector
  (D9 settled). Then every key alone and with Shift, Ctrl and Alt: **808
  events, every code the MCU's keymap can send seen except Caps Lock**
  (`0xc1`, not pressed), **none unknown to the map**. F6–F10 arrived as
  `0x86`–`0x89` and `0x90`; Alt+F as `0x46`, a capital. Two releases were
  dropped as stray, both explained: Del's, after the tear-down's flush;
  and one Space, released with no press after Alt (HW §6.2). The only keys
  left down in the log were held through a tear-down, which flushes.
- **By UART keys**: Space dismissed the welcome; Down, Up and Enter
  launched Fate of Atlantis; Ctrl+Alt+Del quit it, the heap back at
  7,680 kB.
- **The no-card page**, by the owner: powered on with the slot empty, the
  page; the card put in, the program restarted and mounted it.
- **Twenty restarts** through `picocalc_restart()`, asked for over the
  UART: the southbridge answered after every one (version 0), 0 I²C
  errors over 129 heartbeats.
- 109 consecutive heartbeats in the first run and 971 in the
  owner's session: **0 I²C errors, 0 log writes dropped, 0 ring full**.

**Measured** (development build):

- **Card mount** (upstream's `cabal_fs_init`, which includes FatFs's free
  space on a card with 30,739,936 KB free): **246 ms** for a card just
  put in, **63 ms** for one powered through an SWD reset.
- **Scan of `/quest`** (three folders, two games): **6 ms**, every time.
- **Key wait**, from core 1's read of the FIFO to the engine's poll, over
  the owner's 808 events: median **9.2 ms**, 90th percentile **15.4 ms**;
  four over 100 ms, the longest 1.4 s, while a game was loading and
  nothing polled. The MCU's scan (16 ms) and the poll interval (up to
  25 ms) come before it and are not measured.
- Core 1's stack 724 bytes at most (M2: 660).
- RAM used to 128,272 bytes, leaving **330,480 of 448 KB** (M2: 333,788);
  `.bss` 103,928, `.data` 21,916, text 2,670,368 plus `.rodata` 919,352.

**Host:** `test_keymap` passes: all 256 codes, the MCU's accepted and every
other refused; each key's plain and shifted codes one physical key and no
two keys one; the retranslated releases; Insert under Alt and Shift;
stray releases, repeats, lost presses; the ASCII per key and modifier;
the bound chords against the MCU's swallowed ones; Ctrl+Alt+Del as
upstream's detector wants it. Two planted faults (a missing shifted
alternate, a stray release accepted) each failed it.

**Found:**
- **A reset of the RP2350 in the middle of a southbridge transaction can
  leave the MCU answering nothing until the PicoCalc is switched off and
  on.** The first restart after a card went in did it: every transaction
  after failed (NACK of the address, 1.1 ms), 1,933 errors in 67 s, and
  the keyboard was dead though the screen looked right. Both lines read
  high over SWD; nine clocks and a STOP did not bring it back, nor did the
  MCU's own 2.5 s bus reset; the owner had seen it before, and a power
  cycle cleared it. M1 and M2 polled the bus once a second and never hit
  it; M3 is in a transaction about a fifth of the time. Every reboot now
  parks core 1 between transactions first (§6.2), and M7's exit must too.
  An SWD reset (`flash.sh`) can still do it; the banner says so when the
  southbridge does not answer (HW §6.1).
- Keys pressed while the previous image ran wait in the MCU's FIFO and
  arrive at the next boot; upstream's welcome discards what it finds.
- The MCU's power button sends `0x91` pressed with no release (source);
  it is accepted and not passed on. Not observed.

**Not checked:** Caps Lock (`0xc1`) on the keyboard; a card that does not
mount, and a card without `/quest` (their pages are drawn as the no-card
page is, but neither was shown); the PSRAM page; the release build on the
board (built clean, not flashed); key-to-event latency from the key's
travel, rather than from the FIFO.

**M2 (display) is done**, on 2026-10-29, on the same Pimoroni Pico Plus 2 W
in a PicoCalc, flashed over SWD, with Arm GCC 15.2.Rel1 and SDK 2.3.1,
development build. The welcome screen checked on the panel by the owner.

- **The frame pool** (`picocalc/framepool.c`, §7.2, from pico-oric's
  snapshot pool): three of upstream's 320×240 buffers in PSRAM, free,
  filling, ready and rendering, an older ready frame dropped at publish,
  every transition under one SIO spinlock (`picocalc/video_shim.c`). The
  palette is copied into the frame's record at publish. The third slot sits
  in the 512 KB upstream reserves below the mspace and no longer uses, so
  `psram_allocator.c` is not patched and the mspace keeps its 7,680 KB.
- **The shims**: `cabal_get_back_buffer()` claims a buffer if core 0 holds
  none, `cabal_swap_buffers()` publishes it; `rp2350-minimal.cpp` is
  unchanged.
- **The presenter** (`picocalc/core1.c`, §7.3): each ready frame at y = 40,
  each row expanded through its palette (`frame_expand_row`, in SRAM, a word
  per four pixels) into one of two line buffers while the other is on the
  wire; the log pumped every 16 rows. The first frame clears the test
  pattern from the bands.
- **The read strategy**, by `PICOCALC_QUEST_FB_READ`: cached, uncached
  (core 0 cleans the buffer's lines at publish), DMA from the uncached alias
  a row ahead (the default), or none (the control).
- **The heartbeat** adds core 0's busy share (wall time outside
  `cabal_delay`, wrapped at link time), frames published, presented and
  dropped, the present time and its longest, the longest publish, and core
  1's stack high-water mark from a stack painted before launch.

**Checked on the board** (UART logs `out/m2-*.log`, not committed):

- **The owner checked the welcome screen on the panel**: centred, black
  bands above and below, colours right, the animation moving.
- The DMA build: **109 consecutive heartbeats**, 0 I²C errors, 0 log
  writes dropped, 0 frames dropped. The welcome ran 88 s at 27–29 frames a
  second, every frame presented; then the selector found no `/quest` and
  upstream idled, core 0 busy 0.0–0.8 %.
- **Present, full 320×240**: 18.12–18.16 ms through the cache or the
  uncached alias, 18.6–19.1 ms by DMA (longest 21.9 ms); wire math 16.38 ms.
- **The read strategies against core 0** (§7.3), the same sitting, core 0's
  welcome frame rate over the same 15 seconds: none 29.9 a second; DMA
  28.0 (6 % less); uncached 27.6 (8 %); cached 26.5 (11 %). Core 0's
  publish, with the cache clean, took up to 0.45–0.69 ms; without it, 35 µs.
- **Core 1's stack**: 2 KB (SDK default), 660 bytes used at most.
- Core 0 is busy 100 % through the welcome in every build: the welcome
  asks for 62.5 frames a second and gets 26–30 at 300 MHz.

**Host:** `test_framepool` passes: the transitions, 200,000 random steps of
both cores' calls (core 0 never claims what core 1 holds, at most one
ready, each frame's palette the one published with it, every publish
taken, dropped or pending), RGB565 against a division model, and the row
expander at four source alignments. Two planted faults (a lost drop count,
a wrong pixel in the expander) each failed it.

**Measured** (development build, DMA): RAM used to 124,964 bytes, leaving
**333,788 of 448 KB** (M1: 338,116); `.bss` 100,684, `.data` 21,916, text
2,666,496 plus `.rodata` 916,960.

**Found:**
- Core 1's stack is the SDK's 2 KB, not the 4 KB SCRATCH_X holds (HW §9.5).
- The welcome's length is its ticker's: 375 characters at 60 pixels a
  second, about 88 s with the intro and outro, whatever the timeout passed.
  M1's 85 s was this, not a timing fault.
- The third framebuffer slot needed no patch (above).

**Not checked:** the selector's screen with games on the card (none on the
card used; M3); presenting with audio running (M5); the release build on
the board; the read strategies under a real engine (M8 measures again).

**M1 (board bring-up) is done**, on 2026-10-29, on a Pimoroni Pico Plus 2 W
(RP2350B rev 2, 16 MB flash; the image does not probe the radio, the
owner named the board) in a PicoCalc, flashed over SWD, with Arm GCC
15.2.Rel1 and SDK 2.3.1, development build. The test pattern checked on
the panel by the owner.

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
  reset to DISPON, 424 ms; the test pattern 19.4 ms. **The owner checked
  the pattern on the panel**: the four corners' colours, the border on all
  four edges, the colour bars and the grey ramp, all as drawn.
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

**Not checked:** started from the shell, the time against the shell's own
display (the clock was read after an SWD reset, not a shell launch; moved
to M7 by the owner, whose saves check it); the release build on the board
(built clean, not flashed); the temperature under load; the image on a
Plus 2 without the W.

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
