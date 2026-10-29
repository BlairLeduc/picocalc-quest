# PicoCalc Quest: design

A port of [FRANK Quest](https://github.com/rh1tech/frank-quest), a ScummVM
(Cabal) adventure-game player for the RP2350, to the ClockworkPi PicoCalc
fitted with a Pimoroni Pico Plus 2 or Plus 2 W.

This document covers the shape of the port: what is taken from upstream
and how, what replaces FRANK's hardware, how the two cores share the work,
the memory budget, the milestones, and the constants and decisions still
open. It is written against two portable companions in this directory:

- `hardware-notes.md`: the authority on the PicoCalc. Cited as **HW §N**.
- `emulator-lessons.md`: what three earlier PicoCalc ports taught. Cited as
  **EL §N**. This project is not an emulator, but most of the port-side
  lessons (core ownership, frame handoff, audio, keyboard, measuring) apply
  unchanged.

Plain **§N** points within this document. Dates are written in full.
Estimates are labelled as estimates; everything else is read from source or
from the companions' measurements.

**Status, 2026-10-29.** Design only. Nothing is built. Upstream was read at
commit `949301b6ef87a5482340167378cd102fcdb82ef2` (2026-05-12, release
1.00).

**Contents**

1. [Goals and non-goals](#1-goals-and-non-goals) ·
2. [Decisions](#2-decisions) ·
3. [What upstream is](#3-what-upstream-is) ·
4. [Hardware mapping](#4-hardware-mapping) ·
5. [Source strategy: submodule, patch, shims](#5-source-strategy-submodule-patch-shims) ·
6. [Architecture](#6-architecture) ·
7. [Display](#7-display) ·
8. [Audio](#8-audio) ·
9. [Input](#9-input) ·
10. [Storage](#10-storage) ·
11. [Clocks, voltage, PSRAM and flash](#11-clocks-voltage-psram-and-flash) ·
12. [Memory budget](#12-memory-budget) ·
13. [Boot, time and exit](#13-boot-time-and-exit) ·
14. [Development workflow](#14-development-workflow) ·
15. [Testing](#15-testing) ·
16. [Milestones](#16-milestones) ·
17. [Unverified constants](#17-unverified-constants) ·
18. [Risks](#18-risks) ·
19. [Dropped and deferred](#19-dropped-and-deferred)

---

## 1. Goals and non-goals

**Goals**

- Run FRANK Quest's engine set (AGI, SCI0/SCI1/SCI1.1, SCUMM v1–v7, GOB,
  KYRA) on the PicoCalc, with games on the SD card in the same `/quest/<dir>`
  layout FRANK Quest uses, so a card prepared for one works in the other.
- Picture on the PicoCalc's LCD, centred vertically; sound from the
  PicoCalc's PWM audio; input from the PicoCalc's own keyboard, with a
  keyboard-driven pointer standing in for the mouse.
- Carry upstream as an unmodified submodule plus a patch, as
  `picocalc-rogue` carries Rogue, so that following upstream is a patch
  refresh rather than a merge.
- One release image, for the Pimoroni Pico Plus 2. It also runs on a
  Plus 2 W; no Wi-Fi is used.

**Non-goals**

- Any board without PSRAM. Upstream allocates its whole heap from the 8 MB
  PSRAM and does not boot without it (§3). Pico, Pico 2 and Pico 2 W are
  out.
- New engines or games beyond upstream's set. The port follows upstream's
  list; adding an engine is upstream's work or a later decision.
- ScummVM's launcher and GUI dialogs. Upstream builds with `DISABLE_GUI`
  and its own selector; so does this port.
- A USB host or PS/2 port. The PicoCalc has neither wired.

---

## 2. Decisions

Numbered and dated (EL §14.7). **Settled** decisions are the owner's and
are not reopened without the owner. **Proposed** ones are this document's
recommendation and need the owner's word before the milestone that depends
on them.

| # | Date | Decision | State |
|---|---|---|---|
| D1 | 2026-10-29 | Port by patching upstream source, kept as a submodule, as `picocalc-rogue` does. | Settled |
| D2 | 2026-10-29 | Target the Pimoroni Pico Plus 2 and Plus 2 W (RP2350B, 8 MB PSRAM). Ship one image, built for the Plus 2 (no Wi-Fi). | Settled |
| D3 | 2026-10-29 | Centre the picture vertically on the 320×320 panel. | Settled |
| D4 | 2026-10-29 | Do not depend on `picocalc-text-starter`; borrow from it where it helps. | Settled |
| D5 | 2026-10-29 | Ship at **300 MHz**, core rail 1.20 V, not upstream's 504 MHz at 1.65 V (§11). | Proposed; confirmed or revised at the M4 gate |
| D6 | 2026-10-29 | Reuse the PicoCalc drivers from `pico-oric` (same author, GPL-3.0) by copy, rename, re-verify (EL §14.6). | Proposed |
| D7 | 2026-10-29 | Licence the port GPL-3.0-or-later. Upstream's own code is GPL-3.0-or-later and its ScummVM code GPL-2.0-or-later, so the combination can only go out under GPL-3.0. | Proposed |
| D8 | 2026-10-29 | Keyboard pointer: arrows move it; `]` is the left button and `[` the right; **Alt+M** switches the arrows between pointer and arrow keys; each engine has a default mode (§9). | Proposed; settled by play-testing in M6 |
| D9 | 2026-10-29 | Keep upstream's quit-to-selector chord, Ctrl+Alt+Del, if the keyboard MCU delivers it (§9.5). | Proposed; checked in M3 |
| D10 | 2026-10-29 | No internal flash writes. Settings and saves live on the card (HW §7.1, EL §8.7). | Proposed |
| D11 | 2026-10-29 | Work with the PicoCalc shell as Rogue does: leave to the shell (or UF2 Loader's menu) through `picocalc_shell.h`, and take the date and time from the clock the shell keeps (§13). | Settled |
| D12 | 2026-10-29 | The development UART is UART0 on GP2 (TX) and GP3 (RX), through the pins' UART_AUX function, 115200 8-N-1, as the shell's. GP4/GP5 are `i2c0`'s, for the shell's DS3231, and the port leaves them alone (§4, §14.1). | Settled |

---

## 3. What upstream is

FRANK Quest (Mikhail Matveev) wraps **Cabal**, a community fork of ScummVM,
in an RP2350 platform layer. The whole Cabal tree is vendored into `src/`
(about 72 MB, 4,700 C/C++ files), and only a few hundred of those files are
compiled: the five engines and the parts of `common/`, `audio/`,
`graphics/` and `backends/` they need.

### 3.1 FRANK-specific code

| Path | Lines | What it does |
|---|---:|---|
| `src/main.c` | 201 | Clocks, voltage, flash QMI timing, USB-CDC stdio, PSRAM init, two framebuffers, HDMI init, then `cabal_main()` |
| `src/frank_quest_main.cpp` | 1,238 | Per-engine launchers; the outer selector → game → tear-down loop; `psram_reset()` between games |
| `src/frank_quest_selector.cpp` | 734 | Scans `/quest/*`, matches directory names to games, draws the animated list |
| `src/frank_quest_welcome.cpp` | 1,108 | The demoscene intro shown at power-on |
| `src/frank_quest_fs.c` | 474 | Thin file/dir layer over FatFs (`cabal_fs_*`, `cabal_dir_*`) |
| `src/frank_quest_new.cpp`, `_memory.cpp`, `_sbrk.c` | 134 | `new`/`delete` and `malloc` routed to PSRAM |
| `src/backends/platform/rp2350/rp2350-system.cpp` | 590 | The `OSystem` ScummVM sees. Calls the C seam below |
| `src/backends/platform/rp2350/rp2350-minimal.cpp` | 857 | **The C seam**: `cabal_update_screen`, `cabal_poll_event`, palette, cursor, timing, fault handler and crash record |
| `src/board_config.h` | 198 | M1/M2 pin layouts, screen size, PSRAM pin by package |
| `drivers/HDMI.c` | 755 | HSTX/PIO HDMI scan-out of an 8-bit indexed 320×240 buffer, via DMA IRQ |
| `drivers/audio/` | ~650 | I²S out over PIO at 44.1 kHz; the ScummVM mixer runs **inside the DMA IRQ** |
| `drivers/ps2/`, `drivers/usbhid/` | — | PS/2 keyboard and mouse over PIO; TinyUSB host HID |
| `drivers/sdcard/` | — | Elehobica's FatFs disk layer; PIO SPI **or hardware SPI** by define |
| `drivers/psram_init.c` | 88 | QMI CS1 set-up; timing derived from `clk_sys` |
| `drivers/psram_allocator.c`, `dlmalloc.c` | — | dlmalloc mspace over PSRAM; two fixed 96 KB framebuffer slots at its base |
| `linker/memmap_frank_quest.ld` | — | 448 KB RAM, 64 KB main stack, 4 KB each SCRATCH_X/Y |

`rp2350.cpp` and `rp2350.h` hold an older backend that is not compiled.

### 3.2 How it runs

- **One core.** Nothing calls `multicore_launch_core1`. The engine, the
  mixer (in the audio DMA IRQ), HDMI's DMA IRQ and input polling all run on
  core 0.
- **Video:** the engine draws into a 320×200 CLUT8 surface in PSRAM.
  `cabal_update_screen()` copies it into the back framebuffer (320×240, in
  PSRAM, 20 rows of letterbox above and below), draws the cursor there,
  then `cabal_swap_buffers()` hands it to HDMI. The palette is set entry by
  entry with `graphics_set_palette()`.
- **Audio:** `Audio::MixerImpl` at the I²S rate; `cabal_audio_*` owns three
  1,024-sample DMA buffers and mixes into each as it drains.
  `lockMutex` masks the audio IRQ to protect mixer state.
- **Memory:** `malloc`, `new` and the SDK allocator are all wrapped to a
  dlmalloc mspace in PSRAM. Returning to the selector deletes the
  `OSystem`, clears the singletons and wipes the mspace in one step.
- **Clock:** 504 MHz at 1.65 V by default (`vreg_disable_voltage_limit`),
  PSRAM capped at 133 MHz (100 MHz in release builds), flash at 66 MHz.
- **Image size:** the 1.00 release UF2 is 7,322,112 bytes, so about 3.6 MB
  of flash image.
- **No clock:** `OSystem_RP2350::getTimeAndDate()` makes up 1 January 2025
  plus the time since boot, and the SD driver's `get_fattime()` returns 0,
  so every file it writes is dated 1980. The engines read that date for
  save timestamps (SCUMM, SCI, AGI), SCI's wall-clock `kGetTime`, and
  scripts in SCUMM v6 and GOB.

---

## 4. Hardware mapping

| Function | FRANK (M2) | PicoCalc + Plus 2 | Port work |
|---|---|---|---|
| Display | HDMI over HSTX, GP12–19, 320×240 CLUT8 scanned continuously | ST7365P-class 320×320 LCD on `spi1`, GP10–15, RGB565, written on demand (HW §4) | New presenter on core 1 (§7) |
| Audio | I²S DAC over PIO, GP9–11, 44.1 kHz | PWM slice 5, GP26/27, RC filter, no DAC (HW §5) | New PWM ring; mixer kept in the IRQ (§8) |
| Keyboard | PS/2 over PIO, or USB HID host | Southbridge MCU at `i2c1` 0x1F, GP6/7, 10 kHz, translated key events (HW §6) | New input layer on core 1 (§9) |
| Mouse | PS/2 or USB | None | Keyboard pointer (§9.3) |
| SD card | `spi0` pins 4–7 or PIO SPI | `spi0`: MISO GP16, CS GP17, SCK GP18, MOSI GP19; detect GP22 (HW §7.1) | Pin defines and one clock constant (§10) |
| PSRAM | CS GP47 on RP2350B | CS GP47 on RP2350B (HW §2.4) | None; add a verify step (§11.3) |
| Console | USB CDC, or UART | UART0 on GP2 (TX) / GP3 (RX) through the Debug Probe, as the shell (§14.1) | stdio through a log ring (§14.2) |
| Real-time clock | — | The shell's clock in the POWMAN timer, which the shell keeps from a DS3231 on `i2c0`, GP4/GP5 | Read the POWMAN timer only (§13.2); never touch GP4/GP5 |
| Battery, backlight, power off | — | Southbridge registers (HW §6) | Power off on exit (§13) |

GP numbers are the same on the Plus 2 and Plus 2 W (HW §1.1). The port
must not touch GP23–25 or GP29: they are the radio's on a Plus 2 W, and the
Plus 2's board header puts its status LED on GP25 (HW §2.5). The image
therefore never lights the LED, and reads the battery from the southbridge,
not from VSYS.

GP4 and GP5 are not free either (HW §1.1): the shell uses them as `i2c0` (SDA GP4, SCL GP5) for an optional DS3231, and moved
its development UART to UART0 on GP2/GP3 to leave them alone (shell
`design.md` §3.8, 2026-10-22). The port follows the shell: its UART is on
GP2/GP3 (D12), and it never configures GP4/GP5, so a DS3231 fitted there
is undisturbed. GP0/GP1, UART0's usual pins, are where the PicoCalc's
USB-C serial is wired by default; the port does not use them.

---

## 5. Source strategy: submodule, patch, shims

### 5.1 Layout

```
picocalc-quest/
  CMakeLists.txt              the build root; ours
  cmake/upstream-sources.cmake  upstream files we compile, as lists (§5.4)
  modules/frank-quest/        submodule, pinned; never edited
  patches/frank-quest.patch   every change to an upstream file, one patch
  frank-quest/                gitignored: upstream copied and patched by the build
  working/                    gitignored: upstream files being edited, at their upstream paths
  picocalc/                   the port's own code (§5.3)
  test/                       host tests for picocalc/ (§15)
  scripts/                    apply_patch.sh, create_patch.sh, check_sources.sh,
                              build_release.sh, flash.sh, capture.sh
  docs/                       hardware-notes.md, emulator-lessons.md, design.md,
                              milestones.md (from M0)
```

### 5.2 The patch pipeline

As `picocalc-rogue`, with three changes that its size calls for:

1. **Patch at configure time, not build time.** Rogue's custom command
   copies and patches during the build and finds its sources by glob. Here
   the source lists are explicit (§5.4), and CMake needs the patched files
   to exist when it generates. So `CMakeLists.txt` runs the copy and the
   patch with `execute_process` when the patch file or the submodule commit
   has changed since the last copy (a stamp file holding both hashes), and
   lists the patch in `CMAKE_CONFIGURE_DEPENDS` so editing it re-runs
   configure.
2. **`create_patch.sh` walks `working/`** instead of naming files. Every
   file under `working/` is diffed against the same path under
   `modules/frank-quest/`, and the paths are rewritten to `a/` and `b/`.
   Adding a file to the patch is copying it into `working/`.
3. **No new files in the patch.** New code goes in `picocalc/`, as ordinary
   tracked source. The patch holds edits to upstream files only, so it stays
   reviewable and a submodule bump only conflicts where upstream changed a
   file we edit.

Following upstream: bump the submodule, run `apply_patch.sh`, fix rejects
in `working/`, run `create_patch.sh`, run `check_sources.sh` (§5.4), build.

### 5.3 Shims before patches

The cheapest way to keep the patch small is to provide, in `picocalc/`, the
functions upstream's platform code already calls, under the names it calls
them by. Then those upstream files compile **unchanged**.

| Upstream calls | Upstream implementation (not compiled) | PicoCalc implementation |
|---|---|---|
| `graphics_init`, `graphics_set_res`, `graphics_set_buffer`, `graphics_set_palette`, `graphics_set_bgcolor` | `drivers/HDMI.c` | `picocalc/video_shim.c`: palette into a pending table; buffers through the frame pool (§7.2) |
| `cabal_get_framebuffer`, `cabal_get_back_buffer`, `cabal_swap_buffers` | `src/main.c` | `picocalc/main.c` and the pool |
| `cabal_audio_init`, `_shutdown`, `_set_mixer_callback`, `_process_frame`, `_set_volume`, `_needs_samples`, … | `drivers/audio/audio.c` | `picocalc/audio_shim.c` over the PWM driver (§8) |
| `ps2kbd_init`, `ps2kbd_flush` | `drivers/ps2/` | `picocalc/input_shim.c`: start and flush the PicoCalc input queue |
| `main()` | `src/main.c` | `picocalc/main.c` (§13) |

Where upstream's semantics do not fit a shim, the upstream file is patched.
The expected patch, to be confirmed at M0:

| File | Change |
|---|---|
| `src/backends/platform/rp2350/rp2350-minimal.cpp` | `cabal_poll_event()` takes its events from the PicoCalc input layer (§9). Its PS/2 path cannot be shimmed honestly: it gets F-keys as private ASCII bytes that never become `CABAL_KEY_F*`, and it wants mouse deltas. `cabal_system_init()` loses the PS/2 and USB start-up. |
| `src/board_config.h` | M1/M2 layouts replaced by one PicoCalc block. |
| `drivers/psram_allocator.c` | A third framebuffer slot (§7.2). |
| `drivers/sdcard/sdcard.c` | `CLK_FAST` made overridable, set to 25 MHz (§10). Its `get_fattime()` (always 0) compiled out in favour of the PicoCalc one (§13.2). |
| `src/backends/platform/rp2350/rp2350-system.cpp` | `getTimeAndDate()` asks the PicoCalc clock (§13.2), keeping upstream's made-up date as the fallback. |
| `src/frank_quest_welcome.cpp`, `src/frank_quest_selector.cpp` | Text that names PS/2, mouse clicks or HDMI. |

Each patched hunk carries a comment citing the section of this document
that requires it (EL §14.1).

### 5.4 Which upstream files are built

Upstream's `CMakeLists.txt` is not used: it is a whole project (its own
`project()`, `pico_sdk_init()`, board, drivers and pins), and patching it
would put most of our build in the patch. Instead
`cmake/upstream-sources.cmake` holds the lists of upstream sources we
compile, copied from upstream's lists minus the HDMI, I²S, PS/2, USB HID
and PIO-SPI files, and with upstream's compile definitions (`ENABLE_*`,
`DISABLE_GUI`, `CABAL_LOW_MEMORY`, the dlmalloc flags, the `malloc` wraps,
`PICO_CXX_DISABLE_ALLOCATION_OVERRIDES`).

`scripts/check_sources.sh` extracts the `.c`/`.cpp` lists from upstream's
`CMakeLists.txt` and diffs them against ours, so a submodule bump that adds
or removes an engine file is reported, not silently missed. CI runs it.

Upstream's linker script is reused, by path into the patched copy: it gives
the 64 KB main stack the GOB engine needs.

### 5.5 Borrowed code

| Source | What | How |
|---|---|---|
| `pico-oric/src/port` (GPL-3.0, same author) | `board.c` (clocks, 300 MHz path, flash QMI, rail, identity, die temperature), `lcd.c` (init, polled-DMA ping-pong blit), `southbridge.c`, `kbd.c` (FIFO drain, held set, UART key injection), `log.c` (log ring and pump), `handoff.c` (three-buffer pool), `audio.c` (PWM ring) | Copy, rename prefixes, re-verify, bring the tests (EL §14.6) |
| `picocalc-text-starter` (MIT, same author) | `font-8x10.c` for the status band (§7.4), if the pool's text needs a font | Copy with attribution |
| `picocalc-rogue` | `picocalc_shell.h` (return to the shell or UF2 Loader, and the shell's clock; MIT as well as GPL); its `return_to_loader()` and `set_clock()`; the release script's shape | Copy; take the header from the shell's repository when it is newer |

`pico-oric`'s `audio.c` is a push queue paced by its producer; here the
mixer pulls inside the refill IRQ (§8), so it is adapted, not copied.
Anything moving to core 1 is searched for `sleep_` (EL §2.3, HW §9.7).

---

## 6. Architecture

### 6.1 Which core owns what

Upstream runs on one core. The port splits it as the emulators do
(EL §2.3), with one difference: the engine does its own file I/O, so the
**SD card stays on core 0**.

| Core 0 | Core 1 |
|---|---|
| ScummVM engine, `OSystem`, selector, welcome | LCD presenting (§7) |
| Mixer, in the audio DMA IRQ at priority `0x40` (§8) | Southbridge I²C: keyboard FIFO, battery (§9) |
| SD card and FatFs, as the engine asks | Draining the log ring to UART0 (§14.2) |
| Pointer motion from held keys (§9.3) | |

```
            core 0                                         core 1
 ┌─────────────────────────────┐                 ┌───────────────────────────┐
 │ engine ─► 320×200 surface    │                 │ southbridge poll (~40 Hz) │
 │   (PSRAM)                    │   key events    │   held set, canonical     │
 │ cabal_update_screen          │ ◄────SPSC───────┤   codes                    │
 │   copy + letterbox + cursor  │                 │                           │
 │   into a pool buffer (PSRAM) │  pool: ready    │ take ready buffer         │
 │ swap = publish ──────────────┼───────────────► │   palette → RGB565 rows   │
 │                              │                 │   DMA to LCD, rows 40–279 │
 │ DMA IRQ (0x40): mix → PWM    │                 │ release buffer            │
 │ SD/FatFs (spi0)              │   log ring      │ pump log → UART0          │
 │ printf ─► log ring ──────────┼───────────────► │                           │
 └─────────────────────────────┘                 └───────────────────────────┘
```

### 6.2 Invariants

- **The LCD is touched only from core 1, in thread context** (HW §5.7). No
  interrupt masking around a blit.
- **Core 1 never reads engine memory.** It reads only a published pool
  buffer and the palette that came with it (§7.2).
- **No `sleep_*` on core 1**; it waits with `busy_wait_us_32` (HW §9.7).
- **Core 0 never blocks on the UART.** stdio goes into the log ring
  (§14.2).
- **Everything on the audio refill path is in SRAM**, or measured to be
  harmless in flash (§8.3).
- **No flash writes** (D10).

---

## 7. Display

### 7.1 Geometry

Upstream's framebuffer is already 320×240 with the 320×200 game letterboxed
in it, 20 rows above and below. The port draws that framebuffer **at
y = 40**, centred on the 320-row panel:

| Panel rows | Content |
|---|---|
| 0–39 | Status band, normally black (§7.4) |
| 40–59 | Upstream's top letterbox (black) |
| **60–259** | **The 320×200 game picture**, centred: 60 rows above, 60 below |
| 260–279 | Upstream's bottom letterbox (black) |
| 280–319 | Status band |

So both readings of "320×200 (240?)" hold: the 240-row framebuffer is
centred, and so is the 200-row game inside it. The welcome screen and the
selector, which use all 240 rows, are centred too. A game that sets a
height other than 200 is letterboxed by upstream's `cabal_update_screen`
within the 240, which stays centred.

Pixels are 1:1. Nothing is scaled (EL §5.4).

### 7.2 The frame pool

Upstream double-buffers, which is safe only because HDMI scan-out never
falls behind. A presenter on the other core can, so the two framebuffers
become the **three-buffer pool** of EL §2.4: `free`, `filling`, `ready`,
`rendering`, with an older `ready` dropped at publish, each transition
under one SIO spinlock, and a dropped-frame counter.

- The buffers stay in PSRAM, in fixed slots below the mspace, as upstream's
  two do, so `psram_reset()` between games leaves them alone. The third
  slot costs 96 KB of the 8 MB.
- `cabal_get_back_buffer()` returns the buffer core 0 is filling, claiming
  one if it holds none. `cabal_swap_buffers()` publishes it.
  `cabal_update_screen()` calls both and needs no change.
- **The palette travels with the frame.** `graphics_set_palette()` writes
  RGB565 into a pending table on core 0; publishing copies the 512-byte
  table into the buffer's record. Core 1 never sees a palette half-written,
  and a palette change with identical pixels (fades, colour cycling) is
  still a change (EL §5.3, step 1).

### 7.3 The presenter

Core 1 loop: take the `ready` buffer; for each row, expand 320 indexed
pixels through its palette into one of two RGB565 line buffers while DMA
sends the other (HW §4.6); release the buffer; poll the keyboard when due;
pump the log.

Costs, **estimated** from HW §4.7 (a 240×224 window measured at 12.58 ms
on a Plus 2 W at 75 MHz):

| Present | Pixels | Estimate |
|---|---:|---:|
| Full 320×240 | 76,800 | ~18 ms wire and DMA, plus PSRAM reads |
| Game area only, 320×200 | 64,000 | ~15 ms |

**The PSRAM read is the open question.** HW §9.3 measured a PSRAM-to-SRAM
copy at about 118 ns a pixel, which would add ~9 ms to a full present if
it held here, and those reads go through the 16 KB XIP cache that core 0's
3.6 MB of engine code also needs. M2 measures three ways of reading the
frame: through the cached window, through the uncached alias `0x15000000`
after a `xip_cache_clean` of the buffer at publish, and a DMA from PSRAM
into an SRAM row. It keeps the one that costs core 0 least, measured on
core 0, not core 1.

Adventure games redraw at roughly 6–20 frames a second, so even a full
present each frame leaves core 1 idle much of the time. Dirty-row
presenting (§16, M8) is an optimisation, not a requirement.

**Tearing**: there is no TE line (HW §1.2). Accepted.

### 7.4 The status band

The 40-row bands above and below are outside anything upstream draws. The
bottom band shows the input mode (`POINTER` or `KEYS`, §9.3) for two
seconds after it changes, and transient messages (card missing, save
written). It is drawn by core 1 only when its text changes (EL §5.4), with
the starter kit's 8×10 font. It is not a perf line in release builds.

---

## 8. Audio

### 8.1 Output

PWM on GP26/27, both channels of one slice, one 32-bit compare word per
stereo frame, two chained DMA channels over a power-of-two aligned ring
with the hardware read wrap, both read address and count reset on re-arm
(HW §5.1–5.3).

At 300 MHz, divider 1, **TOP = 4095** gives a 73.2 kHz carrier at 12 bits,
and oversample 2 gives a sample rate of **300,000,000 / 8,192 =
36,621.09 Hz** (HW §5.2). All of it is computed from `clock_get_hz(clk_sys)`
at init, never written as a constant.

### 8.2 The mixer stays in the IRQ

Upstream mixes inside the DMA IRQ, and its commit history says why: mixing
from the main loop underran whenever an engine frame took longer than a
buffer. Its `lockMutex` also masks that IRQ to protect mixer state, which
works only because the mixer runs on the same core. Moving the mixer to
core 1 would break that protection, so the port keeps upstream's shape:

- `Audio::MixerImpl` is created at **36,621 Hz** (the integer part; the
  mixer only resamples sources to it, so the 0.0003 % is inaudible and
  feeds no long-running timing).
- Each refill IRQ calls upstream's mixer callback for one half of the ring,
  converts the stereo `int16` frames to compare words (bias, scale to 12
  bits, each written twice for oversample 2), and re-arms the channel.
- `DMA_IRQ_0` at priority `0x40`, above everything else on core 0 (HW §5.4).
- The audio shim keeps upstream's `cabal_audio_*` names and its startup
  mute, so `rp2350-system.cpp` needs no audio changes.

### 8.3 Ring size and deadline

The deadline is one half of the ring. The mixer must finish a half in
less than that, every time, including whatever OPL (AdLib) emulation the
game uses, and including any time `lockMutex` holds the IRQ off.

Starting point: **512 frames a half** (14.0 ms, 4 KB a half, an 8 KB
aligned ring). M5 measures the mixer's worst time per half with OPL music
playing, and resizes. Counters (HW §5.8, EL §6.2): late refills (channel
already running on entry), the mixer's longest run, and its share of
core 0.

If the mixer costs too much, the levers in order: put the mixer, the rate
converter and the OPL emulator in SRAM (§16, M8); a lower mixer rate (TOP
4095 with oversample 3 is 24.4 kHz); upstream's cheaper OPL core.

---

## 9. Input

### 9.1 From the southbridge to the engine

Core 1 drains the southbridge FIFO about every 25 ms (HW §6.1, 4.8 ms a
transaction), builds the held-key set from press and release, canonicalises
shifted releases, and pushes **press/release events** with their translated
codes into an SPSC queue to core 0 (EL §7.1, HW §6.2). Auto-repeat arrives
from the MCU as extra presses; they are kept, flagged as repeats, because
typing in a parser game wants them and the pointer does not.

On core 0, the patched `cabal_poll_event()` (§5.3) pops the queue and the
pointer state (§9.3) and returns `CabalEvent`s: key down and up with
`CABAL_KEY_*` codes and ASCII, mouse motion, button down and up.

### 9.2 Key map

| PicoCalc | Code | To the engine |
|---|---|---|
| Letters, digits, punctuation | ASCII as sent | Key with that ASCII; Shift already applied by the MCU |
| Enter | `0x0a` | `RETURN` |
| Backspace | `0x08` | `BACKSPACE` |
| Tab | `0x09` | `TAB` |
| Esc | `0xb1` | `ESCAPE` (skip cutscene, cancel) |
| Del | `0xd4` | `DELETE` |
| F1–F5 | `0x81`–`0x85` | `F1`–`F5` |
| F6–F10 (Shift+F1…F5) | `0x86`–`0x90` | `F6`–`F10` |
| Arrows | `0xb4`–`0xb7` | Pointer, or arrow keys (§9.3) |
| Home, End, PgUp, PgDn, Insert | shifted alternates (HW §6.3) | Their keys; each with its own binding (EL §7.2) |
| Caps Lock | `0xc1` | Accepted, not passed on |
| `[`, `]` | ASCII | Pointer buttons in pointer mode (§9.3); themselves in keys mode |
| Alt, Ctrl, Shift | `0xa1`, `0xa5`, `0xa2`/`0xa3` | Modifier flags on later keys; Ctrl and Alt also for the chords in §9.5 |

The F7–F9 codes are not yet observed on hardware (§17).

The table is data in `picocalc/`, and a host test checks that every code
the MCU can send maps to exactly one entry and that no binding needs a
chord the MCU swallows (EL §7.2): Shift with an arrow, Space or Backspace,
and Alt with `,` `.` Space `B`, are unreachable (HW §6.3).

### 9.3 The pointer

Most of these games are point-and-click. The PicoCalc has no mouse, so the
arrows drive one:

- **Motion** is computed on core 0 from the held set and elapsed time, not
  from key events: one pixel on the press, then accelerating while held
  (for example 40 px/s rising to 240 px/s over 600 ms; tuned in M6). It is
  frame-rate independent and immune to the MCU's 300 ms/100 ms repeat
  cadence.
- **Ctrl+arrow** moves exactly one pixel per press, for fine placement.
  Ctrl chords reach the host intact (HW §6.3).
- **Buttons**: `]` is the left button and `[` the right, held for as long as
  the key is (so dragging works). Shift is never part of a pointer binding,
  because Shift with Left or Right sends nothing, not even a release
  (HW §6.3).
- **Mode**: **Alt+M** switches the arrows between pointer mode and keys
  mode, in which they are arrow keys and `[` `]` type themselves. Each
  engine starts in the mode its games expect: keys for AGI (the hero walks
  with the arrows), pointer for SCUMM, KYRA, GOB and SCI. SCI0 games take
  both; they start in pointer mode. The status band names the mode when it
  changes (§7.4).

The pointer is clamped to the game surface. Upstream draws the cursor into
the frame on core 0, so the presenter needs nothing for it.

D8 is proposed, not settled: the button keys and the acceleration curve
are chosen by playing one title per engine on the device in M6, and a
`/quest/picocalc.ini` setting can rebind them (§10.3).

### 9.4 Typing

Parser games (AGI, SCI0) need the whole keyboard. In both modes letters,
digits, punctuation, Enter and Backspace go to the engine as keys; only
the arrows and `[` `]` change meaning with the mode. Repeats arrive as
further key-downs, which is what a text field wants.

### 9.5 Chords

| Chord | Action |
|---|---|
| Ctrl+Alt+Del | Quit the game and return to the selector (upstream's; D9) |
| Alt+M | Pointer/keys mode |
| Alt+`,` `.`, Alt+Space, Alt+B | Taken by the MCU: backlights and battery (HW §6.3). Never bound |

Upstream detects Ctrl+Alt+Del from HID codes. The patched poll sets the
same `g_fq_quit_pending` flag from PicoCalc codes, so upstream's quit path
runs unchanged. Whether the MCU delivers Del with Ctrl and Alt both held is
checked on the device in M3; if not, Alt+Q replaces it.

---

## 10. Storage

### 10.1 Driver

Upstream's `sdcard.c` already supports the hardware SPI peripheral when
`SDCARD_PIO` is not defined, so the PicoCalc's card needs only its pins:

```
SDCARD_PIN_SPI0_MISO=16  SDCARD_PIN_SPI0_CS=17
SDCARD_PIN_SPI0_SCK=18   SDCARD_PIN_SPI0_MOSI=19
```

`CLK_SLOW` is 100 kHz, which suits initialisation. `CLK_FAST` is a fixed
30 MHz; the patch makes it overridable and sets 25 MHz, the conservative
operating point of HW §7.1. `pio_spi.c` and `spi.pio` are not compiled.

The card stays on core 0 (§6.1): the engine reads resources, and SCUMM v7
streams speech and music, synchronously from wherever it is. Card latency
(7–45 ms a transaction, HW §7.1) is the engine's to absorb, as it was on
FRANK. Audio is unaffected because the mixer runs from the IRQ; the LCD and
keyboard are on core 1.

Card detect (GP22, active low, pull-up) is read once at boot to show "no
card" instead of an empty selector. Removal while running is not handled.

### 10.2 Card layout

Upstream's, unchanged, so a card works on both:

```
/quest/<game dir>/     game data, names as upstream's README tables
/quest/saves/          ScummVM saves (upstream's DefaultSaveFileManager path)
/quest/picocalc.ini    this port's settings (§10.3)
```

The selector skips macOS's `._` files (HW §7.1) if upstream does not
already.

### 10.3 Settings

A `key = value` text file, read at boot on core 0 before the engine starts,
with every default in one function (EL §8.7). Planned keys: the two button
keys, pointer speed, per-engine starting mode, volume. It is written only
by the user in v1; a menu that saves it is deferred (§19).

---

## 11. Clocks, voltage, PSRAM and flash

### 11.1 The clock (D5)

Upstream ships at 504 MHz with the core at 1.65 V and the voltage limit
disabled. The PicoCalc evidence covers 150 and 300 MHz (HW §3), and the
LCD's SPI divider favours exactly those two: at 300 MHz `clk_peri / 4` is
the panel's 75 MHz, at 504 MHz it is 63 MHz. A 1.65 V core on a
battery-powered handheld with no airflow is also untested territory.

So the port ships at **300 MHz at 1.20 V**, with the bring-up order of
EL §9.4: start at 150 MHz; rail to 1.20 V; flash QMI divider and RX delay
doubled from SRAM; PLL to 300 MHz; `clk_peri` onto `clk_sys`
(`PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=1`); only then stdio, PSRAM,
I²C, LCD, audio and SD. The rail is also set explicitly at power-on in
case another firmware left it high (HW §3).

Whether 300 MHz is fast enough is the M4 gate (§16). If it is not, the
levers come in the order of §16 M8, and only then a higher clock, as an
owner decision with its own SPI, PWM, PSRAM and thermal checks.

### 11.2 Board target

`PICO_BOARD=pimoroni_pico_plus2_rp2350` (SDK 2.3.1 has it: RP2350B,
16 MB flash, PSRAM CS on GP47). The same image runs on a Plus 2 W as long
as it leaves GP23–25 and GP29 alone (§4). The banner logs the physical
board separately from the build target (HW §2.1): flash size from the
flash ID, package from `SYSINFO`, and whether the CYW43 answers is **not**
probed.

### 11.3 PSRAM

Upstream's `psram_init()` derives the QMI timing from `clk_sys` when it
runs, which is correct as long as it runs after the clock moves (HW §3,
item 3); `main()` guarantees that. At 300 MHz with a 100 MHz cap it gets
divider 3 (100 MHz).

Upstream does not check the chip works at speed. The port adds the round
trip of HW §2.4 after init (patterns at spread offsets, clean and
invalidate the cache, read back), and shows a message naming the failure
on the panel instead of booting into a corrupt heap.

### 11.4 Flash

The image is ~3.6 MB of 16 MB. Code runs from flash through the 16 KB XIP
cache shared with PSRAM, and ScummVM is large and branchy, the kind of code
whose speed is instruction fetch (HW §9.2). The bootrom's divider doubled
puts flash at 50 MHz at 300 MHz (HW §3); upstream ran it at 66 MHz. A
faster flash clock (divider 4, 75 MHz) is a lever for M8, qualified for
data integrity before use.

There is no no-flash build: the image is seven times SRAM.

---

## 12. Memory budget

### 12.1 SRAM

Upstream's linker script allots 448 KB to RAM (code in SRAM, data, BSS,
heap) and 64 KB to the main stack, with 4 KB each in SCRATCH_X (core 1's
stack) and SCRATCH_Y. Upstream's actual use is not yet known; M0 reads it
off `arm-none-eabi-size` and the map, and CI prints it on every build
(EL §2.1).

What the port adds, **estimated**:

| Item | Bytes |
|---|---:|
| Two RGB565 line buffers | 1,280 |
| Two palettes in RGB565 (pending, presented) | 1,024 |
| PWM ring, 2 × 512 frames × 2 slots × 4 B | 8,192 |
| Key event queue, 64 entries | 256 |
| Log ring | 4,096 |
| Pool records, counters, status band | < 1,024 |
| **Total added** | **~16 KB** |
| Removed: HDMI, I²S, PS/2, USB HID buffers and code | to be measured |

Core 1's 4 KB stack must hold the presenter, the southbridge and the log
pump; M2 measures its high-water mark with a painted stack.

SRAM placement of hot engine code (M8) spends whatever is left, in measured
tiers.

### 12.2 PSRAM (8 MB)

| Region | Upstream | Port |
|---|---:|---:|
| Framebuffer slots | 2 × 96 KB | 3 × 96 KB |
| Scratch | 512 KB | 512 KB |
| dlmalloc mspace (all engine allocations) | the rest | the rest, 96 KB less |

### 12.3 Flash

~3.6 MB of 16 MB. Ample for the image; nothing else is stored in flash.
UF2 Loader's application partition must also hold it (§17).

---

## 13. Boot, time and exit

The port works with the PicoCalc shell the way `picocalc-rogue` does
(D11), through the one header the shell publishes, `picocalc_shell.h`
(MIT or GPL-3.0). The shell starts a program by having UF2 Loader flash it
and reboot; the header covers the two things a program does with the shell
after that: read the time it keeps, and go back to it.

### 13.1 Boot

Clocks (§11.1); log ring and stdio; banner (board, chip revision, clocks,
rail, PSRAM result, southbridge version, the shell's time if set, version
from `git describe`); southbridge; LCD cleared before `DISPON`; launch
core 1; PSRAM verify; card check; audio; then upstream's `cabal_init()`
and `cabal_main()`: welcome, selector, game.

A failure before the selector (no PSRAM, no card, no `/quest`) shows a page
naming it, never a blank screen (EL §8.1).

### 13.2 The date and time

The PicoCalc has no battery-backed clock of its own (HW §1.2). The shell
keeps UTC in the RP2350's always-on (POWMAN) timer, which survives the
reboot into a program, with a checked record in POWMAN scratch registers
0–2 that holds the local offset in minutes.
`picocalc_shell_time(&utc_ms, &offset_min)` returns both, or false when the
clock is not set (power-on with the stock loader, which starts the last
program without the shell).

`picocalc/clock.c` gives the port one function, `picocalc_clock_now()`,
returning local broken-down time and whether it is real:

- **When the shell's clock is set**, local time is `utc_ms / 1000 +
  offset_min × 60`, converted to calendar fields. It reads the timer **on
  every call**, rather than once at boot as Rogue's `set_clock()` does into
  newlib, so it stays right however long a game runs, and depends on
  nothing else in the C library.
- **When it is not set**, it returns upstream's made-up date (1 January
  2025 plus uptime), so games that read the clock still see it advance.

Two callers:

- `OSystem_RP2350::getTimeAndDate()`, patched (§5.3): the date in save
  descriptions and slots, SCI's wall-clock `kGetTime`, and the SCUMM v6 and
  GOB scripts that read the date.
- `get_fattime()`, replacing the SD driver's (§5.3): save files and
  anything else written to the card carry the real date and time; with no
  clock set, FatFs's fixed default.

What the port must **not** do, by the header's contract: stop the timer,
change its tick source, set it other than through
`picocalc_shell_set_time()` (which the port has no reason to call), or
write POWMAN scratch 0–2. The timer runs from its own tick source, so the
move to 300 MHz (§11.1) should not disturb it; M1 checks that the time read
after the clock change agrees with the time read before it.

The build links `hardware_powman`, which defines
`PICOCALC_SHELL_HAS_TIME`. The conversion from milliseconds and offset to
calendar fields is pure code and has a host test (§15).

### 13.3 Exit

Upstream has no exit; its selector idles if nothing is chosen. The port
adds an exit key to the selector (Esc), and the selector's hint line names
where it goes, decided before showing it, as Rogue's `loader_available()`
does:

1. **To the shell**, when UF2 Loader started the image
   (`picocalc_shell_loader_present()`) and a card is in
   (`picocalc_shell_card_present()`). `picocalc_shell_exit()` sets the
   command in the watchdog scratch registers and reboots; UF2 Loader's
   stage 3 then loads `/BOOT2350.UF2` (the shell, or the loader's own menu
   when the shell is not installed) instead of starting the port again.
2. **Otherwise, power off** through the southbridge
   (`sb_write_power_off_delay`), as Rogue does when there is no loader:
   the image was copied on in BOOTSEL, and a reboot would only start it
   again. If the southbridge firmware cannot power off, it says so and
   stays in the selector.

Before either: nothing may be left half-written on the card, since the
reboot does not wait. At the selector the engine has been torn down and
its files closed by upstream's `cabal_fs_session_reset()`; the exit path
also unmounts FatFs, which flushes it.

Quitting a game always returns to the selector (Ctrl+Alt+Del, or the
game's own quit), through upstream's tear-down, which frees the whole
mspace. Leaving for the shell is one more key from there. Holding Up, F1 or
F5 at power-on also returns to the shell, for any program; that is UF2
Loader's, and needs nothing from the port.

---

## 14. Development workflow

### 14.1 Probe and UART

The owner can connect a Raspberry Pi Debug Probe with SWD and UART to the
PicoCalc, so development follows EL §13.1 and HW §2.7:

- **Flash over SWD** with OpenOCD and `target/rp2350.cfg`, using
  `reset halt` then `resume`, never `reset run`, or core 1 can be lost
  (HW §2.7). `scripts/flash.sh` does this.
- **Capture UART0** (TX GP2, RX GP3, 115200 8-N-1) before flashing so the
  banner is in the log. `scripts/capture.sh` refuses a port another
  process holds, and writes a timestamped log. The probe's UART wires go
  to GP2 and GP3 (crossed: probe RX to GP2) and ground, the same wiring as
  for the shell's development build, so one setup serves both.
- **The UART set-up is the shell's**: `PICO_DEFAULT_UART=0`,
  `PICO_DEFAULT_UART_TX_PIN=2`, `PICO_DEFAULT_UART_RX_PIN=3`. GP2 and GP3
  are UART0's CTS and RTS pins, and the SDK selects their UART_AUX
  function, which carries TX and RX there on the RP2350. The shell's
  development build has run this way on a Plus 2 W since its M12. The
  log code taken from `pico-oric`, which assumed UART1 on GP4/GP5, is
  changed to use `uart_default` and these defines only.
- **Keep the Mac's display awake** (`caffeinate -d`) while the probe is in
  use; a sleeping display wedges it (HW §2.7).
- **Type over the UART**: the development build turns received bytes into
  PicoCalc key events, with reserved bytes for the pointer buttons and the
  chords, so a game can be driven by a script with nobody at the keyboard.

### 14.2 Two builds

- **Development**: stdio on UART0 through a custom `stdio_driver_t` that
  writes into the log ring, which core 1 drains as the UART has room;
  dropped lines are counted (EL §2.3). Upstream prints a great deal, some of
  it in timing-sensitive paths, and a blocking `printf` at 115200 baud
  would distort every measurement. UART keys on. Upstream's crash record
  (kept in `.uninitialized_data` across the watchdog reboot) is printed at
  boot.
- **Release**: no UART, no stdio; GP2/GP3 free. A counter block at a fixed
  symbol with a magic word, read over SWD without halting (HW §2.7).

### 14.3 Heartbeat

Once a second on the development build: core 0 busy share (wall time not
in `cabal_delay`/`delayMillis`), presents, full presents, dropped frames,
longest present, mixer worst half and share, late refills, key events,
I²C errors, log lines dropped, PSRAM heap free, die temperature, battery
and charging.

---

## 15. Testing

The engines are upstream's, tested by upstream and by ScummVM; this port
tests what it adds.

**Host tests** (CTest, no framework, exit 77 is a skip, EL §11.1), for the
portable parts of `picocalc/`, built with the workstation compiler under
`-Wall -Wextra -Werror`:

- The frame pool, driven through a long random interleaving (EL §2.4).
- The key map: every MCU code to exactly one entry; no swallowed chord
  bound; canonicalised releases; Insert decided by Alt (HW §6.2).
- Pointer motion: position after a held time, frame-rate independence,
  Ctrl nudges, clamping.
- Palette to RGB565 and the compare-word conversion, against independent
  models.
- The shell's time to calendar fields (§13.2): offsets either side of UTC
  and across midnight, month ends, leap years including 2100, and the
  fallback when no clock is set.
- The settings parser.
- `create_patch.sh` then `apply_patch.sh` reproduces `working/` exactly.

**Device checks** at every milestone (§16), each dated, on a named board,
with a not-checked list (EL §14.3).

**A title list**, one or two per engine, that every device milestone from
M4 runs. The owner supplies the games; the firmware names none (EL §10).

**CI** (GitHub Actions): host tests; `check_sources.sh`; the firmware build
for both build types; `arm-none-eabi-size` in the log.

---

## 16. Milestones

Each milestone ends with something that runs and something that is
measured (EL §14.3), and is small enough to finish in one or two sittings.
Results go in `docs/milestones.md`: date, board, compiler, what was checked
and by whom, and a **not checked** list carried forward.

**Anything learned about the PicoCalc goes into `hardware-notes.md` as it
is learned**, not at the end: a measurement (present times, card latency,
mixer cost, a clock that does or does not hold), a quirk, a pin use, or a
correction to something the notes say. That is how the notes are kept
current across projects. Each entry is written to stand alone: board,
date, build and workload, measured or read from source, and no reference
to this project's documents or code (EL §14.1). A milestone is not done
until its hardware findings are in the notes.

```
 M0 skeleton, patch pipeline, CI           (host)
 M1 board bring-up                         (device)
 M2 display: welcome screen on the panel
 M3 card and keyboard: the selector works
 M4 first games run ── GATE: is 300 MHz enough?
 M5 audio
 M6 pointer: point-and-click games play
 M7 saves, settings, quit and exit
 M8 performance pass and soak
 M9 release
```

### M0: Skeleton, patch pipeline, CI

- Repository layout of §5.1; submodule pinned at `949301b`.
- `apply_patch.sh`, `create_patch.sh` (walking `working/`), configure-time
  copy and patch with a stamp.
- `cmake/upstream-sources.cmake` and `check_sources.sh`.
- `picocalc/` with empty shims (§5.3) so the whole engine set links for
  `pimoroni_pico_plus2_rp2350`.
- The first patch: `board_config.h` and `sdcard.c` only.
- CI: host test skeleton, source check, firmware build, size.

**Done when** a clean clone builds a UF2 with one command; the patch round
trip test passes; the size report and the SRAM map are recorded.
**Measured**: `.text`, `.data`, `.bss`, SRAM left. **Not on the device.**

### M1: Board bring-up

- `picocalc/main.c` and `board.c` from `pico-oric`: 150 → 300 MHz per
  §11.1, rail at 1.20 V.
- Log ring, development stdio on UART0 at GP2/GP3 (D12), banner,
  heartbeat skeleton.
- `flash.sh` and `capture.sh` with the probe.
- PSRAM init (upstream's) and the round-trip verify.
- Southbridge and LCD from `pico-oric`; a corner-coded test pattern
  (HW §10, EL §13.3).

- `clock.c` (§13.2): the banner prints the shell's time, read before and
  after the move to 300 MHz.

**Done when** the banner and a minute of heartbeats arrive over UART;
PSRAM verifies at 300 MHz; the test pattern is right in all four corners
and colours at 75 MHz SPI (the owner's look at the panel); started from
the shell, the banner's time matches the shell's, before and after the
clock change. **Measured**: the clocks as configured, SPI rate returned,
PSRAM verify time, die temperature after a minute.

### M2: Display

- The frame pool (§7.2) with its host test; the `graphics_*` shims; the
  palette with the frame.
- Core 1 launched; the presenter (§7.3) at y = 40.
- Upstream's welcome screen runs; it auto-advances after 10 s.

**Done when** the welcome animation plays, centred, with correct colours,
on the panel. **Measured**: present time full-frame, frames presented and
dropped per second, core 1 stack high-water, and the three PSRAM read
strategies of §7.3 against core 0's time.

### M3: Card and keyboard

- SD on `spi0` (§10.1); FatFs mounts; the selector lists `/quest`.
- Southbridge polling on core 1, the event queue, the key map (§9.2) with
  its host test, UART key injection.
- Patched `cabal_poll_event()` for keys only; Ctrl+Alt+Del checked on the
  device (D9).
- Error pages: no card, no `/quest`, no PSRAM.

**Done when** the owner moves through the selector with the arrows and
Enter launches a game (it need not play yet); every key on the keyboard,
with each modifier, is logged with its code and none is lost.
**Measured**: card mount time, directory scan time for the test card,
key-to-event latency.

### M4: First games, and the gate

- An AGI title played by keyboard (keys mode, no pointer needed, no audio).
- The intro or attract sequence of one title per engine (SCUMM, SCI, KYRA,
  GOB) runs to its first interactive screen.
- Return to the selector with Ctrl+Alt+Del, and launch another game, ten
  times.

**The gate.** On each test title's heaviest scene, core 0's busy share at
300 MHz (§14.3), with audio not yet present. **Pass**: every title under
75 %, leaving room for the mixer. **Fail**: apply M8's levers in order
(SRAM placement, flash clock, the read strategy) and measure again before
any other milestone; only if those fall short does the owner decide on a
higher clock (D5). Record the result either way.

**Done when** the AGI title can be played for ten minutes; every engine
reaches its first interactive screen; ten selector round trips leave the
PSRAM heap where it started.

### M5: Audio

- PWM driver adapted from `pico-oric` (§8.1); the audio shim; the mixer in
  the refill IRQ (§8.2).
- Counters: late refills, mixer worst half, mixer share.
- The late path forced once with a scratch build (EL §6.4).

**Done when** music and effects play correctly by ear in one title per
engine, including OPL music and SCUMM v7 digital audio if Full Throttle is
on the test card. **Measured**: mixer cost per half with each engine's
music; late refills over ten minutes (zero expected); core 0 busy share
again against the gate.

### M6: Pointer and full input

- The pointer (§9.3): motion, Ctrl nudges, the buttons, Alt+M, per-engine
  starting mode.
- The status band (§7.4).
- Play-testing with the owner settles D8: button keys, acceleration.

**Done when** the owner completes the first scene of one point-and-click
title per engine using only the PicoCalc keyboard, and types a command into
a parser game in each mode. **Measured**: pointer travel time across the
screen; key-to-cursor latency.

### M7: Saves, settings, quit and exit

- Saving and loading in one title per engine, to `/quest/saves`, surviving
  a power cycle.
- `/quest/picocalc.ini` (§10.3) with its host test.
- The patched `getTimeAndDate()` and `get_fattime()` (§13.2).
- Exit from the selector to the shell, or power off (§13.3).

**Done when** each engine's save loads after a power cycle; a save made
after starting from the shell shows the shell's local date and time in the
game's save list and on the file's card timestamp, and one made after a
cold power-on without the shell shows the fallback; a settings file with a
bad line boots with the bad line named and the rest applied; Esc in the
selector returns to the shell when started by it, and powers off when
installed from BOOTSEL. **Measured**: save write time per engine; time from
the exit key to the shell's first screen.

### M8: Performance pass and soak

In measured steps, each against a control build in the same sitting
(EL §12), keeping only what pays:

1. Dirty-row presenting (rows whose bytes or palette changed since the last
   present), against full presents.
2. SRAM placement of hot code in tiers: mixer and rate converter, the OPL
   core, the engines' blitters and decompressors, identified by sampling
   the PC over SWD (HW §9.2, §9.8; each tier in its own build directory,
   symbols checked with `nm`).
3. Flash at 75 MHz, qualified for data integrity first (§11.4).

Then a **30-minute soak** on battery, one title playing, keys typed over
the UART for the whole run, with a script checking the log: one boot,
heartbeats throughout, late refills, I²C errors and dropped log lines zero,
and the game visibly advancing (EL §11.5).

**Done when** the soak passes and every lever is recorded as kept or
dropped with its number.

### M9: Release

- Release build (§14.2); its own soak of 30 minutes with counters read
  over SWD and keys pressed by hand (EL §11.5).
- Checked on a Plus 2 and a Plus 2 W if both are to hand; otherwise say
  which.
- README (card layout, keys, what is supported), credits and licence texts
  (upstream's `COPYING*` and attribution kept), `scripts/build_release.sh`
  producing `dist/picocalc-quest-v<x.y>-pico-plus2.uf2`.

**Done when** the release UF2 boots from UF2 Loader and from BOOTSEL, a
title from each engine plays, and the documents describe what shipped.

---

## 17. Unverified constants

Every value the port depends on that has not been checked on this hardware
(EL §14.2). Each becomes confirmed, or changes, at the milestone named.

| Constant | Value | Source | Confidence | Settled in |
|---|---|---|---|---|
| Stable at 300 MHz, 1.20 V, under this workload | yes | HW §3 (other workloads) | medium | M4, M8 soak |
| PSRAM at 100 MHz (divider 3) at 300 MHz | works | upstream release default; HW §2.4 | medium | M1 |
| LCD SPI at 75 MHz | works | HW §4.2, §4.7 | high | M1 |
| SD at 25 MHz on `spi0` with upstream's driver | works | HW §7.1 (another driver) | medium | M3 |
| F6–F10 codes `0x86`–`0x90` | F7–F9 = `0x87`–`0x89` assumed | HW §6.2 (F6, F10 observed) | medium | M3 |
| Ctrl+Alt+Del delivered by the MCU | yes | MCU keymap source, not observed | low | M3 |
| Mixer half fits 14 ms with OPL at 300 MHz | yes | none | low | M5 |
| PSRAM frame read cost | ~9 ms a full frame | HW §9.3 (a different copy) | low | M2 |
| UF2 Loader's app partition holds 3.6 MB | yes | not checked | low | M9 |
| POWMAN timer unaffected by the move to 300 MHz | yes | timer has its own tick source; not measured | medium | M1 |
| Shell's clock record survives into the port and reads correctly | yes | Rogue uses it; not on this image | high | M1 |
| Upstream's SRAM use leaves room for the port | yes | linker script only | medium | M0 |
| Core 1 stack of 4 KB is enough | yes | none | medium | M2 |

---

## 18. Risks

| Risk | Effect | Mitigation |
|---|---|---|
| 300 MHz is too slow for SCUMM v6/v7 or SCI1.1 with music | Games run slow, audio late | Gate at M4 before audio and pointer work; levers in M8; a higher clock as a last, explicit decision |
| XIP cache contention between engine code, PSRAM data and the presenter | Core 0 slower than upstream's single-core numbers suggest | Measure read strategies in M2; SRAM placement in M8 |
| Mixer IRQ held off by `lockMutex` longer than a half | Clicks | Late-refill counter; ring resized in M5 |
| Keyboard pointer too clumsy for pixel-hunting games | Poor play | Acceleration plus Ctrl nudges; owner play-test in M6; settings |
| Upstream changes files we patch | Patch rejects on a bump | Small patch, shims first, `check_sources.sh`; bump deliberately |
| A game needs a key the MCU cannot send | Unplayable title | Key-map host test; Alt-layer bindings where needed |
| Card latency during SCUMM v7 streaming | Speech stutter | Mixer in IRQ already decouples output; measure in M5 |

---

## 19. Dropped and deferred

**Dropped**, with reasons, so they are not re-planned (EL §14.5):

- **Pico, Pico 2, Pico 2 W.** No PSRAM; upstream cannot run without it.
- **A Plus 2 W image.** Nothing uses the radio (D2).
- **Scaling to fill the panel.** 320×200 to 320×320 is uneven, and costs
  wire time for nothing (EL §5.4).
- **A no-flash (SRAM) build.** The image does not fit.
- **ScummVM's GUI and launcher.** Disabled upstream; the selector replaces
  them.

**Deferred**, with what would bring each back:

- **An overclock above 300 MHz.** If the M4 gate fails after M8's levers.
- **An in-game menu** (volume, pointer speed, mode, save settings). If the
  settings file proves awkward in use.
- **Hardware vertical scroll.** Nothing scrolls in a way it would help.
- **SCUMM v8 and The Dig.** When upstream builds them.
