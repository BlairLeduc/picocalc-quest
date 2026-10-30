#!/usr/bin/env python3
"""test_sof_to_sou.py — scripts/sof_to_sou.py on a made-up speech file.

A .sou of three lines, one-block VOCs at SCUMM's rates with lip-sync
bytes, is compressed as compress_scumm_sou lays a .sof out, converted
back, and must come out byte for byte. Then a .sou whose VOCs had two
blocks: its lines must land at the same offsets. Exit 77 (skip) without
the flac command.
"""

import os
import random
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
import sof_to_sou  # noqa: E402

if shutil.which("flac") is None:
    print("flac not installed; skipping")
    sys.exit(77)

rng = random.Random(6)


def line(code, n, ntags, blocks=1):
    """A VCTL block and its VOC, and the samples and rate in it."""
    samples = bytes(128 + int(60 * rng.random()) - 30 for _ in range(n))
    tags = bytes(rng.randrange(256) for _ in range(ntags))
    voc = bytearray(sof_to_sou.VOC_HEADER)
    cut = [0, n // 2, n] if blocks == 2 else [0, n]
    for a, b in zip(cut, cut[1:]):
        body = bytes([code, 0]) + samples[a:b]
        voc += bytes([1]) + len(body).to_bytes(3, "little") + body
    voc += b"\x00"
    rate = 1000000 // (256 - code)
    return b"VCTL" + struct.pack(">I", 8 + ntags) + tags + bytes(voc), tags, samples, rate


def make(lines):
    sou = bytearray(b"SOU \x00\x00\x00\x00")
    index, body = [], bytearray()
    for vctl, tags, samples, rate in lines:
        flac = subprocess.run(
            ["flac", "--force-raw-format", "--endian=little", "--sign=unsigned",
             "--channels=1", "--bps=8", f"--sample-rate={rate}", "-c", "--totally-silent", "-"],
            input=samples, stdout=subprocess.PIPE, check=True).stdout
        index.append((len(sou), len(body), len(tags), len(flac)))
        sou += vctl
        body += tags + flac
    sof = struct.pack(">I", 16 * len(index))
    sof += b"".join(struct.pack(">IIII", *e) for e in index) + body
    return bytes(sou), sof, [e[0] for e in index]


failures = 0
with tempfile.TemporaryDirectory() as d:
    src, dst = os.path.join(d, "monster.sof"), os.path.join(d, "monster.sou")

    sou, sof, _ = make([line(0xD2, 5000, 14), line(0xA6, 300, 0), line(0xD3, 12000, 40)])
    open(src, "wb").write(sof)
    sof_to_sou.convert(src, dst)
    if open(dst, "rb").read() != sou:
        print("FAIL: one-block lines do not come back byte for byte")
        failures += 1

    sou, sof, offsets = make([line(0xD2, 4000, 6, blocks=2), line(0xD2, 900, 2, blocks=2)])
    open(src, "wb").write(sof)
    sof_to_sou.convert(src, dst)
    out = open(dst, "rb").read()
    if any(out[o:o + 4] != b"VCTL" for o in offsets) or len(out) > len(sou):
        print("FAIL: two-block lines are not at their offsets")
        failures += 1

if failures:
    sys.exit(1)
print("test_sof_to_sou: ok")
