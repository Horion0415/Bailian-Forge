// SPDX-License-Identifier: Apache-2.0
/* Native-only fixture, never included in shipping sources. */
#include "game_internal.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern game_handle_t i18n_test_game;

int i18n_test_preferences(void)
{
    game_handle_t g=i18n_test_game;
    assert(game_get_language(g)==GAME_LANGUAGE_ZH);
    size_t size=game_save_size();assert(size==24032);
    void *save=malloc(size),*after=malloc(size);assert(save&&after);
    assert(game_save_write(g,save,size));uint32_t hash=game_hash(g),rng=g->state.rng;
    game_set_muted(g,true);assert(game_set_language(g,GAME_LANGUAGE_EN));
    assert(strcmp(forge_tr(g,"锻造"),"Forge")==0);
    assert(strcmp(forge_tr(g,"unknown"),"unknown")==0);
    assert(game_hash(g)==hash&&g->state.rng==rng);
    assert(game_save_write(g,after,size)&&memcmp(save,after,size)==0);
    assert(!game_set_language(g,(game_language_t)-1));
    assert(!game_set_language(g,GAME_LANGUAGE_COUNT));
    assert(game_get_language(g)==GAME_LANGUAGE_EN);
    assert(game_save_read(g,save,size)&&game_get_language(g)==GAME_LANGUAGE_EN&&game_is_muted(g));
    game_reset(g);assert(game_get_language(g)==GAME_LANGUAGE_EN&&game_is_muted(g));
    g->state.phase=PHASE_PLAY;forge_active(g)->heated=true;g->state.items[ITEM_CLOCK]=1;
    assert(forge_strike(g,RESULT_HOLD));assert(game_set_language(g,GAME_LANGUAGE_ZH));
    assert(forge_rewind(g)&&game_get_language(g)==GAME_LANGUAGE_ZH&&game_is_muted(g));
    static const struct {const char *value,*text;} money[]={
        {"999","999"},{"1000","1.00K"},{"10000","10.00K"},
        {"999999","999.99K"},{"1000000","1.00M"},{"1000000000","1.00B"},
        {"1000000000000","1.00T"},{"1000000000000000","1.00Qa"},
        {"1000000000000000000","1.00Qi"},{"100000000000000000000","1.00e20"}
    };
    for(unsigned i=0;i<sizeof(money)/sizeof(money[0]);i++){
        forge_money_t value;char text[32];assert(forge_money_parse(&value,money[i].value));
        forge_money_text_locale(&value,text,sizeof(text),true,true);assert(strcmp(text,money[i].text)==0);
    }
    free(after);free(save);game_reset(g);game_set_muted(g,false);return 1;
}

void i18n_test_scene(unsigned scene,unsigned detail)
{
    game_handle_t g=i18n_test_game;game_reset(g);
    if(scene==0)return;
    forge_state_t *s=&g->state;s->phase=PHASE_PLAY;s->coins=forge_money_u64(1234567);s->credits=12;
    forge_blade_t *b=forge_active(g);b->heated=true;b->level=8;
    g->notice_ms=0;g->modal=MODAL_NONE;g->selected=(uint8_t)s->active;
    for(unsigned i=0;i<ITEM_COUNT;i++)s->items[i]=3;
    switch(scene){
    case 2:g->paused=true;break;
    case 3:g->screen=SCREEN_RACK;break;
    case 4:g->screen=SCREEN_ITEMS;s->scale=s->chalk=s->powder=true;break;
    case 5:
        g->screen=SCREEN_MERCHANT;s->merchant_count=10;g->page=(uint8_t)detail;
        for(unsigned i=0;i<10;i++)s->merchant[i]=(forge_offer_t){.kind=i<7?2:1,.id=i%7,.family=i%10,.level=20,.stock=2};
        break;
    case 6:
        g->screen=SCREEN_BOOK;g->book_tab=(uint8_t)detail;s->found[0]=s->found[1]=UINT64_MAX;s->unlocks=UINT16_MAX;
        break;
    case 7:g->modal=MODAL_RELIC;g->detail_relic=(int16_t)detail;g->detail_readonly=true;break;
    case 8:g->modal=MODAL_ITEM;g->detail_item=(uint8_t)detail;break;
    case 9:g->modal=MODAL_FAMILY;g->detail_item=(uint8_t)detail;break;
    case 10:g->modal=(uint8_t)detail;g->detail_relic=R_KNOT;break;
    case 11:
        assert(forge_move(g,s->active,true));assert(forge_tribute_start(g,true));
        forge_tribute_update(g,detail);break;
    case 12:b->heated=false;forge_heat_start(g);game_update_elapsed(g,600);break;
    case 13:b->charge=10;forge_swing_request(g);game_update_elapsed(g,100);break;
    case 14:s->phase=(uint8_t)detail;break;
    case 15:
        g->screen=SCREEN_RELICS;g->shop_page=1;s->owned_count=8;
        for(unsigned i=0;i<8;i++)s->owned[i]=(int8_t)(R_HEADLESS+i);
        break;
    case 16:
        s->day=999;s->credits=UINT16_MAX;s->backlog=1234;g->screen=SCREEN_RACK;
        assert(forge_money_parse(&s->coins,"9999999999999999999999999999999999999999"));break;
    case 17:
        g->modal=MODAL_OFFER;g->detail_offer=0;s->merchant_count=1;
        s->merchant[0]=(forge_offer_t){.kind=1,.family=(uint8_t)detail,.level=20,.stock=1};break;
    case 18:
        if(detail)s->items[ITEM_CLOCK]=0;
        assert(forge_strike(g,RESULT_BREAK));game_update_elapsed(g,2000);g->notice_ms=0;break;
    default:break;
    }
}

const char *i18n_test_notice(void){return i18n_test_game->notice;}
