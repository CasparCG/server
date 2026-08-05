#!/usr/bin/env python3
# Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
#
# This file is part of CasparCG (www.casparcg.com).
#
# CasparCG is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# CasparCG is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
import argparse
import struct

CAPTION_DATA_SIZE = 3
CAPTION_DATA_PER_LINE = 5

KNOWN_FPS = {
    "23.976": (24000, 1001),
    "24": (24, 1),
    "25": (25, 1),
    "29.97": (30000, 1001),
    "30": (30, 1),
    "50": (50, 1),
    "59.94": (60000, 1001),
    "60": (60, 1),
}


def dump_rcwt(
    inp, out, *, offset_ms=0, round_to_fps=None, start_at_zero=False, time_deltas=False
):
    header = inp.read(11)
    if len(header) != 11 or header[:3] != b"\xCC\xCC\xED":
        raise ValueError("input not in RCWT format")
    if header[6:8] != b"\x00\x01":
        raise ValueError("unsupported RCWT version")
    last_time = 0
    while True:
        header = inp.read(10)
        if len(header) == 0:
            break
        if len(header) != 10:
            raise ValueError("truncated RCWT file")
        time, count = struct.unpack("<qH", header)
        if start_at_zero:
            start_at_zero = False
            offset_ms -= time
        time += offset_ms
        if time_deltas:
            time, last_time = time - last_time, time
        if round_to_fps is not None:
            num, denom = round_to_fps
            frame = (time * num + denom * 500) // (denom * 1000)
            time = (frame * 1000 * denom + num // 2) // num
        buf_len = CAPTION_DATA_SIZE * count
        buf = inp.read(buf_len)
        if len(buf) != buf_len:
            raise ValueError("truncated RCWT file")
        time_div_1000 = time // 1000 if time > 0 else -(-time // 1000)
        line_header = f"{time_div_1000:4}.{abs(time) % 1000:03}:"
        lines = [line_header]
        for offset in range(0, len(buf), CAPTION_DATA_SIZE * CAPTION_DATA_PER_LINE):
            if offset != 0:
                lines.append("\n" + line_header)
            for i in range(
                offset,
                min(len(buf), offset + CAPTION_DATA_SIZE * CAPTION_DATA_PER_LINE),
                CAPTION_DATA_SIZE,
            ):
                lines[-1] += f"  {buf[i]:02x} {buf[i + 1]:02x} {buf[i + 2]:02x}"
        lines[-1] += "\n"
        print("".join(lines), file=out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=argparse.FileType("rb"), metavar="<input.rcwt>")
    parser.add_argument("-o", "--output", type=argparse.FileType("w"), default="-")
    parser.add_argument("--offset-ms", type=int, default=0)
    parser.add_argument("--start-at-zero", action="store_true")
    parser.add_argument("--time-deltas", action="store_true")
    parser.add_argument("--round-to-fps", choices=list(KNOWN_FPS.keys()))
    args = parser.parse_args()
    round_to_fps = None
    if args.round_to_fps is not None:
        round_to_fps = KNOWN_FPS[args.round_to_fps]
    dump_rcwt(
        inp=args.input,
        out=args.output,
        offset_ms=args.offset_ms,
        round_to_fps=round_to_fps,
        start_at_zero=args.start_at_zero,
        time_deltas=args.time_deltas,
    )


if __name__ == "__main__":
    main()
