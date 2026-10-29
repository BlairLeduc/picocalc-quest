#!/usr/bin/env bash
# gate.sh — read a UART log for M4's gate and round trips (design.md §16)
#
#   scripts/gate.sh out/m4-scumm.log          # one log
#   scripts/gate.sh out/m4-*.log              # several, in order
#   GATE=75 SETTLE=5 WINDOW=5 scripts/gate.sh out/m4.log
#
# For each game launched: its heartbeats, core 0's busy share (median,
# 90th percentile, the busiest second, and the busiest WINDOW seconds
# averaged, the gate's figure), frames published a second over that
# window, and how the game ended. The first SETTLE seconds after a launch
# are loading, and are left out of the busiest figures; they are shown
# apart. A title passes when its busiest window is under GATE percent.
#
# Then the heap at each selector entry (main.c's heap line): a round trip
# passes when every entry after the first matches the first. Exits 1 if
# a title fails the gate or the heap moved.
set -euo pipefail

case "${1:-}" in
    -h|--help|"") sed -n '2,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
esac

cat "$@" | tr -d '\r' | awk -v gate="${GATE:-75}" -v settle="${SETTLE:-5}" -v window="${WINDOW:-5}" '
function finish(how,    i, j, k, n, sorted, med, p90, max1, sum, best, bestf, at, lmax) {
    if (title == "") return
    n = 0
    lmax = 0
    for (i = 1; i <= nb; i++) {
        if (i <= settle) { if (busy[i] > lmax) lmax = busy[i]; continue }
        sorted[++n] = busy[i]
    }
    # Insertion sort; a few thousand values at most.
    for (i = 2; i <= n; i++) {
        k = sorted[i]
        for (j = i - 1; j >= 1 && sorted[j] > k; j--) sorted[j + 1] = sorted[j]
        sorted[j + 1] = k
    }
    games++
    if (n < window) {
        printf "%-44s %4ds  too short to judge (%d s after loading)  %s\n", title, nb, n, how
        short++
        title = ""
        return
    }
    med = sorted[int((n + 1) / 2)]
    p90 = sorted[int(n * 0.9 + 0.5) < 1 ? 1 : int(n * 0.9 + 0.5)]
    max1 = sorted[n]
    best = -1
    for (i = settle + 1; i + window - 1 <= nb; i++) {
        sum = 0; k = 0
        for (j = i; j < i + window; j++) { sum += busy[j]; k += frames[j] }
        if (sum / window > best) { best = sum / window; bestf = k / window; at = i }
    }
    verdict = best < gate ? "pass" : "FAIL"
    if (best >= gate) failed++
    printf "%-44s %4ds  busy med %5.1f p90 %5.1f max %5.1f  busiest %ds %5.1f%% at +%ds, %4.1f frames/s  load max %5.1f  %s  %s\n",
           title, nb, med, p90, max1, window, best, at, bestf, lmax, verdict, how
    title = ""
}
/^hb 1:/ { finish("RESTARTED"); boots++ }
/picocalc-quest: exit\(/ { finish("ERROR PAGE"); next }
/FRANK Quest: launching / {
    finish("(no end seen)")
    title = $0
    sub(/.*launching /, "", title)
    sub(/ \(engine=.*/, "", title)
    nb = 0
    delete busy; delete frames
    next
}
/FRANK Quest: game completed cleanly/ { finish("ended cleanly"); next }
/FRANK Quest: game dispatch failed/   { finish("DISPATCH FAILED"); next }
/^hb [0-9]+:/ && title != "" {
    b = $0; sub(/.*core 0 busy /, "", b); sub(/%.*/, "", b)
    f = $0; sub(/.*, frames /, "", f); sub(/,.*/, "", f)
    nb++; busy[nb] = b + 0; frames[nb] = f + 0
    next
}
/picocalc-quest: heap: / {
    h = $0; sub(/.*heap: /, "", h)
    heaps++
    heap[heaps] = h
}
END {
    finish("(log ended in the game)")
    printf "\n%d game(s) launched, %d failed the gate at %d%%, %d too short; %d boot(s)\n",
           games, failed, gate, short, boots
    if (heaps > 0) {
        moved = 0
        for (i = 1; i <= heaps; i++) {
            printf "  selector %2d: %s%s\n", i, heap[i], (i > 1 && heap[i] != heap[1]) ? "  <- moved" : ""
            if (i > 1 && heap[i] != heap[1]) moved++
        }
        printf "heap at %d selector entries: %s\n", heaps, moved ? "MOVED" : "unchanged"
    }
    exit (failed > 0 || moved > 0) ? 1 : 0
}'
