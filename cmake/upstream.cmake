# upstream.cmake — the patched copy of upstream the build compiles
# (design.md §5.2).
#
# frank-quest/ is modules/frank-quest/ with patches/frank-quest.patch
# applied by scripts/apply_patch.sh. It is made at configure time, not
# build time, because cmake/upstream-sources.cmake names its files and
# CMake needs them to exist when it generates. A stamp holding the
# submodule's commit and the patch's hash says whether the copy is current;
# the patch and the submodule's HEAD are configure dependencies, so editing
# the patch or checking out another commit re-runs configure.
#
# Also runs as a script, for testing the stamp outside a build:
#   cmake -DROOT=<dir> -P cmake/upstream.cmake

function(picocalc_quest_patch_upstream root)
    set(upstream ${root}/modules/frank-quest)
    set(patch ${root}/patches/frank-quest.patch)
    set(apply ${root}/scripts/apply_patch.sh)
    set(out ${root}/frank-quest)
    set(stamp_file ${out}/.picocalc-quest-stamp)

    file(GLOB upstream_entries ${upstream}/*)
    if (NOT upstream_entries)
        message(FATAL_ERROR "picocalc-quest: ${upstream} is empty; run git submodule update --init")
    endif()

    execute_process(
        COMMAND git -C ${upstream} rev-parse HEAD
        OUTPUT_VARIABLE commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE result)
    if (NOT result EQUAL 0)
        set(commit "not a git checkout")
    endif()
    file(SHA256 ${patch} patch_hash)
    file(SHA256 ${apply} apply_hash)
    set(stamp "upstream ${commit}\npatch ${patch_hash}\napply_patch.sh ${apply_hash}\n")

    set(old "")
    if (EXISTS ${stamp_file})
        file(READ ${stamp_file} old)
    endif()
    if (old STREQUAL stamp)
        message(STATUS "picocalc-quest: frank-quest/ is current (upstream ${commit})")
    else()
        message(STATUS "picocalc-quest: copying and patching upstream ${commit}")
        execute_process(
            COMMAND bash ${apply} --upstream ${upstream} --out ${out} --patch ${patch}
            RESULT_VARIABLE result)
        if (NOT result EQUAL 0)
            file(REMOVE ${stamp_file})
            message(FATAL_ERROR "picocalc-quest: the patch does not apply; see design.md §5.2")
        endif()
        file(WRITE ${stamp_file} "${stamp}")
    endif()

    if (NOT CMAKE_SCRIPT_MODE_FILE)
        execute_process(
            COMMAND git -C ${upstream} rev-parse --path-format=absolute --git-path HEAD
            OUTPUT_VARIABLE head
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE result)
        set(depends ${patch} ${apply})
        if (result EQUAL 0 AND EXISTS ${head})
            list(APPEND depends ${head})
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${depends})
    endif()
endfunction()

if (CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    picocalc_quest_patch_upstream(${ROOT})
endif()
