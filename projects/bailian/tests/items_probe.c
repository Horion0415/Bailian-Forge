// SPDX-License-Identifier: Apache-2.0
/* Test-only fixture; production touch handlers, model and renderer stay intact. */
#include "game_internal.h"
#include <assert.h>
#include <string.h>

extern game_handle_t items_test_game;

void items_test_prepare(unsigned item)
{
    assert(item<ITEM_COUNT);
    game_handle_t g=items_test_game;
    game_reset(g);
    g->state.phase=PHASE_PLAY;
    g->state.day=9;
    g->state.anger=2;
    g->state.coins=forge_money_u64(1000000000);
    g->state.merchant_count=1;
    g->state.merchant[0]=(forge_offer_t){.kind=2,.id=(uint8_t)item,.stock=1};
    forge_active(g)->heated=true;
    memset(g->state.deck,0,sizeof(g->state.deck));
    g->state.deck[RESULT_HOLD]=1000;
    g->screen=SCREEN_MERCHANT;
}

unsigned items_test_read(unsigned field)
{
    game_handle_t g=items_test_game;
    if(field<ITEM_COUNT)return g->state.items[field];
    switch(field){
    case 7:return g->state.scale|(g->state.chalk<<1)|(g->state.powder<<2);
    case 8:return g->state.extra_supply;
    case 9:return g->state.anger;
    case 10:return g->state.incense_used;
    default:return 0;
    }
}

int items_test_strike(int result)
{
    return forge_strike(items_test_game,result);
}

unsigned items_test_guards(void)
{
    game_handle_t g=items_test_game;
    unsigned checks=0;
    for(unsigned item=0;item<ITEM_STORED_COUNT;item++){
        items_test_prepare(item);
        g->state.items[item]=1;
        const char *reason=forge_item_block_reason(g,(int)item);
        forge_state_t before=g->state;
        for(unsigned i=0;i<3;i++)assert(forge_item_block_reason(g,(int)item)==reason);
        assert(memcmp(&before,&g->state,sizeof(before))==0);
        checks++;
        g->state.items[item]=0;
        before=g->state;
        assert(forge_item_block_reason(g,(int)item));
        assert(!forge_use_item(g,(int)item));
        assert(memcmp(&before,&g->state,sizeof(before))==0);
        checks++;
    }
    static const unsigned blocked[]={ITEM_SCALE,ITEM_POWDER,ITEM_STAR};
    for(unsigned i=0;i<sizeof(blocked)/sizeof(blocked[0]);i++){
        for(unsigned frenzy=0;frenzy<2;frenzy++){
            unsigned item=blocked[i];
            items_test_prepare(item);
            g->state.items[item]=1;
            forge_active(g)->charge=frenzy?0:10;
            forge_active(g)->frenzy_ms=frenzy?4000:0;
            forge_state_t before=g->state;
            assert(forge_item_block_reason(g,(int)item));
            assert(!forge_use_item(g,(int)item));
            assert(memcmp(&before,&g->state,sizeof(before))==0);
            checks++;
        }
    }
    static const unsigned armed[]={ITEM_SCALE,ITEM_CHALK,ITEM_POWDER};
    for(unsigned i=0;i<sizeof(armed)/sizeof(armed[0]);i++){
        unsigned item=armed[i];
        items_test_prepare(item);
        g->state.items[item]=2;
        assert(forge_use_item(g,(int)item));
        forge_state_t before=g->state;
        assert(forge_item_block_reason(g,(int)item));
        assert(!forge_use_item(g,(int)item));
        assert(memcmp(&before,&g->state,sizeof(before))==0);
        checks++;
    }
    for(unsigned item=ITEM_INCENSE;item<ITEM_COUNT;item++){
        items_test_prepare(item);
        forge_state_t before=g->state;
        assert(forge_item_block_reason(g,(int)item));
        assert(!forge_use_item(g,(int)item));
        assert(memcmp(&before,&g->state,sizeof(before))==0);
        checks++;
    }
    items_test_prepare(ITEM_CHALK);
    g->state.items[ITEM_CHALK]=1;
    forge_active(g)->frenzy_ms=4000;
    assert(!forge_item_block_reason(g,ITEM_CHALK));
    assert(forge_use_item(g,ITEM_CHALK));
    assert(g->state.chalk&&g->state.items[ITEM_CHALK]==0);
    return checks+1;
}
