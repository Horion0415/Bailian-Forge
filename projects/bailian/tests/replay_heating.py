#!/usr/bin/env python3
"""Validate and record the shared native C heating flow (no Python renderer)."""
from pathlib import Path
import importlib.util,json,shutil,tempfile,io
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_heat_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/heating';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT)
frames=[];captures=[]
with tempfile.TemporaryDirectory(prefix='forge-heat-') as temp:
    project=Path(temp)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    (project/'tests').mkdir();shutil.copy2(ROOT/'tests/fx_host_module.c',project/'tests/fx_host_module.c')
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    manifest=json.loads((ROOT/'game.sim.json').read_text());manifest['sources'].remove('main/game_module.c');manifest['sources'].append('tests/fx_host_module.c')
    (project/'game.sim.json').write_text(json.dumps(manifest));build=Path(temp)/'build';build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    def action(code):runtime.action(code,True);runtime.action(code,False)
    def step(n):
        for _ in range(n):runtime.step(False,False,False)
    def state():return runtime.metadata()
    def capture(name):
        before=state()['state_hash'];(ART/f'{name}.png').write_bytes(runtime.frame());assert state()['state_hash']==before
        value=state();(ART/f'{name}.json').write_text(json.dumps(value,ensure_ascii=False,indent=2));captures.append({'stage':name,'fx':value['fx']})
    def tap(x,y):
        runtime.pointer(1,x,y,True);step(1);runtime.pointer(1,x,y,False);step(1)
    try:
        action(808);assert state()['fx']['heat_stage']==1 and not state()['fx']['heated'];capture('cold')
        action(501);assert state()['fx']['heat_ms']==1200
        action(501);assert state()['fx']['heat_ms']==1200 and state()['hits']==0
        step(6);assert state()['fx']['heat_stage']==2;capture('insert')
        step(12);assert state()['fx']['heat_stage']==3 and state()['fx']['heat_ms']==600;capture('warm')
        runtime.control(1);paused=runtime.frame();step(45)
        assert state()['fx']['heat_ms']==600 and runtime.frame()==paused
        runtime.control(2);step(10);assert state()['fx']['heat_stage']==4;capture('return')
        step(7);assert state()['fx']['heat_ms']==34 and not state()['fx']['heated']
        step(1);assert state()['fx']['heat_ms']==0 and state()['fx']['heated'] and state()['fx']['heat_ready_ms']==1100;capture('ready')
        assert state()['hits']==0
        step(33);assert state()['fx']['heat_stage']==6 and state()['fx']['heat_ready_ms']==0;capture('hot')
        action(501);step(7);assert state()['hits']==1
        # Selecting another sword cancels heating without warming that sword.
        action(807);step(6);action(521)
        assert state()['fx']['heat_ms']==0 and not state()['fx']['heated'];step(60)
        assert not state()['fx']['heated']
        # The existing take-out-to-tribute action remains available while warming.
        action(807);step(3);tap(120,355)
        assert state()['active_id']==0 and state()['fx']['heat_ms']==0 and state()['tribute']!='0';step(40)
        assert state()['fx']['heat_stage']==0
        # Record a complete uninterrupted cycle, using real C frames only.
        action(808)
        for _ in range(5):frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        action(501)
        for _ in range(43):frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        palette=frames[0].quantize(colors=224)
        images=[frame.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for frame in frames]
        images[0].save(ART/'preview.gif',save_all=True,append_images=images[1:],duration=67,loop=0,optimize=False)
        result={'status':'PASS','renderer':'shared native C','duration_ms':1200,'ready_feedback_ms':1100,'captures':captures,'frames':len(frames),'checks':['cold/insert/warm/return/ready/hot','exact heat boundary at 1200ms','pause freezes timer and image','repeated input does not restart or auto-strike','switch blade cancels heating','take out to tribute cancels heating','rendering preserves model hash']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(json.dumps({'status':'PASS','frames':len(frames),'stages':len(captures),'preview':str(ART/'preview.gif')},ensure_ascii=False))
    finally:runtime.close()
