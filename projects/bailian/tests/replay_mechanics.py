#!/usr/bin/env python3
"""Native rule regressions, balance samples and real pointer UI flows."""
from pathlib import Path
import ctypes,importlib.util,json,shutil,statistics,tempfile

ROOT=Path(__file__).resolve().parents[1];ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('mechanics_host',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/mechanics';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT)
with tempfile.TemporaryDirectory(prefix='mechanics-') as scratch:
    project=Path(scratch)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    for name in ('mechanics_probe.c','mechanics_host_module.c'):shutil.copy2(ROOT/'tests'/name,project/'main'/name)
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    manifest=json.loads((ROOT/'game.sim.json').read_text());manifest['sources'].remove('main/game_module.c')
    manifest['sources']+=['main/mechanics_host_module.c','main/mechanics_probe.c']
    (project/'game.sim.json').write_text(json.dumps(manifest));build=Path(scratch)/'build';build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    try:
        api=runtime.api;buf=ctypes.create_string_buffer(4096)
        api.audit_cases.argtypes=[ctypes.c_char_p,ctypes.c_size_t]
        assert 0<api.audit_cases(buf,len(buf))<len(buf)
        result={'status':'PASS','cases':json.loads(buf.value),'profiles':[],'captures':[]}
        api.audit_extra.argtypes=[];assert api.audit_extra()==1
        api.audit_frenzy.argtypes=[ctypes.c_uint32,ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_char_p,ctypes.c_size_t]
        for profile,fps,tap,label in [(1,30,0,'base'),(2,30,0,'speed'),(3,30,0,'speed_duration'),(1,20,0,'base_20hz'),(1,30,500,'base_2taps')]:
            trials=[]
            for seed in range(1,1001):
                assert 0<api.audit_frenzy(seed,profile,fps,tap,buf,len(buf))<len(buf)
                trials.append(json.loads(buf.value))
            full=sorted(set(t['full_strikes'] for t in trials));values=sorted(int(t['value']) for t in trials)
            assert max(full)<=((7000 if profile==3 else 4000)*(145 if profile>=2 else 100)//64000+1)
            result['profiles'].append({'label':label,'trials':1000,'full_strikes':full,'contacts':sorted(set(t['hits'] for t in trials)),
                'value_p10_median_p90':[values[99],statistics.median(values),values[899]],'median_end_level':statistics.median(t['end_level'] for t in trials)})
        api.audit_campaign.argtypes=[ctypes.c_uint32,ctypes.c_int,ctypes.c_char_p,ctypes.c_size_t]
        result['campaigns']=[]
        for policy in (0,1):
            trials=[]
            for seed in range(1,129):
                assert 0<api.audit_campaign(seed,policy,buf,len(buf))<len(buf);trials.append(json.loads(buf.value))
            result['campaigns'].append({'policy':'cash_out_at_quota' if policy==0 else 'buy_relics_and_swords_finish_frenzy',
                'runs':128,'clears':sum(t['clear'] for t in trials),'median_final_day':statistics.median(t['day'] for t in trials),
                'limits':'Fixed synthetic policies, not player win rates; no rerolls, consumables or evolution choices.'})
        def action(code):runtime.action(code,True);runtime.action(code,False)
        def tap(x,y):runtime.pointer(1,x,y,True);runtime.pointer(1,x,y,False)
        def capture(name):
            before=runtime.metadata()['state_hash'];(ART/(name+'.png')).write_bytes(runtime.frame())
            assert before==runtime.metadata()['state_hash'];result['captures'].append(name)
        for i,name in enumerate(('energy','family','item','merchant','progress','codex','armed','rules')):
            action(4000+i);capture(name)
        action(4001);tap(300,370);assert runtime.metadata()['active_id']>0  # Confirm displayed split route.
        action(4002);tap(300,340);assert runtime.metadata()['screen']==0;capture('item-used')
        action(4003);before=runtime.metadata()['coins'];tap(300,340);assert runtime.metadata()['coins']!=before;capture('merchant-bought')
        action(4005);before=runtime.metadata()['relics'];tap(100,190);capture('codex-detail');tap(320,340)
        assert runtime.metadata()['relics']==before
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
        print(json.dumps(result,ensure_ascii=False),flush=True)
    finally:runtime.close()
