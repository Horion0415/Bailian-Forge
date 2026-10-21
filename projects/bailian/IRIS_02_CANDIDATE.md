# Iris 0.2 candidate

This isolated 0.3.5 candidate includes the 0.3.4 fix for the platform's rejection of the 0.3.3
`esp-iris-system-update/v1` archive. It uses the matching official utils revision
`c6b43a6be7fb8aba832fc4016d4076702f5414ae` and must pass device acceptance before
upload. Changing the old archive's schema string does not migrate an application.

The ESP-IDF revision, BSP, patched engine, rule data and save payload stay
pinned. The item selection UI is clearer; stored consumables still require manual
use, while incense and delivery remain instant purchases. The application was
rebuilt from a fresh build directory with the Iris
0.2 component and application recovery integration. It uses two USB CDC
interfaces, wire version 2, Recovery ABI 2 and the test-partition boot contract.

The retained `factory/app/factory` entry becomes `vibe_mode/app/test` at the same
`0x20000` address and `0x1c0000` capacity. The normal slot is `main_app/app/ota_0`
at `0x210000` with `0xdb0000` bytes. The `forge_save` NVS partition stays at
`0xfc0000`, size `0x40000`, with the unchanged 24032-byte gameplay payload.
Unchanged data addresses alone do not establish successful save migration.

The application archive contains only its partition table and application;
the official Vibe Mode base bootloader and retained firmware are separate.
Follow the pinned official migration guide for a 0.1 device: export needed
data with the old environment, install the matching 0.2 base through the
consuming workspace CLI, verify live identity and product contract, then install
the application through System Update. Do not use an old Gateway or `app-update`
to cross the 0.1/0.2 layout boundary.

No hardware is currently available. Device identity, migration, normal and
maintenance startup, touch/display/audio, independent USB transfer, saved-game
and preference restoration, and website installation remain unverified.
Host builds and package checks are recorded separately and are not hardware
acceptance. The original 0.3.3 source, artifacts and simulator are retained.
