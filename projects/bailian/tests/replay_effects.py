#!/usr/bin/env python3
"""Record real shared C renderer frames; Python only drives/encodes the fixture.
Forced outcomes exist only in fx_host_module.c, excluded from the shipping build.
"""
from pathlib import Path
import importlib.util,json,shutil,tempfile,io
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_fx_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/effects';ART.mkdir(exist_ok=True)
host.prepare_project_assets(ROOT)
frames=[];reports=[]
with tempfile.TemporaryDirectory(prefix='forge-fx-') as temp:
    project=Path(temp)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    (project/'tests').mkdir();shutil.copy2(ROOT/'tests/fx_host_module.c',project/'tests/fx_host_module.c')
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    manifest=json.loads((ROOT/'game.sim.json').read_text());manifest['sources'].remove('main/game_module.c');manifest['sources'].append('tests/fx_host_module.c')
    (project/'game.sim.json').write_text(json.dumps(manifest))
    build=Path(temp)/'build';build.mkdir();runtime=host.GenericHostRuntime(project,build)
    def step(n):
        for _ in range(n):runtime.step(False,False,False)
    def image():return runtime.frame()
    try:
        for name,code in [('heat',807),('success',803),('great',804),('damage',801),('hold',802),('break',800),('ready',806),('frenzy',805)]:
            runtime.action(code,True);runtime.action(code,False);step(1)
            if name=='frenzy':step(5)
            state=runtime.metadata();before=state['state_hash'];image();assert runtime.metadata()['state_hash']==before
            if name in ('success','great','damage','hold','break'):
                assert state['fx']['active'] and state['fx']['result']==code-800 and state['fx']['particles']>0
            if name=='frenzy':assert state['frenzy_ms']>0 and state['fx']['frenzy_pulse_ms']>0
            (ART/f'{name}.png').write_bytes(image());(ART/f'{name}.json').write_text(json.dumps(state,ensure_ascii=False,indent=2))
            reports.append({'case':name,'fx':state['fx'],'frenzy_ms':state['frenzy_ms']})
            if name=='great':
                runtime.control(1);age=state['fx']['age_ms'];step(30);assert runtime.metadata()['fx']['age_ms']==age;runtime.control(2)
            for _ in range(12 if name!='break' else 17):
                frames.append(Image.open(io.BytesIO(image())).convert('RGB'));step(2)
            if name=='break':
                runtime.action(809,True);runtime.action(809,False)
                assert not runtime.metadata()['fx']['active'] and runtime.metadata()['fx']['particles']==0
        palette=frames[0].quantize(colors=224)
        quantized=[frame.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for frame in frames]
        quantized[0].save(ART/'preview.gif',save_all=True,append_images=quantized[1:],duration=67,loop=0,optimize=False)
        result={'status':'PASS','renderer':'shared native C','outcomes':'forced only in non-shipping branch fixture','cases':reports,'assertions':['all five result branches','heating and ready glow','frenzy burst','pause freezes effects','visual update and rendering preserve game hash/RNG','effects expire'],'frames':len(frames)}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','cases':len(reports),'frames':len(frames),'preview':str(ART/'preview.gif')},ensure_ascii=False))
    finally:runtime.close()
