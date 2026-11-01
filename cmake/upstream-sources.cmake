# upstream-sources.cmake — the upstream files the firmware compiles
# (design.md §5.4), as data only: no targets, so scripts/check_sources.sh
# can read it with cmake -P.
#
# The lists from COMMON_SOURCES to BACKEND_SOURCES are upstream's own, copied
# from its CMakeLists.txt at the submodule's commit, comments and all. After
# a submodule bump, check_sources.sh reports any file upstream added or
# removed; copy the change here. Paths are relative to the patched copy,
# frank-quest/.

# Upstream's add_executable(frank-quest) sources, src/main.c excepted.
set(FQ_MAIN_SOURCES
    src/frank_quest_main.cpp
    src/frank_quest_selector.cpp
    src/frank_quest_welcome.cpp
    src/frank_quest_new.cpp
    src/frank_quest_fs.c
    src/frank_quest_sbrk.c
)

# Upstream's drivers library, HDMI.c excepted.
set(FQ_DRIVER_SOURCES
    drivers/psram_init.c
    drivers/psram_allocator.c
    drivers/dlmalloc.c
)

# Upstream's fatfs and sdcard libraries (src/fatfs/CMakeLists.txt,
# drivers/sdcard/sdcard.cmake), pio_spi.c excepted.
set(FQ_STORAGE_SOURCES
    src/fatfs/ff.c
    src/fatfs/ffunicode.c
    src/fatfs/ffsystem.c
    drivers/sdcard/sdcard.c
)

# ScummVM Common library
set(FQ_COMMON_SOURCES
    src/common/archive.cpp
    src/common/config-manager.cpp
    src/common/debug.cpp
    src/common/error.cpp
    src/common/EventDispatcher.cpp
    src/common/EventMapper.cpp
    src/common/fs.cpp
    src/common/file.cpp
    src/common/hashmap.cpp
    src/common/language.cpp
    src/common/memorypool.cpp
    src/common/mutex.cpp
    src/common/platform.cpp
    src/common/random.cpp
    src/common/rational.cpp
    src/common/rendermode.cpp
    src/common/str.cpp
    src/common/stream.cpp
    src/common/system.cpp
    src/common/textconsole.cpp
    src/common/tokenizer.cpp
    src/common/util.cpp
    src/common/md5.cpp
    src/common/gui_options.cpp
    src/common/macresman.cpp
    src/common/timestamp.cpp
    src/common/iff_container.cpp
    src/common/ini-file.cpp
    src/common/winexe.cpp
    src/common/winexe_pe.cpp
    src/common/localization.cpp
    src/common/dcl.cpp
)

# ScummVM Graphics library
set(FQ_GRAPHICS_SOURCES
    src/graphics/pixelformat.cpp
    src/graphics/surface.cpp
    src/graphics/primitives.cpp
    src/graphics/cursorman.cpp
    src/graphics/thumbnail.cpp
    # thumbnail_stub.cpp replaced by thumbnail_intern.cpp (for Kyrandia save thumbnails)
    src/graphics/yuv_to_rgb.cpp
    src/graphics/wincursor.cpp
    src/graphics/maccursor.cpp
)

# GUI stubs for embedded
set(FQ_GUI_SOURCES
    src/gui/gui-manager.cpp
)

# Video decoder library (for GOB engine)
set(FQ_VIDEO_SOURCES
    src/video/video_decoder.cpp
    src/video/coktel_decoder.cpp
)

# Image decoder library (for GOB engine)
set(FQ_IMAGE_SOURCES
    src/image/iff.cpp
    src/image/codecs/codec.cpp
    src/image/codecs/indeo3.cpp
)

# ScummVM Audio library
set(FQ_AUDIO_SOURCES
    src/audio/audiostream.cpp
    src/audio/mixer.cpp
    src/audio/rate.cpp
    src/audio/mididrv.cpp
    src/audio/midiparser.cpp
    src/audio/midiparser_smf.cpp
    src/audio/midiplayer.cpp
    src/audio/mpu401.cpp
    src/audio/musicplugin.cpp
    src/audio/null.cpp
    src/audio/softsynth/emu_pcspk.cpp
    # Additional audio for GOB engine
    src/audio/decoders/wave.cpp
    src/audio/decoders/adpcm.cpp
    src/audio/decoders/pcm.cpp
    src/audio/mods/infogrames.cpp
    src/audio/mods/module.cpp
    src/audio/mods/paula.cpp
    src/audio/mods/protracker.cpp
    src/audio/softsynth/opl/dbopl.cpp
    src/audio/softsynth/opl/dosbox.cpp
    src/audio/softsynth/opl/mame.cpp
    src/audio/audiodev/audiodev.cpp
    src/audio/audiodev/opl.cpp
    src/audio/audiodev/pcspk.cpp
    src/audio/adlib.cpp
    src/audio/softsynth/cms.cpp
    src/audio/softsynth/sid.cpp
    src/audio/softsynth/wave6581.cpp
    src/audio/mods/tfmx.cpp
)

# ScummVM Base library
set(FQ_BASE_SOURCES
    src/base/plugins.cpp
    src/base/version.cpp
)

# ScummVM Engine infrastructure
set(FQ_ENGINE_SOURCES
    src/engines/engine.cpp
    src/engines/advancedDetector.cpp
    src/engines/game.cpp
    src/engines/savestate.cpp
    src/engines/obsolete.cpp
    src/engines/dialogs.cpp
)

# AGI Engine sources
set(FQ_AGI_ENGINE_SOURCES
    src/engines/agi/agi.cpp
    src/engines/agi/checks.cpp
    src/engines/agi/console.cpp
    src/engines/agi/cycle.cpp
    src/engines/agi/detection.cpp
    src/engines/agi/global.cpp
    src/engines/agi/graphics.cpp
    src/engines/agi/id.cpp
    src/engines/agi/inv.cpp
    src/engines/agi/keyboard.cpp
    src/engines/agi/loader_v1.cpp
    src/engines/agi/loader_v2.cpp
    src/engines/agi/loader_v3.cpp
    src/engines/agi/logic.cpp
    src/engines/agi/lzw.cpp
    src/engines/agi/menu.cpp
    src/engines/agi/motion.cpp
    src/engines/agi/objects.cpp
    src/engines/agi/op_cmd.cpp
    src/engines/agi/op_dbg.cpp
    src/engines/agi/op_test.cpp
    src/engines/agi/opcodes.cpp
    src/engines/agi/picture.cpp
    src/engines/agi/preagi.cpp
    src/engines/agi/preagi_mickey.cpp
    src/engines/agi/preagi_troll.cpp
    src/engines/agi/preagi_winnie.cpp
    src/engines/agi/saveload.cpp
    src/engines/agi/sound.cpp
    src/engines/agi/sound_2gs.cpp
    src/engines/agi/sound_coco3.cpp
    src/engines/agi/sound_midi.cpp
    src/engines/agi/sound_pcjr.cpp
    src/engines/agi/sound_sarien.cpp
    src/engines/agi/sprite.cpp
    src/engines/agi/text.cpp
    src/engines/agi/view.cpp
    src/engines/agi/wagparser.cpp
    src/engines/agi/words.cpp
)

# Gob (Gobliins) Engine sources
set(FQ_GOB_ENGINE_SOURCES
    src/engines/gob/anifile.cpp
    src/engines/gob/aniobject.cpp
    src/engines/gob/backbuffer.cpp
    src/engines/gob/cheater.cpp
    src/engines/gob/cheater_geisha.cpp
    src/engines/gob/cmpfile.cpp
    src/engines/gob/console.cpp
    src/engines/gob/databases.cpp
    src/engines/gob/dataio.cpp
    src/engines/gob/dbase.cpp
    src/engines/gob/decfile.cpp
    src/engines/gob/demos/batplayer.cpp
    src/engines/gob/demos/demoplayer.cpp
    src/engines/gob/demos/scnplayer.cpp
    src/engines/gob/detection/detection.cpp
    src/engines/gob/draw.cpp
    src/engines/gob/draw_bargon.cpp
    src/engines/gob/draw_fascin.cpp
    src/engines/gob/draw_playtoons.cpp
    src/engines/gob/draw_v1.cpp
    src/engines/gob/draw_v2.cpp
    src/engines/gob/expression.cpp
    src/engines/gob/game.cpp
    src/engines/gob/global.cpp
    src/engines/gob/gob.cpp
    src/engines/gob/goblin.cpp
    src/engines/gob/goblin_v1.cpp
    src/engines/gob/goblin_v2.cpp
    src/engines/gob/goblin_v3.cpp
    src/engines/gob/goblin_v4.cpp
    src/engines/gob/hotspots.cpp
    src/engines/gob/iniconfig.cpp
    src/engines/gob/init.cpp
    src/engines/gob/init_fascin.cpp
    src/engines/gob/init_geisha.cpp
    src/engines/gob/init_v1.cpp
    src/engines/gob/init_v2.cpp
    src/engines/gob/init_v3.cpp
    src/engines/gob/init_v4.cpp
    src/engines/gob/init_v6.cpp
    src/engines/gob/init_v7.cpp
    src/engines/gob/inter.cpp
    src/engines/gob/inter_bargon.cpp
    src/engines/gob/inter_fascin.cpp
    src/engines/gob/inter_geisha.cpp
    src/engines/gob/inter_inca2.cpp
    src/engines/gob/inter_littlered.cpp
    src/engines/gob/inter_playtoons.cpp
    src/engines/gob/inter_v1.cpp
    src/engines/gob/inter_v2.cpp
    src/engines/gob/inter_v3.cpp
    src/engines/gob/inter_v4.cpp
    src/engines/gob/inter_v5.cpp
    src/engines/gob/inter_v6.cpp
    src/engines/gob/inter_v7.cpp
    src/engines/gob/map.cpp
    src/engines/gob/map_v1.cpp
    src/engines/gob/map_v2.cpp
    src/engines/gob/minigames/geisha/diving.cpp
    src/engines/gob/minigames/geisha/evilfish.cpp
    src/engines/gob/minigames/geisha/meter.cpp
    src/engines/gob/minigames/geisha/mouth.cpp
    src/engines/gob/minigames/geisha/oko.cpp
    src/engines/gob/minigames/geisha/penetration.cpp
    src/engines/gob/minigames/geisha/submarine.cpp
    src/engines/gob/mult.cpp
    src/engines/gob/mult_v1.cpp
    src/engines/gob/mult_v2.cpp
    src/engines/gob/palanim.cpp
    src/engines/gob/pregob/gctfile.cpp
    src/engines/gob/pregob/onceupon/abracadabra.cpp
    src/engines/gob/pregob/onceupon/babayaga.cpp
    src/engines/gob/pregob/onceupon/chargenchild.cpp
    src/engines/gob/pregob/onceupon/onceupon.cpp
    src/engines/gob/pregob/onceupon/parents.cpp
    src/engines/gob/pregob/onceupon/stork.cpp
    src/engines/gob/pregob/onceupon/title.cpp
    src/engines/gob/pregob/pregob.cpp
    src/engines/gob/pregob/seqfile.cpp
    src/engines/gob/pregob/txtfile.cpp
    src/engines/gob/resources.cpp
    src/engines/gob/rxyfile.cpp
    src/engines/gob/save/saveconverter.cpp
    src/engines/gob/save/saveconverter_v2.cpp
    src/engines/gob/save/saveconverter_v3.cpp
    src/engines/gob/save/saveconverter_v4.cpp
    src/engines/gob/save/savefile.cpp
    src/engines/gob/save/savehandler.cpp
    src/engines/gob/save/saveload.cpp
    src/engines/gob/save/saveload_ajworld.cpp
    src/engines/gob/save/saveload_fascin.cpp
    src/engines/gob/save/saveload_geisha.cpp
    src/engines/gob/save/saveload_inca2.cpp
    src/engines/gob/save/saveload_playtoons.cpp
    src/engines/gob/save/saveload_v2.cpp
    src/engines/gob/save/saveload_v3.cpp
    src/engines/gob/save/saveload_v4.cpp
    src/engines/gob/save/saveload_v6.cpp
    src/engines/gob/save/saveload_v7.cpp
    src/engines/gob/scenery.cpp
    src/engines/gob/scenery_v1.cpp
    src/engines/gob/scenery_v2.cpp
    src/engines/gob/script.cpp
    src/engines/gob/sound/adlib.cpp
    src/engines/gob/sound/adlplayer.cpp
    src/engines/gob/sound/bgatmosphere.cpp
    src/engines/gob/sound/cdrom.cpp
    src/engines/gob/sound/infogrames.cpp
    src/engines/gob/sound/musplayer.cpp
    src/engines/gob/sound/pcspeaker.cpp
    src/engines/gob/sound/protracker.cpp
    src/engines/gob/sound/sound.cpp
    src/engines/gob/sound/soundblaster.cpp
    src/engines/gob/sound/sounddesc.cpp
    src/engines/gob/sound/soundmixer.cpp
    src/engines/gob/surface.cpp
    src/engines/gob/totfile.cpp
    src/engines/gob/util.cpp
    src/engines/gob/variables.cpp
    src/engines/gob/video.cpp
    src/engines/gob/video_v1.cpp
    src/engines/gob/video_v2.cpp
    src/engines/gob/video_v6.cpp
    src/engines/gob/videoplayer.cpp
)

# Kyrandia Engine sources
set(FQ_KYRA_ENGINE_SOURCES
    src/engines/kyra/detection.cpp
    src/engines/kyra/debugger.cpp
    src/engines/kyra/gui.cpp
    src/engines/kyra/resource.cpp
    src/engines/kyra/resource_intern.cpp
    src/engines/kyra/saveload.cpp
    src/engines/kyra/screen.cpp
    src/engines/kyra/script.cpp
    src/engines/kyra/sound.cpp
    src/engines/kyra/sound_adlib.cpp
    src/engines/kyra/sound_digital.cpp
    src/engines/kyra/sound_midi.cpp
    src/engines/kyra/sound_pcspk.cpp
    src/engines/kyra/sprites.cpp
    src/engines/kyra/staticres.cpp
    src/engines/kyra/text.cpp
    src/engines/kyra/timer.cpp
    src/engines/kyra/util.cpp
    src/engines/kyra/wsamovie.cpp
    src/engines/kyra/vqa.cpp
    src/engines/kyra/seqplayer.cpp
    src/engines/kyra/animator_tim.cpp
    src/engines/kyra/script_tim.cpp
    # Kyrandia 1
    src/engines/kyra/kyra_v1.cpp
    src/engines/kyra/kyra_lok.cpp
    src/engines/kyra/animator_lok.cpp
    src/engines/kyra/gui_v1.cpp
    src/engines/kyra/gui_lok.cpp
    src/engines/kyra/items_lok.cpp
    src/engines/kyra/saveload_lok.cpp
    src/engines/kyra/scene_lok.cpp
    src/engines/kyra/scene_v1.cpp
    src/engines/kyra/screen_lok.cpp
    src/engines/kyra/script_lok.cpp
    src/engines/kyra/script_v1.cpp
    src/engines/kyra/sequences_lok.cpp
    src/engines/kyra/sound_lok.cpp
    src/engines/kyra/text_lok.cpp
    src/engines/kyra/timer_lok.cpp
    # Kyrandia 2
    src/engines/kyra/kyra_v2.cpp
    src/engines/kyra/kyra_hof.cpp
    src/engines/kyra/animator_hof.cpp
    src/engines/kyra/animator_v2.cpp
    src/engines/kyra/gui_hof.cpp
    src/engines/kyra/gui_v2.cpp
    src/engines/kyra/items_hof.cpp
    src/engines/kyra/items_v2.cpp
    src/engines/kyra/saveload_hof.cpp
    src/engines/kyra/scene_hof.cpp
    src/engines/kyra/scene_v2.cpp
    src/engines/kyra/screen_hof.cpp
    src/engines/kyra/screen_v2.cpp
    src/engines/kyra/script_hof.cpp
    src/engines/kyra/script_v2.cpp
    src/engines/kyra/sequences_hof.cpp
    src/engines/kyra/sequences_v2.cpp
    src/engines/kyra/text_hof.cpp
    src/engines/kyra/timer_hof.cpp
    # Kyrandia 3
    src/engines/kyra/kyra_mr.cpp
    src/engines/kyra/animator_mr.cpp
    src/engines/kyra/gui_mr.cpp
    src/engines/kyra/items_mr.cpp
    src/engines/kyra/saveload_mr.cpp
    src/engines/kyra/scene_mr.cpp
    src/engines/kyra/screen_mr.cpp
    src/engines/kyra/script_mr.cpp
    src/engines/kyra/sequences_mr.cpp
    src/engines/kyra/text_mr.cpp
    src/engines/kyra/timer_mr.cpp
    src/engines/kyra/sound_amiga.cpp
    src/engines/kyra/sound_towns.cpp
)

# SCI Engine sources (Sierra SCI0 / SCI1 / SCI1.1 — no SCI32, no video)
set(FQ_SCI_ENGINE_SOURCES
    src/engines/sci/console.cpp
    src/engines/sci/decompressor.cpp
    src/engines/sci/detection.cpp
    src/engines/sci/event.cpp
    src/engines/sci/resource.cpp
    src/engines/sci/resource_audio.cpp
    src/engines/sci/sci.cpp
    src/engines/sci/util.cpp
    # engine/*
    src/engines/sci/engine/features.cpp
    src/engines/sci/engine/file.cpp
    src/engines/sci/engine/gc.cpp
    src/engines/sci/engine/kernel.cpp
    src/engines/sci/engine/kevent.cpp
    src/engines/sci/engine/kfile.cpp
    src/engines/sci/engine/kgraphics.cpp
    src/engines/sci/engine/klists.cpp
    src/engines/sci/engine/kmath.cpp
    src/engines/sci/engine/kmenu.cpp
    src/engines/sci/engine/kmisc.cpp
    src/engines/sci/engine/kmovement.cpp
    src/engines/sci/engine/kparse.cpp
    src/engines/sci/engine/kpathing.cpp
    src/engines/sci/engine/kscripts.cpp
    src/engines/sci/engine/ksound.cpp
    src/engines/sci/engine/kstring.cpp
    src/engines/sci/engine/kvideo.cpp
    src/engines/sci/engine/message.cpp
    src/engines/sci/engine/object.cpp
    src/engines/sci/engine/savegame.cpp
    src/engines/sci/engine/script.cpp
    src/engines/sci/engine/scriptdebug.cpp
    src/engines/sci/engine/script_patches.cpp
    src/engines/sci/engine/selector.cpp
    src/engines/sci/engine/seg_manager.cpp
    src/engines/sci/engine/segment.cpp
    src/engines/sci/engine/state.cpp
    src/engines/sci/engine/static_selectors.cpp
    src/engines/sci/engine/vm.cpp
    src/engines/sci/engine/vm_types.cpp
    src/engines/sci/engine/workarounds.cpp
    # graphics/* (SCI16 only)
    src/engines/sci/graphics/animate.cpp
    src/engines/sci/graphics/cache.cpp
    src/engines/sci/graphics/compare.cpp
    src/engines/sci/graphics/controls16.cpp
    src/engines/sci/graphics/coordadjuster.cpp
    src/engines/sci/graphics/cursor.cpp
    src/engines/sci/graphics/font.cpp
    src/engines/sci/graphics/fontsjis.cpp
    src/engines/sci/graphics/maciconbar.cpp
    src/engines/sci/graphics/menu.cpp
    src/engines/sci/graphics/paint.cpp
    src/engines/sci/graphics/paint16.cpp
    src/engines/sci/graphics/palette.cpp
    src/engines/sci/graphics/picture.cpp
    src/engines/sci/graphics/portrait.cpp
    src/engines/sci/graphics/ports.cpp
    src/engines/sci/graphics/screen.cpp
    src/engines/sci/graphics/text16.cpp
    src/engines/sci/graphics/transitions.cpp
    src/engines/sci/graphics/view.cpp
    # parser/*
    src/engines/sci/parser/grammar.cpp
    src/engines/sci/parser/said.cpp
    src/engines/sci/parser/vocabulary.cpp
    # sound/*
    src/engines/sci/sound/audio.cpp
    src/engines/sci/sound/midiparser_sci.cpp
    src/engines/sci/sound/music.cpp
    src/engines/sci/sound/soundcmd.cpp
    src/engines/sci/sound/drivers/adlib.cpp
    src/engines/sci/sound/drivers/amigamac.cpp
    src/engines/sci/sound/drivers/cms.cpp
    src/engines/sci/sound/drivers/fb01.cpp
    src/engines/sci/sound/drivers/fmtowns.cpp
    src/engines/sci/sound/drivers/midi.cpp
    src/engines/sci/sound/drivers/pcjr.cpp
    # video/* (SEQ only — Robot decoder is SCI32)
    src/engines/sci/video/seq_decoder.cpp
)

# SCUMM Engine sources (core v0-v6)
set(FQ_SCUMM_ENGINE_SOURCES
    src/engines/scumm/detection.cpp
    src/engines/scumm/scumm.cpp
    src/engines/scumm/actor.cpp
    src/engines/scumm/akos.cpp
    src/engines/scumm/base-costume.cpp
    src/engines/scumm/bomp.cpp
    src/engines/scumm/boxes.cpp
    src/engines/scumm/camera.cpp
    src/engines/scumm/cdda.cpp
    src/engines/scumm/charset.cpp
    src/engines/scumm/charset-fontdata.cpp
    src/engines/scumm/costume.cpp
    src/engines/scumm/cursor.cpp
    src/engines/scumm/debugger.cpp
    src/engines/scumm/dialogs.cpp
    src/engines/scumm/file.cpp
    src/engines/scumm/file_nes.cpp
    src/engines/scumm/gfx.cpp
    src/engines/scumm/gfx_towns.cpp
    src/engines/scumm/help.cpp
    src/engines/scumm/input.cpp
    src/engines/scumm/midiparser_ro.cpp
    src/engines/scumm/nut_renderer.cpp
    src/engines/scumm/object.cpp
    src/engines/scumm/palette.cpp
    src/engines/scumm/resource.cpp
    src/engines/scumm/resource_v2.cpp
    src/engines/scumm/resource_v3.cpp
    src/engines/scumm/resource_v4.cpp
    src/engines/scumm/room.cpp
    src/engines/scumm/saveload.cpp
    src/engines/scumm/script.cpp
    src/engines/scumm/script_v0.cpp
    src/engines/scumm/script_v2.cpp
    src/engines/scumm/script_v3.cpp
    src/engines/scumm/script_v4.cpp
    src/engines/scumm/script_v5.cpp
    src/engines/scumm/script_v6.cpp
    src/engines/scumm/sound.cpp
    src/engines/scumm/string.cpp
    src/engines/scumm/usage_bits.cpp
    src/engines/scumm/util.cpp
    src/engines/scumm/vars.cpp
    src/engines/scumm/verbs.cpp
    # iMUSE (classic MIDI-based, used by v5-v6)
    src/engines/scumm/imuse/imuse.cpp
    src/engines/scumm/imuse/imuse_part.cpp
    src/engines/scumm/imuse/imuse_player.cpp
    src/engines/scumm/imuse/instrument.cpp
    src/engines/scumm/imuse/mac_m68k.cpp
    src/engines/scumm/imuse/pcspk.cpp
    src/engines/scumm/imuse/sysex_samnmax.cpp
    src/engines/scumm/imuse/sysex_scumm.cpp
    # Players
    src/engines/scumm/players/player_ad.cpp
    src/engines/scumm/players/player_apple2.cpp
    src/engines/scumm/players/player_mac.cpp
    src/engines/scumm/players/player_mod.cpp
    src/engines/scumm/players/player_nes.cpp
    src/engines/scumm/players/player_pce.cpp
    src/engines/scumm/players/player_sid.cpp
    src/engines/scumm/players/player_towns.cpp
    src/engines/scumm/players/player_v1.cpp
    src/engines/scumm/players/player_v2.cpp
    src/engines/scumm/players/player_v2a.cpp
    src/engines/scumm/players/player_v2base.cpp
    src/engines/scumm/players/player_v2cms.cpp
    src/engines/scumm/players/player_v3a.cpp
    src/engines/scumm/players/player_v3m.cpp
    src/engines/scumm/players/player_v4a.cpp
    src/engines/scumm/players/player_v5m.cpp
    # HE engine files (required by v6/v7 inheritance chain)
    src/engines/scumm/he/sound_he.cpp
    src/engines/scumm/he/resource_he.cpp
    src/engines/scumm/he/script_v60he.cpp
    src/engines/scumm/he/script_v70he.cpp
    src/engines/scumm/he/palette_he.cpp
    src/engines/scumm/he/floodfill_he.cpp
    src/engines/scumm/he/animation_he.cpp
    src/engines/scumm/he/sprite_he.cpp
    src/engines/scumm/he/wiz_he.cpp
    src/engines/scumm/he/logic_he.cpp
    src/engines/scumm/he/cup_player_he.cpp
    src/engines/scumm/he/script_v71he.cpp
    src/engines/scumm/he/script_v72he.cpp
    src/engines/scumm/he/script_v80he.cpp
    src/engines/scumm/he/script_v90he.cpp
    src/engines/scumm/he/script_v100he.cpp
)

# SCUMM v7/v8 sources (Full Throttle, The Dig, COMI)
set(FQ_SCUMM_7_8_SOURCES
    src/engines/scumm/script_v8.cpp
    # iMUSE Digital (v7/v8 audio)
    src/engines/scumm/imuse_digi/dimuse.cpp
    src/engines/scumm/imuse_digi/dimuse_bndmgr.cpp
    src/engines/scumm/imuse_digi/dimuse_codecs.cpp
    src/engines/scumm/imuse_digi/dimuse_music.cpp
    src/engines/scumm/imuse_digi/dimuse_script.cpp
    src/engines/scumm/imuse_digi/dimuse_sndmgr.cpp
    src/engines/scumm/imuse_digi/dimuse_tables.cpp
    src/engines/scumm/imuse_digi/dimuse_track.cpp
    # SMUSH video player
    src/engines/scumm/smush/channel.cpp
    src/engines/scumm/smush/codec1.cpp
    src/engines/scumm/smush/codec37.cpp
    src/engines/scumm/smush/codec47.cpp
    src/engines/scumm/smush/imuse_channel.cpp
    src/engines/scumm/smush/saud_channel.cpp
    src/engines/scumm/smush/smush_font.cpp
    src/engines/scumm/smush/smush_mixer.cpp
    src/engines/scumm/smush/smush_player.cpp
    # INSANE (action sequences)
    src/engines/scumm/insane/insane.cpp
    src/engines/scumm/insane/insane_ben.cpp
    src/engines/scumm/insane/insane_enemy.cpp
    src/engines/scumm/insane/insane_iact.cpp
    src/engines/scumm/insane/insane_scenes.cpp
)

# ScummVM Backends
set(FQ_BACKEND_SOURCES
    src/backends/base-backend.cpp
    src/backends/fs/abstract-fs.cpp
    src/backends/fs/rp2350/rp2350-fs.cpp
    src/backends/fs/rp2350/rp2350-fs-factory.cpp
    src/backends/saves/default/default-saves.cpp
    src/backends/saves/savefile.cpp
    src/common/zlib.cpp
    src/audio/midiparser_xmidi.cpp
    src/audio/decoders/voc.cpp
    src/audio/softsynth/fmtowns_pc98/towns_audio.cpp
    src/audio/softsynth/fmtowns_pc98/towns_euphony.cpp
    src/audio/softsynth/fmtowns_pc98/towns_pc98_driver.cpp
    src/audio/softsynth/fmtowns_pc98/towns_pc98_fmsynth.cpp
    src/audio/mods/maxtrax.cpp
    src/graphics/thumbnail.cpp
    src/graphics/scaler/thumbnail_intern.cpp
    src/graphics/sjis.cpp
    src/backends/events/default/default-events.cpp
    src/backends/timer/default/default-timer.cpp
    src/backends/platform/rp2350/rp2350-minimal.cpp
    src/backends/platform/rp2350/rp2350-system.cpp
)

# Beneath a Steel Sky: in upstream's tree, not in its build. Added by the
# port, from the engine's module.mk (§5.4).
set(FQ_SKY_ENGINE_SOURCES
    src/engines/sky/autoroute.cpp
    src/engines/sky/compact.cpp
    src/engines/sky/control.cpp
    src/engines/sky/debug.cpp
    src/engines/sky/detection.cpp
    src/engines/sky/disk.cpp
    src/engines/sky/grid.cpp
    src/engines/sky/hufftext.cpp
    src/engines/sky/intro.cpp
    src/engines/sky/logic.cpp
    src/engines/sky/mouse.cpp
    src/engines/sky/rnc_deco.cpp
    src/engines/sky/screen.cpp
    src/engines/sky/sky.cpp
    src/engines/sky/sound.cpp
    src/engines/sky/text.cpp
    src/engines/sky/music/adlibchannel.cpp
    src/engines/sky/music/adlibmusic.cpp
    src/engines/sky/music/gmchannel.cpp
    src/engines/sky/music/gmmusic.cpp
    src/engines/sky/music/mt32music.cpp
    src/engines/sky/music/musicbase.cpp
)

# Compiled here though upstream does not build them; check_sources.sh
# accepts these and no others.
set(FQ_ADDED_SOURCES ${FQ_SKY_ENGINE_SOURCES})

# Every list the firmware compiles.
set(FQ_ALL_SOURCE_LISTS
    FQ_MAIN_SOURCES
    FQ_DRIVER_SOURCES
    FQ_STORAGE_SOURCES
    FQ_COMMON_SOURCES
    FQ_GRAPHICS_SOURCES
    FQ_GUI_SOURCES
    FQ_VIDEO_SOURCES
    FQ_IMAGE_SOURCES
    FQ_AUDIO_SOURCES
    FQ_BASE_SOURCES
    FQ_ENGINE_SOURCES
    FQ_AGI_ENGINE_SOURCES
    FQ_GOB_ENGINE_SOURCES
    FQ_KYRA_ENGINE_SOURCES
    FQ_SCI_ENGINE_SOURCES
    FQ_SCUMM_ENGINE_SOURCES
    FQ_SCUMM_7_8_SOURCES
    FQ_SKY_ENGINE_SOURCES
    FQ_BACKEND_SOURCES
)

# Upstream builds these and the port does not; each is replaced by
# picocalc/ (§5.3) or has no hardware to drive.
set(FQ_EXCLUDED_SOURCES
    src/main.c                          # picocalc/main.c (§13.1)
    drivers/HDMI.c                      # the LCD presenter (§7); graphics_* in picocalc/video_shim.c
    drivers/audio/audio.c               # PWM audio (§8); cabal_audio_* in picocalc/audio_shim.c
    drivers/ps2/ps2.c                   # the southbridge keyboard (§9)
    drivers/ps2/kbd/ps2kbd_mrmltr.cpp   # the same
    drivers/ps2/kbd/ps2kbd_wrapper.cpp  # the same; ps2kbd_* in picocalc/input_shim.c
    drivers/sdcard/pio_spi.c            # the card is on hardware spi0 (§10.1)
)
