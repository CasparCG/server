# Tests

## End-to-end tests

The tests in `e2e/` are pytest files. Each test starts the installed CasparCG server
(`cmake --install` output) with a config of its own, on a free port, and talks AMCP to it.
`AGENTS.md` at the repo root has the build and setup commands.

### Running

```sh
ctest --test-dir build -L e2e --output-on-failure   # all of them
ctest --test-dir build -R e2e.lifecycle             # one file
ctest --test-dir build -L e2e -LE accelerator       # only the ones that don't need a GPU
```

To run single tests, or to use pytest options, run pytest directly. CTest sets `CASPAR_BINARY`;
set it yourself when you don't go through CTest:

```sh
cd build/tests && CASPAR_BINARY=$PWD/../staging/bin/casparcg \
  ../venv/bin/python -m pytest ../../src/tests/e2e/test_lifecycle.py -k kill
```

`CASPAR_TEST_ACCELERATOR=vulkan|opengl` picks the accelerator (default `auto`).
`CASPAR_TEST_OUTPUT` moves the output folder (default `./test-output`).

### Adding a test file

Write `e2e/test_<name>.py`, then register it in `CMakeLists.txt`:

```cmake
e2e_test(<name> LABELS accelerator)
```

It runs as the CTest test `e2e.<name>`. Every file gets the label `e2e`; add `accelerator` if its
tests start channels, since those need a GPU or Mesa's drivers.

### Fixtures

The fixtures are in `e2e/conftest.py` and the server class is in `e2e/caspar.py`.

#### `server`

A running server with one 720p5000 channel and no consumers.

```python
def test_version(server):
    reply = server.command("VERSION")
    assert reply.code == 201
    assert reply.lines[0].startswith("2.")
```

`server.command()` opens a connection, sends one command and returns a `Reply`:

- `code`: the status code, e.g. `201`.
- `status`: the whole first line, e.g. `"201 VERSION OK"`.
- `lines`: the data lines after it. A `201` reply has one, and a `200` reply has all the lines up
  to the empty line that ends it.

To send several commands on one connection, use `AmcpClient`:

```python
from caspar import AmcpClient

def test_two_commands(server):
    with AmcpClient(server.port) as client:
        assert client.send("VERSION").code == 201
        assert client.send("INFO").code == 200
```

#### `make_server`

Starts servers with other settings. Call it once for each server you need:

```python
def test_two_channels(make_server):
    server = make_server(channels=("1080i5000", "720p5000"))
    ...

def test_on_vulkan(make_server):
    server = make_server(accelerator="vulkan")
    ...
```

The `server` fixture is `make_server()` with the defaults.

#### `output_dir`

The test's own folder, `test-output/e2e/<file>/<test>/`. It is emptied when the test starts and
kept afterwards. Each server gets a subfolder (`server0`, `server1`, ...) holding its config,
`server.log` and its `media`, `data` and `template` folders. Put a test's input files in
`server.folder / "media"`, and its own output files in `output_dir`.

### Checks when a test ends

Every server is checked when its test ends, so a test doesn't have to:

- A server that is still running is stopped with `KILL`.
- It must exit with code 0. A test that expects a different exit code sets
  `server.expected_exit`.
- Its log must have no `error` or `fatal` lines.
- If it doesn't exit within 15 s, its threads' stacks are written to `stacks.txt` in its folder
  and it is killed.

A test that shuts the server down itself waits for it to exit:

```python
def test_kill_shuts_down(server):
    server.kill()
    server.wait_exit()
```

`kill()` accepts the server closing the connection without a reply, because the server starts
shutting down while the KILL command runs and may close the connection before replying.
`server.log()` returns the server's output so far, read from its stdout (and copied to
`server.log`). After `wait_exit()` it holds everything:

```python
def test_logs_shutdown(server):
    server.kill()
    server.wait_exit()
    assert "Executing command: KILL" in server.log()
```

While the server runs, a line may arrive after the reply to the command that logged it.

When a test fails, read `server.log` in its server folder.
