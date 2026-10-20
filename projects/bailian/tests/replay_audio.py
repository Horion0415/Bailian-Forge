#!/usr/bin/env python3
"""Exercise production Host audio gating and native pointer controls in isolation."""
from pathlib import Path
import ctypes,importlib.util,json,shutil,tempfile
ROOT=Path(__file__).resolve().parents[1];ENGINE=ROOT.parents[1]/'submodule/raylib-lite-engine'
spec=importlib.util.spec_from_file_location('audio_host',ENGINE/'host/run_game.py')
host=importlib.util.module_from_spec(spec);spec.loader.exec_module(host)
ART=ROOT/'tests/artifacts/audio';ART.mkdir(parents=True,exist_ok=True)
host.prepare_project_assets(ROOT)
with tempfile.TemporaryDirectory(prefix='forge-audio-') as scratch:
    project=Path(scratch)/'project';project.mkdir();shutil.copytree(ROOT/'main',project/'main')
    (project/'assets').mkdir();(project/'assets/generated').symlink_to(ROOT/'assets/generated',target_is_directory=True)
    # Test helpers are injected only into this temporary copy, never the manifest.
    source=(ROOT/'main/game_module.c').read_text()
    source=source.replace('    InitWindow(GAME_WIDTH','    audio_test_game=state->game;\n    InitWindow(GAME_WIDTH',1)
    source='#include "game_internal.h"\n#include <assert.h>\n#include <stdlib.h>\nstatic game_handle_t audio_test_game;\n'+source
    source+='''
void audio_test_emit(int effect){audio_test_game->feedback=(uint16_t)effect;}
void audio_test_pair(void){audio_test_game->swing.frenzy_sound=true;audio_test_game->feedback=FB_HIT;}
void audio_test_context(int phase,int tribute,int paused){
    game_handle_t g=audio_test_game;g->state.phase=(uint8_t)phase;
    g->tribute.active=tribute!=0;g->tribute.from_day=1;g->paused=paused!=0;
}
int audio_test_invariants(void){
    game_handle_t g=audio_test_game;uint32_t hash=game_hash(g),rng=g->state.rng;
    game_set_muted(g,true);assert(game_hash(g)==hash&&g->state.rng==rng);
    g->state.phase=PHASE_PLAY;forge_active(g)->heated=true;g->state.items[ITEM_CLOCK]=1;
    assert(forge_strike(g,RESULT_HOLD)&&forge_rewind(g)&&game_is_muted(g));
    size_t size=game_save_size();assert(size==24032);void *save=malloc(size);assert(save);
    assert(game_save_write(g,save,size));game_set_muted(g,false);
    assert(game_save_read(g,save,size)&&!game_is_muted(g));free(save);
    game_set_muted(g,true);game_reset(g);assert(game_is_muted(g));
    game_set_muted(g,false);g->feedback=0;return 1;
}
'''
    (project/'main/audio_test_module.c').write_text(source)
    manifest=json.loads((ROOT/'game.sim.json').read_text());manifest['sources'].remove('main/game_module.c');manifest['sources'].append('main/audio_test_module.c')
    (project/'game.sim.json').write_text(json.dumps(manifest));build=Path(scratch)/'build';build.mkdir()
    runtime=host.GenericHostRuntime(project,build)
    try:
        api=runtime.api;api.audio_test_invariants.argtypes=[];assert api.audio_test_invariants()==1
        api.audio_test_emit.argtypes=[ctypes.c_int];api.audio_test_context.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int]
        def tap(x,y):runtime.pointer(1,x,y,True);runtime.pointer(1,x,y,False)
        def state():return runtime.metadata()
        captures=[]
        def capture(name):
            before=state()['state_hash'];(ART/(name+'.png')).write_bytes(runtime.frame());assert before==state()['state_hash'];captures.append(name)
        # Gift/play/clear/lost, pause, and a live ritual all expose the setting.
        for phase,tribute,paused in [(0,0,0),(1,0,0),(2,0,0),(3,0,0),(1,0,1),(1,1,0),(1,1,1)]:
            api.audio_test_context(phase,tribute,paused);before=state();tap(420 if tribute else 109,40 if tribute else 25)
            assert state()['audio_muted'] and state()['state_hash']==before['state_hash'] and state()['paused']==before['paused']
            tap(420 if tribute else 109,40 if tribute else 25);assert not state()['audio_muted']
        api.audio_test_context(1,0,1);tap(240,385);assert state()['audio_muted'];capture('pause-muted')
        api.audio_test_context(1,0,0);capture('forge-muted')
        for effect in range(1,9):
            api.audio_test_emit(effect);runtime.step(False,False,False);assert state()['audio_events']==[]
        api.audio_test_pair();runtime.step(False,False,False);assert state()['audio_events']==[]
        tap(109,25);assert not state()['audio_muted'];runtime.step(False,False,False)
        assert state()['audio_events']==[]  # Discarded effects never replay.
        clips=set()
        for effect in range(1,9):
            api.audio_test_emit(effect);runtime.step(False,False,False);clips.add(state()['audio_events'][-1]['clip'])
        assert len(clips)==8
        tap(109,25);runtime.step(False,False,False);assert state()['audio_events']==[]
        api.audio_test_context(1,1,0);capture('tribute-muted')
        result={'status':'PASS','clips':sorted(clips),'captures':captures,
                'checks':['all eight clips suppressed while muted','unmute restores new effects without replay',
                          'settings work in gift/play/clear/lost/pause/tribute','no rule hash or RNG changes when toggling',
                          'rewind/new run/game save loading preserve independent audio preference','run payload remains 24032 bytes']}
        (ART/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
    finally:runtime.close()
