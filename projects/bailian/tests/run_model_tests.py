#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
with tempfile.TemporaryDirectory(prefix='bailian-model-',dir=ROOT/'tests') as temp:
    binary=Path(temp)/'test_model'
    command=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',
        '-I'+str(ROOT/'main'),'-I'+str(ENGINE/'host/include'),
        '-I'+str(ENGINE/'components/mosaico_game_2d/include'),'-I'+str(ENGINE/'components/mosaico_game_assets/include'),
        str(ROOT/'main/forge_money.c'),str(ROOT/'main/forge_data.c'),str(ROOT/'main/forge_model.c'),str(ROOT/'tests/test_model.c'),'-lm','-o',str(binary)]
    subprocess.run(command,check=True)
    subprocess.run([str(binary)],check=True)
