"""Starting and stopping the server. The fixtures check every server's exit (code 0, or the test's
expected_exit, and no errors in its log), so tests only wait for it."""


def test_starts_and_answers_version(server):
    reply = server.command("VERSION")
    assert reply.code == 201
    assert reply.lines[0].startswith("2.")


def test_kill_shuts_down(server):
    server.kill()
    server.wait_exit()
