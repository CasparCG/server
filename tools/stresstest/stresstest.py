#!/usr/bin/env python3
"""CasparCG hardware stress test (macOS).

Starts a dedicated CasparCG instance, ramps up load step by step and measures whether the
output keeps real time. The output is received over NDI and every test clip carries a burned-in
frame counter, so dropped, duplicated and late frames are detected exactly.

Test suites (each in a fresh server instance):
  h264    1080p50 H.264 clips, one more layer per step (CPU decode)
  prores  1080p50 ProRes 422 HQ clips (CPU decode, high bitrate)
  html    animated HTML templates (canvas + CSS), one more per step (CEF)
  gpu     full-frame image layers with rotation / opacity / blend modes (mixer, GPU only)
  uhd     2160p50 channel with H.264 2160p50 clips (optional, --uhd)

A step passes when the output runs at the nominal frame rate with no duplicated / skipped
frames and no late frames. The report lists the highest passing step of each suite.

usage: tools/stresstest/stresstest.py [--suites h264,prores,html,gpu] [--uhd] [--seconds 7]
"""

import argparse
import datetime
import json
import math
import os
import platform
import shutil
import signal
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
APP = os.path.join(ROOT, "build", "CasparCG.app")
EXE = os.path.join(APP, "Contents", "MacOS", "casparcg")
NDI_LIB_CANDIDATES = [
    os.path.join(APP, "Contents", "Frameworks", "libndi.dylib"),
    "/usr/local/lib/libndi.dylib",
    "/Library/NDI SDK for Apple/lib/macOS/libndi.dylib",
]

CLIP_FRAMES = 250  # 5 s at 50 fps
NDI_NAME = "casparcg-stress"

STEPS = {
    "h264": [1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32],
    "prores": [1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32],
    "html": [1, 2, 3, 4, 6, 8, 10, 12, 16],
    "gpu": [8, 16, 32, 64, 96, 128, 192, 256],
    "uhd": [1, 2, 3, 4, 6, 8],
}


# --------------------------------------------------------------------------------------------
# helpers


def log(msg):
    print(msg, flush=True)


def run(*args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, text=True, **kwargs).stdout


class Amcp:
    def __init__(self, port):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=10)
        self.buf = b""

    def cmd(self, line):
        self.sock.sendall((line + "\r\n").encode())
        # Responses: "2xx ..." single line, or "200/201 ..." followed by data
        while b"\r\n" not in self.buf:
            self.buf += self.sock.recv(65536)
        first, self.buf = self.buf.split(b"\r\n", 1)
        status = first.decode(errors="replace")
        if status.startswith("201"):
            while b"\r\n" not in self.buf:
                self.buf += self.sock.recv(65536)
            _, self.buf = self.buf.split(b"\r\n", 1)
        elif status.startswith("200"):
            while b"\r\n\r\n" not in self.buf:
                self.buf += self.sock.recv(65536)
            _, self.buf = self.buf.split(b"\r\n\r\n", 1)
        if not status[:1] == "2":
            raise RuntimeError(f"AMCP '{line}' failed: {status}")
        return status

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def gpu_utilization():
    try:
        out = run("ioreg", "-r", "-d", "1", "-w", "0", "-c", "IOAccelerator")
        for token in out.replace(",", "\n").splitlines():
            if '"Device Utilization %"' in token:
                return int(token.split("=")[1].strip(" }"))
    except Exception:
        pass
    return None


def cpu_utilization(pids):
    """Summed %CPU of the given processes (100 = one core)."""
    try:
        out = run("ps", "-o", "%cpu=", "-p", ",".join(str(p) for p in pids))
        return sum(float(x) for x in out.split())
    except Exception:
        return None


def process_tree(pid):
    pids = [pid]
    try:
        out = run("pgrep", "-P", str(pid))
        pids += [int(x) for x in out.split()]
    except subprocess.CalledProcessError:
        pass
    return pids


# --------------------------------------------------------------------------------------------
# media


def make_media(media, template, uhd):
    os.makedirs(media, exist_ok=True)
    os.makedirs(template, exist_ok=True)

    def counter_clip(path, size, codec_args, rate=50):
        if os.path.exists(path):
            return
        w, h = size
        scale = w / 1920
        block = int(40 * scale)
        log(f"  generating {os.path.basename(path)} ...")
        run(
            "ffmpeg", "-v", "error", "-y",
            "-f", "lavfi", "-i", f"testsrc2=size={w}x{h}:rate={rate}:duration={CLIP_FRAMES / rate}",
            "-f", "lavfi", "-i",
            f"color=black:size={16 * block}x{block}:rate={rate}:duration={CLIP_FRAMES / rate},format=gray,"
            f"geq=lum='255*mod(floor(N/pow(2\\,floor(X/{block})))\\,2)'",
            "-f", "lavfi", "-i", f"sine=frequency=440:duration={CLIP_FRAMES / rate}",
            "-filter_complex",
            f"[0:v]drawbox=x='mod(t*{int(480 * scale)}\\,{w})':y=0:w={int(24 * scale)}:h={h}:color=white:t=fill[bg];"
            "[1:v]format=yuv420p[code];[bg][code]overlay=0:0[v]",
            "-map", "[v]", "-map", "2:a", *codec_args, path,
        )

    counter_clip(os.path.join(media, "stress_h264.mp4"), (1920, 1080),
                 ["-c:v", "libx264", "-preset", "fast", "-crf", "16", "-pix_fmt", "yuv420p", "-g", "50", "-c:a", "aac"])
    counter_clip(os.path.join(media, "stress_prores.mov"), (1920, 1080),
                 ["-c:v", "prores_ks", "-profile:v", "3", "-pix_fmt", "yuv422p10le", "-c:a", "pcm_s24le"])
    if uhd:
        counter_clip(os.path.join(media, "stress_uhd.mp4"), (3840, 2160),
                     ["-c:v", "libx264", "-preset", "fast", "-crf", "18", "-pix_fmt", "yuv420p", "-g", "50",
                      "-c:a", "aac"])

    still = os.path.join(media, "stress_still.png")
    if not os.path.exists(still):
        run("ffmpeg", "-v", "error", "-y", "-f", "lavfi", "-i", "testsrc2=size=1920x1080", "-frames:v", "1", still)

    with open(os.path.join(template, "stress.html"), "w") as f:
        f.write(STRESS_HTML)


STRESS_HTML = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><style>
html,body{margin:0;width:1920px;height:1080px;overflow:hidden;background:transparent;font-family:Helvetica,Arial}
canvas{position:absolute;left:0;top:0}
.ticker{position:absolute;left:0;bottom:60px;width:1920px;height:80px;background:rgba(10,30,90,.85);color:#fff;
  font-size:48px;line-height:80px;white-space:nowrap;overflow:hidden}
.ticker span{display:inline-block;padding-left:100%;animation:scroll 12s linear infinite}
@keyframes scroll{from{transform:translateX(0)}to{transform:translateX(-100%)}}
.tile{position:absolute;width:120px;height:120px;border-radius:20px;opacity:.85;animation:spin 3s linear infinite}
@keyframes spin{from{transform:rotate(0) scale(1)}50%{transform:rotate(180deg) scale(1.3)}to{transform:rotate(360deg) scale(1)}}
</style></head><body>
<canvas id="c" width="1920" height="1080"></canvas>
<div class="ticker"><span>CasparCG stress test - animated HTML template with canvas particles, CSS animations
and a scrolling ticker - CasparCG stress test - animated HTML template</span></div>
<script>
for (let i = 0; i < 24; i++) { const d = document.createElement('div'); d.className = 'tile';
  d.style.left = (80 + (i % 12) * 150) + 'px'; d.style.top = (120 + Math.floor(i / 12) * 180) + 'px';
  d.style.background = 'hsl(' + (i * 15) + ',80%,55%)'; d.style.animationDelay = (-i * 0.1) + 's';
  document.body.appendChild(d); }
const c = document.getElementById('c'), g = c.getContext('2d'); const P = [];
for (let i = 0; i < 1500; i++) P.push({x: Math.random() * 1920, y: Math.random() * 1080,
  vx: (Math.random() - .5) * 8, vy: (Math.random() - .5) * 8, r: 2 + Math.random() * 6, h: Math.random() * 360});
function frame() { g.clearRect(0, 0, 1920, 1080);
  for (const p of P) { p.x += p.vx; p.y += p.vy; if (p.x < 0 || p.x > 1920) p.vx *= -1; if (p.y < 0 || p.y > 1080) p.vy *= -1;
    g.fillStyle = 'hsla(' + p.h + ',90%,60%,.7)'; g.beginPath(); g.arc(p.x, p.y, p.r, 0, 6.283); g.fill(); }
  requestAnimationFrame(frame); }
requestAnimationFrame(frame);
function update(){} function play(){} function stop(){}
</script></body></html>
"""


# --------------------------------------------------------------------------------------------
# server


CONFIG = """<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <log-level>info</log-level>
    <accelerator>vulkan</accelerator>
    <paths>
        <media-path>media/</media-path>
        <log-path disable="false">log/</log-path>
        <data-path>data/</data-path>
        <template-path>template/</template-path>
    </paths>
    <channels>
        <channel>
            <video-mode>{mode}</video-mode>
            <consumers>
                <ndi><name>{ndi_name}</name></ndi>
            </consumers>
        </channel>
    </channels>
    <controllers>
        <tcp><port>{port}</port><protocol>AMCP</protocol></tcp>
    </controllers>
</configuration>
"""


class Server:
    def __init__(self, workdir, port, mode, ndi_name):
        self.workdir = workdir
        self.port = port
        self.config = os.path.join(workdir, f"stress-{mode}.config")
        with open(self.config, "w") as f:
            f.write(CONFIG.format(mode=mode, ndi_name=ndi_name, port=port))
        self.proc = None

    def __enter__(self):
        if self._port_open():
            raise RuntimeError(f"port {self.port} is already in use")
        self.out = open(os.path.join(self.workdir, "server.out"), "w")
        self.proc = subprocess.Popen([EXE, os.path.basename(self.config)], cwd=self.workdir, stdout=self.out,
                                     stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        for _ in range(120):
            if self._port_open():
                break
            if self.proc.poll() is not None:
                raise RuntimeError("server exited during startup, see server.out")
            time.sleep(0.25)
        else:
            raise RuntimeError("server did not open the AMCP port")
        self.amcp = Amcp(self.port)
        return self

    def __exit__(self, *exc):
        try:
            self.amcp.cmd("KILL")
        except Exception:
            pass
        self.amcp.close()
        try:
            self.proc.wait(timeout=20)
        except subprocess.TimeoutExpired:
            self.proc.send_signal(signal.SIGKILL)
        self.out.close()

    def _port_open(self):
        try:
            socket.create_connection(("127.0.0.1", self.port), timeout=0.2).close()
            return True
        except OSError:
            return False

    def pids(self):
        return process_tree(self.proc.pid)


# --------------------------------------------------------------------------------------------
# measurement


def measure(probe, ndi_lib, ndi_name, seconds, server, uhd=False):
    samples_cpu, samples_gpu = [], []
    proc = subprocess.Popen([probe, ndi_lib, ndi_name, str(seconds), str(CLIP_FRAMES)], stdout=subprocess.PIPE,
                            text=True)
    time.sleep(1.5)
    while proc.poll() is None:
        cpu = cpu_utilization(server.pids())
        gpu = gpu_utilization()
        if cpu is not None:
            samples_cpu.append(cpu)
        if gpu is not None:
            samples_gpu.append(gpu)
        time.sleep(1)
    out = proc.stdout.read().strip().splitlines()
    result = json.loads(out[-1]) if out else {"error": "no probe output"}
    result["cpu"] = round(sum(samples_cpu) / len(samples_cpu)) if samples_cpu else None
    result["gpu"] = round(sum(samples_gpu) / len(samples_gpu)) if samples_gpu else None
    nominal_fps = 1000.0 / result.get("nominal_ms", 20.0)
    result["ok"] = (
        "error" not in result
        and result["fps"] >= nominal_fps * 0.99
        and result["duplicated"] == 0
        and result["skipped"] == 0
        and result["late"] == 0
    )
    return result


def grid(n):
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)
    return cols, rows


PROBE_LAYER = 1000


def start_probe_layer(amcp, clip):
    # Topmost layer plays the counter clip, cropped to the counter strip only
    amcp.cmd(f"PLAY 1-{PROBE_LAYER} {clip} LOOP")
    amcp.cmd(f"MIXER 1-{PROBE_LAYER} CROP 0 0 0.3334 0.0371")


def setup_step(suite, amcp, previous, count):
    """Add layers previous+1 .. count for this suite."""
    for i in range(previous + 1, count + 1):
        layer = 10 + i
        if suite in ("h264", "prores", "uhd"):
            clip = {"h264": "stress_h264", "prores": "stress_prores", "uhd": "stress_uhd"}[suite]
            amcp.cmd(f"PLAY 1-{layer} {clip} LOOP SEEK {(i * 37) % CLIP_FRAMES}")
        elif suite == "html":
            amcp.cmd(f"CG 1-{layer} ADD 1 stress 1")
        elif suite == "gpu":
            amcp.cmd(f"PLAY 1-{layer} stress_still")
            amcp.cmd(f"MIXER 1-{layer} ANCHOR 0.5 0.5")
            amcp.cmd(f"MIXER 1-{layer} FILL 0.5 0.5 1 1")
            amcp.cmd(f"MIXER 1-{layer} ROTATION {(i * 7) % 360}")
            amcp.cmd(f"MIXER 1-{layer} OPACITY 0.5")
            amcp.cmd(f"MIXER 1-{layer} BLEND {['normal', 'screen', 'overlay', 'multiply'][i % 4]}")

    if suite in ("h264", "prores", "uhd", "html"):
        # Lay all layers out in a grid (multiviewer style)
        cols, rows = grid(count)
        for i in range(1, count + 1):
            layer = 10 + i
            x, y = (i - 1) % cols, (i - 1) // cols
            amcp.cmd(f"MIXER 1-{layer} FILL {x / cols:.4f} {y / rows:.4f} {1 / cols:.4f} {1 / rows:.4f}")


def run_suite(suite, args, probe, ndi_lib, workdir):
    mode = "2160p5000" if suite == "uhd" else "1080p5000"
    probe_clip = "stress_uhd" if suite == "uhd" else "stress_h264"
    ndi_name = f"{NDI_NAME}-{suite}-{os.getpid()}"
    results = []
    log(f"\n=== {suite} ({mode}) ===")
    with Server(workdir, args.port, mode, ndi_name) as server:
        amcp = server.amcp
        start_probe_layer(amcp, probe_clip)
        time.sleep(2)
        baseline = measure(probe, ndi_lib, ndi_name, args.seconds, server)
        log(format_row("baseline", baseline))
        previous = 0
        for count in STEPS[suite]:
            if args.max and count > args.max:
                break
            setup_step(suite, amcp, previous, count)
            previous = count
            time.sleep(args.settle + (2 if suite == "html" else 0))
            result = measure(probe, ndi_lib, ndi_name, args.seconds, server)
            result["step"] = count
            results.append(result)
            log(format_row(f"{count} {UNITS[suite]}", result))
            # Stop after two failing steps in a row (a single failure may be a one-off hiccup)
            if len(results) >= 2 and not results[-1]["ok"] and not results[-2]["ok"]:
                break
    return baseline, results


UNITS = {"h264": "layers", "prores": "layers", "html": "templates", "gpu": "layers", "uhd": "layers"}


def format_row(label, r):
    if "error" in r:
        return f"  {label:<14} ERROR {r['error']}"
    status = "OK  " if r.get("ok", True) else "FAIL"
    return (f"  {label:<14} {status} fps={r['fps']:6.2f} late={r['late']:<3} dup={r['duplicated']:<3} "
            f"skip={r['skipped']:<3} max={r['interval_max_ms']:6.1f}ms cpu={r['cpu']}% gpu={r['gpu']}%")


# --------------------------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--suites", default="h264,prores,html,gpu")
    parser.add_argument("--uhd", action="store_true", help="also run the 2160p50 suite")
    parser.add_argument("--seconds", type=int, default=7, help="measurement time per step")
    parser.add_argument("--settle", type=float, default=3, help="settle time after adding load")
    parser.add_argument("--max", type=int, default=0, help="limit the number of layers per suite")
    parser.add_argument("--port", type=int, default=5260)
    parser.add_argument("--workdir", default=os.path.join(ROOT, "build", "stresstest"))
    args = parser.parse_args()

    suites = [s for s in args.suites.split(",") if s]
    if args.uhd and "uhd" not in suites:
        suites.append("uhd")

    if not os.path.exists(EXE):
        sys.exit(f"{EXE} not found, build first (tools/macos/build.sh)")
    ndi_lib = next((p for p in NDI_LIB_CANDIDATES if os.path.exists(p)), None)
    if not ndi_lib:
        sys.exit("libndi.dylib not found (install NDI Tools or bundle it)")

    workdir = os.path.abspath(args.workdir)
    for sub in ("media", "template", "log", "data"):
        os.makedirs(os.path.join(workdir, sub), exist_ok=True)

    probe = os.path.join(workdir, "ndi_counter")
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ndi_counter.cpp")
    if not os.path.exists(probe) or os.path.getmtime(probe) < os.path.getmtime(src):
        log("building NDI probe ...")
        run("clang++", "-std=c++17", "-O2", "-I", os.path.join(ROOT, "src", "modules", "newtek", "interop"), src,
            "-o", probe)

    log("preparing media ...")
    make_media(os.path.join(workdir, "media"), os.path.join(workdir, "template"), "uhd" in suites)

    found = subprocess.run(["pgrep", "-fl", "MacOS/casparcg"], capture_output=True, text=True).stdout
    others = "\n".join(line[:100] for line in found.splitlines() if "Helper" not in line)
    if others:
        log("WARNING: other CasparCG instances are running, results will be lower than the hardware allows:")
        log("  " + others.replace("\n", "\n  "))

    hw = {
        "cpu": run("sysctl", "-n", "machdep.cpu.brand_string").strip(),
        "performance_cores": run("sysctl", "-n", "hw.perflevel0.physicalcpu").strip(),
        "efficiency_cores": run("sysctl", "-n", "hw.perflevel1.physicalcpu").strip(),
        "memory_gb": int(run("sysctl", "-n", "hw.memsize")) // 2**30,
        "macos": platform.mac_ver()[0],
    }
    log(f"hardware: {hw['cpu']}, {hw['performance_cores']}P+{hw['efficiency_cores']}E cores, "
        f"{hw['memory_gb']} GB, macOS {hw['macos']}")

    report = {"date": datetime.datetime.now().isoformat(timespec="seconds"), "hardware": hw,
              "other_instances_running": bool(others), "suites": {}}
    for suite in suites:
        try:
            baseline, results = run_suite(suite, args, probe, ndi_lib, workdir)
        except Exception as e:
            log(f"  suite {suite} failed: {e}")
            report["suites"][suite] = {"error": str(e)}
            continue
        passed = [r["step"] for r in results if r["ok"]]
        failed = [r["step"] for r in results if not r["ok"]]
        best = max(passed) if passed else 0
        report["suites"][suite] = {"baseline": baseline, "steps": results, "max_realtime": best,
                                   "failures": failed,
                                   "stopped": len(results) >= 2 and not results[-1]["ok"] and not results[-2]["ok"]}

    log("\n=== summary ===")
    for suite, data in report["suites"].items():
        if "error" in data:
            log(f"  {suite:<8} error: {data['error']}")
            continue
        unit = UNITS[suite]
        if data["stopped"]:
            tail = f"(failed at {', '.join(str(f) for f in data['failures'])})"
        elif data["failures"]:
            tail = f"(isolated hiccups at {', '.join(str(f) for f in data['failures'])}; limit not reached)"
        else:
            tail = "(limit not reached within the tested range)"
        log(f"  {suite:<8} max real-time: {data['max_realtime']} {unit} {tail}")

    path = os.path.join(workdir, f"report-{datetime.datetime.now():%Y%m%d-%H%M%S}.json")
    with open(path, "w") as f:
        json.dump(report, f, indent=2)
    log(f"\nreport: {path}")


if __name__ == "__main__":
    main()
