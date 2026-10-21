# Changelog

## 0.3.3

- Preserve rewind after inspecting/cancelling an item and returning to Forge.
- Keep extra taps on a shattered blade at the anvil when a clock can restore it.
- Show clock quantity, highlight an available rewind and clarify missing-item/snapshot notices.
- Cover both shatter touch paths and preserve invalidation after actual gameplay changes.

## 0.3.2

- Replace the tribute silhouette with a detailed transparent dragon sprite.
- Add breathing, an acceptance nod, and an underpayment lunge with red eyes and embers.
- Keep tribute settlement, timing, RNG and the gameplay save unchanged.
- Verify application update, running firmware identity, startup and saved-game/mute restoration on ESP-Mosaico.

## 0.3.1

- Clear buffered swings, stale impact/shatter effects and sword transitions after rewind.
- Preserve an active swing when rewind fails and retain undo on the current Forge tab.
- Add native rewind regressions; keep snapshot, RNG, item cost and save-format rules unchanged.

## 0.3.0

- Add immediate Chinese/English switching in the top bar and pause menu.
- Translate 435 strings, including all relics, items, sword families, rules and effects.
- Add independent device language persistence, English amount units and native text fitting/word wrapping.
- Keep the 24032-byte gameplay save and existing save migration unchanged.
- Guard the pinned S31 GCC 16.1 against SLP crashes during fresh builds.

## 0.2.1

- Add an independent device mute preference with immediate stopping and no queued replay.
- Include 21-day tribute, 94 relics, 7 consumables, 10 sword families,
  50 sword forms and illustrated heating/frenzy/tribute effects.
- Retain the 0.2.0 frenzy balance: fast contacts add fine-forging value,
  while energy determines full upgrades. Preserve legacy save migration.

Browser and device share native C gameplay and the RGB565 renderer. Device mute
was committed and read back from NVS; a separate power-cycle test was not performed.
Platform download/installation remains unverified; see README.
