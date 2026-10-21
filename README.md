# Bailian Forge · 百炼

[简体中文](README_CN.md)

A touch-controlled sword-forging game for ESP-Mosaico. Forge evolving blades,
collect relics, and survive 21 days of tribute. Switch between Simplified Chinese and English from the top bar or pause menu.

![Native simulator gameplay](projects/bailian/publish/cover.png)

Screenshots use the shared C renderer in native replay scenarios. They do not
establish hardware acceptance of the 0.3.5 candidate.

- Heat, hammer, protect and offer blades, risking damage or shattering.
- 10 sword families, 50 illustrated forms and upgrades up to +99.
- 94 classic relics, 7 merchant consumables and an extended post-victory contract.
- Sparks, impact, frenzy and tribute animations, audio and haptic feedback.
- Independent device language/mute preferences and a native C browser simulator.

Inspired by ForgeTax, with independently implemented rules, generated artwork and
project-specific balance. This is an adaptation with its own implementation.

## Run the simulator

Checked on Linux with Python 3.10+, a C compiler and Noto CJK fonts. Audio tests
also need Node.js. The asset pipeline reads NotoSansCJK-Regular.ttc and
NotoSansCJK-Bold.ttc from `/usr/share/fonts/opentype/noto`.

```sh
git clone https://github.com/Horion0415/Bailian-Forge.git
cd Bailian-Forge
python3 tools/bootstrap.py
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-host.txt
# Ubuntu/Debian: install build-essential and fonts-noto-cjk if absent.
python3 mosaico.py game sim --project projects/bailian --port 8476
```

Open <http://127.0.0.1:8476/>. Use mouse/touch to play, F or Ctrl to heat/hammer,
and P to pause. Audio starts after a gesture; both the game and preview toolbar
provide mute controls. The simulator requires neither a board nor ESP-IDF.

## Language

Use **EN / ZH** in the top bar, or the language button beside audio in the pause
menu. Switching is immediate, including during tribute. Relics, items, rules,
notices and effects are translated; English amounts use K/M/B/T/Qa/Qi.
The device stores the preference separately from gameplay saves. Undo, new runs
and save loading keep it. Browser preferences last for the simulator process.

![English UI in the native replay test scenario](projects/bailian/publish/english.png)

## Items

The bottom **Items** tab shows how many stored consumables you own. Buying a
scale, clock, chalk, powder pouch or star shard stores it without activating it.
Choose an item, read its current requirements, then confirm one use; Back cancels
without consuming stock. A valid clock can restore a shattered blade from this tab.
Incense reduces anger on purchase and the delivery slip adds two blades to the
next supply. Their purchase dialogs explicitly say that they apply immediately.

## Build and install

Target: **ESP-Mosaico / ESP32-S31, 480×480 touch display**. Use ESP-IDF at exactly
`7b9cc1ac79f865983f59bb8ff3ff43eb74ff1dbe` with its S31 toolchain. Activate that
SDK's Python environment before building:

```sh
python mosaico.py game build --project projects/bailian --idf-path /path/to/pinned-esp-idf
```

Keep the complete repository layout. Dependency versions, registry lockfile and
the required engine patch are included. See [application notes](projects/bailian/README.md)
and the [pinned application integration guide](https://github.com/esp-mosaico/esp-mosaico-utils/blob/c6b43a6be7fb8aba832fc4016d4076702f5414ae/mosaico-tools/docs/application-integration.md).
Version 0.3.5 is a candidate for **ESP-Iris / Vibe Mode 0.2**. It
uses wire protocol 2, Recovery ABI 2 and the `esp-iris-system-update/0.2` bundle
format. An old 0.1 application or base installation must follow the
[official 0.2 migration guide](https://github.com/esp-mosaico/esp-mosaico-utils/blob/c6b43a6be7fb8aba832fc4016d4076702f5414ae/esp-mosaico-vibe-mode/docs/migration-0.2.md)
before installation. Use System Update for the changed partition contract;
`app-update` applies only after the complete partition table and resources match.

Hardware migration, startup, save restoration and mosaico.ai installation of
this candidate remain unverified because no test device is currently available.
The earlier 0.3.3 hardware results apply to the separate 0.1 build. See the
[candidate notes](projects/bailian/IRIS_02_CANDIDATE.md). Browser save data is
separate from the device; its mute choice lasts for the simulator process.

## Verify

```sh
python3 tools/bootstrap.py --check
python3 projects/bailian/tests/run_model_tests.py
python3 projects/bailian/tests/replay_gameplay.py
python3 projects/bailian/tests/replay_audio.py
python3 projects/bailian/tests/replay_i18n.py
python3 projects/bailian/tests/replay_rewind.py
python3 projects/bailian/tests/replay_items.py
```

Tests execute the shared native C model and renderer. Build/test output,
credentials and device diagnostics are excluded from Git. After setup, the
engine submodule is intentionally modified by the tracked patch.

## License and credits

Code: [Apache-2.0](LICENSE). Fonts: [SIL OFL 1.1](projects/bailian/licenses/Noto-OFL.txt).
Dependencies retain their upstream licenses. See [NOTICE](NOTICE),
[asset and rule sources](projects/bailian/ASSETS.md), and [release notes](CHANGELOG.md).
