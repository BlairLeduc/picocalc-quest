#!/usr/bin/env bash
# soak.sh — M8's soak: one title played for a while with keys typed over
# UART0, and the log checked (design.md §16, EL §11.5).
#
#   scripts/soak.sh 12                    # Space Quest IV, 30 min, then check
#   SOAK_MIN=5 scripts/soak.sh 9          # Kyrandia for 5 min
#   scripts/soak.sh --check out/soak.log  # check a log only (SOAK_MIN too)
#
# INDEX is the title's place in the selector, from 0 at the top. The log
# goes to out/soak-<time>.log, or SOAK_LOG. Start it, then switch the
# PicoCalc on (on battery, for the real soak): it waits up to 60 s for
# the banner (SOAK_WAIT), skips the welcome, launches the title, and for SOAK_MIN
# minutes holds the arrows in turn and clicks with Enter now and then.
#
# The check passes when the log shows one boot; from it, a heartbeat
# every second from the first to the last, none missing; late refills,
# I2C errors and dropped log lines zero at the end; the one title
# launched and still playing at the end, with no failed start, error
# page, quit or second launch; at least SOAK_MIN minutes of play; and
# frames published in every whole minute of it. Exits 1 if it does not.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"

check() {
    LC_ALL=C tr -d '\r' < "$1" | LC_ALL=C awk -v want="$minutes" '
    # From the first boot on: a log started while the board ran begins
    # with the end of that run.
    /^picocalc-quest [0-9a-f]+(-dirty)? \(/ { boots++; first = ""; gaps = 0; gapat = ""; beats = 0 }
    boots == 0 { next }
    /launching .* \(engine=/ {
        if (launches++) left = left "; a second launch"
        playing = 1; sub(/.*launching /, ""); sub(/ \(engine=.*/, ""); title = $0
    }
    /game dispatch failed/                 { playing = 0; left = left "; the start failed" }
    /exit\([0-9-]+\); the error page/       { playing = 0; left = left "; the error page" }
    /requesting engine quit|tearing down engine state/ { if (playing) left = left "; the game quit"; playing = 0 }
    /^hb [0-9]+: / {
        n = $2 + 0
        if (first == "") first = n
        else if (n != last + 1) { gaps++; if (gapat == "") gapat = last " to " n }
        last = n; beats++
        if (match($0, /late [0-9]+/))         late = substr($0, RSTART + 5, RLENGTH - 5) + 0
        if (match($0, /i2c errors [0-9]+/))   i2c = substr($0, RSTART + 11, RLENGTH - 11) + 0
        if (match($0, /log dropped [0-9]+/))  dropped = substr($0, RSTART + 12, RLENGTH - 12) + 0
        if (playing && match($0, /frames [0-9]+/)) {
            m = int(played / 60); frames[m] += substr($0, RSTART + 7, RLENGTH - 7) + 0; played++
        }
    }
    END {
        whole = int(played / 60); still = 0
        for (m = 0; m < whole; m++) if (frames[m] == 0) still++   # a part minute at the end is not judged
        printf "title %s; %d boot(s); heartbeats %d (hb %s to %d), %d gap(s)%s\n",
               (title == "" ? "(none)" : title), boots, beats, first, last, gaps, (gapat == "" ? "" : ", first at " gapat)
        printf "late refills %d, i2c errors %d, log dropped %d; %d s of play of %d wanted, %d whole minute(s) without a frame%s\n",
               late, i2c, dropped, played, want * 60, still, (left == "" ? "" : "; left the game:" substr(left, 2))
        ok = boots == 1 && beats > 0 && gaps == 0 && late == 0 && i2c == 0 && dropped == 0 &&
             launches == 1 && left == "" && played >= want * 60 && still == 0
        print (ok ? "PASS" : "FAIL")
        exit (ok ? 0 : 1)
    }'
}

minutes="${SOAK_MIN:-30}"

case "${1:-}" in
    -h|--help|"") sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    --check) check "$2"; exit ;;
esac

index="$1"
log="${SOAK_LOG:-$root/out/soak-$(date +%Y%m%d-%H%M%S).log}"
keys="$root/scripts/keys.sh"

"$root/scripts/capture.sh" 0 "$log" >/dev/null 2>&1 &
cap=$!
trap 'kill $cap 2>/dev/null || true' EXIT
echo "soak.sh: logging to $log; switch the PicoCalc on"

wait_for() {   # PATTERN SECONDS
    local end=$((SECONDS + $2))
    until LC_ALL=C grep -aq "$1" "$log" 2>/dev/null; do
        [ $SECONDS -lt $end ] || { echo "soak.sh: no \"$1\" in $2 s" >&2; return 1; }
        sleep 0.5
    done
}
wait_for '^hb 3:' "${SOAK_WAIT:-60}"
for try in 1 2 3 4; do   # the welcome: a key the first moments can lose
    "$keys" space
    wait_for 'Selector: found' 5 2>/dev/null && break
done
sleep 2
downs=()
for ((n = 0; n < index; n++)); do downs+=(down); done
"$keys" home ${downs[@]+"${downs[@]}"} wait:0.5 enter
wait_for 'launching' 10
echo "soak.sh: $(LC_ALL=C grep -a -m1 'launching' "$log" | sed 's/.*launching //; s/ (engine=.*//') for $minutes min"

end=$((SECONDS + minutes * 60))
while [ $SECONDS -lt $end ]; do
    "$keys" hold:right 0.8 hold:down 0.5 enter wait:2 hold:left 0.8 hold:up 0.5 enter wait:2
done
sleep 3
kill $cap 2>/dev/null || true
trap - EXIT
check "$log"
