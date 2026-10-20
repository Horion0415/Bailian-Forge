# Bailian Forge application

See the repository [English guide](../../README.md) or [中文说明](../../README_CN.md)
for setup, target SDK, preview, installation limits and licenses. Run commands
from the repository root; this directory relies on ../../submodule.

## Play and balance

Choose an opening relic, heat a blade, then tap to hammer. Offer blades before
the daily quota rises. Three accumulated underpayments end the run; sufficient
tribute does not reset anger. Tribute earns gold; trading swords earns a separate
currency for relics. The merchant visits every three days.

Plain swords select a family at +6. Forms change at +0/+3/+6/+10/+20, with the
legendary form retained above +20. Upgrades cap at +99. Clear day 21 to extend the
contract, currently capped at day 999. Four extension relics exist in the data
but are not exposed by the current touch interface.

Normal/frenzy swing intervals are 200/140 ms, with a 100 ms floor. One click in
the last 80 ms can buffer a swing; holding does not auto-repeat. In frenzy,
contacts add fine-forging value and a 640 ms base energy cycle triggers full
upgrades. Speed relics increase energy; frenzy lasts 4 seconds, or up to 7 with
duration relics. Effects use a separate random stream.

## Source and assets

- forge_model, forge_money, forge_data: rules, large integer values and rule data.
- game, forge_input, forge_view: shared update/input/render lifecycle.
- forge_effects, forge_heating, forge_frenzy, forge_tribute, forge_sword, forge_swing:
  presentation, evolving sword forms and swing scheduling.
- forge_device, forge_screen_mirror: board audio/haptics, saves and Iris integration.
- game_module: native Host ABI used by the browser simulator and replay tests.

assets_src/prepare_assets.py generates RGB565 atlases, audio and 15 native-size
A8 font subsets. Font outlines are required only at build time. The device does
not need Python, FreeType or complete font files. Assets are embedded in the application, which uses the large OTA partition
while preserving the retained Vibe Mode prefix.
For ESP32-S31 with GCC 16.1, SLP is disabled because it crashes across game,
network and DMA sources in fresh builds. Other -O2 passes are retained; the
workaround does not apply to other compiler versions or targets.
When reusing an existing build sdkconfig, align CONFIG_APP_PROJECT_VER with the
source release; updating defaults alone does not replace a cached version.

Rule JSON and generated C tables are included. The optional reference importer
accepts an explicitly supplied independently implemented bailian.cjs; normal
builds do not need it.

## Persistence

The independent forge_save partition uses two versioned CRC-checked slots.
Progress commits after about 1.8 seconds without gameplay input. Power loss
before a commit can lose recent actions. Version 0.2.x uses a 24032-byte payload
and migrates the 23544-byte 0.1.0 payload; older firmware cannot read newer saves.
Mute and language preferences are independent of undo, saved gameplay and new games.
The language key is separate from the unchanged gameplay payload.

## Tests

run_model_tests.py checks the model with UBSan. The replay_*.py scripts execute
the actual native Host ABI for gameplay, audio, swing input, sword evolution,
tribute, heating, frenzy, effects and mechanics. Output goes to ignored tests/artifacts.
Forced-result modules are test fixtures excluded from production sources.
Demonstration GIFs from those fixtures are illustrative replays.

Host checks do not establish physical speaker loudness, touch feel or device
frame rate. See [asset and rule sources](ASSETS.md) for provenance.

## Translation maintenance

forge_i18n.c stores UTF-8-sorted Chinese/English pairs with checked printf placeholders.
The same renderer measures and wraps CJK characters and English words at native font sizes.
Run tests/replay_i18n.py after editing text; it checks translation coverage, all relic/item
details, native text bounds, language controls and save/RNG isolation. Snapshot currency
fields keep their existing machine-facing format; only display amounts are localized.

## Regret Clockwork

A clock restores the complete snapshot immediately before the last contact,
including a shatter's rewards, inheritance, armed item effects, frenzy and energy.
It consumes exactly one clock and keeps the current RNG sequence. Undo is single-use;
a new contact creates a new opportunity. Rewinding clears pending/buffered swings,
impact particles and the previous sword transition, without replaying frenzy entry.

Changing blades, offering/trading, shopping/rerolling, using another item, evolving,
switching to Rack/Relics/Tribute or loading a save invalidates the opportunity. Opening the item
bag to use the clock is allowed. Re-selecting the current Forge tab, pausing, and
changing language/audio preferences preserve it. A failed rewind leaves a swing
in progress intact. Gameplay save size remains unchanged.

`tests/replay_rewind.py` covers all five results, +99, shatter side effects,
clone/backlog rollback, item-bag input, invalidation and controller/visual cleanup.
