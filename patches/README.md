# Engine patch

The patch applies to the exact revision in `projects/bailian/workspace-lock.json`.
It contains RGB565 channel blending, native-size A8 glyph masks, project-local
browser WAV playback, immediate mute/queue clearing, and host regression tests.
Optional en/zh metadata also localizes preview audio labels without changing playback.
New files are included in the patch.

Run `python3 tools/bootstrap.py` to initialize dependencies and apply it.
The script checks dependency commits, patch checksum and resulting file hashes.
Repeated setup preserves an already applied patch; `--check` verifies without writes.

The engine appears modified in `git status` after setup; this is expected.
Its upstream commit remains pinned. Update the patch and checksums when changing
the engine, rather than pointing to an unpublished local commit.
