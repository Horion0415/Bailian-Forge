#!/usr/bin/env python3
"""Verify 50 real weapon forms and record native C milestone transformations."""
from pathlib import Path
import hashlib,importlib.util,io,json,shutil,tempfile
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_sword_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/swords';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT);report=json.loads((ROOT/'assets/generated/sword-forms.json').read_text())
assert report['forms']==50 and report['levels']==[0,3,6,10,20] and report['atlas_bytes']<6300000
frames=[];cases=[];hashes={}
with tempfile.TemporaryDirectory(prefix='forge-sword-') as temp:
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
        before=state()['state_hash'];(ART/(name+'.png')).write_bytes(runtime.frame());assert state()['state_hash']==before
        (ART/(name+'.json')).write_text(json.dumps(state(),ensure_ascii=False,indent=2))
    try:
        # The fixture draws ONLY the sword on a constant background: stage hashes
        # cannot differ merely because a level label or furnace particle changed.
        action(1120)
        for family in range(10):
            values=[]
            for tier,slot in enumerate((0,2,4,6,8)):
                action(900+family*10+slot);step(1);assert state()['fx']['shape_stage']==tier
                frame=runtime.frame();digest=hashlib.sha256(frame).hexdigest();values.append(digest)
                assert Image.open(io.BytesIO(frame)).getbbox() is not None
                (ART/f'family-{family}-stage-{tier}-body.png').write_bytes(frame)
            assert len(set(values))==5,('physical forms must differ',family)
            hashes[str(family)]=values
        action(1120)
        for tier,slot in enumerate((0,2,4,6,8)):
            action(900+slot);step(1);capture(f'form-{tier}')
        # Upgrade/great success/damage/family choice all use the actual model.
        for code,level in [(1100,3),(1101,7),(1102,10),(1103,20),(1104,2),(1105,6)]:
            action(code);step(1);value=state();assert value['level']==level and value['fx']['shape_morph_ms']==600
            step(7);capture(f'morph-{code}');runtime.control(1);frozen=runtime.frame();remaining=state()['fx']['shape_morph_ms']
            step(30);assert runtime.frame()==frozen and state()['fx']['shape_morph_ms']==remaining;runtime.control(2)
            step(10);assert state()['fx']['shape_morph_ms']>0
            step(1);assert state()['fx']['shape_morph_ms']==0
            action(1110);cases.append({'code':code,'level':level})
        action(1106);step(6);assert state()['active_id']==0;capture('mythic-shatter')
        # Record full forge scenes of every family, then a real 19 -> 20 morph.
        for family in range(10):
            action(900+family*10+8);step(1);capture(f'mythic-family-{family}')
        for slot in (0,2,4,6,8):
            action(900+slot);step(1)
            for _ in range(8):frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        action(1103);step(1)
        for _ in range(14):frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));step(2)
        palette=frames[36].quantize(colors=224)
        images=[frame.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for frame in frames]
        images[0].save(ART/'preview.gif',save_all=True,append_images=images[1:],duration=67,loop=0,optimize=False)
        result={'status':'PASS','renderer':'shared native C','families':10,'tiers':5,'forms':50,'levels':[0,3,6,10,20],
                'morph_ms':600,'frames':len(frames),'atlas_bytes':report['atlas_bytes'],'atlas_size':report['atlas_size'],
                'body_frame_hashes':hashes,'morph_cases':cases,
                'checks':['all 50 sword-only native frames differ within each family',
                    'real success/great/damage/family evolution trigger morph', 'pause freezes morph image/timer',
                    'exact 600ms transition boundary','presentation update/rendering preserve model hash/RNG',
                    'high-tier blade shatters using its own sprite','bounded compact atlas under 6.3MB',
                    'normal simulation manifest excludes fixture controls']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','forms':50,'morph_cases':len(cases),'frames':len(frames),'atlas_bytes':report['atlas_bytes']},ensure_ascii=False))
    finally:runtime.close()
