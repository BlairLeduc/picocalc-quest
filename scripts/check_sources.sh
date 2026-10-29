#!/usr/bin/env bash
# check_sources.sh — does cmake/upstream-sources.cmake still match
# upstream's build? (design.md §5.4)
#
#   scripts/check_sources.sh
#
# Takes every .c, .cpp and .S that upstream's CMake files name (its
# CMakeLists.txt and the driver files it always includes) and checks that
# each is either compiled here or in FQ_EXCLUDED_SOURCES, that nothing is
# both, that nothing here is unknown to upstream, and that every file
# exists. Run it after a submodule bump; CI runs it on every push.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
upstream="${UPSTREAM:-$root/modules/frank-quest}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

if [ ! -f "$upstream/CMakeLists.txt" ]; then
    echo "check_sources.sh: no $upstream/CMakeLists.txt — run git submodule update --init" >&2
    exit 1
fi

# Upstream's sources. ${CMAKE_CURRENT_LIST_DIR} is the naming file's own
# directory; drivers/usbhid is left out because upstream builds it only
# with USB_HID_ENABLED, which is off by default.
for f in CMakeLists.txt src/fatfs/CMakeLists.txt drivers/sdcard/sdcard.cmake \
         drivers/ps2/CMakeLists.txt drivers/audio/CMakeLists.txt; do
    dir="$(dirname "$f")"
    sed -e 's/#.*//' -e "s|\${CMAKE_CURRENT_LIST_DIR}/|$dir/|g" "$upstream/$f" |
        grep -oE '[A-Za-z0-9_./-]+\.(c|cpp|S)\b' |
        sed 's|^\./||'
done | LC_ALL=C sort -u >"$tmp/upstream"

# Ours, read by CMake itself so that comments and quoting are CMake's.
cat >"$tmp/print.cmake" <<'EOF'
include(${LISTS})
set(all)
foreach(list ${FQ_ALL_SOURCE_LISTS})
    list(APPEND all ${${list}})
endforeach()
string(REPLACE ";" "\n" all "${all}")
string(REPLACE ";" "\n" excluded "${FQ_EXCLUDED_SOURCES}")
file(WRITE ${OUT}/compiled "${all}\n")
file(WRITE ${OUT}/excluded "${excluded}\n")
EOF
cmake -DLISTS="$root/cmake/upstream-sources.cmake" -DOUT="$tmp" -P "$tmp/print.cmake"
LC_ALL=C sort -u "$tmp/compiled" -o "$tmp/compiled"
LC_ALL=C sort -u "$tmp/excluded" -o "$tmp/excluded"
LC_ALL=C sort -u "$tmp/compiled" "$tmp/excluded" >"$tmp/ours"

status=0
report() {
    if [ -s "$2" ]; then
        echo "$1:"
        sed 's/^/    /' "$2"
        status=1
    fi
}
comm -23 "$tmp/upstream" "$tmp/ours" >"$tmp/missing"
comm -13 "$tmp/upstream" "$tmp/ours" >"$tmp/unknown"
comm -12 "$tmp/compiled" "$tmp/excluded" >"$tmp/both"
while IFS= read -r f; do
    [ -f "$upstream/$f" ] || echo "$f"
done <"$tmp/ours" >"$tmp/absent"

report "upstream builds these, and they are neither compiled nor excluded" "$tmp/missing"
report "listed here, but upstream does not build them" "$tmp/unknown"
report "both compiled and excluded" "$tmp/both"
report "listed here, but not in the submodule" "$tmp/absent"

if [ "$status" = 0 ]; then
    echo "check_sources.sh: $(wc -l <"$tmp/compiled" | tr -d ' ') compiled, $(wc -l <"$tmp/excluded" | tr -d ' ') excluded, matching upstream $(git -C "$upstream" rev-parse --short HEAD 2>/dev/null || echo '(unknown commit)')"
fi
exit "$status"
