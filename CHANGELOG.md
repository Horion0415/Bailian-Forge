# Changelog

## 0.2.1

- Add an independent device mute preference with immediate stopping and no queued replay.
- Include 21-day tribute, 94 relics, 7 consumables, 10 sword families,
  50 sword forms and illustrated heating/frenzy/tribute effects.
- Retain the 0.2.0 frenzy balance: fast contacts add fine-forging value,
  while energy determines full upgrades. Preserve legacy save migration.

Browser and device share native C gameplay and the RGB565 renderer. Device mute
was committed and read back from NVS; a separate power-cycle test was not performed.
Platform download/installation remains unverified; see README.
