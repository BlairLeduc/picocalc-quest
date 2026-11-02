#!/usr/bin/env bash
# build_release.sh — build the image that ships, from a clean tree, into
# dist/ (design.md §14.2, §16 M9).
#
#   scripts/build_release.sh 1.0
#   scripts/build_release.sh             # the version from a v<x.y> tag on HEAD
#
# Writes, for version x.y:
#   dist/picocalc-quest-v<x.y>-pico-plus2.uf2   the image, for UF2 Loader or BOOTSEL
#   dist/picocalc-quest-v<x.y>-pico-plus2.elf   its symbols, for scripts/counters.sh
#   dist/picocalc-quest-v<x.y>-pico-plus2.zip   the image with README, LICENSE and
#                                               upstream's licence texts
#
# The build is the release one (PICOCALC_QUEST_UART=OFF: no stdio, GP2/GP3
# free), configured afresh in build/release with every other option at
# its default. It refuses a tree with changes, so the image is what the
# commit says; the version in its counter block is git describe's, and
# HEAD should carry the tag v<x.y> before the image goes out.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
case "${1:-}" in
    -h|--help) sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
esac

tag="$(git -C "$root" describe --tags --exact-match 2>/dev/null || true)"
version="${1:-${tag#v}}"
if ! [[ "$version" =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "build_release.sh: give the version as x.y, or tag HEAD v<x.y>" >&2
    exit 2
fi
if [ -n "$(git -C "$root" status --porcelain --ignore-submodules=none)" ]; then
    echo "build_release.sh: the tree has changes; commit them first" >&2
    git -C "$root" status --short >&2
    exit 1
fi
if [ "$tag" != "v$version" ]; then
    echo "build_release.sh: note: HEAD is not tagged v$version (git describe: $(git -C "$root" describe --always))" >&2
fi

dir="$root/build/release"
rm -rf "$dir"
"$root/scripts/build.sh" -DPICOCALC_QUEST_UART=OFF "$dir"

name="picocalc-quest-v$version-pico-plus2"
dist="$root/dist"
mkdir -p "$dist"
rm -f "$dist/$name".*
cp "$dir/picocalc-quest.uf2" "$dist/$name.uf2"
cp "$dir/picocalc-quest.elf" "$dist/$name.elf"

# A binary carries its licences: the port's, and upstream's (§5.5).
stage="$dir/zip/$name"
mkdir -p "$stage/upstream"
cp "$dist/$name.uf2" "$root/README.md" "$root/LICENSE" "$stage/"
up="$root/modules/frank-quest"
cp "$up"/COPYING* "$up/COPYRIGHT" "$stage/upstream/"
(cd "$dir/zip" && zip -qr "$dist/$name.zip" "$name")

echo "build_release.sh: v$version ($(git -C "$root" describe --always))"
ls -l "$dist/$name".*
