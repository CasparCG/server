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
import re

def read_exact(inp, size, *, eof_allowed=False):
    retval = inp.read(size)
    if eof_allowed and len(retval) == 0:
        return None
    assert len(retval) == size, "truncated read"
    return retval

def read_be(inp, bits, *, signed=False, eof_allowed=False):
    assert bits & 7 == 0, "must be a whole number of bytes"
    bytes_ = read_exact(inp, bits >> 3, eof_allowed=eof_allowed)
    if bytes_ is None:
        return None
    return int.from_bytes(bytes_, "big", signed=signed)

def read_u8(inp, *, eof_allowed=False):
    retval = read_exact(inp, 1, eof_allowed=eof_allowed)
    if retval is None:
        return None
    return retval[0]

def read_u16_be(inp, *, eof_allowed=False):
    return read_be(inp, 16, eof_allowed=eof_allowed)

def read_u32_be(inp, *, eof_allowed=False):
    return read_be(inp, 32, eof_allowed=eof_allowed)

def dump_vanc(line_num, vanc, out):
    if len(vanc) < 2:
        return
    did = vanc[0]
    sdid = vanc[1]
    print(f"Line {line_num}:   DID: {did:02x}; SDID: {sdid:02x}; Data: {vanc[3:].hex(' ')}", file=out)

def dump_vanc_from_mxf_data_stream(inp, out):
    while True:
        line_count = read_u16_be(inp, eof_allowed=True)
        if line_count == None:
            break
        assert line_count == 1
        for _ in range(line_count):
            line_num = read_u16_be(inp)
            wrapping_type = read_u8(inp)
            sample_coding = read_u8(inp)
            assert sample_coding in {4, 5, 6, 10, 11, 12}, "unsupported sample coding"
            sample_count = read_u16_be(inp)
            array_len = read_u32_be(inp)
            array_element_size = read_u32_be(inp)
            assert array_element_size == 1, "array element size must be 1 byte"
            array_bytes = read_exact(inp, array_len)
            dump_vanc(line_num, array_bytes[:sample_count], out)


def dump_vanc_from_mcc(inp, out):
    MAX_LINE_SIZE = 1 << 12
    TRANSLATION_TABLE = {
        ord("G"): "FA0000",
        ord("H"): "FA0000" * 2,
        ord("I"): "FA0000" * 3,
        ord("J"): "FA0000" * 4,
        ord("K"): "FA0000" * 5,
        ord("L"): "FA0000" * 6,
        ord("M"): "FA0000" * 7,
        ord("N"): "FA0000" * 8,
        ord("O"): "FA0000" * 9,
        ord("P"): "FB8080",
        ord("Q"): "FC8080",
        ord("R"): "FD8080",
        ord("S"): "9669",
        ord("T"): "6101",
        ord("U"): "E10000",
        ord("Z"): "00",
    }
    first_line = inp.readline(MAX_LINE_SIZE)
    assert first_line == "File Format=MacCaption_MCC V1.0\n", "file not in MCC format"
    vanc_line_regex = re.compile(
        r"^(?P<hour>[0-9]{2}):(?P<minute>[0-9]{2}):(?P<second>[0-9]{2}):"
        r"(?P<frame>[0-9]{2})\t(?P<encoded>[0-9A-TUZ]*)$",
        re.IGNORECASE,
    )
    buffer = bytearray()
    while True:
        line = inp.readline(MAX_LINE_SIZE)
        if line == "":
            break
        line = line.strip()
        if line == "" or line.startswith("//"):
            continue
        lhs, eq, rhs = line.partition("=")
        if eq != "":
            print(f"Attribute: {lhs!r}={rhs!r}", file=out)
            continue
        m = vanc_line_regex.fullmatch(line)
        assert m, f"unknown line: {line!r}"
        decoded_hex = m['encoded'].upper().translate(TRANSLATION_TABLE)
        buffer.clear()
        for i in range(0, len(decoded_hex), 2):
            buffer.append(int(decoded_hex[i:i + 2], 16))
        dump_vanc(9, bytes(buffer), out)


def main():
    parser = argparse.ArgumentParser()
    inp = parser.add_mutually_exclusive_group(required=True)
    inp.add_argument("--mxf-data-stream", type=argparse.FileType("rb"), metavar="<input.mxf.dat>")
    inp.add_argument("--mcc", type=argparse.FileType("r"), metavar="<input.mcc>")
    parser.add_argument("-o", "--output", type=argparse.FileType("w"), default="-")
    args = parser.parse_args()
    if args.mxf_data_stream:
        dump_vanc_from_mxf_data_stream(
            inp=args.mxf_data_stream,
            out=args.output,
        )
    if args.mcc:
        dump_vanc_from_mcc(
            inp=args.mcc,
            out=args.output,
        )

if __name__ == "__main__":
    main()
