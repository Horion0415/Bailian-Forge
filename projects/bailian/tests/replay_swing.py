#!/usr/bin/env python3
"""Measure native swing timing, single-input buffering and safe cancellation."""
from pathlib import Path
import importlib.util,io,json,shutil,tempfile
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('forge_swing_runner',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/swing';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT);profiles=[];frames=[]
with tempfile.TemporaryDirectory(prefix='forge-swing-') as temp:
    project=Path(temp)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    (project/'tests').mkdir();shutil.copy2(ROOT/'tests/fx_host_module.c',project/'tests/fx_host_module.c')
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    manifest=json.loads((ROOT/'game.sim.json').read_text());manifest['sources'].remove('main/game_module.c');manifest['sources'].append('tests/fx_host_module.c')
    (project/'game.sim.json').write_text(json.dumps(manifest));build=Path(temp)/'build';build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    def action(code):runtime.action(code,True);runtime.action(code,False)
    def advance(ms):action(1300+ms)
    def state():return runtime.metadata()
    def tap(x,y):runtime.pointer(1,x,y,True);runtime.pointer(1,x,y,False)
    def arm():
        action(1200);action(501);advance(130);assert state()['hits']==1
        action(501);assert state()['fx']['swing_queued']
    try:
        for kind,cycle in enumerate((200,174,167,149,140,122,108,100)):
            action(1200+kind);action(501);value=state();contact=(cycle*30+99)//100
            assert value['fx']['swing_ms']==cycle and value['fx']['contact_ms']==contact
            advance(contact-1);assert state()['hits']==0
            advance(1);assert state()['hits']==1
            times=[contact];elapsed=contact
            while len(times)<6:
                action(501);previous=state()['hits'];advance(1);elapsed+=1
                if state()['hits']>previous:times.append(elapsed)
            assert [b-a for a,b in zip(times,times[1:])]==[cycle]*5,(kind,times)
            action(1216);profiles.append({'case':kind,'cycle_ms':cycle,'contact_ms':contact,'hit_times_ms':times,'nominal_hits_per_second':round(1000/cycle,3)})
        action(1200);action(501);advance(60);assert state()['hits']==1
        for _ in range(20):action(501)
        assert not state()['fx']['swing_queued']
        advance(60)
        for _ in range(100):action(501)
        assert state()['fx']['swing_queued'];advance(79);assert state()['hits']==1
        advance(1);assert state()['fx']['swing_pending'] and not state()['fx']['swing_queued'] and state()['hits']==1
        advance(59);assert state()['hits']==1
        advance(1);assert state()['hits']==2
        advance(1000);assert state()['hits']==2
        action(1200);action(501);advance(60);tap(60,450);action(501);advance(60)
        assert state()['hits']==1 and not state()['fx']['swing_pending']
        # A held finger is one press; motion events do not create extra strikes.
        action(1200);runtime.pointer(1,260,250,True)
        for _ in range(30):runtime.pointer(1,261,250,True);advance(20)
        runtime.pointer(1,261,250,False);assert state()['hits']==1
        arm();runtime.control(1);remaining=state()['fx']['swing_pending'];advance(1000)
        assert not state()['fx']['swing_queued'] and state()['hits']==1
        runtime.control(2);advance(1000);assert state()['hits']==1
        arm();action(502);advance(1);action(520);advance(500);assert state()['hits']==1
        arm();action(502);action(520);advance(500);assert state()['hits']==1
        arm();action(521);action(520);advance(500);assert state()['hits']==1
        arm();action(521);advance(500);assert state()['hits']==1 and state()['active_id']!=1
        arm();tap(120,355);advance(500);assert state()['hits']==1 and state()['active_id']==0
        arm();tap(350,355);advance(1000);assert state()['hits']==0 and not state()['fx']['swing_queued']
        arm();action(1213);advance(500);assert state()['hits']==2 and not state()['fx']['swing_queued']
        arm();action(4);advance(500);assert state()['hits']==0 and state()['phase']=='gift'
        arm();action(504);action(505);tap(350,330);advance(1000)
        assert state()['day']==2 and state()['hits']==1 and not state()['fx']['swing_queued']
        # One delayed update resolves at most one hit; buffered input only arms the next.
        action(1200);action(501);advance(2000);assert state()['hits']==1
        arm();advance(2000);assert state()['hits']==1 and state()['fx']['swing_pending']
        advance(60);assert state()['hits']==2
        action(1207);action(1211);action(501);advance(1000)
        assert state()['hits']==1;action(1212)
        action(1214)  # Bytewise controller-vs-direct-model comparison at every hit.
        # Record real native 30Hz frames, with at most one actual tap per frame.
        action(1215);hit_log=[];previous=0
        for index in range(75):
            tap(260,250);runtime.step(False,False,False);value=state()
            if value['hits']>previous:hit_log.append({'frame':index,'hits':value['hits']});previous=value['hits']
            before=value['state_hash'];frames.append(Image.open(io.BytesIO(runtime.frame())).convert('RGB'));assert state()['state_hash']==before
        assert 24<=state()['hits']<=25,(state()['hits'],hit_log)
        palette=frames[30].quantize(colors=224)
        images=[f.quantize(palette=palette,dither=getattr(Image,'Dither',Image).NONE) for f in frames]
        images[0].save(ART/'preview.gif',save_all=True,append_images=images[1:],duration=33,loop=0,optimize=False)
        result={'status':'PASS','renderer':'shared native C','profiles':profiles,'recorded_frames':len(frames),'rapid_hits':state()['hits'],'rapid_hit_log':hit_log,
                'checks':['eight base/frenzy/relic timing profiles','exact contact and six-hit intervals','single 80ms end-window buffer',
                    'early taps are not queued','100 repeated taps cannot backlog more than one hit','held finger/motion produces one press',
                    'pause/navigation/sword switch/tribute/rewind/break/reset/settlement cancel buffer','at most one result per delayed update',
                    'entry sound and hit sound both retained','controller state equals direct model at every tested hit','1024-byte device snapshot fits',
                    'native 30Hz recording approaches 10 hits per second with maximum modifiers','rendering preserves model hash']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
        print(json.dumps({'status':'PASS','profiles':len(profiles),'rapid_hits':result['rapid_hits'],'frames':len(frames)},ensure_ascii=False))
    finally:runtime.close()
