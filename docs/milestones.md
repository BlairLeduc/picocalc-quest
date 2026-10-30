# Milestone log

What each milestone verified, on which board, on what date, and what was
not checked, newest first. `design.md` §16 holds each milestone's scope
and done-when criteria; this file keeps the full record.

**M7 (saves, settings, quit and exit) is done**, on 2026-10-30, on the
same Pimoroni Pico Plus 2 W in a PicoCalc, flashed over SWD, and on a
Pimoroni Pico Plus 2 started from the PicoCalc shell under UF2 Loader,
with Arm GCC 15.2.Rel1 and SDK 2.3.1, development build. The owner saved
and loaded in every engine across a power cycle, left by Esc both ways,
and saw the shell's local time on a save. Four faults found and fixed on
the way, two of them upstream's.

- **Saves** (`frank_quest_main.cpp`, §10.2): every game under its own
  config domain, `quest-<folder>`. Engines name save files after the
  domain, and upstream's one domain per engine gave every SCUMM game the
  same `quest-scumm.s01`. SCI and AGI now use the games' own save and
  restore screens: ScummVM's chooser, which they called, is upstream's
  stub that always returns slot 0.
- **Settings** (`picocalc/settings.c`, §10.3): `/quest/picocalc.ini`, the
  two button keys, pointer speed, volume and each engine's starting mode,
  every default in one function. Bad lines are logged with number, text
  and why; the first is named in the status band for 10 s.
- **The date and time** (§13.2): the patched `getTimeAndDate()`, and
  `get_fattime()` in `clock.c` with the driver's compiled out
  (`SDCARD_EXTERNAL_FATTIME`). Upstream's gave 0, no date at all.
- **Exit** (`picocalc/exit.c`, §13.3): Esc in the selector, which the
  legend labels; unmount, then the shell after 2 s on a page, or the
  southbridge's power off after 6 s. The banner says whether the
  southbridge can power off and which way Esc goes.
- **A card taken out during a game** (found in M6): the no-card page when
  the selector next opens, or a restart if it was put back.
- **The patch** (still ten files): the domains and save screens, the time,
  the selector's Esc and legend, and the warp (below).
- **Host:** `test_settings`: every key good and bad, comments, case, CRLF,
  a BOM, a bad line named with the rest applied, a bad value leaving its
  setting alone, the buttons on one key, more bad lines than room, the
  gain. A planted fault (the later of two clashing buttons) failed it.
  `test_clock` adds FAT's time stamp, both ends of its range.
  `test_keymap` checks the alternates arrive without their modifier.

**Checked on the board** (UART logs `out/m7-*.log`, not committed):

- **Saved, switched off and on, and loaded**, by the owner: Day of the
  Tentacle (Alt+1, then Ctrl+1); Space Quest III (its menu, F5 and F7);
  Space Quest IV (the control panel); King's Quest I (F5, F7); Kyrandia
  (its Options); Gobliins 2 (its menu). Gobliiins has no saves.
- **The settings**: a file with `volme = 3` on line 5 booted with "line 5
  not used, no such setting" in the log and the band, and the log's
  summary showed the other lines applied (pointer speed 150 %, volume
  80 %, AGI in pointer mode).
- **Esc, installed over SWD**: "power off in 6 s, asked for 5 ms after
  the key", and the PicoCalc switched itself off.
- **From the shell** (the Plus 2): Esc went back to the shell; a King's
  Quest I save showed the shell's local date and time in the game's list,
  and the file's time on the card was right. With the exit page held
  2 s, the owner could read it. With the card taken out at the selector,
  Esc powered off instead, there being no shell to load.
- **The release build** on the Plus 2, started from the shell: the same
  exits, by the owner.
- **Without the shell**: the save files on the card read 31 December 2021,
  11 pm on a Mac, which is FatFs's 1 January 2022 00:00 shifted an hour
  by macOS's daylight-saving handling of FAT times (not read raw).

**Measured** (development build), from the open of each save file to its
close, so the engine's time as well as the card's:

| Title | Save file | Size | Time |
|---|---|---:|---:|
| Space Quest III | `quest-sq3.000` | 70,526 B | 286 ms |
| Space Quest IV | `quest-sq4.000` | 84,859 B | 951 ms |
| King's Quest I | `quest-kq1.001` | 44,099 B | 473 ms |
| Kyrandia | `quest-kyr1.000`, `.001` | 51,489 B, 51,502 B | 221, 184 ms |
| Gobliins 2 | `quest-gob2.s00` | 19,285 B | 49 ms |

Day of the Tentacle's save, by the owner's watch: under a second (no
capture was running, so no size or exact time).

From Esc to the shell's first screen, release build on the Plus 2, by the
owner's watch: **just over 2 s**, of which 2 s is the exit page.

**Found:**
- **Saves overwrote each other** across the games of an engine, upstream's
  one config domain per engine; fixed as above. Saves made under
  upstream's names are no longer listed.
- **SCI and AGI saved to one slot**, through upstream's stubbed chooser;
  fixed with the games' own screens.
- **F7 did nothing in Space Quest III**: F6–F10, reached with Shift, came
  with Shift held, and SCI took Shift+F7 for another key. Every key the
  PicoCalc has only as an alternate now comes without the modifier that
  reaches it (§9.2).
- **The pointer went back after a game moved it**: upstream's
  `warpMouse()` moved only the cursor drawn, and the next arrow moved on
  from the old place. Both move now (§9.3).
- The exit page went by too fast to read; now 2 s, at the owner's ask.
- Upstream writes a 651-byte `scummvm.ini` at the card's root on each
  return to the selector.
- The southbridge (version 0) answers a read of `OFF` with a value not 0
  and powers off when it is written: in the hardware notes.

**Not checked:** the banner's time against the shell's on a launch from
the shell (no UART on the Plus 2); the fallback date in a game's save
list; Day of the Tentacle's exact save time and size; Esc with neither way out; a card taken out during a game, since the fix;
the button, mode and volume settings by the owner; the release build on
the Plus 2 W.

**M6 (pointer and full input) is done**, on 2026-10-30, on the same
Pimoroni Pico Plus 2 W in a PicoCalc, flashed over SWD, with Arm GCC
15.2.Rel1 and SDK 2.3.1, development build. The owner played the first
scene of a point-and-click title in every engine, and typed into a parser
game in each mode, with the PicoCalc's keyboard alone. **D8 is settled as
proposed**: the owner kept `]` left and `[` right, and the curve as first
set ("movement feels natural").

- **The pointer** (`picocalc/pointer.c`, §9.3): one pixel on the press,
  then 40 px/s rising to 240 px/s over 600 ms, integrated from each key's
  read time, each axis on its own; Ctrl+arrow one pixel. Deltas through
  `ps2_mouse_get_state()`, which upstream's poll already read: upstream
  keeps the place and the clamp, so no screen size is needed here.
- **Routing** (`picocalc/input_shim.c`): each key's destination (engine,
  pointer, Alt+M) fixed at its press and kept to its release. AGI starts in
  keys mode, the rest in pointer mode; the selector gets every key.
- **The status band** (`picocalc/status.c`, §7.4): the mode and a line of
  help when a game starts and on Alt+M. The owner found 2 s too short; now
  4 s.
- **The patch** (still ten files): one hunk in `frank_quest_main.cpp` sets
  the engine's mode around each launch.
- **Host:** `test_pointer`: the curve, the same pixels at 1, 17 and 250 ms
  a poll, a tap, both axes and opposites, nudges, the ±127 clamp with the
  rest kept, events read before the last poll, the timer's wrap. Alt+M
  joins the chord table `test_keymap` checks. Also `test_sof_to_sou`, for
  a converter written on the way (below).

**Checked on the board** (UART logs `out/m6-*.log`, not committed):

- **By the owner, keyboard only**: Day of the Tentacle (the first SCUMM v6
  title on the board; speech, picking up and looking at an object); Fate
  of Atlantis (the ticket agent, walking, the fight with the doorman);
  Kyrandia (speech, talking, picking up, giving); Gobliiins (selecting and
  moving each goblin; effects only, its music being CD audio, deferred);
  Space Quest IV (menus, the icon bar, the inventory).
- **Typing in each mode**: Space Quest III (`look`, walking with the arrows
  and with the pointer); King's Quest III (walking and typing in keys mode,
  then the pointer after Alt+M).
- **By UART** (`keys.sh`), in Fate of Atlantis: Right held 1.0 s, 181 px;
  Down 0.3 s, 30 px; Ctrl+Left twice, one pixel each; Alt+M both ways, an
  arrow in keys mode reaching the engine as a key.
- 7 launches ended cleanly; the PSRAM heap 263,808 B at all 8 selector
  entries; strays only the releases of the Enter that launched a game and
  the Del of Ctrl+Alt+Del, after the flush.

**Measured** (development build):

- **Pointer travel**: 320 px in **1.58 s** from the press (the curve, in
  the host test; on the board, the held times logged give the curve's
  pixels).
- **Key to pointer motion**, from core 1's read of the key to the first
  delta given to the engine, 756 presses: median **7.3 ms**, 90th
  percentile 24.5 ms; 19 over 50 ms, the longest 223 ms, while the engine
  was not polling (loading, scene changes). Before core 1's read, up to
  25 ms in the MCU's FIFO (§9.1).
- **The gate with the pointer in play** (`gate.sh`, busiest 5 s):

| Title | Busy, median / busiest 5 s | Frames/s then | Longest refill |
|---|---:|---:|---:|
| Day of the Tentacle | 32 / 62 | 10 | 8.6 ms |
| Fate of Atlantis | 25 / 74 | 32 | 7.2 ms |
| **Kyrandia** | 37 / **88** | 55 | 8.7 ms |
| **Gobliiins** | 76 / **89** | 26 | 0.5 ms |
| **Space Quest IV** | 76 / **100** | 2 | 7.6 ms |
| Space Quest III | 34 / 53 | 20 | 5.7 ms |
| King's Quest III | 16 / 36 | 30 | 1.0 ms |

  Late refills 0 in every title. **Kyrandia and Gobliiins now fail** (M5:
  69 and 22, measured with little moving): a moving cursor redraws, and
  each redraw is the 64 KB screen copy M8 already has to cut (M5's SQ4
  finding). The longest refill, 8.7 ms of 14.0, is a new high.

**Found:**
- **A card pulled during a game** gave upstream's "not accessible" and an
  empty selector, not the no-card page; putting it back restarted as it
  should. For M7.
- The owner tried the buttons the other way round at first, then kept them
  as they are.
- `scripts/sof_to_sou.py`: a FLAC-compressed SCUMM speech file
  (`monster.sof`) back to the `monster.sou` this build plays, since it has
  no FLAC, Vorbis or MP3 decoder. Byte for byte on a made-up file; not run
  on a real one (the owner found the CD's `.sou`).
- Nothing new about the PicoCalc's hardware: no entry in the hardware
  notes.

**Not checked:** Ctrl+arrow from the PicoCalc's own keyboard (every one of
the owner's 2,147 arrow presses was plain; the UART's were Ctrl); dragging
(no title on the card needs it); the 4 s band, by the owner; the release
build on the board; rebinding from `picocalc.ini` (M7); Day of the
Tentacle's Maniac Mansion (upstream's launcher has no game list for it).

**M5 (audio) is done**, on 2026-10-29, on the same Pimoroni Pico Plus 2 W
in a PicoCalc, flashed over SWD, with Arm GCC 15.2.Rel1 and SDK 2.3.1,
development build. Music, speech and effects checked by ear by the owner in
one title per engine, Full Throttle's digital iMUSE among them. Two faults
found and fixed on the way, one of them upstream's; core 0's busy share
passes the gate with audio in every title but Space Quest IV.

- **The output** (`picocalc/audio_shim.c`, §8.1–8.2, adapted from
  pico-oric): PWM on GP26/27, TOP 4095, divider 1, oversample 2, so
  **36,621 Hz at 12 bits**; two chained DMA channels over an 8 KB ring of
  two 512-frame halves, with the read wrap; upstream's mixer called inside
  the refill IRQ, as upstream's I2S driver did. The IRQ is **DMA_IRQ_1**,
  not the design's line 0: upstream masks line 1 in `lockMutex()` and
  around every PSRAM heap call, and the mixer frees memory in the IRQ.
- **The patch** (now ten files): the mixer created at the shim's rate, not
  a literal 44,100; `lockMutex()` rewritten (below); the OPL emulator the
  build's for every engine (`PICOCALC_QUEST_OPL`); no key repeats of the
  arrows and diagonals in upstream's event manager (below).
- **The OPL emulator** (§8.3): in Fate of Atlantis's intro, MAME's (what
  `auto` picks) took 45–55 % of core 0, longest half 10.8 ms; DOSBox's
  13–18 %, 4.4 ms. The owner heard MAME's as a little softer and better,
  DOSBox's as not bad, and chose by CPU: **DOSBox by default**, MAME's a
  build option, to revisit after M8's SRAM placement.
- **Counters** (§14.3): the heartbeat adds the audio IRQ's share of core 0,
  halves refilled, the longest refill and late refills; the busy share now
  counts the IRQ's time inside `cabal_delay` as work. `gate.sh` reports
  the audio per title, and reads logs byte-safely.
- **The late path, forced** (`PICOCALC_QUEST_AUDIO_STALL=30`): see below.
- **Host:** `test_sound`: every int16 sample at every volume against a
  model in doubles, monotonic, the extremes, left and right in their
  halves of the word. A planted fault (division for the shift) failed it.

**Checked on the board** (UART logs `out/m5-*.log`, not committed):

- **By the owner, by ear**, DOSBox's OPL: Fate of Atlantis's intro;
  Goblins 3's effects (named Gobliins 2 at the time; the log has Goblins 3); Kyrandia (good, its music quiet); Space Quest III;
  Space Quest IV (good, "a bit harsh"); King's Quest III; **Full Throttle,
  about ten minutes, "sounds great"**. Gobliiins and Goblins 3 had no music:
  the card's are CD releases, whose music is CD audio, and upstream's CD
  manager is an empty stub (deferred, §19).
- **Late refills**: 0 over Space Quest III (some 30 minutes in all), Space
  Quest IV (7 minutes), Kyrandia (4), King's Quest III (5), the Gob games
  and Fate of Atlantis. Full Throttle: 6 in one ten-minute run and 4 in
  another, each group of 3 at a scene change, explained below.
- **The late path forced**: core 0's interrupts masked 30 ms every 5 s
  through Fate of Atlantis's music: 2.8 late refills a stall, refills
  70–73 a second around each, no storm, playback on after every stall.
- **The mutex fix**: Space Quest III launched nine times, two minutes each
  (roundtrip by UART): no stop, 0 late refills, the PSRAM heap 263,808 B
  at all eight selector entries between.
- **Held arrows** in Space Quest III, by the owner: Roger walks.

**The gate again** (busy share with audio, `gate.sh`, the busiest 5 s after
loading, %):

| Title | Busy, median / busiest 5 s | Audio, median / max | Longest refill |
|---|---:|---:|---:|
| Fate of Atlantis (intro) | 25 / 74 | 15 / 21 | 6.8 ms |
| Gobliiins / Goblins 3 | 20 / 22, 19 / 26 | 1 / 1, 1 / 3 | 0.7 ms |
| Kyrandia | 27 / 69 | 9 / 18 | 7.0 ms |
| Space Quest III | 37 / 63 | 28 / 38 | 6.8 ms |
| **Space Quest IV** | **87 / 100** | 36 / 45 | 7.8 ms |
| King's Quest III | 15 / 47 | 3 / 4 | 1.2 ms |
| Full Throttle | 50 / 68 | 3 / 10 | 5.4 ms |

**Space Quest IV fails** with audio (86–94 % busy in play). PC samples, 15 s
in its intro: `memcpy` 26 % (the one remaining copy of the 64 KB screen,
20–22 frames a second), DOSBox's OPL about 20 % (SCI asks for two OPL2 in
stereo), sleep 11 %, the interpreter a few percent. M8's levers for it, in
order: copy only what changed, the OPL and mixer in SRAM, one mono OPL2.

**Measured** (development build):

- Refills **71.5 a second** (14.0 ms); a refill with nothing playing
  0.1–0.4 ms; the longest in any title 7.8 ms (Space Quest IV), under
  half the deadline.
- RAM used to 147,316 bytes, leaving **311,436 of 448 KB** (M4: 328,064);
  `.bss` 114,548 (the 8 KB ring, aligned to its size, and 2 KB of mix
  buffer), `.data` 22,276, text 2,673,788 plus `.rodata` 1,045,888.
- Core 1's stack 804 bytes at most (M4: 748).

**Host:** the six tests pass, the patch round trip with the patch now ten
files.

**Found:**
- **Upstream's `lockMutex()` could leave the audio IRQ masked for good.**
  It kept the IRQ's state in each mutex; SCI's music mutex is taken by the
  engine and by its MIDI timer inside the mixer, and the IRQ, taken just
  after the engine's mask (no barrier), recorded "was disabled" over the
  engine's "was enabled". The DMA then replayed the ring's last 28 ms
  through every later launch until a power cycle. Space Quest III's intro
  did it in two runs of three; SWD showed the IRQ's enable bit clear and
  its pending bit set; a scratch build recording each mask named the
  unmatched one (`SoundCommandParser` under `kGetEvent`). Fixed (§8.2);
  HW §5.9.
- **Held arrows walked Roger in fits**: the MCU's repeats went through as
  presses, and upstream's event manager makes its own (400 ms, then every
  100 ms); SCI0 and AGI take a second press of the walking direction as
  stop. The MCU's are dropped, and the event manager repeats no arrow or
  diagonal (§9.1).
- **Full Throttle's iMUSE reads from the card under its mutex**, which
  masks the audio IRQ: **163 ms** at a scene change, 3 late refills,
  not heard by the owner. Recorded for M8 by the owner's choice; HW §5.9.
- **A SMUSH picture fault in Full Throttle** at a scene change (about
  6 min 15 s in, the same second as the hold-off): each row a few pixels
  short, the picture sheared, the moving parts right. Not investigated;
  M8 by the owner's choice.
- Kyrandia alone calls upstream's heap walk, which prints up to 1,024
  chunks with the audio IRQ masked while it loads; no late refill seen.
- An OpenOCD `halt` and `resume` on core 0 left core 1 halted (the
  configuration treats the cores together): no log, no panel. Read the
  core without halting (PC sampling, memory reads) while it runs.

**Not checked:** the release build on the board; MAME's OPL beyond Fate of
Atlantis's intro; Kyrandia's quiet music and SQ4's harshness against
another player; the volume (unity gain, not tuned); CD-audio music (Gob CD,
deferred); a held arrow in a menu (now taps); the pointer (M6); saves (M7).

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
