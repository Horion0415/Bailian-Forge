# Changelog

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
