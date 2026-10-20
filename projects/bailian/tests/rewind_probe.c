// SPDX-License-Identifier: Apache-2.0
/* Test-only, linked to the production model, input, controller and renderer. */
#include "game_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern game_handle_t rewind_test_game;

static void fresh(game_handle_t g)
{
    game_reset(g);g->state.phase=PHASE_PLAY;g->screen=SCREEN_FORGE;
    forge_active(g)->heated=true;g->state.items[ITEM_CLOCK]=2;
    memset(g->state.deck,0,sizeof(g->state.deck));g->state.deck[RESULT_HOLD]=1000;
}
static void tap(game_handle_t g,int x,int y)
{
    game_set_pointer(g,x,y,true);game_set_pointer(g,x,y,false);
}
static unsigned snapshots(game_handle_t g)
{
    forge_state_t *before=malloc(sizeof(*before));assert(before);
    unsigned count=0;
    for(int scenario=0;scenario<10;scenario++){
        int result=scenario%5;
        fresh(g);forge_active(g)->level=scenario<5?19:98;forge_active(g)->family=4;
        static const int relics[]={R_ASH,R_CHALICE,R_HOLY,R_DAWN,R_GRUDGE,R_OSSUARY,R_UNDERTAKER,R_IDOL};
        for(unsigned i=0;i<8;i++)g->state.owned[i]=(int8_t)relics[i];
        g->state.owned_count=8;g->state.chalk=g->state.powder=true;
        game_set_muted(g,true);assert(game_set_language(g,GAME_LANGUAGE_EN));
        *before=g->state;assert(forge_strike(g,result));
        before->rng=g->state.rng;before->revision=g->state.revision+1;before->items[ITEM_CLOCK]--;
        assert(forge_rewind(g));assert(memcmp(before,&g->state,sizeof(*before))==0);
        assert(game_is_muted(g)&&game_get_language(g)==GAME_LANGUAGE_EN);
        *before=g->state;assert(!forge_rewind(g));assert(memcmp(before,&g->state,sizeof(*before))==0);
        count++;
    }
    free(before);return count;
}
int rewind_test_report(char *out,size_t capacity)
{
    game_handle_t g=rewind_test_game;unsigned restored=snapshots(g);
    fresh(g);g->state.items[ITEM_CLOCK]=0;forge_swing_request(g);
    tap(g,350,355);game_update_elapsed(g,60);
    bool failed_no_cancel=g->state.hits==1;

    fresh(g);forge_swing_request(g);game_update_elapsed(g,120);forge_swing_request(g);
    assert(g->swing.queued&&g->state.hits==1);assert(forge_use_item(g,ITEM_CLOCK));
    bool queue_cleared=!g->swing.queued&&!g->pending_strike&&!g->swing.frenzy_sound;
    game_update_elapsed(g,1);game_update_elapsed(g,500);
    bool no_auto=g->state.hits==0;

    fresh(g);forge_active(g)->level=8;assert(forge_strike(g,RESULT_BREAK));
    assert(forge_fx_break_visible(g)&&forge_fx_particle_count(g)>0);assert(forge_rewind(g));
    bool break_clear=!forge_fx_break_visible(g)&&forge_fx_particle_count(g)==0&&!g->fx.active;

    fresh(g);forge_active(g)->level=2;forge_sword_update(g,0);
    assert(forge_strike(g,RESULT_SUCCESS));forge_sword_update(g,0);assert(g->shape.morphing);
    assert(forge_rewind(g));bool shape_clear=!g->shape.morphing&&g->shape.level==2;

    fresh(g);assert(forge_strike(g,RESULT_HOLD));tap(g,60,450);
    bool same_tab=g->undo_valid&&forge_rewind(g);

    fresh(g);assert(forge_strike(g,RESULT_HOLD));tap(g,440,20);assert(g->screen==SCREEN_ITEMS);
    tap(g,200,180);assert(g->modal==MODAL_ITEM&&g->detail_item==ITEM_CLOCK);tap(g,330,340);
    bool bag=g->state.hits==0&&g->state.items[ITEM_CLOCK]==1&&g->screen==SCREEN_FORGE;

    fresh(g);forge_active(g)->charge=10;forge_swing_request(g);game_update_elapsed(g,42);
    assert(g->undo_valid&&g->swing.frenzy_sound);assert(forge_rewind(g));
    bool entry_clear=game_take_feedback(g)==FB_SELECT&&game_take_feedback(g)==FB_NONE;
    bool entry_restore=forge_active(g)->charge==10&&forge_active(g)->frenzy_ms==0;

    fresh(g);forge_active(g)->frenzy_ms=4000;forge_frenzy_update(g,0);
    assert(forge_strike(g,-1));game_update_elapsed(g,4100);assert(forge_active(g)->frenzy_ms==0);
    assert(forge_rewind(g));bool frenzy_restore=forge_active(g)->frenzy_ms==4000;
    bool frenzy_view=g->fx.frenzy.active&&g->fx.frenzy.total_ms==4000&&g->fx.frenzy.age_ms>=900;

    bool twins=false;
    for(unsigned seed=1;seed<=1000&&!twins;seed++){
        fresh(g);g->state.rng=seed;g->state.owned[0]=R_TWINS;g->state.owned_count=1;
        uint32_t next=g->state.next_id;assert(forge_strike(g,RESULT_HOLD));
        if(g->state.next_id!=next){assert(forge_rewind(g));twins=g->state.next_id==next&&g->state.blades[3].place==PLACE_FREE;}
    }
    assert(twins);

    fresh(g);forge_blade_t prototype=*forge_active(g);
    for(unsigned i=0;i<FORGE_BLADES;i++){g->state.blades[i]=prototype;g->state.blades[i].id=i+1;g->state.blades[i].place=PLACE_RACK;}
    g->state.active=0;g->state.blades[0].place=PLACE_ANVIL;g->state.next_id=97;
    g->state.pending_levels[7]=1;g->state.backlog=1;
    assert(forge_strike(g,RESULT_BREAK));game_update_elapsed(g,1);
    assert(g->state.backlog==0&&g->state.blades[0].id==97);assert(forge_rewind(g));
    bool backlog=g->state.backlog==1&&g->state.pending_levels[7]==1&&forge_active(g)->id==1&&g->state.next_id==97;

    unsigned invalidated=0;
    for(unsigned action=0;action<10;action++){
        fresh(g);forge_active(g)->level=6;assert(forge_strike(g,RESULT_HOLD));
        if(action==0)assert(forge_select(g,1));
        if(action==1)assert(forge_move(g,g->state.active,true));
        if(action==2)assert(forge_trade(g,g->state.active));
        if(action==3){g->state.items[ITEM_CHALK]=1;assert(forge_use_item(g,ITEM_CHALK));}
        if(action==4)assert(forge_family(g,1));
        if(action==5)assert(forge_settle(g,true));
        if(action==6){g->state.coins=forge_money_u64(1000000);g->state.merchant_count=1;
            g->state.merchant[0]=(forge_offer_t){.kind=2,.id=ITEM_CLOCK,.stock=1};assert(forge_buy_merchant(g,0));}
        if(action==7){g->state.day=2;g->state.coins=forge_money_u64(1000000);assert(forge_reroll(g));}
        if(action==8){size_t size=game_save_size();void *save=malloc(size);assert(save);
            assert(game_save_write(g,save,size)&&game_save_read(g,save,size));free(save);}
        if(action==9)tap(g,300,450);
        uint16_t clocks=g->state.items[ITEM_CLOCK];
        assert(!forge_rewind(g)&&g->state.items[ITEM_CLOCK]==clocks);invalidated++;
    }
    int n=snprintf(out,capacity,
        "{\"snapshots\":%u,\"invalidations\":%u,\"failed_action_preserved\":%s,\"queue_cleared\":%s,\"no_auto_strike\":%s,\"break_fx_cleared\":%s,\"shape_synced\":%s,\"same_tab_preserved\":%s,\"bag_clock\":%s,\"entry_sound_cleared\":%s,\"entry_state_restored\":%s,\"frenzy_state_restored\":%s,\"frenzy_view_synced\":%s,\"clone_retracted\":%s,\"backlog_restored\":%s}",
        restored,invalidated,failed_no_cancel?"true":"false",queue_cleared?"true":"false",no_auto?"true":"false",
        break_clear?"true":"false",shape_clear?"true":"false",same_tab?"true":"false",bag?"true":"false",
        entry_clear?"true":"false",entry_restore?"true":"false",frenzy_restore?"true":"false",frenzy_view?"true":"false",twins?"true":"false",backlog?"true":"false");
    fresh(g);return n;
}
