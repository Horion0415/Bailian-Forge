// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include <string.h>

void forge_swing_cancel(game_handle_t g,bool stop_current)
{
    g->swing.queued=false;
    if(stop_current){g->pending_strike=false;g->animation_ms=0;
        g->swing.blade_id=0;g->swing.frenzy_sound=false;}
}
void forge_swing_reset(game_handle_t g)
{
    memset(&g->swing,0,sizeof(g->swing));forge_swing_cancel(g,true);g->cooldown_ms=0;
}
static void begin(game_handle_t g,const forge_blade_t *blade)
{
    bool inside=blade->frenzy_ms>0||blade->charge>=10;
    unsigned speed=100+(forge_has(&g->state,R_GRIP)?15:0)+
        (inside&&forge_has(&g->state,R_BELLOWS)?30:0)+(!inside&&forge_has(&g->state,R_LEATHER)?20:0);
    unsigned base=inside?FORGE_SWING_FRENZY_MS:FORGE_SWING_NORMAL_MS;
    unsigned cycle=(base*100+speed-1)/speed;
    if(cycle<FORGE_SWING_MIN_MS)cycle=FORGE_SWING_MIN_MS;
    g->swing.cycle_ms=(uint16_t)cycle;g->swing.contact_ms=(uint16_t)((cycle*30+99)/100);
    g->swing.blade_id=blade->id;g->swing.queued=false;
    if(!blade->frenzy_ms&&blade->charge>=10)g->swing.frenzy_sound=true;
    g->heating.ready_ms=0;g->animation_ms=1;g->pending_strike=true;g->cooldown_ms=(uint16_t)cycle;
}
void forge_swing_request(game_handle_t g)
{
    if(g->paused||g->modal||g->tribute.active||g->state.phase!=PHASE_PLAY||g->screen!=SCREEN_FORGE)return;
    forge_blade_t *b=forge_active(g);
    if(!b){
        if(forge_can_rewind(g)){forge_notice(g,"碎刀仍可回溯，点击下方发条回溯");return;}
        g->screen=SCREEN_RACK;forge_notice(g,"先在剑架选择一把剑");return;
    }
    if(g->heat_ms)return;
    if(b->level>=99){forge_notice(g,"已到+99，可将名刀留存或进贡");return;}
    if(g->cooldown_ms||g->pending_strike){
        if(g->cooldown_ms&&g->cooldown_ms<=FORGE_SWING_BUFFER_MS&&b->heated&&g->swing.blade_id==b->id)
            g->swing.queued=true;
        return;
    }
    if(!b->heated){forge_heat_start(g);return;}
    begin(g,b);
}
void forge_swing_update(game_handle_t g,uint32_t ms)
{
    forge_swing_t *s=&g->swing;forge_blade_t *b=forge_active(g);
    /* Navigation cancels animation/input, never time already owed by a hit. */
    if(g->cooldown_ms)g->cooldown_ms=(uint16_t)(g->cooldown_ms>ms?g->cooldown_ms-ms:0);
    if(g->state.phase!=PHASE_PLAY||g->screen!=SCREEN_FORGE||g->modal||g->tribute.active){forge_swing_cancel(g,true);return;}
    if(b&&s->blade_id&&b->id!=s->blade_id)forge_swing_cancel(g,true);
    if(!b){s->queued=false;g->pending_strike=false;
        if(!forge_fx_break_visible(g)){forge_swing_cancel(g,true);return;}}
    if(b)s->blade_id=b->id;
    unsigned cycle=s->cycle_ms?s->cycle_ms:FORGE_SWING_LEGACY_MS;
    unsigned contact=s->contact_ms?s->contact_ms:FORGE_SWING_LEGACY_CONTACT_MS;
    if(g->animation_ms){unsigned age=g->animation_ms+ms;
        g->animation_ms=(uint16_t)(age>cycle?cycle:age);
        if(g->pending_strike&&age>contact){
            g->pending_strike=false;(void)forge_strike(g,-1);
        }
        if(age>=cycle)g->animation_ms=0;
    }
    b=forge_active(g);
    if(!b||b->id!=s->blade_id||!b->heated||b->level>=99)s->queued=false;
    /* One buffered press can start one new wind-up. Never advance that wind-up
     * in this same update: even a long frame cannot resolve multiple strikes. */
    if(s->queued&&!g->cooldown_ms&&!g->pending_strike&&b){s->queued=false;begin(g,b);}
}
