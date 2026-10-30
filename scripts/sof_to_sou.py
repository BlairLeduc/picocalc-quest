#!/usr/bin/env python3
"""sof_to_sou.py — a SCUMM speech file compressed with FLAC (monster.sof,
as ScummVM's compress_scumm_sou writes it) back to the uncompressed
monster.sou the firmware can play: it has no FLAC, Vorbis or MP3 decoder.

    scripts/sof_to_sou.py monster.sof monster.sou

Needs the flac command (brew install flac). FLAC is lossless, so each
line of speech comes back sample for sample.

The .sou is "SOU " and four zero bytes, then for each line a VCTL block
(its size, then the lip-sync times) followed by a Creative Voice File of
8-bit unsigned samples. The game's scripts name a line by the VCTL's
offset in the .sou, so each goes back at exactly that offset. The .sof
keeps those offsets in its index (engines/scumm/sound.cpp, setupSfxFile):
a big-endian length, then per line the original offset, the offset of
its data after the index, the lip-sync bytes' count and the compressed
size; each line's data is its lip-sync bytes, then a FLAC stream.

The original VOC files may have used several blocks; each comes back as
one, which is never longer, and any gap left before the next line is
zeros the engine never reads (a VOC ends at its terminator).
"""

import struct
import subprocess
import sys

VOC_HEADER = b"Creative Voice File\x1a" + struct.pack("<HHH", 0x1A, 0x010A, 0x1129)


def voc(rate, samples):
    """One block of 8-bit unsigned mono at `rate`, and the terminator."""
    code = round(256 - 1000000 / rate)
    if not 0 <= code <= 255:
        raise ValueError(f"sample rate {rate} has no VOC code")
    body = bytes([code, 0]) + samples
    out = bytearray(VOC_HEADER)
    if len(body) < 1 << 24:
        out += bytes([1]) + len(body).to_bytes(3, "little") + body
    else:
        raise ValueError("a line longer than a VOC block holds")
    out += b"\x00"
    return bytes(out)


def decode_flac(data):
    """FLAC bytes to (rate, 8-bit unsigned mono samples). The rate and
    sample size come from the STREAMINFO block, which leads the stream;
    the samples are decoded raw, since a stream encoded to a pipe has no
    sample count for a WAV header."""
    if data[:4] != b"fLaC" or data[4] & 0x7F != 0:
        raise ValueError("not a FLAC stream")
    info = int.from_bytes(data[18:21] + data[21:22], "big")
    rate = info >> 12
    channels = ((info >> 9) & 0x7) + 1
    bits = ((info >> 4) & 0x1F) + 1
    if channels != 1:
        raise ValueError(f"{channels} channels; SCUMM speech is mono")
    if bits not in (8, 16):
        raise ValueError(f"{bits}-bit samples")
    r = subprocess.run(["flac", "-d", "-c", "--totally-silent", "--force-raw-format",
                        "--endian=little", "--sign=" + ("unsigned" if bits == 8 else "signed"),
                        "-"], input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        raise RuntimeError("flac could not decode a line: " + r.stderr.decode(errors="replace"))
    frames = r.stdout
    if bits == 16:
        # Not what compress_scumm_sou writes, but keep the top byte.
        frames = bytes(((s >> 8) + 128) & 0xFF for (s,) in struct.iter_unpack("<h", frames))
    return rate, frames


def convert(src, dst):
    with open(src, "rb") as f:
        data = f.read()
    (index_len,) = struct.unpack_from(">I", data, 0)
    if index_len % 16 or index_len + 4 > len(data):
        sys.exit(f"{src}: not a compressed SCUMM speech file (index length {index_len})")
    entries = sorted(struct.unpack_from(">IIII", data, 4 + i * 16)
                     for i in range(index_len // 16))

    out = bytearray(b"SOU \x00\x00\x00\x00")
    for n, (org, new, ntags, size) in enumerate(entries):
        start = new + index_len + 4
        tags = data[start:start + ntags]
        stream = data[start + ntags:start + ntags + size]
        if len(tags) != ntags or len(stream) != size:
            sys.exit(f"{src}: line {n} runs past the end of the file")
        if len(out) > org:
            sys.exit(f"line {n} at {org} overlaps the one before, which ends at {len(out)}")
        out += bytes(org - len(out))
        rate, samples = decode_flac(stream)
        out += b"VCTL" + struct.pack(">I", 8 + ntags) + tags + voc(rate, samples)
        if n % 200 == 0 or n == len(entries) - 1:
            print(f"\r{n + 1}/{len(entries)} lines", end="", file=sys.stderr, flush=True)
    print(file=sys.stderr)

    with open(dst, "wb") as f:
        f.write(out)
    print(f"{dst}: {len(entries)} lines, {len(out)} bytes")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] in ("-h", "--help"):
        sys.exit(__doc__.split("\n\n")[1].strip())
    convert(sys.argv[1], sys.argv[2])
