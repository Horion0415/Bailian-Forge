#!/usr/bin/env python3
"""Exercise purchase, cancellation, optional use and shatter undo via native touch."""
from pathlib import Path
import ctypes
import importlib.util
import json
import shutil
import tempfile

ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
ART=ROOT/'tests/artifacts/items'
ART.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('items_host',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)
host.prepare_project_assets(ROOT)

with tempfile.TemporaryDirectory(prefix='forge-items-') as scratch:
    project=Path(scratch)/'project'
    project.mkdir()
    shutil.copytree(ROOT/'main',project/'main')
    shutil.copy2(ROOT/'tests/items_probe.c',project/'main/items_probe.c')
    (project/'assets').mkdir()
    (project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    module=(project/'main/game_module.c').read_text()
    marker='    InitWindow(GAME_WIDTH'
    assert marker in module
    module='#include "game_internal.h"\ngame_handle_t items_test_game;\n'+module.replace(marker,'    items_test_game=state->game;\n'+marker,1)
    (project/'main/game_module.c').write_text(module)
    manifest=json.loads((ROOT/'game.sim.json').read_text())
    manifest['sources'].append('main/items_probe.c')
    (project/'game.sim.json').write_text(json.dumps(manifest))
    build=Path(scratch)/'build'
    build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    try:
        api=runtime.api
        api.items_test_prepare.argtypes=[ctypes.c_uint]
        api.items_test_read.argtypes=[ctypes.c_uint]
        api.items_test_read.restype=ctypes.c_uint
        api.items_test_strike.argtypes=[ctypes.c_int]
        api.items_test_strike.restype=ctypes.c_int
        api.items_test_guards.restype=ctypes.c_uint
        def tap(x,y):
            runtime.pointer(1,x,y,True)
            runtime.pointer(1,x,y,False)
        def capture(name):
            before=runtime.metadata()['state_hash']
            (ART/(name+'.png')).write_bytes(runtime.frame())
            assert before==runtime.metadata()['state_hash']
        def buy(item):
            api.items_test_prepare(item)
            tap(200,150)
            tap(330,340)
        checks=[]
        # Buy first, retain the item, cancel without consumption, then use once.
        for item in (0,2,3,4):
            buy(item)
            assert api.items_test_read(item)==1 and api.items_test_read(7)==0
            assert runtime.metadata()['charge']==0
            tap(240,450)
            assert runtime.metadata()['screen']==5
            tap(180,125+item*60)
            tap(100,340)
            assert api.items_test_read(item)==1 and api.items_test_read(7)==0
            tap(180,125+item*60)
            if item==2:capture('choose-chalk-zh')
            tap(330,340)
            assert api.items_test_read(item)==0 and runtime.metadata()['screen']==0
            if item==4:assert runtime.metadata()['charge']==3
            else:
                assert api.items_test_read(7)=={0:1,2:2,3:4}[item]
                assert api.items_test_strike(2)==1
                assert api.items_test_read(7)==0
            checks.append('manual-item-'+str(item))
        buy(1)
        tap(48,450)
        assert api.items_test_strike(0)==1 and runtime.metadata()['active_id']==0
        tap(240,450)
        tap(180,185)
        capture('clock-after-shatter-zh')
        tap(330,340)
        assert runtime.metadata()['active_id']!=0 and runtime.metadata()['hits']==0
        assert runtime.metadata()['breaks']==0 and api.items_test_read(1)==0
        checks.append('clock-after-shatter-via-visible-bag')
        for item in (5,6):
            buy(item)
            assert api.items_test_read(item)==0
            assert api.items_test_read(9)==(1 if item==5 else 2)
            assert api.items_test_read(10)==(1 if item==5 else 0)
            assert api.items_test_read(8)==(2 if item==6 else 0)
            checks.append('instant-purchase-'+str(item))
        for item,expected in [(2,'stored'),(5,'instant'),(6,'instant')]:
            api.items_test_prepare(item)
            if runtime.metadata()['language']!='en':
                runtime.action(8,True)
                runtime.action(8,False)
            tap(200,150)
            capture('merchant-'+expected+'-'+str(item)+'-en')
        api.items_test_prepare(2)
        for x,screen in [(48,0),(144,1),(240,5),(336,2),(432,3)]:
            tap(x,450)
            assert runtime.metadata()['screen']==screen
        checks.append('all-five-visible-tabs')
        count=api.items_test_guards()
        assert count==22
        if runtime.metadata()['language']!='zh':
            runtime.action(8,True)
            runtime.action(8,False)
        tap(240,450)
        capture('bag-conditions-zh')
        result={'status':'PASS','touch_checks':checks,'guard_checks':count,
                'production_model':True,'native_renderer':True}
        (ART/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(result),flush=True)
    finally:
        runtime.close()
