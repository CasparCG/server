"""Fixtures for the end-to-end tests. See caspar.py and AGENTS.md."""

import os
import re
import shutil
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent))
from caspar import Server  # noqa: E402


@pytest.fixture
def output_dir(request) -> Path:
    """The test's own folder, emptied first and kept afterwards:
    $CASPAR_TEST_OUTPUT/e2e/<file>/<test>, by default ./test-output/e2e/<file>/<test>."""
    root = Path(os.environ.get("CASPAR_TEST_OUTPUT") or "test-output").absolute()
    name = re.sub(r"[^\w.-]", "_", request.node.name)
    folder = root / "e2e" / Path(request.node.fspath).stem / name
    shutil.rmtree(folder, ignore_errors=True)
    folder.mkdir(parents=True)
    return folder


@pytest.fixture
def make_server(output_dir):
    """Returns a function that starts a server, by default with one 720p5000 channel and no
    consumers: make_server(channels=("1080i5000",), accelerator="vulkan"). Every server is checked
    when the test ends: stopped with KILL if still running, it must exit with code 0 and log no
    errors."""
    binary = os.environ.get("CASPAR_BINARY")
    if not binary:
        pytest.skip("CASPAR_BINARY isn't set; run through ctest, or point it at an installed casparcg")
    if not Path(binary).is_file():
        pytest.fail(f"{binary} doesn't exist; run `cmake --install <build folder>` first")

    servers = []

    def make(channels=("720p5000",), accelerator=None) -> Server:
        accelerator = accelerator or os.environ.get("CASPAR_TEST_ACCELERATOR", "auto")
        server = Server(Path(binary), output_dir / f"server{len(servers)}", channels, accelerator)
        servers.append(server)
        server.start()
        return server

    yield make

    failures = []
    for server in servers:
        try:
            server.check_shutdown()
        except Exception as e:  # check the other servers too
            failures.append(str(e) if isinstance(e, AssertionError) else repr(e))
    if failures:
        pytest.fail("\n\n".join(failures), pytrace=False)


@pytest.fixture
def server(make_server) -> Server:
    """A running server with one 720p5000 channel."""
    return make_server()
