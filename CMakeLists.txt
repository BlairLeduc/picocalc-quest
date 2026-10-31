# picocalc-quest — one source tree, two targets (design.md §5, §15):
#   - the UF2 for the PicoCalc with a Pimoroni Pico Plus 2 (default)
#   - a host test build, with the system compiler and no Pico SDK, behind
#     -DPICOCALC_QUEST_HOST=ON, run under CTest.
#
# Upstream's CMakeLists.txt is not used (§5.4): the build compiles a
# patched copy of upstream, made at configure time (cmake/upstream.cmake),
# from the lists in cmake/upstream-sources.cmake.

cmake_minimum_required(VERSION 3.18)

option(PICOCALC_QUEST_HOST "Build the host tests, with no Pico SDK" OFF)

set(PICOCALC_QUEST_WARNINGS -Wall -Wextra -Werror)

if (PICOCALC_QUEST_HOST)
    project(picocalc_quest_host C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)

    enable_testing()
    add_subdirectory(test/host)
else()
    # One image, for the Plus 2; it runs on a Plus 2 W too (D2, §11.2).
    if (NOT DEFINED PICO_BOARD)
        set(PICO_BOARD pimoroni_pico_plus2_rp2350)
    endif()

    # Upstream's new and delete go to PSRAM (frank_quest_new.cpp), so the
    # SDK's must not be built.
    set(PICO_CXX_DISABLE_ALLOCATION_OVERRIDES 1)

    include(cmake/pico_sdk_import.cmake)

    project(picocalc_quest C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # As upstream: malloc and friends are wrapped to its PSRAM allocator,
    # in place of the SDK's pico_malloc, which pico_sdk_init() only defines
    # if no target of that name exists yet.
    add_library(pico_malloc INTERFACE)
    add_library(pico_malloc_headers INTERFACE)
    target_include_directories(pico_malloc_headers SYSTEM INTERFACE
        ${PICO_SDK_PATH}/src/rp2_common/pico_malloc/include)
    target_link_options(pico_malloc INTERFACE
        -Wl,--wrap=malloc
        -Wl,--wrap=calloc
        -Wl,--wrap=realloc
        -Wl,--wrap=free)

    pico_sdk_init()

    include(cmake/upstream.cmake)
    picocalc_quest_patch_upstream(${CMAKE_CURRENT_SOURCE_DIR})
    set(PICOCALC_QUEST_UPSTREAM ${CMAKE_CURRENT_SOURCE_DIR}/frank-quest)

    # The firmware's version, git describe --always --dirty, for the
    # banner (§13.1). A target that always runs, so it is taken at build
    # time; the header only changes when the text does.
    set(PICOCALC_QUEST_GENERATED ${CMAKE_BINARY_DIR}/generated)
    add_custom_target(picocalc_quest_version ALL
        COMMAND ${CMAKE_COMMAND} -DSRC=${CMAKE_CURRENT_SOURCE_DIR}
                -DOUT=${PICOCALC_QUEST_GENERATED}/picocalc_quest_version.h
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake
        BYPRODUCTS ${PICOCALC_QUEST_GENERATED}/picocalc_quest_version.h
        COMMENT "picocalc-quest: version from git describe"
        VERBATIM)

    # PICOCALC_QUEST_UART=OFF is the build that ships: no stdio, GP2/GP3
    # free. ON, the default, is the development build (§14.2).
    option(PICOCALC_QUEST_UART "stdio on UART0, GP2/GP3 (development)" ON)
    # 0 cached, 1 uncached, 2 DMA, 3 none (picocalc/video.h); M2 measured
    # them and kept DMA, which cost core 0 least (§7.3).
    set(PICOCALC_QUEST_FB_READ 2 CACHE STRING "How core 1 reads frames from the PSRAM")
    # ON checks upstream's skipped screen updates (rp2350-system.cpp, M4):
    # a checksum of the game's screen at each skip, and a log line when it
    # changed with nothing marking it. A diagnostic: it costs core 0 a
    # PSRAM read of the screen per skip.
    option(PICOCALC_QUEST_SCREEN_CHECK "log screen writes the update skip missed" OFF)
    # M8's row levers (picocalc/video.h), each OFF for its control build:
    # core 0 refills only a frame buffer's stale rows; core 1 sends the
    # panel only the rows changed since its last frame.
    option(PICOCALC_QUEST_ROW_COPY "refill only the rows changed since a buffer's frame" ON)
    option(PICOCALC_QUEST_ROW_PRESENT "present only the rows changed since the panel's frame" ON)
    # M8's flash lever (picocalc/board.c, §11.4): the QMI divider and RX
    # delay at 300 MHz, 0 for the bootrom's doubled (6 and 4, 50 MHz).
    # M8 qualified 4 at RX delays 1-7 and kept 4, 4: 75 MHz.
    # FLASH_SWEEP ON is the diagnostic build that qualifies them: every
    # divider and RX delay read against the image's checksum at boot.
    set(PICOCALC_QUEST_FLASH_DIV 4 CACHE STRING "flash QMI divider at 300 MHz, 0 the bootrom's doubled")
    set(PICOCALC_QUEST_FLASH_RXDELAY 4 CACHE STRING "flash QMI RX delay with FLASH_DIV")
    option(PICOCALC_QUEST_FLASH_SWEEP "qualify flash timings at boot (diagnostic)" OFF)
    # SCI's AdLib music on one OPL2, mono, in place of two (M8; the
    # patched sci/sound/drivers/adlib.cpp). It halves the music's cost
    # but loses the panning, so it stays off: the owner listens through
    # the headphone jack.
    option(PICOCALC_QUEST_SCI_OPL_MONO "SCI's AdLib on one OPL2, mono" OFF)
    # ON draws M1's corner-coded test pattern at boot, for bring-up
    # (picocalc/core1.c); OFF, the panel stays black until the first frame.
    option(PICOCALC_QUEST_TEST_PATTERN "draw the test pattern at boot" OFF)
    # Non-zero is the scratch build that forces the audio's late path
    # (picocalc/audio_shim.c, §16 M5): core 0's interrupts masked for this
    # many milliseconds every five seconds.
    set(PICOCALC_QUEST_AUDIO_STALL 0 CACHE STRING "ms to mask core 0's interrupts every 5 s (scratch)")
    # ScummVM's OPL (AdLib) emulator for every engine: "db" (DOSBox) or
    # "mame" (§8.3, M5).
    set(PICOCALC_QUEST_OPL db CACHE STRING "OPL emulator: db or mame")
    if (PICOCALC_QUEST_SCREEN_CHECK)
        set(PICOCALC_QUEST_SCREEN_CHECK_ON 1)
    else()
        set(PICOCALC_QUEST_SCREEN_CHECK_ON 0)
    endif()
    if (PICOCALC_QUEST_UART)
        set(PICOCALC_QUEST_UART_ON 1)
    else()
        set(PICOCALC_QUEST_UART_ON 0)
    endif()

    # Upstream's version, for its own banner and selector: version.txt is
    # "MAJOR MINOR", shown as MAJOR.MINOR with MINOR in two digits.
    file(STRINGS ${PICOCALC_QUEST_UPSTREAM}/version.txt fq_version LIMIT_COUNT 1)
    if (NOT fq_version MATCHES "^([0-9]+) ([0-9]+)$")
        message(FATAL_ERROR "picocalc-quest: upstream's version.txt is not 'MAJOR MINOR': ${fq_version}")
    endif()
    set(fq_minor ${CMAKE_MATCH_2})
    string(LENGTH "${fq_minor}" fq_minor_len)
    if (fq_minor_len LESS 2)
        set(fq_minor "0${fq_minor}")
    endif()
    set(PICOCALC_QUEST_CABAL_VERSION "${CMAKE_MATCH_1}.${fq_minor}")

    include(cmake/upstream-sources.cmake)
    set(PICOCALC_QUEST_UPSTREAM_SOURCES)
    foreach(list ${FQ_ALL_SOURCE_LISTS})
        list(APPEND PICOCALC_QUEST_UPSTREAM_SOURCES ${${list}})
    endforeach()
    list(TRANSFORM PICOCALC_QUEST_UPSTREAM_SOURCES PREPEND ${PICOCALC_QUEST_UPSTREAM}/)

    # The port's own code (§5.3), held to our warnings; upstream's is
    # built as it comes.
    set(PICOCALC_QUEST_SOURCES
        picocalc/audio_shim.c
        picocalc/board.c
        picocalc/clock.c
        picocalc/core1.c
        picocalc/exit.c
        picocalc/fatal.c
        picocalc/font8x10.c
        picocalc/framepool.c
        picocalc/input_shim.c
        picocalc/kbd.c
        picocalc/keymap.c
        picocalc/lcd.c
        picocalc/log.c
        picocalc/main.c
        picocalc/page.c
        picocalc/pointer.c
        picocalc/psram_check.c
        picocalc/settings.c
        picocalc/southbridge.c
        picocalc/status.c
        picocalc/syscalls.c
        picocalc/video_shim.c
    )
    set_source_files_properties(${PICOCALC_QUEST_SOURCES} PROPERTIES
        COMPILE_OPTIONS "${PICOCALC_QUEST_WARNINGS}")

    add_executable(picocalc-quest
        ${PICOCALC_QUEST_SOURCES}
        ${PICOCALC_QUEST_UPSTREAM_SOURCES}
    )
    add_dependencies(picocalc-quest picocalc_quest_version)

    # Upstream's include paths, the PS/2 and I2S headers among them: the
    # shims implement what those headers declare.
    target_include_directories(picocalc-quest PRIVATE
        picocalc
        ${PICOCALC_QUEST_GENERATED}
        ${PICOCALC_QUEST_UPSTREAM}/src
        ${PICOCALC_QUEST_UPSTREAM}/src/engines
        ${PICOCALC_QUEST_UPSTREAM}/src/fatfs
        ${PICOCALC_QUEST_UPSTREAM}/drivers
        ${PICOCALC_QUEST_UPSTREAM}/drivers/sdcard
        ${PICOCALC_QUEST_UPSTREAM}/drivers/ps2
        ${PICOCALC_QUEST_UPSTREAM}/drivers/ps2/kbd
        ${PICOCALC_QUEST_UPSTREAM}/drivers/audio
    )

    # Upstream's definitions, less its board variant, clock, HDMI and I2S
    # pin settings (§5.4). USE_I2S_AUDIO stays: it switches upstream's
    # audio path on, and picocalc/audio_shim.c answers to it (§8.2).
    target_compile_definitions(picocalc-quest PRIVATE
        PSRAM_MAX_FREQ_MHZ=100              # §11.3
        CABAL_PSRAM_SIZE_BYTES=8388608
        CABAL_VERSION="${PICOCALC_QUEST_CABAL_VERSION}"
        CABAL_SCREEN_WIDTH=320
        CABAL_SCREEN_HEIGHT=200
        CABAL_HDMI_HEIGHT=240
        USE_I2S_AUDIO=1

        ENABLE_AGI=1
        ENABLE_GOB=1
        ENABLE_SCI=1
        ENABLE_KYRA=1
        ENABLE_SCUMM=1
        ENABLE_SCUMM_7_8=1

        CABAL_LOW_MEMORY=1
        PICO_MALLOC_PANIC=0
        SKIP_PICO_MALLOC=1
        PICO_ON_DEVICE=1
        __RP2350__=1
        CABAL_HAVE_STDINT_H=1
        DISABLE_GUI=1
        DISABLE_DEFAULT_AUDIOCD_MANAGER=1
        CABAL_MINIMAL_AUDIO=1
        PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1

        # The card on hardware spi0 at 25 MHz (§10.1).
        SDCARD_PIN_SPI0_MISO=16
        SDCARD_PIN_SPI0_CS=17
        SDCARD_PIN_SPI0_SCK=18
        SDCARD_PIN_SPI0_MOSI=19
        SDCARD_CLK_FAST=25000000
        # get_fattime() from the PicoCalc clock, not the driver's (§13.2).
        SDCARD_EXTERNAL_FATTIME=1

        # clk_peri follows clk_sys, for the LCD's SPI divider (§11.1).
        PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=1
        # UART0 at 115200 8-N-1 on GP2/GP3, as the shell's (D12).
        PICOCALC_QUEST_UART=${PICOCALC_QUEST_UART_ON}
        PICO_DEFAULT_UART=0
        PICO_DEFAULT_UART_TX_PIN=2
        PICO_DEFAULT_UART_RX_PIN=3
        # How core 1 reads frames from the PSRAM (video.h, §7.3).
        PICOCALC_QUEST_FB_READ=${PICOCALC_QUEST_FB_READ}
        PICOCALC_QUEST_SCREEN_CHECK=${PICOCALC_QUEST_SCREEN_CHECK_ON}
        PICOCALC_QUEST_ROW_COPY=$<BOOL:${PICOCALC_QUEST_ROW_COPY}>
        PICOCALC_QUEST_ROW_PRESENT=$<BOOL:${PICOCALC_QUEST_ROW_PRESENT}>
        PICOCALC_QUEST_FLASH_DIV=${PICOCALC_QUEST_FLASH_DIV}
        PICOCALC_QUEST_FLASH_RXDELAY=${PICOCALC_QUEST_FLASH_RXDELAY}
        PICOCALC_QUEST_FLASH_SWEEP=$<BOOL:${PICOCALC_QUEST_FLASH_SWEEP}>
        PICOCALC_QUEST_SCI_OPL_MONO=$<BOOL:${PICOCALC_QUEST_SCI_OPL_MONO}>
        PICOCALC_QUEST_TEST_PATTERN=$<BOOL:${PICOCALC_QUEST_TEST_PATTERN}>
        PICOCALC_QUEST_AUDIO_STALL=${PICOCALC_QUEST_AUDIO_STALL}
        PICOCALC_QUEST_OPL="${PICOCALC_QUEST_OPL}"
    )

    # As upstream: dlmalloc in mspace-only mode over PSRAM, with footers
    # to catch bad frees early.
    set_source_files_properties(${PICOCALC_QUEST_UPSTREAM}/drivers/dlmalloc.c PROPERTIES COMPILE_DEFINITIONS
        "ONLY_MSPACES=1;HAVE_MMAP=0;HAVE_MORECORE=0;NO_MALLOC_STATS=1;USE_LOCKS=0;ABORT_ON_ASSERT_FAILURE=0;MALLOC_FAILURE_ACTION=;LACKS_SYS_MMAN_H=1;LACKS_UNISTD_H=1;LACKS_SYS_PARAM_H=1;FOOTERS=1;PROCEED_ON_ERROR=0;MALLOC_INSPECT_ALL=1")

    target_link_libraries(picocalc-quest
        pico_stdlib
        pico_multicore
        pico_unique_id
        hardware_adc
        hardware_dma
        hardware_flash
        hardware_i2c
        hardware_irq
        hardware_pio
        hardware_powman     # the shell's clock (§13.2)
        hardware_pwm
        hardware_spi
        hardware_watchdog
        hardware_xip_cache
    )

    # stdio is picocalc/log.c's driver, into the log ring (§14.2), not the
    # SDK's UART driver, which blocks.
    pico_enable_stdio_uart(picocalc-quest 0)
    pico_enable_stdio_usb(picocalc-quest 0)

    # Core 0's time in upstream's delay and screen update, for the
    # heartbeat (main.c, §14.3); upstream's scan of /quest, timed (main.c,
    # §16 M3), by its C++ name: frank_quest_scan_games(QuestGame *, int);
    # the PSRAM mspace's creation, so the selector can report the heap
    # (main.c, M4); exit(), which error() ends in (fatal.c); and a file's
    # open for writing and close, to time saves (main.c, M7).
    target_link_options(picocalc-quest PRIVATE
        -Wl,--wrap=cabal_delay
        -Wl,--wrap=cabal_update_screen
        -Wl,--wrap=_Z22frank_quest_scan_gamesP9QuestGamei
        -Wl,--wrap=create_mspace_with_base
        -Wl,--wrap=exit
        -Wl,--wrap=cabal_file_open_write
        -Wl,--wrap=cabal_file_close)

    # Upstream's linker script: the 64 KB main stack the GOB engine needs
    # (§5.4, §12.1). M8's SRAM tiers (§16): the code of each tier's object
    # files runs from SRAM, the way the SDK's script already keeps
    # libgcc's out of flash: .text excludes them, so .data takes them.
    # Each tier includes the ones before it; 0 is the control build. M8
    # measured all four and kept them (milestones.md).
    set(PICOCALC_QUEST_SRAM_TIER 4 CACHE STRING "SRAM placement tier, 0 none (M8)")
    set(PICOCALC_QUEST_SRAM_TIERS
        # 1: the mixer, the rate converter and DOSBox's OPL.
        "*audio/mixer.cpp.o *audio/rate.cpp.o *opl/dosbox.cpp.o *opl/dbopl.cpp.o"
        # 2: SCI's AdLib driver and its music.
        "*sci/sound/drivers/adlib.cpp.o *sci/sound/music.cpp.o"
        # 3: SCI's interpreter: the VM, the kernel's dispatch, objects and
        #    scripts.
        "*sci/engine/vm.cpp.o *sci/engine/vm_types.cpp.o *sci/engine/selector.cpp.o *sci/engine/object.cpp.o *sci/engine/script.cpp.o *sci/engine/seg_manager.cpp.o *sci/engine/kernel.cpp.o"
        # 4: the PSRAM heap.
        "*drivers/psram_allocator.c.o *drivers/dlmalloc.c.o")
    set(PICOCALC_QUEST_SRAM_FILES "")
    foreach(tier RANGE 1 4)
        if (tier LESS_EQUAL PICOCALC_QUEST_SRAM_TIER)
            math(EXPR i "${tier} - 1")
            list(GET PICOCALC_QUEST_SRAM_TIERS ${i} files)
            string(APPEND PICOCALC_QUEST_SRAM_FILES " ${files}")
        endif()
    endforeach()
    file(READ ${PICOCALC_QUEST_UPSTREAM}/linker/memmap_frank_quest.ld PICOCALC_QUEST_LD)
    set(PICOCALC_QUEST_LD_EXCLUDE "EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:")
    string(FIND "${PICOCALC_QUEST_LD}" "${PICOCALC_QUEST_LD_EXCLUDE}" at)
    if (at LESS 0)
        message(FATAL_ERROR "upstream's linker script has no ${PICOCALC_QUEST_LD_EXCLUDE}")
    endif()
    string(REPLACE "${PICOCALC_QUEST_LD_EXCLUDE}" "${PICOCALC_QUEST_LD_EXCLUDE}${PICOCALC_QUEST_SRAM_FILES}"
        PICOCALC_QUEST_LD "${PICOCALC_QUEST_LD}")
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/memmap_picocalc_quest.ld CONTENT "${PICOCALC_QUEST_LD}")
    pico_set_linker_script(picocalc-quest ${CMAKE_BINARY_DIR}/memmap_picocalc_quest.ld)

    pico_add_extra_outputs(picocalc-quest)

    # The image's size on every build (§12, §15).
    get_filename_component(PICOCALC_QUEST_TOOLCHAIN_BIN ${CMAKE_C_COMPILER} DIRECTORY)
    find_program(PICOCALC_QUEST_SIZE arm-none-eabi-size HINTS ${PICOCALC_QUEST_TOOLCHAIN_BIN} REQUIRED)
    add_custom_command(TARGET picocalc-quest POST_BUILD
        COMMAND ${PICOCALC_QUEST_SIZE} $<TARGET_FILE:picocalc-quest>
        VERBATIM)
endif()
