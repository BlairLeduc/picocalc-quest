# Host tests (design.md §15). C tests build the portable parts of
# picocalc/ with the system compiler and no Pico SDK, one binary per area;
# exit 77 is a skip.

# create_patch.sh then apply_patch.sh reproduces working/ exactly: on a
# made-up upstream, and on the real patch when the submodule is present.
foreach(mode synthetic repo)
    add_test(NAME test_patch_roundtrip_${mode}
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_patch_roundtrip.sh ${mode})
    set_tests_properties(test_patch_roundtrip_${mode} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()

# The shell's time to calendar fields (§13.2).
add_executable(test_clock test_clock.c ${PROJECT_SOURCE_DIR}/picocalc/clock.c)
target_include_directories(test_clock PRIVATE ${PROJECT_SOURCE_DIR}/picocalc)
target_compile_definitions(test_clock PRIVATE PICOCALC_QUEST_HOST=1)
target_compile_options(test_clock PRIVATE ${PICOCALC_QUEST_WARNINGS})
add_test(NAME test_clock COMMAND test_clock)

# The frame pool, the colour conversion and the row expander (§7.2).
add_executable(test_framepool test_framepool.c ${PROJECT_SOURCE_DIR}/picocalc/framepool.c)
target_include_directories(test_framepool PRIVATE ${PROJECT_SOURCE_DIR}/picocalc)
target_compile_definitions(test_framepool PRIVATE PICOCALC_QUEST_HOST=1)
target_compile_options(test_framepool PRIVATE ${PICOCALC_QUEST_WARNINGS})
add_test(NAME test_framepool COMMAND test_framepool)

# The key map: every MCU code, retranslated releases, Insert, the chords (§9.2).
add_executable(test_keymap test_keymap.c ${PROJECT_SOURCE_DIR}/picocalc/keymap.c)
target_include_directories(test_keymap PRIVATE ${PROJECT_SOURCE_DIR}/picocalc)
target_compile_definitions(test_keymap PRIVATE PICOCALC_QUEST_HOST=1)
target_compile_options(test_keymap PRIVATE ${PICOCALC_QUEST_WARNINGS})
add_test(NAME test_keymap COMMAND test_keymap)

# The mixer's samples to PWM compare words (§8.2).
add_executable(test_sound test_sound.c)
target_include_directories(test_sound PRIVATE ${PROJECT_SOURCE_DIR}/picocalc)
target_compile_definitions(test_sound PRIVATE PICOCALC_QUEST_HOST=1)
target_compile_options(test_sound PRIVATE ${PICOCALC_QUEST_WARNINGS})
target_link_libraries(test_sound PRIVATE m)
add_test(NAME test_sound COMMAND test_sound)
