# PicoCalc Quest

PicoCalc Quest runs classic adventure games on the
[ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc): a port of
[FRANK Quest](https://github.com/rh1tech/frank-quest), the ScummVM-derived
adventure engine for the RP2350, to the PicoCalc's screen, keyboard,
speakers and SD card.

You can buy games on [GOG.com](https://gog.com), and find some on the
[Internet Archive](https://archive.org). Copy the data files of your games onto
the card, pick one from the list, and play. Copy all of a game's files: a missing
patch file, such as a Sierra game's `.HEP`, can stop a game with a script error.

## What you need

- A PicoCalc with a **Pimoroni Pico Plus 2** or **Pico Plus 2 W** installed.
  The engines require the Plus 2's 8 MB of PSRAM, so it will not run on
  Pico, Pico 2 or Pico 2 W. The one image works on both Plus 2 boards; it does not
  use the radio.
- An SD card, FAT32, with your games on it.
- Optionally, [UF2 Loader](https://github.com/pelrun/uf2loader) and the
  [PicoCalc shell](https://github.com/BlairLeduc/picocalc-shell), to start it
  from the card and go back to the shell afterwards.

## Installing

Download `picocalc-quest-v<x.y>-pico-plus2.uf2` from the
[releases page](https://github.com/BlairLeduc/picocalc-quest/releases).
Either:

- **With UF2 Loader**: copy the `.uf2` to the card's `pico2-apps` folder and
  choose it from the loader's menu or the shell. Esc in the game list then goes back to the
  shell. This is the way to install on a board that already has the loader.
- **From BOOTSEL, with no loader**: hold BOOTSEL on the Plus 2 while
  plugging it into a computer, and copy the `.uf2` to the drive that
  appears. The drive goes away by itself when the copy ends. Esc in the game
  list then switches the PicoCalc off.

With UF2 Loader installed, a BOOTSEL copy goes into the loader's program
area instead of replacing it. If you have trouble installing the UF2,
erase the flash first (Raspberry Pi's `flash_nuke.uf2`), which removes the
loader too. The shell's own loader build starts the shell at power-on; hold
Up, F1 or F5 to start the game instead.

## The card

```
/quest/<game>/        one folder per game, holding its data files
/quest/saves/         saved games, made by the port
/quest/picocalc.ini   your settings (optional)
```

Each game goes in its own folder under `/quest`, named as
[FRANK Quest's README](https://github.com/rh1tech/frank-quest#game-directory-names)
lists (for example `monkey`, `atlantis`, `dott`, `ft`, `sq3`, `kq4`, `gob1`,
`kyra1`), or as in the table below. The game list shows every folder it
recognises. The CD version of Kyrandia also needs FRANK Quest's
[`KYRA.DAT`](https://github.com/rh1tech/frank-quest/blob/main/dists/engine-data/KYRA.DAT)
in its folder.

### Added to FRANK Quest's list

| Folder | Game | Engine | Needs |
|---|---|---|---|
| `indy3` | Indiana Jones and the Last Crusade (DOS VGA; EGA is untested) | SCUMM v3 | `00.LFL` |
| `dig` | The Dig | SCUMM v7 | `dig.la0` |
| `kq1`, `kq2`, `kq3` | King's Quest I–III | AGI | `LOGDIR` |
| `sq1`, `sq2` | Space Quest I–II | AGI | `LOGDIR` |
| `lsl1` | Leisure Suit Larry I | AGI | `LOGDIR` |
| `pq1` | Police Quest I | AGI | `LOGDIR` |
| `sky`, `bass`, `steelsky` | Beneath a Steel Sky | SKY | `sky.dsk`, `sky.dnr` and [ScummVM's `sky.cpt`](https://www.scummvm.org/frs/extras/Beneath%20a%20Steel%20Sky/), of 419,427 bytes |

FRANK Quest already uses `kq1`, `sq1`, `lsl1` and `pq1` for the later SCI
remakes. A folder with one of those names runs as AGI if it has a `LOGDIR`
file, and as the SCI remake if it does not.

## What runs

| Engine | Games |
|---|---|
| SCUMM v3–v7 | Lucasfilm and LucasArts: Indiana Jones and the Last Crusade (DOS), Loom (VGA), Monkey Island 1 and 2, Fate of Atlantis, Day of the Tentacle, Sam & Max, Full Throttle, The Dig |
| SCI0–SCI1.1 | Sierra: King's Quest IV–VI, Space Quest III–V, Leisure Suit Larry II, III, V and VI, Police Quest II–III, Quest for Glory I–III |
| AGI | Sierra: King's Quest I–III, Space Quest I–II, Leisure Suit Larry I, Police Quest I |
| GOB | Coktel Vision: Gobliiins, Gobliins 2, Goblins 3 |
| KYRA | Westwood: The Legend of Kyrandia, Hand of Fate |
| SKY | Revolution: Beneath a Steel Sky |

Played on the PicoCalc: The Last Crusade, Fate of Atlantis, Day of the
Tentacle, Full Throttle and The Dig; King's Quest I to III and Space Quest I
to V; Gobliiins, Gobliins 2 and Goblins 3; Kyrandia and Hand of Fate;
Beneath a Steel Sky. Other titles of these engines should run but have not
been tried.

Known problems:

- **CD versions of the Gob games** play without music: it is on CD audio
  tracks, which the engine cannot play.
- **SCUMM v8** (The Curse of Monkey Island) is not built.
- **Maniac Mansion and Zak McKracken** (SCUMM v1 and v2) cannot be started,
  nor can the Steam release of Indiana Jones and the Last Crusade.
- **Malcolm's Revenge** (Kyrandia 3) is not offered: on the PicoCalc its
  movie's sound cut out and it locked up at the first change of screen.

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

The arrows start as a pointer in SCUMM, SCI, GOB, Kyrandia and Beneath a
Steel Sky, and as arrow keys in the AGI games, where the hero walks with
them. The other engines save from the game's own menu (F5 in Beneath a
Steel Sky), except Gobliiins, which has only level codes. With the stock
UF2 Loader, holding Up, F1 or F5 while switching on goes to its menu, for
any program; the shell's build differs (see [Installing](#installing)).

## Settings

`/quest/picocalc.ini` is optional. Each line is `key = value`; `#` or `;`
starts a comment. Every setting and its default:

```
left_button   = ]        # any printable key
right_button  = [
pointer_speed = 100      # per cent, 25-400
music_volume  = 192      # 0-255
sfx_volume    = 192      # 0-255
speech_volume = 192      # 0-255
agi_mode      = keys     # what the arrows start as: pointer or keys
sci_mode      = pointer
scumm_mode    = pointer
kyra_mode     = pointer
gob_mode      = pointer
sky_mode      = pointer
opl           = mame     # the AdLib emulator: mame or dosbox
subtitles     = on       # text with speech: on or off
talkspeed     = 60       # SCUMM: how long text stays, 0-255; higher is longer
tempo         = 100      # SCUMM: music speed, per cent, 50-200
```

`opl`, `subtitles`, the volumes, `talkspeed` and `tempo` can also be set
for one game by its folder, for example `opl.dott = dosbox` (lighter on the
processor), `subtitles.dig = off` or `music_volume.kyra1 = 255`.

Effects played as AdLib notes follow `music_volume`, not `sfx_volume`, and
all AGI sound does. `tempo` affects games with iMUSE MIDI music, such as
Monkey Island 2, Fate of Atlantis, Day of the Tentacle and Sam & Max; not
Full Throttle or The Dig. A line the port cannot use is named on the status
line when the game list first shows, and the rest of the file still
applies.

## Coming from FRANK Quest

PicoCalc Quest keeps FRANK Quest's engines, game selector and card layout.
Most of what differs is the hardware it runs on:

| | FRANK Quest | PicoCalc Quest |
|---|---|---|
| Hardware | FRANK or Murmulator boards; RP2350 with 8 MB PSRAM | PicoCalc with a Pico Plus 2 or Plus 2 W |
| Picture | HDMI | The PicoCalc's 320×320 screen, with the picture centred and not scaled |
| Sound | I2S or HDMI | PWM stereo, to the speakers or headphones |
| Input | PS/2 or USB keyboard and mouse | The PicoCalc's keyboard; the arrows move the pointer |
| Clock | 504 MHz at 1.65 V by default | 300 MHz at 1.20 V |
| Options | Board, clocks and USB input, at build time | Keys, volumes, AdLib emulator and more, in `/quest/picocalc.ini` |
| Leaving | The selector has no exit | Esc goes back to the shell, or powers off |

The games also behave differently in a few ways:

- **More games:** see [the table above](#added-to-frank-quests-list).
  FRANK Quest's engine list names some of them, but its selector has only one
  AGI folder, `agi`, and its launcher starts SCUMM v4 and up.
- **Saves:** each game saves under its own name. FRANK Quest uses one name per
  engine, so one SCUMM game's save can overwrite another's. Saves made under
  those names are not listed here. SCI and AGI games use their own save and
  restore screens.
- **Game versions:** Gob, SCI and Kyra games are matched against ScummVM's
  tables by their files, so a CD release runs as the CD version. FRANK Quest
  starts each of these games as one fixed version.

`patches/frank-quest.patch` holds every change to FRANK Quest's code.

## Building

Needs the Pico SDK 2.x and the Arm GNU toolchain; the
[Pico VS Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)'s
installs are found automatically.

```
git clone --recurse-submodules https://github.com/BlairLeduc/picocalc-quest.git
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
