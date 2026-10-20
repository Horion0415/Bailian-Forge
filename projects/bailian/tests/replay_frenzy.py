#!/usr/bin/env python3
"""Check and record a complete frenzy using production C update/render paths.
Only the non-shipping fixture supplies a charged blade and relic combinations.
"""
from pathlib import Path
import importlib.util,io,json,shutil,tempfile
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_frenzy_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/frenzy';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT)
frames=[];captures=[]
with tempfile.TemporaryDirectory(prefix='forge-frenzy-') as temp:
    project=Path(temp)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    (project/'tests').mkdir();shutil.copy2(ROOT/'tests/fx_host_module.c',project/'tests/fx_host_module.c')
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    manifest=json.loads((ROOT/'game.sim.json').read_text())
    manifest['sources'].remove('main/game_module.c');manifest['sources'].append('tests/fx_host_module.c')
    (project/'game.sim.json').write_text(json.dumps(manifest));build=Path(temp)/'build';build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    def action(code):runtime.action(code,True);runtime.action(code,False)
    def step(n):
        for _ in range(n):runtime.step(False,False,False)
    def state():return runtime.metadata()
    def enter(code):
        action(code)
        for _ in range(10):
            step(1)
            if state()['hits']:return
        raise AssertionError('frenzy entry strike did not land')
    def capture(name):
        value=state();(ART/f'{name}.png').write_bytes(runtime.frame())
        assert state()['state_hash']==value['state_hash']
        (ART/f'{name}.json').write_text(json.dumps(value,ensure_ascii=False,indent=2))
        captures.append({'stage':name,'frenzy_ms':value['frenzy_ms'],'fx':value['fx']})
    try:
        enter(805)
        assert state()['frenzy_ms']==4000 and state()['fx']['frenzy_total_ms']==4000
        step(7);capture('entry')
        runtime.control(1);frozen=runtime.frame();before=state();step(45)
        assert runtime.frame()==frozen and state()['frenzy_ms']==before['frenzy_ms']
        assert state()['fx']['frenzy_age_ms']==before['fx']['frenzy_age_ms'];runtime.control(2)
        # The following 113 frames complete exactly 120 updates / 4000ms.
        step(38);capture('orbit');step(60);capture('fading');step(14)
        assert 0<state()['frenzy_ms']<=34
        step(1);assert state()['frenzy_ms']==0;capture('finished')
        # Repeated real strikes must boost particles without restarting the aura.
        enter(805);last_age=0
        for index in range(5):
            step(15);action(501);step(5);value=state()
            assert value['hits']==index+2 and value['frenzy_ms']>0
            assert value['fx']['frenzy_hit'] and value['fx']['particles']==64
            assert value['fx']['frenzy_age_ms']>last_age and value['fx']['frenzy_total_ms']==4000
            last_age=value['fx']['frenzy_age_ms']
            if index==1:capture('strike')
        action(809)  # Presentation-only time must leave game hash/RNG unchanged.
        # Both duration relics keep the original 7-second rule and matching fade.
        enter(810);assert state()['frenzy_ms']==7000 and state()['fx']['frenzy_total_ms']==7000
        step(209);assert 0<state()['frenzy_ms']<=34
        step(1);assert state()['frenzy_ms']==0
        # Record ready -> entry -> two more real hammer strikes -> smooth expiry.
        action(806)
        for _ in range(5):frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        action(501)
        for index in range(73):
            if index in (12,24):action(501)
            frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        assert state()['frenzy_ms']==0
        # A palette taken from the burst preserves its gold highlights as well as the room.
        palette=frames[12].quantize(colors=224)
        images=[frame.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for frame in frames]
        images[0].save(ART/'preview.gif',save_all=True,append_images=images[1:],duration=67,loop=0,optimize=False)
        result={'status':'PASS','renderer':'shared native C','base_duration_ms':4000,'extended_duration_ms':7000,
                'entry_ms':900,'fade_ms':500,'particle_limit':64,'frames':len(frames),'captures':captures,
                'checks':['exact 4s and 7s duration boundaries','pause freezes aura and gameplay timer',
                          'five repeated real strikes do not restart entry or extend frenzy',
                          'frenzy strikes use bounded 64-particle pool','native rendering and visual updates preserve model hash/RNG',
                          'complete ready/entry/orbit/hit/fade/expiry recording','test controls excluded from shipping manifest']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','frames':len(frames),'captures':len(captures),'preview':str(ART/'preview.gif')},ensure_ascii=False))
    finally:runtime.close()
