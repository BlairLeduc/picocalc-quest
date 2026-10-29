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
        picocalc/input_shim.c
        picocalc/main.c
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

        # clk_peri follows clk_sys, for the LCD's SPI divider (§11.1).
        PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=1
        # UART0 at 115200 8-N-1 on GP2/GP3, as the shell's (D12).
        PICOCALC_QUEST_UART=${PICOCALC_QUEST_UART_ON}
        PICO_DEFAULT_UART=0
        PICO_DEFAULT_UART_TX_PIN=2
        PICO_DEFAULT_UART_RX_PIN=3
    )

    # As upstream: dlmalloc in mspace-only mode over PSRAM, with footers
    # to catch bad frees early.
    set_source_files_properties(${PICOCALC_QUEST_UPSTREAM}/drivers/dlmalloc.c PROPERTIES COMPILE_DEFINITIONS
        "ONLY_MSPACES=1;HAVE_MMAP=0;HAVE_MORECORE=0;NO_MALLOC_STATS=1;USE_LOCKS=0;ABORT_ON_ASSERT_FAILURE=0;MALLOC_FAILURE_ACTION=;LACKS_SYS_MMAN_H=1;LACKS_UNISTD_H=1;LACKS_SYS_PARAM_H=1;FOOTERS=1;PROCEED_ON_ERROR=0;MALLOC_INSPECT_ALL=1")

    target_link_libraries(picocalc-quest
        pico_stdlib
        pico_multicore
        hardware_dma
        hardware_flash
        hardware_irq
        hardware_pio
        hardware_pwm
        hardware_spi
        hardware_watchdog
    )

    pico_enable_stdio_uart(picocalc-quest ${PICOCALC_QUEST_UART_ON})
    pico_enable_stdio_usb(picocalc-quest 0)

    # Upstream's linker script: the 64 KB main stack the GOB engine needs
    # (§5.4, §12.1).
    pico_set_linker_script(picocalc-quest ${PICOCALC_QUEST_UPSTREAM}/linker/memmap_frank_quest.ld)

    pico_add_extra_outputs(picocalc-quest)

    # The image's size on every build (§12, §15).
    get_filename_component(PICOCALC_QUEST_TOOLCHAIN_BIN ${CMAKE_C_COMPILER} DIRECTORY)
    find_program(PICOCALC_QUEST_SIZE arm-none-eabi-size HINTS ${PICOCALC_QUEST_TOOLCHAIN_BIN} REQUIRED)
    add_custom_command(TARGET picocalc-quest POST_BUILD
        COMMAND ${PICOCALC_QUEST_SIZE} $<TARGET_FILE:picocalc-quest>
        VERBATIM)
endif()
