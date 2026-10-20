#!/usr/bin/env python3
"""Exercise actual shared C input, model and renderer via the engine Host ABI.
No Python gameplay model or Python scene renderer is implemented here.
"""
from pathlib import Path
from decimal import Decimal
import importlib.util
import json
import tempfile

ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('bailian_host_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ARTIFACTS=ROOT/'tests/artifacts';ARTIFACTS.mkdir(exist_ok=True)
rules=json.loads((ROOT/'assets_src/rules.json').read_text())

def money(text):
    for unit,n in [('万亿',10**12),('京',10**16),('亿',10**8),('万',10**4)]:
        if text.endswith(unit):return int(Decimal(text[:-len(unit)])*n)
    return int(Decimal(text))

host.prepare_project_assets(ROOT)
with tempfile.TemporaryDirectory(prefix='native-',dir=ARTIFACTS) as build:
    runtime=host.GenericHostRuntime(ROOT,Path(build));events=[];frame=0;sfx_clips=set()
    def steps(count):
        global frame
        for _ in range(count):
            runtime.step(False,False,False);frame+=1
            sfx_clips.update(event['clip'] for event in runtime.metadata().get('audio_events',[]))
    def tap(x,y):
        # Browser mouse uses track 1, unlike the board's normalized track 0.
        events.append({'frame':frame,'type':'pointer','track':1,'x':x,'y':y,'pressed':True})
        runtime.pointer(1,x,y,True);steps(1)
        events.append({'frame':frame,'type':'pointer','track':1,'x':x,'y':y,'pressed':False})
        runtime.pointer(1,x,y,False);steps(1)
    def capture(name):
        (ARTIFACTS/f'{name}.png').write_bytes(runtime.frame())
        (ARTIFACTS/f'{name}.json').write_text(json.dumps(runtime.metadata(),ensure_ascii=False,indent=2))
    try:
        assert runtime.metadata()['phase']=='gift';capture('gift')
        tap(80,200);assert runtime.metadata()['phase']=='play'
        tap(125,180);steps(40)
        for attempt in range(60):
            state=runtime.metadata()
            if state['active_id']==0:
                tap(180,450);tap(110,160);tap(80,385)
                assert runtime.metadata()['active_id']>0,'no selectable replacement sword'
                tap(125,180);steps(40)
            state=runtime.metadata()
            if money(state['value'])>=money(state['quota']):break
            tap(260,250);steps(5)
            if runtime.metadata()['active_id']>0:capture('strike')
            steps(18)
        else:raise AssertionError('could not craft enough tribute in bounded run')
        capture('forge')
        tap(120,355);assert runtime.metadata()['active_id']==0
        tap(420,450);assert runtime.metadata()['screen']==3;capture('tribute')
        assert money(runtime.metadata()['tribute'])>=money(runtime.metadata()['quota'])
        tap(350,385);state=runtime.metadata();assert state['day']==2 and state['anger']==0
        assert money(state['coins'])>=1800
        assert state['fx']['tribute_stage']==1
        steps(70);assert runtime.metadata()['fx']['tribute_stage']==0
        tap(180,450);tap(110,160);tap(390,385)
        assert runtime.metadata()['credits']==1
        tap(300,450);state=runtime.metadata();assert state['screen']==2
        affordable=[i for i,id in enumerate(state['offers']) if id>=0 and rules['relics'][id]['cost']<=state['credits']]
        assert affordable,'expected an affordable day-two relic'
        tap(affordable[0]*160+80,210);tap(340,340)
        assert runtime.metadata()['relics']==2 and runtime.metadata()['credits']==0
        capture('shop')
        tap(180,450);capture('rack')
        tap(35,20);assert runtime.metadata()['paused'];tick=runtime.metadata()['tick']
        steps(60);assert runtime.metadata()['tick']==tick
        tap(240,170);assert not runtime.metadata()['paused']
        capture('final')
        scenario={'events':events}
        (ARTIFACTS/'touch-flow.json').write_text(json.dumps(scenario,indent=2))
        assert {'select','heat','bell'}<=sfx_clips and {'hit','great','damage','break'}&sfx_clips,sfx_clips
        result={'audio_clips':sorted(sfx_clips),'frames':frame,'events':len(events),'state':runtime.metadata(),'native_renderer':True,'input_track':1}
        (ARTIFACTS/'flow-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','frames':frame,'events':len(events),'day':state['day'],'checks':['gift','heat','strike','value','tribute','day2','trade','relic purchase','pause','Chinese/raster captures']},ensure_ascii=False))
    finally:runtime.close()
