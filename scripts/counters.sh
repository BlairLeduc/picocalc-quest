#!/usr/bin/env bash
# counters.sh — read the heartbeat's counter block over SWD, with both
# cores running (design.md §14.2, picocalc/counters.h; hardware-notes.md
# §2.7). It is how the build that ships, which has no UART, is watched.
#
#   scripts/counters.sh                          # one read, build/pico-release's image
#   scripts/counters.sh path/to/image.elf        # the image that is running
#   scripts/counters.sh --watch 30               # M9's soak: read every 60 s for 30 min, then check
#   WATCH_EVERY=10 scripts/counters.sh --watch 2
#   scripts/counters.sh --check out/counters.log # check a watch's log only
#
# The block is found by its symbol in the image, so the image must be the
# one running. OpenOCD starts with `init` alone: no reset, no halt, so
# the audio plays on. A watch keeps one OpenOCD for the whole run and
# writes a line a read to out/counters-<time>.log.
#
# The check passes when every read finds the block; its seconds never go
# back (no reboot) and keep pace with the wall clock; frames were
# published between every two reads; keys were pressed during the run;
# and late refills, I2C errors and lost key events are zero at the end;
# over at least the minutes asked for. Exits 1 if it does not.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
mode=read
minutes=0
image=
log=
while [ $# -gt 0 ]; do
    case "$1" in
        --watch) mode=watch; minutes="$2"; shift 2 ;;
        --check) mode=check; log="$2"; shift 2 ;;
        -h|--help) sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        -*) echo "counters.sh: unknown option $1" >&2; exit 2 ;;
        *) image="$1"; shift ;;
    esac
done
image="${image:-$root/build/pico-release/picocalc-quest.elf}"
every="${WATCH_EVERY:-60}"

# The block's 32-bit words, in counters.h's order; then the version's 8.
check() {
    python3 -I - "$1" "$2" <<'EOF'
import sys

FIELDS = ["magic", "layout", "seconds", "busy", "busy_max", "audio", "frames",
          "presents", "dropped", "late", "mix_max_us", "keys", "ring_full",
          "unknown", "stray", "i2c_errors", "log_dropped", "core1_stack",
          "temp_c", "battery", "loader", "image_kb", "partition_kb"]
MAGIC, LAYOUT = 0x51434351, 1

def parse(line):
    parts = line.split()
    ms, words = int(parts[0]), [int(w, 0) for w in parts[1:]]
    if len(words) != len(FIELDS) + 8:
        return ms, None
    c = dict(zip(FIELDS, words))
    for k in ("temp_c", "battery"):
        if c[k] >= 1 << 31: c[k] -= 1 << 32
    raw = b"".join(w.to_bytes(4, "little") for w in words[len(FIELDS):])
    c["version"] = raw.split(b"\0")[0].decode("ascii", "replace")
    return ms, c

def show(c):
    bat = c["battery"]
    battery = "unknown" if bat < 0 else "%d%%%s" % (bat & 0x7F, " charging" if bat & 0x80 else "")
    pm = lambda v: "%d.%d%%" % (v // 10, v % 10)
    return ("%s, up %d s: core 0 busy %s (max %s), audio %s, mixer max %d us, late %d; "
            "frames %d, presented %d, dropped %d; keys %d, ring full %d, unknown %d, stray %d; "
            "i2c errors %d, log dropped %d, core 1 stack %d B; temp %d C, battery %s; "
            "%s, image %d KB, partition %s" % (
            c["version"], c["seconds"], pm(c["busy"]), pm(c["busy_max"]), pm(c["audio"]),
            c["mix_max_us"], c["late"], c["frames"], c["presents"], c["dropped"], c["keys"],
            c["ring_full"], c["unknown"], c["stray"], c["i2c_errors"], c["log_dropped"],
            c["core1_stack"], c["temp_c"], battery,
            "from UF2 Loader" if c["loader"] else "not from UF2 Loader", c["image_kb"],
            "%d KB" % c["partition_kb"] if c["partition_kb"] else "none"))

path, minutes = sys.argv[1], float(sys.argv[2])
reads, problems = [], []
for n, line in enumerate(open(path), 1):
    line = line.strip()
    if not line:
        continue
    ms, c = parse(line)
    if c is None or c["magic"] != MAGIC:
        problems.append("read %d found no counter block" % n)
        continue
    if c["layout"] != LAYOUT:
        problems.append("read %d is layout %d, not %d" % (n, c["layout"], LAYOUT))
        continue
    reads.append((ms, c))

if not reads:
    print("counters.sh: no reads")
    sys.exit(1)
if minutes == 0:                       # one read: show it
    print(show(reads[-1][1]))
    sys.exit(1 if problems else 0)

for (ms0, a), (ms1, b) in zip(reads, reads[1:]):
    wall = (ms1 - ms0) / 1000.0
    up = b["seconds"] - a["seconds"]
    if up < 0:
        problems.append("a reboot between %d s and %d s of uptime" % (a["seconds"], b["seconds"]))
    elif abs(up - wall) > max(3.0, wall * 0.05):
        problems.append("%d heartbeats in %.0f s at %d s of uptime" % (up, wall, a["seconds"]))
    if b["frames"] == a["frames"]:
        problems.append("no frames published from %d s to %d s" % (a["seconds"], b["seconds"]))
first, last = reads[0][1], reads[-1][1]
span = (reads[-1][0] - reads[0][0]) / 60000.0
if span < minutes - 0.5:
    problems.append("%.1f minutes read, not %g" % (span, minutes))
if last["keys"] == first["keys"]:
    problems.append("no keys pressed")
for k in ("late", "i2c_errors", "ring_full"):
    if last[k]:
        problems.append("%s %d" % (k.replace("_", " "), last[k]))

print("first: " + show(first))
print("last:  " + show(last))
print("%d reads over %.1f minutes" % (len(reads), span))
for p in problems[:20]:
    print("  FAIL: " + p)
print("counters.sh: %s" % ("FAILED" if problems else "passed"))
sys.exit(1 if problems else 0)
EOF
}

if [ "$mode" = check ]; then
    check "$log" "${SOAK_MIN:-30}"
    exit
fi

if [ ! -f "$image" ]; then
    echo "counters.sh: no image at $image" >&2
    exit 1
fi

# OpenOCD and the toolchain's nm, found as profile.sh finds them.
openocd="${OPENOCD:-}"
if [ -z "$openocd" ]; then
    if command -v openocd >/dev/null 2>&1; then
        openocd="$(command -v openocd)"
    else
        openocd="$(ls -d "$HOME"/.pico-sdk/openocd/*/openocd 2>/dev/null | sort -V | tail -n1 || true)"
    fi
fi
if [ -z "$openocd" ] || [ ! -x "$openocd" ]; then
    echo "counters.sh: OpenOCD not found; install the Raspberry Pi fork or set OPENOCD" >&2
    exit 1
fi
scripts=()
if [ -d "$(dirname "$openocd")/scripts" ]; then
    scripts=(-s "$(dirname "$openocd")/scripts")
fi
nm="$(command -v arm-none-eabi-nm || ls -d "$HOME"/.pico-sdk/toolchain/*/bin/arm-none-eabi-nm 2>/dev/null | sort -V | tail -n1 || true)"
if [ -z "$nm" ]; then
    echo "counters.sh: arm-none-eabi-nm not found" >&2
    exit 1
fi

addr="$("$nm" "$image" | awk '$3 == "picocalc_quest_counters" { print "0x" $1 }')"
if [ -z "$addr" ]; then
    echo "counters.sh: $image has no picocalc_quest_counters" >&2
    exit 1
fi
words=31                                   # 23 fields and the version's 8

mkdir -p "$root/out"
if [ "$mode" = watch ]; then
    log="$root/out/counters-$(date +%Y%m%d-%H%M%S).log"
    reads=$(( minutes * 60 / every + 1 ))
    echo "counters.sh: $reads reads, every ${every}s, at $addr -> $log" >&2
else
    log="$root/out/counters-last.log"
    reads=1
fi
: > "$log"

# Each read is a line: the wall clock in ms, then the words.
"$openocd" ${scripts[@]+"${scripts[@]}"} \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed ${SWD_KHZ:-5000}" \
    -c "gdb_port disabled" -c "tcl_port disabled" -c "telnet_port disabled" \
    -c "init" \
    -c "for {set i 0} {\$i < $reads} {incr i} {
            if {\$i > 0} { sleep [expr {$every * 1000}] }
            set f [open {$log} a]
            puts \$f \"[clock milliseconds] [read_memory $addr 32 $words]\"
            close \$f
        }" \
    -c "shutdown" 2>"$log.openocd" || {
    echo "counters.sh: OpenOCD failed; its output is in $log.openocd" >&2
    tail -n 5 "$log.openocd" >&2
    exit 1
}

check "$log" "$minutes"
