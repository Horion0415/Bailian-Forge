// SPDX-License-Identifier: Apache-2.0
/* Branch fixture, compiled only by replay_effects.py. Never in game.sim.json or
 * firmware. Outcomes are forced here to cover visual branches reproducibly. */
#include "game_internal.h"
#include "mosaico_game_module.h"
#include "mosaico_raylib_fast.h"
#include "host_asset_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {game_handle_t game;bool body_only;} fixture_t;
static int initialize(void *ptr,const char *root)
{
    fixture_t *f=ptr;const game_config_t c={480,480,0xb411a};
    mosaico_host_assets_set_root(root);
    if(!game_create(&c,&f->game))return-1;
    InitWindow(480,480,"FX branch fixture");SetTargetFPS(30);
    return game_load_assets(f->game)?0:-1;
}
static void shutdown(void *ptr){game_delete(((fixture_t*)ptr)->game);}
static void fresh(game_handle_t g)
{
    game_reset(g);g->state.phase=PHASE_PLAY;
    memset(g->state.acquired,0,sizeof(g->state.acquired));g->state.owned_count=0;
    g->notice_ms=0;g->notice[0]=0;g->feedback=0;
    forge_blade_t *b=forge_active(g);b->heated=true;b->level=8;
}
static void input(void *ptr,const mosaico_host_input_v1_t *e)
{
    game_handle_t g=((fixture_t*)ptr)->game;
    if(e->type==MOSAICO_HOST_INPUT_POINTER){game_set_pointer(g,e->x,e->y,e->pressed);return;}
    if(e->type==MOSAICO_HOST_INPUT_CONTROL){game_set_paused(g,e->code==1);return;}
    if(e->type!=MOSAICO_HOST_INPUT_ACTION||!e->pressed)return;
    if(e->code>=800&&e->code<=808){
        fresh(g);forge_blade_t *b=forge_active(g);
        if(e->code<805){g->animation_ms=167;assert(forge_strike(g,e->code-800));}
        else if(e->code==805){b->charge=10;game_action(g,501);}
        else if(e->code==806)b->charge=10;
        else{b->heated=false;b->level=0;if(e->code==807)game_action(g,501);}
    }else if(e->code==810){
        fresh(g);g->state.owned[0]=R_HOURGLASS;g->state.owned[1]=R_WICK;g->state.owned_count=2;
        forge_active(g)->charge=10;game_action(g,501);
    }else if(e->code>=811&&e->code<=817){
        fresh(g);forge_blade_t *b=forge_active(g);b->level=e->code==811?5:e->code==814?50:0;
        if(e->code==813||e->code==815)g->state.anger=2;
        if(e->code==814)g->state.day=21;
        if(e->code==815){g->state.owned[0]=R_PARDON;g->state.owned_count=1;}
        if(e->code!=817)assert(forge_move(g,g->state.active,true));
        if(e->code==816){
            /* Ninety-six offerings exercise the bounded six-sprite sample. */
            for(unsigned i=0;i<FORGE_BLADES;i++){
                g->state.blades[i]=g->state.blades[0];g->state.blades[i].id=i+1;
                g->state.blades[i].place=PLACE_TRIBUTE;g->state.blades[i].family=i%10;
            }
        }
        g->screen=SCREEN_TRIBUTE;
    }else if(e->code==818){
        fresh(g);forge_active(g)->level=5;assert(forge_move(g,g->state.active,true));
        game_handle_t plain=NULL;assert(game_create(&g->config,&plain));
        plain->state=g->state;
        assert(forge_tribute_start(g,false)&&forge_settle(plain,false));
        assert(memcmp(&g->state,&plain->state,sizeof(g->state))==0);
        game_delete(plain);
    }else if(e->code==819){
        uint32_t hash=game_hash(g),rng=g->state.rng;
        assert(g->tribute.sample_count<=FORGE_TRIBUTE_SAMPLES);
        if(g->tribute.count>FORGE_TRIBUTE_SAMPLES)assert(g->tribute.sample_count==FORGE_TRIBUTE_SAMPLES);
        forge_tribute_update(g,FORGE_TRIBUTE_DURATION_MS);
        assert(hash==game_hash(g)&&rng==g->state.rng);
        size_t size=game_save_size();void *save=malloc(size);assert(save);
        assert(game_save_write(g,save,size)&&game_save_read(g,save,size));free(save);
        assert(!g->tribute.active&&game_hash(g)==hash);
    }else if(e->code>=900&&e->code<1000){
        static const uint8_t levels[]={0,2,3,5,6,9,10,19,20,99};
        fresh(g);forge_blade_t *b=forge_active(g);unsigned at=e->code-900;
        b->family=at/10;b->level=levels[at%10];
    }else if(e->code>=1100&&e->code<=1104){
        static const uint8_t levels[]={2,5,9,19,3};
        fresh(g);forge_active(g)->level=levels[e->code-1100];forge_sword_update(g,0);
        g->animation_ms=167;
        assert(forge_strike(g,e->code==1104?RESULT_DAMAGE:e->code==1101?RESULT_GREAT:RESULT_SUCCESS));
    }else if(e->code==1105){
        fresh(g);forge_active(g)->level=6;forge_sword_update(g,0);assert(forge_family(g,5));
    }else if(e->code==1106){
        fresh(g);forge_active(g)->level=20;forge_active(g)->family=4;forge_sword_update(g,0);
        assert(forge_strike(g,RESULT_BREAK));
    }else if(e->code==1110){
        uint32_t hash=game_hash(g),rng=g->state.rng;forge_sword_update(g,650);
        assert(hash==game_hash(g)&&rng==g->state.rng);
    }else if(e->code==1120){((fixture_t*)ptr)->body_only=!((fixture_t*)ptr)->body_only;
    }else if(e->code>=1200&&e->code<=1207){
        fresh(g);forge_blade_t *b=forge_active(g);unsigned type=e->code-1200;b->level=0;
        memset(g->state.deck,0,sizeof(g->state.deck));g->state.deck[RESULT_HOLD]=1000;
        g->state.items[ITEM_CLOCK]=1;
        if(type>=4)b->frenzy_ms=4000;
        if(type==1||type==3||type==5||type==7)g->state.owned[g->state.owned_count++]=R_GRIP;
        if(type==2||type==3)g->state.owned[g->state.owned_count++]=R_LEATHER;
        if(type==6||type==7)g->state.owned[g->state.owned_count++]=R_BELLOWS;
    }else if(e->code==1211){forge_active(g)->frenzy_ms=0;forge_active(g)->charge=10;
    }else if(e->code==1212){
        assert(game_take_feedback(g)==FB_FRENZY);assert(game_take_feedback(g)==FB_HIT);assert(game_take_feedback(g)==FB_NONE);
    }else if(e->code==1213){assert(forge_strike(g,RESULT_BREAK));
    }else if(e->code==1214){
        fresh(g);forge_active(g)->level=0;memset(g->state.deck,0,sizeof(g->state.deck));g->state.deck[RESULT_HOLD]=1000;
        game_handle_t plain=NULL;assert(game_create(&g->config,&plain));plain->state=g->state;
        unsigned hits=0;
        for(unsigned ms=0;ms<2400;ms++){
            forge_swing_request(g);game_update_elapsed(g,1);game_update_elapsed(plain,1);
            if(g->state.hits>hits){hits=g->state.hits;assert(forge_strike(plain,-1));
                assert(memcmp(&g->state,&plain->state,sizeof(g->state))==0);}
        }
        assert(hits>=12);game_delete(plain);
    }else if(e->code==1215){
        fresh(g);forge_blade_t *b=forge_active(g);b->level=2;b->frenzy_ms=4000;
        memset(g->state.deck,0,sizeof(g->state.deck));g->state.deck[RESULT_SUCCESS]=1000;
        g->state.owned[0]=R_GRIP;g->state.owned[1]=R_BELLOWS;g->state.owned_count=2;
    }else if(e->code==1216){
        char *snapshot=malloc(1024);assert(snapshot);assert(game_write_json(g,snapshot,1024)>0);free(snapshot);
    }else if(e->code>1300&&e->code<=3300){game_update_elapsed(g,(uint32_t)(e->code-1300));
    }else if(e->code==809){
        uint32_t hash=game_hash(g),rng=g->state.rng;
        forge_fx_update(g,2000);
        assert(hash==game_hash(g)&&rng==g->state.rng);
    }else game_action(g,e->code);
}
static void update(void *ptr){game_update(((fixture_t*)ptr)->game);}
static int render(void *ptr)
{
    game_handle_t g=((fixture_t*)ptr)->game;uint32_t hash=game_hash(g);
    if(((fixture_t*)ptr)->body_only){
        forge_blade_t *b=forge_active(g);assert(b);BeginDrawing();ClearBackground((Color){20,14,10,255});
        forge_sword_draw(g,b->family,b->level,(Rectangle){27,110,384,230},(Vector2){0,0},0,WHITE,false);
        EndDrawing();assert(hash==game_hash(g));return 0;
    }
    bool ok=game_render(g);assert(hash==game_hash(g));return ok?0:-1;
}
static uint32_t hash(const void *ptr){return game_hash(((const fixture_t*)ptr)->game);}
static int json(const void *ptr,char *out,size_t capacity)
{return game_write_json(((const fixture_t*)ptr)->game,out,capacity);}
static const mosaico_game_module_v1_t module={
    .descriptor={MOSAICO_HOST_GAME_ABI_V1,"bailian-fx-fixture","Native C effect branches",480,480,30,1},
    .state_size=sizeof(fixture_t),.initialize=initialize,.shutdown=shutdown,.input=input,
    .update=update,.render=render,.state_hash=hash,.state_json=json};
const mosaico_game_module_v1_t *mosaico_game_module_v1(void){return&module;}
