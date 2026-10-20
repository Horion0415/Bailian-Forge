// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include "forge_font.h"
#include "mosaico_raylib_fast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool game_create(const game_config_t *config,game_handle_t *out)
{
    if(!out)return false;
    *out=NULL;
    if(!config||config->width!=480||config->height!=480)return false;
    game_handle_t g=calloc(1,sizeof(*g));
    if(!g)return false;
    g->undo=calloc(1,sizeof(*g->undo));
    if(!g->undo){free(g);
        return false;
        }
    g->config=*config;
    forge_new(g,config->seed?config->seed:UINT32_C(0xb411a),false);
    forge_fx_reset(g);
    forge_heat_reset(g);
    g->selected=255;
    *out=g;
    return true;
}
void game_delete(game_handle_t g)
{
    if(!g)return;
    if(g->assets_loaded){UnloadMosaicoAtlas(g->room);
        UnloadMosaicoAtlas(g->hammer);
        UnloadMosaicoAtlas(g->swords);
        UnloadMosaicoAtlas(g->relic_atlas);
        }
    free(g->undo);
    free(g->save_buffer);
    free(g);
}
bool game_load_assets(game_handle_t g)
{
    if(!g)return false;
    if(g->assets_loaded)return true;
    g->room=LoadMosaicoAtlas("room.atlas");
    g->hammer=LoadMosaicoAtlas("hammer.atlas");
    g->swords=LoadMosaicoAtlas("swords.atlas");
    g->relic_atlas=LoadMosaicoAtlas("relics.atlas");
    bool font_ready=mosaico_game_asset_open("font.mask",&g->font)==ESP_OK&&g->font.size==FORGE_FONT_BYTES;
    g->assets_loaded=g->room.texture.id&&g->hammer.texture.id&&g->swords.texture.id&&g->relic_atlas.texture.id&&font_ready;
    if(!g->assets_loaded){UnloadMosaicoAtlas(g->room);
        UnloadMosaicoAtlas(g->hammer);
        UnloadMosaicoAtlas(g->swords);
        UnloadMosaicoAtlas(g->relic_atlas);
        }
    return g->assets_loaded;
}
void game_reset(game_handle_t g){if(g){forge_new(g,g->state.rng^UINT32_C(0x85ebca6b),true);
        g->pending_strike=false;g->animation_ms=0;g->paused=false;forge_fx_reset(g);forge_heat_reset(g);}
    }
void game_set_paused(game_handle_t g,bool paused){if(g){if(paused)forge_swing_cancel(g,false);g->paused=paused;}
    }
void game_set_muted(game_handle_t g,bool muted){if(g)g->audio_muted=muted;}
bool game_is_muted(game_handle_t g){return g&&g->audio_muted;}
void game_set_pointer(game_handle_t g,int32_t x,int32_t y,bool down)
{
    if(!g)return;
    g->pointer_x=x<0?0:x>479?479:x;
    g->pointer_y=y<0?0:y>479?479:y;
    bool edge=down&&!g->pointer_down;
    g->pointer_down=down;
    if(edge)forge_pointer(g,g->pointer_x,g->pointer_y);
}
static unsigned elapsed(game_handle_t g){g->ms_remainder+=1000;
    unsigned ms=g->ms_remainder/30;
    g->ms_remainder%=30;
    return ms;
    }
void game_update(game_handle_t g)
{
    if(!g)return;
    game_update_elapsed(g,elapsed(g));
}
void game_update_elapsed(game_handle_t g,uint32_t elapsed_ms)
{
    if(!g||g->paused)return;
    g->tick++;
    unsigned ms=elapsed_ms>10000?10000:elapsed_ms;
    forge_fx_update(g,ms);
    forge_state_t *s=&g->state;
    s->elapsed_ms+=ms;
    if(g->notice_ms)g->notice_ms=(uint16_t)(g->notice_ms>ms?g->notice_ms-ms:0);
    for(unsigned i=0;i<FORGE_BLADES;i++){
        forge_blade_t *b=&s->blades[i];
        if(!b->place||b->frenzy_ms<=0)continue;
        forge_energy_update(s,b,ms<(unsigned)b->frenzy_ms?ms:(unsigned)b->frenzy_ms);
        b->frenzy_ms=(int16_t)(b->frenzy_ms>(int)ms?b->frenzy_ms-(int)ms:0);
        if(!b->frenzy_ms){
            s->frenzy_energy[i]=0;s->frenzy_fraction[i]=0;
            if(s->active==(int)i){forge_swing_cancel(g,true);forge_notice(g,"入神结束 · 下一锤恢复破碎风险");}
            if(forge_has(s,R_HEADLESS)&&b->level<99){b->level++;forge_remember(g,b);}
            forge_changed(g,false);
            }
    }
    forge_heat_update(g,ms);
    forge_swing_update(g,ms);
    forge_sword_update(g,ms);
    forge_claim_supply(g);
}
bool game_render(game_handle_t g)
{
    if(!g)return false;
    BeginDrawing();
    if(!MosaicoFastFrameAvailable()){EndDrawing();
        return false;
        }
    forge_render(g);
    EndDrawing();
    return true;
}
game_snapshot_t game_read(game_handle_t g)
{
    game_snapshot_t v={0};
    if(!g)return v;
    forge_state_t *s=&g->state;
    forge_blade_t *b=forge_active(g);
    v.tick=g->tick;
    v.pointer_x=g->pointer_x;
    v.pointer_y=g->pointer_y;
    v.pointer_down=g->pointer_down;
    v.paused=g->paused;
    v.day=s->day;
    v.phase=s->phase;
    v.screen=g->screen;
    v.anger=s->anger;
    v.relic_count=s->owned_count;
    v.last_result=g->last_result;
    for(unsigned i=0;i<FORGE_BLADES;i++)if(s->blades[i].place)v.blade_count++;
    if(b){v.level=b->level;
        v.charge=b->charge;
        v.active_id=b->id;
        v.frenzy_ms=b->frenzy_ms>0?(uint16_t)b->frenzy_ms:0;
        }
    return v;
}
uint32_t game_hash(game_handle_t g)
{
    /* Persistent state is zero-initialized fixed-width POD, without pointers. */
    uint32_t h=2166136261U;
    const uint8_t *p=(const uint8_t*)&g->state;
    for(size_t i=0;i<sizeof(g->state);i++)h=(h^p[i])*16777619U;
    return h;
}
int game_write_json(game_handle_t g,char *out,size_t capacity)
{
    if(!g||!out||!capacity)return-1;
    game_snapshot_t v=game_read(g);
    forge_money_t value=forge_value(&g->state,forge_active(g)),quota=forge_quota(&g->state),tribute=forge_tribute(&g->state);
    char coins[32],price[32],need[32],offered[32];
    forge_money_text(&g->state.coins,coins,sizeof(coins),true);
    forge_money_text(&value,price,sizeof(price),true);
    forge_money_text(&quota,need,sizeof(need),true);
    forge_money_text(&tribute,offered,sizeof(offered),true);
    static const char *const phases[]={"gift","play","clear","lost"};
    int n=snprintf(out,capacity,"{\"phase\":\"%s\",\"tick\":%lu,\"day\":%u,\"screen\":%u,\"anger\":%u,\"blades\":%u,\"active_id\":%lu,\"level\":%u,\"charge\":%u,\"frenzy_ms\":%u,\"coins\":\"%s\",\"value\":\"%s\",\"tribute\":\"%s\",\"quota\":\"%s\",\"credits\":%u,\"relics\":%u,\"offers\":[%d,%d,%d],\"hits\":%lu,\"breaks\":%lu,\"paused\":%s,\"state_hash\":\"%08lx\"}",phases[v.phase],(unsigned long)v.tick,v.day,v.screen,v.anger,v.blade_count,(unsigned long)v.active_id,v.level,v.charge,v.frenzy_ms,coins,price,offered,need,g->state.credits,v.relic_count,g->state.offers[0],g->state.offers[1],g->state.offers[2],(unsigned long)g->state.hits,(unsigned long)g->state.breaks,v.paused?"true":"false",(unsigned long)game_hash(g));
    if(n<1||(size_t)n>=capacity)return-1;
    size_t used=(size_t)n-1;
    forge_blade_t *b=forge_active(g);
    n=snprintf(out+used,capacity-used,",\"audio_muted\":%s,\"fx\":{\"active\":%s,\"result\":%u,\"age_ms\":%u,\"particles\":%u,\"frenzy_pulse_ms\":%u,\"frenzy_age_ms\":%u,\"frenzy_total_ms\":%u,\"frenzy_hit\":%s,\"heat_ms\":%u,\"heat_stage\":%u,\"heat_ready_ms\":%u,\"heated\":%s,\"tribute_stage\":%u,\"tribute_age_ms\":%u,\"tribute_paid\":%s,\"shape_stage\":%u,\"shape_morph_ms\":%u,\"swing_ms\":%u,\"contact_ms\":%u,\"swing_queued\":%s,\"swing_pending\":%s}}",
        game_is_muted(g)?"true":"false",g->fx.active?"true":"false",g->fx.result,g->fx.age_ms,forge_fx_particle_count(g),g->fx.frenzy_pulse_ms,
        g->fx.frenzy.age_ms,g->fx.frenzy.total_ms,g->fx.frenzy.hit?"true":"false",
        g->heat_ms,forge_heat_phase(g),g->heating.ready_ms,b&&b->heated?"true":"false",
        forge_tribute_stage(g),g->tribute.age_ms,g->tribute.paid?"true":"false",b?forge_sword_stage(b->level):0,
        g->shape.morphing?FORGE_SWORD_MORPH_MS-g->shape.age_ms:0,
        g->swing.cycle_ms,g->swing.contact_ms,g->swing.queued?"true":"false",g->pending_strike?"true":"false");
    return n<0||(size_t)n>=capacity-used?-1:(int)(used+(size_t)n);
}
size_t game_save_size(void){return sizeof(forge_state_t);
    }
bool game_save_write(game_handle_t g,void *out,size_t capacity){if(!g||!out||capacity<sizeof(g->state))return false;
    memcpy(out,&g->state,sizeof(g->state));
    return true;
    }
bool game_save_read(game_handle_t g,const void *data,size_t size)
{
    size_t legacy_size=offsetof(forge_state_t,pending_levels);
    if(!g||!data||(size!=sizeof(forge_state_t)&&size!=legacy_size))return false;
    const forge_state_t *s=data;
    if(!s->day||s->day>999||s->phase>PHASE_LOST||s->anger>3||s->owned_count>8||s->merchant_count>10||s->active<-1||s->active>=FORGE_BLADES)return false;
    for(unsigned i=0;i<s->owned_count;i++)if(s->owned[i]<0||s->owned[i]>=R_COUNT)return false;
    for(unsigned i=0;i<3;i++)if(s->offers[i]<-1||s->offers[i]>=R_COUNT)return false;
    for(unsigned i=0;i<FORGE_BLADES;i++){
        const forge_blade_t *b=&s->blades[i];
        if(b->place>PLACE_TRIBUTE||b->family>=10||b->level>99||b->charge>10||b->frenzy_ms<0||b->frenzy_ms>7000)return false;
        for(unsigned j=0;j<FORGE_MONEY_LIMBS;j++)if(b->base.v[j]>=1000000000U||b->perm.v[j]>=1000000000U||b->flat.v[j]>=1000000000U||b->peak.v[j]>=1000000000U)return false;
    }
    if(s->active>=0&&s->blades[s->active].place!=PLACE_ANVIL)return false;
    if(size!=legacy_size){
        unsigned queued=0;for(unsigned i=0;i<100;i++)queued+=s->pending_levels[i];
        if(queued!=s->backlog)return false;
        for(unsigned i=0;i<FORGE_BLADES;i++)if(s->frenzy_energy[i]>FORGE_ENERGY_MAX||s->frenzy_fraction[i]>=100)return false;
    }
    memcpy(&g->state,data,size);
    if(size==legacy_size){
        memset((uint8_t*)&g->state+legacy_size,0,sizeof(g->state)-legacy_size);
        g->state.pending_levels[0]=g->state.backlog;
    }
    g->undo_valid=false;
    g->screen=SCREEN_FORGE;
    g->modal=MODAL_NONE;
    g->pending_strike=false;g->animation_ms=0;forge_fx_reset(g);forge_heat_reset(g);
    forge_notice(g,"已恢复上一次锻造进度");
    return true;
}
uint32_t game_revision(game_handle_t g){return g?g->state.revision:0;
    }
int game_take_feedback(game_handle_t g){if(!g)return 0;
    /* Entry and contact may fall in one frame at high speed. Preserve both,
     * emitting the entry tone first and the contact result on the next read. */
    if(g->swing.frenzy_sound){g->swing.frenzy_sound=false;return FB_FRENZY;}
    int event=g->feedback;
    g->feedback=0;
    return event;
    }
