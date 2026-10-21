#!/usr/bin/env python3
"""Verify bilingual production rendering, layouts and preference isolation."""
from pathlib import Path
import ctypes
import importlib.util
import json
import re
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT.parents[1] / "submodule/raylib-lite-engine"
ART = ROOT / "tests/artifacts/i18n"
ART.mkdir(parents=True, exist_ok=True)
LITERAL = r'"(?:[^"\\]|\\.)*"'
dictionary = {}
for left, right in re.findall(r'\{(' + LITERAL + r'),(' + LITERAL + r')\},', (ROOT / "main/forge_i18n.c").read_text()):
    zh, en = json.loads(left), json.loads(right)
    assert zh not in dictionary and en, zh
    dictionary[zh] = en
assert list(dictionary) == sorted(dictionary, key=lambda value: value.encode())
for path in (ROOT / "main").glob("*.c"):
    if path.name in {"forge_i18n.c", "forge_money.c"}:
        continue
    for literal in re.findall(LITERAL, path.read_text()):
        value = json.loads(literal)
        if any("\u3400" <= character <= "\u9fff" for character in value):
            assert value in dictionary, (path.name, value)
for zh, en in dictionary.items():
    placeholders = re.findall(r'%(?:l?[us]|%)', zh)
    if placeholders:
        assert placeholders == re.findall(r'%(?:l?[us]|%)', en), zh

spec = importlib.util.spec_from_file_location("i18n_host", ENGINE / "host/run_game.py")
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)
host.prepare_project_assets(ROOT)
with tempfile.TemporaryDirectory(prefix="forge-i18n-") as scratch:
    project = Path(scratch) / "project"
    project.mkdir()
    shutil.copytree(ROOT / "main", project / "main")
    shutil.copy2(ROOT / "tests/i18n_probe.c", project / "main/i18n_probe.c")
    (project / "assets").mkdir()
    (project / "assets/generated").symlink_to(ROOT / "assets/generated", target_is_directory=True)
    module = (project / "main/game_module.c").read_text()
    marker = "    InitWindow(GAME_WIDTH"
    assert marker in module
    module = '#include "game_internal.h"\ngame_handle_t i18n_test_game;\n' + module.replace(
        marker, "    i18n_test_game=state->game;\n" + marker, 1)
    (project / "main/game_module.c").write_text(module)
    # Assertions inspect the actual production layout, not a second renderer.
    view = (project / "main/forge_view.c").read_text()
    marker = "    text=forge_tr(g,text);\n"
    assert marker in view
    probe = '''    if(g->language==GAME_LANGUAGE_EN){
        const char *check=text;int width=0;
        while(*check){uint32_t c=decode(&check);assert(c<0x3400||c>0x9fff);width+=advance(c,size);}
        if(x<0||x+width>480){fprintf(stderr,"Text outside frame: %s at %d width %d\\n",text,x,width);assert(0);}
    }
'''
    view = '#include <assert.h>\n' + view.replace(marker, marker + probe, 1)
    marker = "    (void)flow_text(g,text,x,y,width,size,lines,true,color);"
    assert marker in view
    view = view.replace(marker, '''    unsigned rows=flow_text(g,text,x,y,width,size,lines,true,color);
    if(g->language==GAME_LANGUAGE_EN&&rows>lines){fprintf(stderr,"Text clipped: %s (%u/%u)\\n",text,rows,lines);assert(0);}''')
    (project / "main/forge_view.c").write_text(view)
    manifest = json.loads((ROOT / "game.sim.json").read_text())
    manifest["sources"].append("main/i18n_probe.c")
    (project / "game.sim.json").write_text(json.dumps(manifest))
    build = Path(scratch) / "build"
    build.mkdir()
    runtime = host.GenericHostRuntime(project, build)
    try:
        api = runtime.api
        api.i18n_test_preferences.restype = ctypes.c_int
        assert api.i18n_test_preferences() == 1
        api.i18n_test_scene.argtypes = [ctypes.c_uint, ctypes.c_uint]
        api.i18n_test_notice.restype = ctypes.c_char_p
        def tap(x, y):
            runtime.pointer(1, x, y, True)
            runtime.pointer(1, x, y, False)
        def action(code):
            runtime.action(code, True)
            runtime.action(code, False)
        def capture(name):
            before = runtime.metadata()["state_hash"]
            data = runtime.frame()
            assert runtime.metadata()["state_hash"] == before
            (ART / (name + ".png")).write_bytes(data)
            return data

        scenes = [("gift", 0, 0), ("forge", 1, 0), ("pause", 2, 0), ("rack", 3, 0),
                  ("items", 4, 0), ("merchant", 5, 0), ("merchant-page2", 5, 1),
                  ("rules", 6, 0), ("mastery", 6, 1), ("codex", 6, 2),
                  ("family", 9, 1), ("warning", 10, 1), ("new-run", 10, 2),
                  ("trade", 10, 5), ("discard", 10, 6), ("tribute-rise", 11, 0),
                  ("tribute-dragon", 11, 900), ("tribute-reward", 11, 1700),
                  ("heating", 12, 0), ("frenzy", 13, 0), ("clear", 14, 2),
                  ("lost", 14, 3), ("owned", 15, 0), ("large-values", 16, 0),
                  ("rewind-shatter", 18, 0), ("rewind-empty", 18, 1)]
        captures = []
        for name, scene, detail in scenes:
            api.i18n_test_scene(scene, detail)
            if runtime.metadata()["language"] != "en":
                action(8)
            english = capture(name + "-en")
            before = runtime.metadata()
            tribute = before["fx"]["tribute_stage"] != 0
            tap(367 if tribute else 154, 40 if tribute else 25)
            assert runtime.metadata()["language"] == "zh"
            assert runtime.metadata()["state_hash"] == before["state_hash"]
            assert runtime.metadata()["paused"] == before["paused"]
            assert capture(name + "-zh") != english
            tap(367 if tribute else 154, 40 if tribute else 25)
            assert runtime.metadata()["language"] == "en"
            captures.append(name)
        # All relics, seven items and nine specialized families use English.
        for scene, details in [(7, range(98)), (8, range(7)), (17, range(1, 10))]:
            for detail in details:
                api.i18n_test_scene(scene, detail)
                runtime.frame()
        api.i18n_test_scene(11, 900)
        runtime.control(1)
        before = runtime.metadata()
        tap(367, 40)
        assert runtime.metadata()["language"] == "zh" and runtime.metadata()["paused"]
        tap(367, 40)
        runtime.step(False, False, False)
        assert runtime.metadata()["language"] == "en" and runtime.metadata()["paused"]
        assert runtime.metadata()["fx"]["tribute_age_ms"] == before["fx"]["tribute_age_ms"]
        capture("tribute-paused-en")
        runtime.control(2)
        api.i18n_test_scene(2, 0)
        tap(355, 385)
        assert runtime.metadata()["language"] == "zh" and runtime.metadata()["paused"]
        tap(355, 385)
        assert runtime.metadata()["language"] == "en" and runtime.metadata()["paused"]
        api.i18n_test_scene(0, 0)
        tap(80, 200)
        notice = api.i18n_test_notice().decode()
        assert notice.startswith("Acquired: ") and not any("\u3400" <= char <= "\u9fff" for char in notice)
        assert runtime.metadata()["language"] == "en"
        result = {"status": "PASS", "translations": len(dictionary), "scenes": captures,
                  "relic_details": 98, "item_details": 7, "family_details": 9,
                  "checks": ["no untranslated CJK in English rendering", "native text bounds and wrapping",
                             "instant switching in every phase and paused ritual", "same gameplay hash/RNG/save payload",
                             "language survives undo/new run/save loading", "independent mute preference",
                             "K/M/B/T/Qa/Qi formatting", "formatted notices and printf placeholders"]}
        (ART / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result))
    finally:
        runtime.close()
