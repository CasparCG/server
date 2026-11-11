#!/usr/bin/env python3
"""Show one stress test step on a running CasparCG server (channel 1), to watch it in the preview.

usage:
  tools/stresstest/demo.py <suite> <count> [--port 5250] [--probe]
  tools/stresstest/demo.py clear [--port 5250]
  tools/stresstest/demo.py cmd "<AMCP command>" [--port 5250]

suites: h264, prores, html, gpu, uhd  (same scenes as stresstest.py; uhd plays the 2160p clips,
        on a 1080p channel they are scaled down, so it is heavier than h264 but not a real 2160p50 channel)

The media (stress_*.mp4/.mov/.png, template stress.html) must be in the server's media / template
folder - run stresstest.py once to generate them in build/stresstest and copy them over.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from stresstest import Amcp, setup_step, start_probe_layer  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("suite", choices=["h264", "prores", "html", "gpu", "uhd", "clear", "cmd"])
    parser.add_argument("arg", nargs="?", default="1", help="number of layers / templates, or the AMCP command")
    parser.add_argument("--port", type=int, default=5250)
    parser.add_argument("--probe", action="store_true", help="also show the frame counter strip (layer 1000)")
    args = parser.parse_args()

    amcp = Amcp(args.port)
    if args.suite == "cmd":
        print(amcp.cmd(args.arg))
        return
    amcp.cmd("CLEAR 1")
    if args.suite == "clear":
        print("channel 1 cleared")
        return

    count = int(args.arg)
    setup_step(args.suite, amcp, 0, count)
    if args.probe:
        start_probe_layer(amcp, "stress_uhd" if args.suite == "uhd" else "stress_h264")
    print(f"channel 1: {args.suite} x {count}")


if __name__ == "__main__":
    main()
