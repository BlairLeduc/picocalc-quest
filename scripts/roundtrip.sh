#!/usr/bin/env bash
# roundtrip.sh — M4's selector round trips over the UART (design.md §16):
# launch a game, let it run, quit with Ctrl+Alt+Del, and launch another,
# N times. Development build.
#
#   scripts/roundtrip.sh                  # 10 trips, 20 s in each game
#   scripts/roundtrip.sh 10 30            # 10 trips, 30 s in each
#   BACK=8 scripts/roundtrip.sh 10        # seconds from the quit to the selector
#
# Start with the selector on the panel and scripts/capture.sh running;
# then read the log with scripts/gate.sh, which checks the heap at each
# selector entry. Trips alternate Down and Up before Enter, so they
# alternate between two neighbouring games in the list.
set -euo pipefail

case "${1:-}" in
    -h|--help) sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
esac

here="$(cd "$(dirname "$0")" && pwd)"
trips="${1:-10}"
play="${2:-20}"
back="${BACK:-5}"

for ((i = 1; i <= trips; i++)); do
    if ((i % 2)); then move=down; else move=up; fi
    echo "roundtrip.sh: trip $i of $trips: $move, enter, ${play}s, ctrl+alt+del" >&2
    "$here/keys.sh" "$move" enter
    sleep "$play"
    "$here/keys.sh" ctrl+alt+del
    sleep "$back"
done
echo "roundtrip.sh: done" >&2
