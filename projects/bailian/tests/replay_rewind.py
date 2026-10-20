#!/usr/bin/env python3
"""Exercise Regret Clockwork through the actual native model and touch handlers."""
from pathlib import Path
import argparse
import ctypes
import importlib.util
import json
import shutil
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--audit", action="store_true", help="Report baseline failures without enforcing them.")
args = parser.parse_args()
ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT.parents[1] / "submodule/raylib-lite-engine"
ART = ROOT / "tests/artifacts/rewind"
ART.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("rewind_host", ENGINE / "host/run_game.py")
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)
host.prepare_project_assets(ROOT)
with tempfile.TemporaryDirectory(prefix="forge-rewind-") as scratch:
    project = Path(scratch) / "project"
    project.mkdir()
    shutil.copytree(ROOT / "main", project / "main")
    shutil.copy2(ROOT / "tests/rewind_probe.c", project / "main/rewind_probe.c")
    (project / "assets").mkdir()
    (project / "assets/generated").symlink_to(ROOT / "assets/generated", target_is_directory=True)
    module = (project / "main/game_module.c").read_text()
    marker = "    InitWindow(GAME_WIDTH"
    assert marker in module
    module = '#include "game_internal.h"\ngame_handle_t rewind_test_game;\n' + module.replace(
        marker, "    rewind_test_game=state->game;\n" + marker, 1)
    (project / "main/game_module.c").write_text(module)
    manifest = json.loads((ROOT / "game.sim.json").read_text())
    manifest["sources"].append("main/rewind_probe.c")
    (project / "game.sim.json").write_text(json.dumps(manifest))
    build = Path(scratch) / "build"
    build.mkdir()
    runtime = host.GenericHostRuntime(project, build)
    try:
        api = runtime.api
        api.rewind_test_report.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        output = ctypes.create_string_buffer(4096)
        assert 0 < api.rewind_test_report(output, len(output)) < len(output)
        checks = json.loads(output.value)
        failed = [name for name, value in checks.items() if value is False]
        result = {"status": "FAIL" if failed else "PASS", "checks": checks, "failed": failed}
        (ART / ("baseline.json" if args.audit else "result.json")).write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result), flush=True)
        if not args.audit:
            assert not failed, failed
        runtime.frame()  # Exercise the real renderer after restoration and cleanup.
    finally:
        runtime.close()
