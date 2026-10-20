#!/usr/bin/env python3
"""Record real C tribute frames and verify single-transaction settlement.
Fixtures prepare offerings; production C input/model/rendering does the rest.
"""
from pathlib import Path
import importlib.util,io,json,shutil,tempfile
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_tribute_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/tribute-fx';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT)
captures=[];reports=[]
with tempfile.TemporaryDirectory(prefix='forge-tribute-') as temp:
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
    def tap(x,y):runtime.pointer(1,x,y,True);runtime.pointer(1,x,y,False)
    def capture(name):
        value=state();(ART/f'{name}.png').write_bytes(runtime.frame());assert state()['state_hash']==value['state_hash']
        captures.append({'stage':name,'state':value});(ART/f'{name}.json').write_text(json.dumps(value,ensure_ascii=False,indent=2))
    def start(code):
        action(code);before=state();tap(350,385)
        if not state()['fx']['tribute_stage']:
            assert state()['day']==before['day'] and state()['coins']==before['coins']
            tap(100,330)
            assert state()['day']==before['day'] and state()['fx']['tribute_stage']==0
            tap(350,385);tap(350,330)
        assert state()['fx']['tribute_stage']==1
    try:
        action(818);assert state()['day']==2 and state()['fx']['tribute_stage']==1
        for name,code,phase,day,anger in [('paid',811,'play',2,0),('short',812,'play',2,1),
                ('fatal',813,'lost',1,3),('clear',814,'clear',21,0),('pardon',815,'play',2,2),
                ('many',816,'play',2,0),('empty',817,'play',2,1)]:
            start(code);value=state();assert (value['phase'],value['day'],value['anger'])==(phase,day,anger)
            if code==811:assert value['coins']=='2430'
            if code in (812,813,815):assert value['coins']=='320'
            if code==817:assert value['coins']=='0'
            before=state()
            for _ in range(5):
                for code2 in (505,501,502,504,590):action(code2)
                tap(350,385);tap(300,450)
            assert state()['state_hash']==before['state_hash']
            step(9);capture(name+'-gather');step(16);assert state()['fx']['tribute_stage']==2;capture(name+'-response')
            runtime.control(1);frozen=runtime.frame();age=state()['fx']['tribute_age_ms'];step(30)
            assert state()['fx']['tribute_age_ms']==age and runtime.frame()==frozen;runtime.control(2)
            step(20);assert state()['fx']['tribute_stage']==3;capture(name+'-coins')
            step(23);assert state()['fx']['tribute_stage']==3
            step(1);assert state()['fx']['tribute_stage']==0 and state()['fx']['tribute_age_ms']==2300
            assert state()['coins']==before['coins'] and state()['day']==day
            action(819)
            reports.append({'case':name,'phase':phase,'day':day,'anger':anger,'coins':before['coins']})
        start(811);step(42);tap(240,431)
        assert state()['fx']['tribute_stage']==0 and state()['day']==2 and state()['coins']=='2430'
        counts={}
        for name,code in [('paid',811),('short',812)]:
            start(code);frames=[]
            for _ in range(38):
                frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
            palette=frames[23].quantize(colors=224)
            images=[frame.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for frame in frames]
            images[0].save(ART/(name+'.gif'),save_all=True,append_images=images[1:],duration=67,loop=0,optimize=False)
            counts[name]=len(frames)
        result={'status':'PASS','renderer':'shared native C','duration_ms':2300,'skip_after_ms':1400,
                'cases':reports,'captures':len(captures),'recorded_frames':counts,
                'checks':['persistent state equals original direct settlement','shortfall cancel preserves progress',
                    'single transaction despite repeated bell/touch/navigation','paid/short/fatal/clear/pardon/96 offerings/empty tribute',
                    'pause freezes animation and rendering','exact 2300ms expiry and optional skip after 1400ms',
                    'visual update/rendering preserve model hash and RNG','six-sprite sample bound','save reload never replays settlement']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','cases':len(reports),'captures':len(captures),'frames':counts},ensure_ascii=False))
    finally:runtime.close()
