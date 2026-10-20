/* Review-only probes. Linked to an isolated copy of the shipping C game. */
#include "game_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static game_handle_t fresh(uint32_t seed)
{
    game_handle_t g=NULL;
    game_config_t config={480,480,seed};
    assert(game_create(&config,&g));
    g->state.phase=PHASE_PLAY;
    g->state.owned_count=0;
    memset(g->state.owned,-1,sizeof(g->state.owned));
    forge_active(g)->heated=true;
    return g;
}
static void equip(game_handle_t g,int id)
{
    assert(g->state.owned_count<8);
    g->state.owned[g->state.owned_count++]=(int8_t)id;
}
static unsigned count(game_handle_t g)
{
    unsigned n=0;
    for(unsigned i=0;i<FORGE_BLADES;i++)n+=g->state.blades[i].place!=PLACE_FREE;
    return n;
}
static void money(forge_money_t v,char text[109])
{forge_money_text(&v,text,109,false);}

int audit_cases(char *out,size_t capacity)
{
    game_handle_t a=fresh(123),b=fresh(123);
    equip(a,R_IDOL);equip(a,R_MIRROR);equip(b,R_IDOL);
    forge_active(a)->level=forge_active(b)->level=2;
    assert(forge_strike(a,RESULT_SUCCESS)&&forge_strike(a,RESULT_SUCCESS));
    assert(forge_strike(b,RESULT_SUCCESS)&&forge_strike(b,RESULT_SUCCESS));
    char mirror[109],control[109];
    money(forge_value(&a->state,forge_active(a)),mirror);
    money(forge_value(&b->state,forge_active(b)),control);
    assert(strcmp(mirror,"2142")==0&&strcmp(control,"1863")==0);
    game_delete(a);game_delete(b);

    a=fresh(123);equip(a,R_HEADLESS);
    forge_blade_t *blade=forge_active(a);blade->family=1;blade->level=19;
    blade->frenzy_ms=1;a->state.max_level=19;
    blade->peak=forge_value(&a->state,blade);a->state.peak=blade->peak;
    game_update_elapsed(a,1);
    unsigned headless_level=blade->level,headless_max=a->state.max_level;
    unsigned headless_unlock=a->state.unlocks;
    assert(headless_level==20&&headless_max==20&&headless_unlock==2);
    char actual_peak[109],display_peak[109];
    money(forge_value(&a->state,blade),actual_peak);money(a->state.peak,display_peak);
    blade->level=98;blade->frenzy_ms=1;
    game_update_elapsed(a,1);
    unsigned level99_achievement=a->state.achievements;
    assert(blade->level==99&&(level99_achievement&1));
    game_delete(a);

    a=fresh(123);
    memset(a->state.deck,0,sizeof(a->state.deck));a->state.deck[RESULT_HOLD]=1000;
    forge_pointer(a,250,250);game_update_elapsed(a,60);
    assert(a->state.hits==1&&a->cooldown_ms==140);
    forge_pointer(a,180,450);forge_pointer(a,60,450);
    forge_pointer(a,250,250);game_update_elapsed(a,60);
    unsigned bypass_hits=a->state.hits;
    assert(bypass_hits==1);
    game_delete(a);
    b=fresh(123);
    memset(b->state.deck,0,sizeof(b->state.deck));b->state.deck[RESULT_HOLD]=1000;
    forge_pointer(b,250,250);game_update_elapsed(b,60);
    forge_pointer(b,250,250);game_update_elapsed(b,60);
    unsigned normal_hits=b->state.hits;
    assert(normal_hits==1);game_delete(b);
    a=fresh(123);
    memset(a->state.deck,0,sizeof(a->state.deck));a->state.deck[RESULT_HOLD]=1000;
    forge_pointer(a,250,250);game_update_elapsed(a,60);
    assert(a->state.items[ITEM_CLOCK]==0);
    forge_pointer(a,350,355); /* Failed rewind still cancels cooldown. */
    forge_pointer(a,250,250);game_update_elapsed(a,60);
    unsigned failed_rewind_hits=a->state.hits;
    assert(failed_rewind_hits==1);game_delete(a);

    a=fresh(123);forge_blade_t prototype=*forge_active(a);
    for(unsigned i=0;i<FORGE_BLADES;i++){
        a->state.blades[i]=prototype;a->state.blades[i].id=i+1;
        a->state.blades[i].place=PLACE_RACK;
    }
    a->state.active=-1;a->state.next_id=97;
    assert(forge_settle(a,true));
    unsigned backlog_before=a->state.backlog;
    assert(backlog_before==3&&count(a)==96);
    assert(forge_trade(a,0));game_update_elapsed(a,1000);
    unsigned after_space=count(a),backlog_after=a->state.backlog;
    assert(after_space==96&&backlog_after==2);game_delete(a);

    a=fresh(123);forge_blade_t *weapon=forge_active(a);weapon->level=8;
    uint16_t reference[5];forge_tickets(&a->state,weapon,reference);
    forge_money_t reference_value=forge_value(&a->state,weapon);
    unsigned equal_families=0;
    for(int family=1;family<=6;family++){
        weapon->family=0;assert(forge_family(a,family));uint16_t odds[5];
        forge_tickets(&a->state,weapon,odds);forge_money_t value=forge_value(&a->state,weapon);
        equal_families+=!memcmp(odds,reference,sizeof(odds))&&!forge_money_compare(&value,&reference_value);
    }
    assert(equal_families<6);game_delete(a);

    a=fresh(123);assert(forge_settle(a,true));
    forge_active(a)->level=10;assert(forge_move(a,a->state.active,true));
    assert(forge_settle(a,false));unsigned anger_after_paid=a->state.anger;
    assert(anger_after_paid==1);assert(forge_settle(a,true));assert(forge_settle(a,true));
    assert(a->state.phase==PHASE_LOST);game_delete(a);

    return snprintf(out,capacity,
        "{\"mirror_idol\":{\"with_mirror\":\"%s\",\"without_mirror\":\"%s\"},"
        "\"headless\":{\"level\":%u,\"recorded_max_level\":%u,\"unlock_bits\":%u,\"value\":\"%s\",\"recorded_peak\":\"%s\",\"achievement_bits_at_99\":%u},"
        "\"cooldown_navigation\":{\"elapsed_ms\":120,\"hits_with_navigation\":%u,\"hits_without_navigation\":%u,\"hits_with_failed_rewind\":%u},"
        "\"inventory_full\":{\"backlog_before\":%u,\"swords_after_freeing_slot\":%u,\"backlog_after\":%u},"
        "\"equal_normal_sword_routes\":%u,\"anger_after_paid_day\":%u}",
        mirror,control,headless_level,headless_max,headless_unlock,actual_peak,display_peak,
        level99_achievement,bypass_hits,normal_hits,failed_rewind_hits,backlog_before,after_space,backlog_after,equal_families,anger_after_paid);
}

/* One isolated frenzy. All outcomes use shipping RNG and ticket rules. No
 * forced successes, shops or campaign policy. Legacy profile overrides only
 * the windup/cooldown to 640/160ms, keeping the same native C rules. */
int audit_frenzy(uint32_t seed,int profile,unsigned fps,unsigned tap_ms,char *out,size_t capacity)
{
    game_handle_t g=fresh(seed);forge_blade_t *blade=forge_active(g);
    blade->level=5;blade->charge=0;blade->frenzy_ms=profile==3?7000:4000;
    if(profile>=2){equip(g,R_GRIP);equip(g,R_BELLOWS);}
    if(profile==3){equip(g,R_HOURGLASS);equip(g,R_WICK);}
    unsigned t=0,remainder=0,next_tap=0;
    while(blade->frenzy_ms>0){
        remainder+=1000;unsigned ms=remainder/fps;remainder%=fps;
        if(ms>=(unsigned)blade->frenzy_ms){
            forge_swing_cancel(g,true);game_update_elapsed(g,(unsigned)blade->frenzy_ms);break;
        }
        if(t>=next_tap){
            forge_swing_request(g);
            if(profile==0&&g->pending_strike&&g->swing.cycle_ms!=640){
                g->swing.cycle_ms=640;g->swing.contact_ms=160;g->cooldown_ms=640;
            }
            next_tap=t+(tap_ms?tap_ms:1);
        }
        game_update_elapsed(g,ms);t+=ms;
        /* The buffer can arm a new windup from inside the update itself. */
        if(profile==0&&g->pending_strike&&g->swing.cycle_ms!=640){
            g->swing.cycle_ms=640;g->swing.contact_ms=160;g->cooldown_ms=640;
        }
        assert(forge_active(g)&&g->state.breaks==0);
    }
    char value[109];money(forge_value(&g->state,blade),value);
    int n=snprintf(out,capacity,"{\"hits\":%u,\"full_strikes\":%u,\"end_level\":%u,\"value\":\"%s\"}",g->state.hits,blade->hits,blade->level,value);
    game_delete(g);return n;
}

int audit_extra(void)
{
    game_handle_t a=fresh(321),b=fresh(321);
    /* Genuine v1-sized payload, including backlog migration, then a v2 roundtrip. */
    size_t legacy=offsetof(forge_state_t,pending_levels);assert(legacy==23544);
    a->state.backlog=3;a->state.pending_levels[0]=3;
    void *bytes=malloc(game_save_size());assert(bytes);
    assert(game_save_write(a,bytes,game_save_size()));
    assert(game_save_read(b,bytes,legacy));assert(!memcmp(&a->state,&b->state,legacy));
    assert(b->state.pending_levels[0]==3&&b->state.frenzy_energy[0]==0);
    assert(game_save_write(b,bytes,game_save_size())&&game_save_read(a,bytes,game_save_size()));
    assert(!memcmp(&a->state,&b->state,sizeof(a->state)));
    b->state.pending_levels[1]=1;assert(!game_save_read(a,&b->state,sizeof(b->state)));
    free(bytes);game_delete(a);game_delete(b);

    a=fresh(321);forge_blade_t prototype=*forge_active(a);
    for(unsigned i=0;i<FORGE_BLADES;i++){a->state.blades[i]=prototype;a->state.blades[i].id=i+1;a->state.blades[i].place=PLACE_RACK;}
    a->state.active=-1;a->state.backlog=2;a->state.pending_levels[2]=1;a->state.pending_levels[7]=1;
    assert(forge_trade(a,0));assert(a->state.blades[0].level==7&&a->state.backlog==1);
    assert(forge_trade(a,1));assert(a->state.blades[1].level==2&&a->state.backlog==0);game_delete(a);

    a=fresh(321);forge_blade_t *blade=forge_active(a);blade->frenzy_ms=4000;
    a->state.items[ITEM_CHALK]=1;a->state.items[ITEM_CLOCK]=1;
    memset(a->state.deck,0,sizeof(a->state.deck));a->state.deck[RESULT_HOLD]=1000;
    assert(forge_use_item(a,ITEM_CHALK));uint32_t rng=a->state.rng;
    assert(forge_strike(a,-1));assert(a->last_result==RESULT_REFINE&&blade->hits==0&&a->state.hits==1);
    assert(a->state.rng==rng&&a->state.chalk&&a->state.items[ITEM_CHALK]==0);
    forge_energy_update(&a->state,blade,640);assert(forge_energy_percent(&a->state,blade)==100);
    assert(forge_strike(a,-1));assert(blade->hits==1&&!a->state.chalk);
    assert(forge_rewind(a));assert(a->state.chalk&&a->state.items[ITEM_CLOCK]==0&&forge_energy_percent(&a->state,forge_active(a))==100);
    game_delete(a);

    a=fresh(321);b=fresh(321);equip(a,R_GRIP);equip(a,R_BELLOWS);equip(b,R_GRIP);equip(b,R_BELLOWS);
    forge_active(a)->family=forge_active(b)->family=5;forge_active(a)->frenzy_ms=forge_active(b)->frenzy_ms=4000;
    game_update_elapsed(a,500);for(unsigned i=0;i<500;i++)game_update_elapsed(b,1);
    assert(!memcmp(&a->state,&b->state,sizeof(a->state)));game_delete(a);game_delete(b);

    a=fresh(321);blade=forge_active(a);blade->level=3;blade->frenzy_ms=1;equip(a,R_IDOL);equip(a,R_HEADLESS);
    game_update_elapsed(a,1);assert(blade->level==4&&blade->milestone==1&&a->state.max_level==4);game_delete(a);

    a=fresh(321);blade=forge_active(a);blade->level=8;uint16_t normal[5],odds[5];forge_tickets(&a->state,blade,normal);
    blade->family=1;forge_tickets(&a->state,blade,odds);assert(odds[0]<normal[0]);
    blade->family=2;forge_tickets(&a->state,blade,odds);assert(odds[4]>normal[4]);
    blade->family=3;blade->last=RESULT_DAMAGE;forge_tickets(&a->state,blade,odds);assert(odds[3]>normal[3]);
    blade->family=4;blade->level=10;forge_money_t valued=forge_value(&a->state,blade);blade->family=0;forge_money_t plain=forge_value(&a->state,blade);assert(forge_money_compare(&valued,&plain)>0);
    blade->family=5;blade->level=0;assert(forge_strike(a,RESULT_HOLD));assert(blade->charge==2);
    blade->family=6;assert(forge_strike(a,RESULT_GREAT));assert(blade->level==3);
    blade->family=7;forge_money_t base=blade->base;assert(forge_strike(a,RESULT_HOLD));assert(forge_money_compare(&blade->base,&base)>0);
    blade->family=8;forge_money_t flat=blade->flat;assert(forge_strike(a,RESULT_DAMAGE));assert(forge_money_compare(&blade->flat,&flat)>0);
    blade->family=9;blade->peak=forge_money_u64(10000);blade->charge=0;assert(forge_strike(a,RESULT_BREAK));assert(!forge_money_zero(&a->state.scrap));game_delete(a);
    return 1;
}

static int relic_score(int id){
    switch(id){
    case R_CORNER:case R_PHOENIX:case R_WIRE:case R_SPLICE:return 100;
    case R_HOLY:case R_DAWN:case R_CHALICE:case R_HOOP:case R_LOCKET:return 85;
    case R_WATCH:case R_CHAIN:case R_GRIP:case R_WICK:case R_HOURGLASS:case R_BELLOWS:return 75;
    case R_GLASS:case R_HEART:case R_WIND:case R_SLEEP:case R_KING:case R_BADGE:return 70;
    default:return 40;
    }
}
int audit_campaign(uint32_t seed,int policy,char *out,size_t capacity){
    game_handle_t g=NULL;game_config_t config={480,480,seed};assert(game_create(&config,&g));
    unsigned gift=0;for(unsigned i=1;i<3;i++)if(relic_score(g->state.offers[i])>relic_score(g->state.offers[gift]))gift=i;
    assert(forge_choose_gift(g,(int)gift));unsigned frames=0;
    while(g->state.phase==PHASE_PLAY&&g->state.day<=21){
        if(policy&&g->state.day>1){
            /* Keep three working blades; spend spare swords on offered relics. */
            for(unsigned i=0;i<FORGE_BLADES&&count(g)>3;i++)if(g->state.blades[i].place==PLACE_RACK&&!g->state.blades[i].hits)forge_trade(g,(int)i);
            for(unsigned purchase=0;purchase<3&&g->state.owned_count<8;purchase++){
                int chosen=-1;
                for(unsigned i=0;i<3;i++){int id=g->state.offers[i];if(id<0||forge_relics[id].cost>g->state.credits)continue;
                    if(chosen<0||relic_score(id)>relic_score(g->state.offers[chosen]))chosen=(int)i;
                }
                if(chosen<0)break;
                assert(forge_buy_relic(g,chosen));
            }
            for(unsigned slot=0;slot<g->state.merchant_count;slot++)while(g->state.merchant[slot].kind<2&&g->state.merchant[slot].stock&&count(g)<5){
                if(!forge_buy_merchant(g,slot))break;
            }
        }
        for(unsigned attempt=0;attempt<FORGE_BLADES;attempt++){
            forge_money_t need=forge_quota(&g->state),offered=forge_tribute(&g->state);
            if(forge_money_compare(&offered,&need)>=0)break;
            forge_money_sub(&need,&offered);int best=-1;forge_money_t best_value=forge_money_u64(0);
            for(unsigned i=0;i<FORGE_BLADES;i++)if(g->state.blades[i].place==PLACE_RACK||g->state.blades[i].place==PLACE_ANVIL){
                forge_money_t value=forge_value(&g->state,&g->state.blades[i]);
                if(best<0||forge_money_compare(&value,&best_value)>0){best=(int)i;best_value=value;}
            }
            if(best<0)break;
            assert(forge_select(g,best));forge_active(g)->heated=true;
            for(unsigned step=0;step<6000&&forge_active(g);step++){
                forge_blade_t *blade=forge_active(g);forge_money_t value=forge_value(&g->state,blade);
                if(forge_money_compare(&value,&need)>=0&&(!policy||blade->frenzy_ms<=0)){assert(forge_move(g,g->state.active,true));break;}
                forge_swing_request(g);game_update_elapsed(g,(++frames%3)?33:34);
            }
        }
        assert(forge_settle(g,true));
    }
    int n=snprintf(out,capacity,"{\"clear\":%s,\"day\":%u,\"paid\":%u,\"hits\":%u,\"relics\":%u}",g->state.phase==PHASE_CLEAR?"true":"false",g->state.day,g->state.paid,g->state.hits,g->state.owned_count);
    game_delete(g);return n;
}
