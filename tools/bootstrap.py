#!/usr/bin/env python3
"""Initialize pinned dependencies and apply the game's engine patch."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def git(*args: str, directory: Path = ROOT) -> str:
    return subprocess.check_output(["git", "-C", str(directory), *args], text=True).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Verify without changing files.")
    args = parser.parse_args()
    lock = json.loads((ROOT / "projects/bailian/workspace-lock.json").read_text())
    if Path(git("rev-parse", "--show-toplevel")).resolve() != ROOT:
        raise ValueError("Run from a Git clone of Bailian Forge.")
    if not args.check:
        subprocess.run(["git", "-C", str(ROOT), "submodule", "update", "--init",
                        "--recursive", "--depth", "1", "--jobs", "3"], check=True)
    for name, expected in lock["submodules"].items():
        directory = ROOT / "submodule" / name
        if not (directory / ".git").exists():
            raise ValueError(f"Missing dependency {name}; run python3 tools/bootstrap.py.")
        if git("rev-parse", "HEAD", directory=directory) != expected:
            raise ValueError(f"Unexpected revision for {name}; expected {expected}.")
    engine = ROOT / "submodule/raylib-lite-engine"
    patch = ROOT / "patches/raylib-lite-engine.patch"
    expected = lock["local_patches"]["raylib-lite-engine"]
    if hashlib.sha256(patch.read_bytes()).hexdigest() != expected["patch_sha256"]:
        raise ValueError("Engine patch checksum does not match workspace-lock.json.")

    def matches() -> bool:
        return all((engine / name).is_file()
                   and hashlib.sha256((engine / name).read_bytes()).hexdigest() == digest
                   for name, digest in expected["files"].items())

    if not matches() and not args.check:
        subprocess.run(["git", "-C", str(engine), "apply", "--check", str(patch)], check=True)
        subprocess.run(["git", "-C", str(engine), "apply", str(patch)], check=True)
    if not matches():
        raise ValueError("Engine patch is missing or modified; existing edits were preserved.")
    print("Pinned dependencies and engine patch verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"bootstrap: {error}", file=sys.stderr)
        raise SystemExit(1)
