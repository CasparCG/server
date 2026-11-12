# Notes for AI agents

CasparCG Server: a C++20 playout server. Sources are under `src/`, which is also the CMake root.

| Directory         | What's in it                                                         |
| ----------------- | -------------------------------------------------------------------- |
| `src/common`      | Shared utilities: logging, executors, environment/config             |
| `src/core`        | Channels, stage/layer, mixer, frame and producer/consumer interfaces |
| `src/accelerator` | Image mixer backends: `ogl` (OpenGL) and `vulkan`                    |
| `src/modules/*`   | Producers and consumers: ffmpeg, decklink, html, image, ...          |
| `src/protocol`    | AMCP and the TCP server                                              |
| `src/shell`       | `main()`, server setup, the default `casparcg.config`                |
| `src/tests`       | Tests                                                                |

## Building (Linux)

```sh
cmake -S src -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=$PWD/build/staging
cmake --build build --parallel   # add -DENABLE_HTML=OFF above if CEF isn't installed
cmake --install build
```

`cmake --build` alone does not update `build/staging`. Anything that runs the installed server,
including the tests, needs `cmake --install` after every build, or it silently runs the previous
binary. See `BUILDING.md` for dependencies, Windows and the remaining build options.

## Code style

- Format C++ with the repo's `.clang-format`.
- Write code that reads like the code around it: naming, comment density, idioms.
- Keep comments short. Explain what isn't obvious.

## Tests

End-to-end tests (`src/tests/e2e/`, pytest) start the installed server on a free port with a config
of their own and talk AMCP to it. They need pytest, and are registered with CTest when
`-DBUILD_TESTING=ON` and the Python CMake finds has it:

```sh
python3 -m venv build/venv && build/venv/bin/pip install pytest
cmake -S src -B build -DBUILD_TESTING=ON -DPython3_EXECUTABLE=$PWD/build/venv/bin/python \
  -DCMAKE_INSTALL_PREFIX=$PWD/build/staging
cmake --build build --parallel && cmake --install build
ctest --test-dir build -L e2e --output-on-failure
ctest --test-dir build -R e2e.lifecycle   # one file; -LE accelerator skips the ones needing a GPU
# one test, directly (ctest sets CASPAR_BINARY itself):
cd build/tests && CASPAR_BINARY=$PWD/../staging/bin/casparcg \
  ../venv/bin/python -m pytest ../../src/tests/e2e/test_lifecycle.py -k kill
```

CTest runs `<CMAKE_INSTALL_PREFIX>/bin/casparcg`. Channels need an accelerator (a GPU, or Mesa's
drivers); `CASPAR_TEST_ACCELERATOR=vulkan|opengl` picks one. `src/tests/README.md` shows how to write
tests with the fixtures.

- Fixtures (`e2e/conftest.py`): `server` (one 720p5000 channel), `make_server(channels=(...),
  accelerator=...)` and `output_dir`. `Server` (`e2e/caspar.py`) has `command("VERSION")`,
  `kill()`, `wait_exit()` and `log()`.
- Every server is checked when its test ends: stopped with `KILL` if still running, it must exit
  with code 0 (or the test's `server.expected_exit`) and log no `error` or `fatal` lines.
- A new test file `e2e/test_<name>.py` must be registered in `src/tests/CMakeLists.txt` with
  `e2e_test(<name> LABELS <label>...)`, which makes it the CTest test `e2e.<name>`. Every file gets
  the label `e2e`; add `accelerator` if it starts channels.
- A test's server folder (config, `server.log`, and `stacks.txt` if it hung) is kept under
  `test-output/e2e/<file>/<test>/`. When a test fails, read `server.log` there.

Rules:

- **Commit the tests you write.** A test written to check a fix or a feature belongs in the repo,
  not in a scratch folder or a one-off script.
- **Use the fixtures** instead of starting servers yourself; extend them if something is missing.
- **Never delete a test, disable it or loosen a check to make it pass** without saying so, and why,
  in the commit message.
- Don't commit a test that fails, or is skipped, because of a bug you aren't fixing in the same
  change. Describe the bug to the person you're working with instead; bugs are tracked as GitHub
  issues, and only a person files them.

## Commits

Small, focused commits that each build. Conventional prefixes (`feat(ffmpeg):`, `fix(core):`,
`test(e2e):`, `build:`, `docs:`) are used in the history.
