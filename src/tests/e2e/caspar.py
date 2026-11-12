"""Runs the real CasparCG server for end-to-end tests: config, process, AMCP client and log checks.

Tests use it through the fixtures in conftest.py; see AGENTS.md.
"""

import os
import re
import signal
import socket
import subprocess
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Reply:
    code: int
    status: str  # the first line, e.g. "201 VERSION OK"
    lines: list = field(default_factory=list)  # the data lines after it


class AmcpClient:
    """One AMCP connection. send() returns the parsed reply."""

    def __init__(self, port, timeout=10.0):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        self.buffer = b""

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.socket.close()

    def read_line(self) -> str:
        while b"\r\n" not in self.buffer:
            chunk = self.socket.recv(65536)
            if not chunk:
                raise ConnectionError("the server closed the connection")
            self.buffer += chunk
        line, self.buffer = self.buffer.split(b"\r\n", 1)
        return line.decode("utf-8", errors="replace")

    def send(self, command: str) -> Reply:
        self.socket.sendall(command.encode("utf-8") + b"\r\n")
        status = self.read_line()
        reply = Reply(int(status.split(" ", 1)[0]), status)
        if reply.code == 200:  # data lines up to an empty one
            while (line := self.read_line()) != "":
                reply.lines.append(line)
        elif reply.code in (201, 400):  # one data line
            reply.lines.append(self.read_line())
        return reply


def free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def describe_exit(code: int) -> str:
    return f"killed by {signal.Signals(-code).name}" if code < 0 else f"exit code {code}"


class Server:
    """A casparcg process with its own config, folders and AMCP port, all under `folder`."""

    def __init__(self, binary: Path, folder: Path, channels=("720p5000",), accelerator="auto"):
        self.binary = binary
        self.folder = folder
        self.port = free_port()
        self.process = None
        self.exit_code = None
        self.expected_exit = 0  # what check_shutdown() expects; a test can change it
        for sub in ("media", "data", "template"):
            (folder / sub).mkdir(parents=True, exist_ok=True)

        channels_xml = "".join(
            f"<channel><video-mode>{mode}</video-mode><consumers /></channel>" for mode in channels
        )
        self.config = folder / "casparcg.config"
        self.config.write_text(
            f"""<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <paths>
    <media-path>{folder}/media/</media-path>
    <log-path disabled="true" />
    <data-path>{folder}/data/</data-path>
    <template-path>{folder}/template/</template-path>
  </paths>
  <log-level>debug</log-level>
  <accelerator>{accelerator}</accelerator>
  <channels>{channels_xml}</channels>
  <controllers><tcp><port>{self.port}</port><protocol>AMCP</protocol></tcp></controllers>
</configuration>
"""
        )
        self.log_path = folder / "server.log"
        self.output = []  # the server's output lines so far, from read_output()

    def start(self, timeout=30.0):
        """Starts the server and waits until AMCP answers VERSION."""
        # The installed binary has no rpath; like run.sh, find libcef and friends in <prefix>/lib.
        env = dict(os.environ)
        env["LD_LIBRARY_PATH"] = os.pathsep.join(
            filter(None, [str(self.binary.parent.parent / "lib"), env.get("LD_LIBRARY_PATH")])
        )
        self.process = subprocess.Popen(
            [str(self.binary), str(self.config)],
            cwd=self.folder,
            env=env,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        self.reader = threading.Thread(target=self.read_output, daemon=True)
        self.reader.start()
        deadline = time.monotonic() + timeout
        while True:
            if self.process.poll() is not None:
                code = self.wait_exit()
                raise AssertionError(f"the server ended ({describe_exit(code)}) before answering AMCP\n{self.tail()}")
            try:
                if self.command("VERSION", timeout=2.0).code == 201:
                    return
            except OSError:
                pass
            if time.monotonic() > deadline:
                raise AssertionError(f"AMCP didn't answer within {timeout} s\n{self.tail()}")
            time.sleep(0.1)

    def wait_exit(self, timeout=15.0) -> int:
        """Waits for the process to end and returns its exit code (-signal when killed by one). If it
        doesn't end, writes its threads' stacks to stacks.txt and kills it."""
        try:
            self.exit_code = self.process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            stacks = self.folder / "stacks.txt"
            try:
                with open(stacks, "w") as out:
                    subprocess.run(
                        ["gdb", "-q", "-batch", "-p", str(self.process.pid), "-ex", "thread apply all bt"],
                        stdout=out,
                        stderr=subprocess.STDOUT,
                        timeout=120,
                    )
            except (OSError, subprocess.TimeoutExpired) as e:  # no gdb, or it hung; kill the server anyway
                stacks = f"none ({e})"
            self.process.kill()
            self.process.wait()
            raise AssertionError(f"the server didn't exit within {timeout} s; thread stacks: {stacks}\n{self.tail()}")
        finally:
            # CEF's subprocesses can keep the pipe open after the server has gone; don't wait on them.
            self.reader.join(timeout=5.0)
        return self.exit_code

    def read_output(self):
        """Collects the server's output in `output`, and copies it to server.log as it comes."""
        with open(self.log_path, "wb") as log_file:
            for line in self.process.stdout:
                log_file.write(line)
                log_file.flush()
                self.output.append(line.decode("utf-8", errors="replace"))

    def command(self, command: str, timeout=10.0) -> Reply:
        """Sends one command on a connection of its own."""
        with AmcpClient(self.port, timeout) as client:
            return client.send(command)

    def kill(self):
        """Sends KILL. The server starts shutting down while the command runs and may close the
        connection before "202 KILL OK" is sent, so a closed connection counts as success too."""
        with AmcpClient(self.port) as client:
            try:
                reply = client.send("KILL")
            except ConnectionError:
                return
        assert reply.code == 202, reply.status

    def log(self) -> str:
        """The server's output so far."""
        return "".join(self.output)

    def tail(self, lines=30) -> str:
        return f"--- last lines of {self.log_path}\n" + "\n".join(self.log().splitlines()[-lines:])

    def check_shutdown(self):
        """Stops the server if it is still running (KILL), then fails on an exit code other than
        expected_exit or on error lines in the log."""
        if self.process.poll() is None and self.exit_code is None:
            try:
                self.kill()
            except OSError:  # couldn't connect
                self.process.terminate()
        if self.exit_code is None:
            self.wait_exit()

        problems = []
        if self.exit_code != self.expected_exit:
            problems.append(f"the server ended with {describe_exit(self.exit_code)}")
        if errors := [line for line in self.log().splitlines() if re.search(r"\[(error|fatal)\]", line)]:
            problems.append("errors in the log:\n" + "\n".join(errors[:20]))
        if problems:
            raise AssertionError("\n".join(problems) + f"\nlog: {self.log_path}")
