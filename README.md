# PicoCalc Quest

Classic point-and-click and parser adventure games on the
[ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc): a port of
[FRANK Quest](https://github.com/rh1tech/frank-quest), the ScummVM-derived
adventure engine for the RP2350, to the PicoCalc's screen, keyboard,
speakers and SD card.

You supply the games. Copy the data files of games you own onto the card,
pick one from the list, and play.

## What you need

- A PicoCalc with a **Pimoroni Pico Plus 2** or **Pico Plus 2 W** in it.
  The engines need the Plus 2's 8 MB of PSRAM, so a Pico, Pico 2 or
  Pico 2 W will not run it. The one image works on both Plus 2 boards; it
  does not use the radio.
- An SD card, FAT32, with your games on it.
- Optionally, [UF2 Loader](https://github.com/pelrun/uf2loader) and the
  PicoCalc shell, to start it from the card and go back to the shell
  afterwards.

## Installing

Download `picocalc-quest-v<x.y>-pico-plus2.uf2` from the releases page.
Either:

- **With UF2 Loader**: copy the `.uf2` to the card and choose it from the
  loader's menu or the shell. Esc in the game list then goes back to the
  shell.
- **From BOOTSEL**: hold BOOTSEL on the Plus 2 while plugging it into a
  computer, and copy the `.uf2` to the drive that appears. Esc in the game
  list then switches the PicoCalc off.

## The card

```
/quest/<game>/        one folder per game, holding its data files
/quest/saves/         saved games, made by the port
/quest/picocalc.ini   your settings (optional)
```

Each game goes in its own folder under `/quest`, named as FRANK Quest's
README lists (for example `monkey`, `atlantis`, `dott`, `ft`, `dig`,
`sq3`, `kq4`, `gob1`, `kyra1`). The game list shows every folder it
recognises. For the Sierra AGI games, `kq1`, `kq2`, `kq3`, `sq1`, `sq2`,
`lsl1` and `pq1` are taken as AGI when the folder has a `LOGDIR` file, and
as the later SCI remakes when it does not. The CD version of Kyrandia
also needs FRANK Quest's `KYRA.DAT` in its folder.

## What runs

| Engine | Games |
|---|---|
| SCUMM v1–v7 | Lucasfilm and LucasArts: Maniac Mansion to Monkey Island 2, Fate of Atlantis, Day of the Tentacle, Sam & Max, Full Throttle, The Dig |
| SCI0 / SCI1 | Sierra: King's Quest IV–VI, Space Quest III–V, Leisure Suit Larry II–V, Police Quest II–III, Quest for Glory I–III |
| AGI | Sierra: King's Quest I–III, Space Quest I–II, Leisure Suit Larry I, Police Quest I |
| GOB | Coktel Vision: Gobliiins, Gobliins 2, Goblins 3 |
| KYRA | Westwood: The Legend of Kyrandia, Hand of Fate |

Played on the PicoCalc during development: Fate of Atlantis, Day of the
Tentacle, Full Throttle and The Dig; Space Quest III and IV; King's Quest I
and III (AGI) and Space Quest II; Gobliiins, Gobliins 2 and Goblins 3;
Kyrandia. Other titles of these engines should run but have not been
tried.

Known problems:

- **Space Quest IV** can stop with a script error near its start, in the
  room where the droids arrive.
- **Full Throttle** has shown patches of white in the backgrounds of some
  movies. The game plays through.
- **CD versions of the Gob games** play without music: it is on CD audio
  tracks, which the engine cannot play.
- **SCUMM v8** (The Curse of Monkey Island) is not built.

## Keys

| Key | Does |
|---|---|
| Arrows | Move the pointer (pointer mode), or are arrow keys (keys mode) |
| `]` / `[` | Left / right mouse button, held as long as the key (pointer mode) |
| Ctrl + arrow | Move the pointer one pixel |
| Alt+M | Switch the arrows between pointer and keys mode |
| Letters, Enter, Backspace | Typing, for the parser games |
| Esc | Skip a cutscene; in the game list, leave (to the shell, or power off) |
| F1–F5, Shift+F1–F5 | F1–F10 |
| Alt+1…9 / Ctrl+1…9 | SCUMM games: quick save / load slot 1–9 |
| Ctrl+Alt+Del | Quit the game, back to the game list |

The arrows start as a pointer in SCUMM, SCI, GOB and Kyrandia, and as
arrow keys in the AGI games, where the hero walks with them. The other
engines save from the game's own menu. Holding Up, F1 or F5 while
switching on goes to UF2 Loader's menu, for any program.

## Settings

`/quest/picocalc.ini` is optional. Each line is `key = value`; `#` or `;`
starts a comment. Every setting and its default:

```
left_button   = ]        any printable key
right_button  = [
pointer_speed = 100      per cent, 25-400
volume        = 100      per cent, 0-200
agi_mode      = keys     what the arrows start as: pointer or keys
sci_mode      = pointer
scumm_mode    = pointer
kyra_mode     = pointer
gob_mode      = pointer
opl           = mame     the AdLib emulator: mame or dosbox
subtitles     = on       text with speech: on or off
```

`opl` and `subtitles` can also be set for one game by its folder, for
example `opl.dott = dosbox` (lighter on the processor) or
`subtitles.dig = off`. A line the port cannot use is named on the status
line when the game list first shows, and the rest of the file still
applies.

## Building

Needs the Pico SDK 2.x and the Arm GNU toolchain; the
[Pico VS Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)'s
installs are found automatically.

```
git clone --recurse-submodules <this repository>
scripts/build.sh                   # development build, log on UART0 (GP2/GP3)
scripts/build_release.sh 1.0       # dist/picocalc-quest-v1.0-pico-plus2.uf2
```

`docs/design.md` describes how the port works, `docs/milestones.md` what
was measured on the device, and `docs/hardware-notes.md` what was learned
about the PicoCalc along the way.

## Credits

- **ScummVM**, by the ScummVM team, and **[Cabal](https://github.com/project-cabal/cabal)**,
  its community fork: the engines and their runtime, including the MAME
  and DOSBox OPL emulators. Their contributors are listed in FRANK
  Quest's `COPYRIGHT`.
- **[FRANK Quest](https://github.com/rh1tech/frank-quest)**, by Mikhail
  Matveev: Cabal brought to the RP2350, with the PSRAM heap and game
  selector this port runs on.
- From FRANK Quest's tree: **FatFs** by ChaN, the SD card driver from
  **pico_fatfs** by Elehobica, and **dlmalloc** by Doug Lea.
- The **[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)**.
- **[UF2 Loader](https://github.com/pelrun/uf2loader)**, by pelrun, and
  the PicoCalc shell's `picocalc_shell.h`, for starting from the card and
  going back to the shell.
- The 8×10 font from `picocalc-text-starter`; the PicoCalc drivers from
  `pico-oric`; the release shape from `picocalc-rogue`.

Game titles are trademarks of their owners. No game data is included.

## Licence

GPL-3.0-or-later; see `LICENSE`. FRANK Quest's own code is GPL-3.0-or-later
and the ScummVM code it contains GPL-2.0-or-later, so the whole is
distributed under the GPL version 3. FRANK Quest's licence texts
(`COPYING`, `COPYING.LGPL`, `COPYING.BSD`, `COPYING.FREEFONT`) and its
`COPYRIGHT` are in `modules/frank-quest/`, and in the release zip under
`upstream/`.
