// SPDX-License-Identifier: Apache-2.0
#include "game_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Model tests intentionally do not link a renderer or the device BSP. */
void forge_particles(game_handle_t g,int kind){(void)g;(void)kind;}
void forge_heat_start(game_handle_t g){(void)g;}
bool forge_fx_break_visible(game_handle_t g){(void)g;return false;}
static game_handle_t fresh(void)
{
    game_handle_t g=calloc(1,sizeof(*g));assert(g);g->undo=calloc(1,sizeof(*g->undo));assert(g->undo);
    forge_new(g,123,false);g->state.phase=PHASE_PLAY;memset(g->state.offers,-1,3);forge_active(g)->heated=true;return g;
}
static void dispose(game_handle_t g){free(g->undo);free(g);}
static void equip(game_handle_t g,int id)
{
    g->state.day=10;g->state.offers[0]=(int8_t)id;g->state.credits=100;
    assert(forge_buy_relic(g,0));
}
static void expect(const forge_money_t *money,const char *decimal)
{
    forge_money_t wanted;assert(forge_money_parse(&wanted,decimal));
    if(forge_money_compare(money,&wanted)){char got[109];forge_money_text(money,got,sizeof(got),false);fprintf(stderr,"expected %s got %s\n",decimal,got);abort();}
}
int main(void)
{
    forge_money_t a,b;assert(forge_money_parse(&a,"100000000000000000000"));b=forge_money_u64(3);forge_money_mul(&a,&b);forge_money_div(&a,2);expect(&a,"150000000000000000000");
    game_handle_t g=fresh();forge_blade_t *sword=forge_active(g);forge_money_t value=forge_value(&g->state,sword);expect(&value,"320");
    sword->level=5;value=forge_value(&g->state,sword);expect(&value,"2430");sword->level=10;value=forge_value(&g->state,sword);expect(&value,"18453");
    sword->level=99;value=forge_value(&g->state,sword);expect(&value,"86733051207512583978");
    sword->level=8;forge_strike(g,RESULT_GREAT);value=forge_value(&g->state,sword);expect(&value,"20503");forge_strike(g,RESULT_DAMAGE);value=forge_value(&g->state,sword);expect(&value,"13669");
    uint16_t odds[5];sword->level=0;forge_tickets(&g->state,sword,odds);assert(odds[0]==0);sword->level=10;forge_tickets(&g->state,sword,odds);assert(odds[0]==467);
    for(unsigned level=0;level<100;level++){sword->level=(uint8_t)level;forge_tickets(&g->state,sword,odds);unsigned sum=0;for(unsigned i=0;i<5;i++)sum+=odds[i];assert(sum==1000);}
    dispose(g);

    g=fresh();equip(g,R_SPINE);sword=forge_active(g);sword->level=10;forge_strike(g,RESULT_DAMAGE);forge_strike(g,RESULT_DAMAGE);assert(sword->spine_ready);
    forge_strike(g,RESULT_BREAK);assert(forge_active(g));assert(!sword->spine_ready);assert(sword->spine_count==0);forge_strike(g,RESULT_BREAK);assert(!forge_active(g));dispose(g);
    g=fresh();equip(g,R_KING);equip(g,R_APRON);sword=forge_active(g);forge_strike(g,RESULT_DAMAGE);assert(sword->level==0&&sword->king==1);value=forge_value(&g->state,sword);expect(&value,"460");forge_strike(g,RESULT_SUCCESS);assert(sword->king==1);forge_strike(g,RESULT_GREAT);assert(sword->level==4&&sword->king==0);dispose(g);
    g=fresh();equip(g,R_HOLY);equip(g,R_DAWN);sword=forge_active(g);sword->level=10;forge_strike(g,RESULT_BREAK);assert(g->state.inherit_level==7);assert(g->state.inherit_charge==7);forge_select(g,1);assert(forge_active(g)->level==7&&forge_active(g)->charge==7);dispose(g);
    g=fresh();equip(g,R_ASH);equip(g,R_CHALICE);sword=forge_active(g);sword->level=8;g->state.items[ITEM_CLOCK]=1;forge_strike(g,RESULT_BREAK);assert(!forge_active(g));expect(&g->state.coins,"500");assert(forge_rewind(g));assert(forge_active(g)->level==8);assert(g->state.items[ITEM_CLOCK]==0);expect(&g->state.coins,"0");expect(&g->state.scrap,"0");dispose(g);
    g=fresh();g->state.coins=forge_money_u64(999999);assert(!forge_settle(g,false));assert(g->state.day==1);g->modal=MODAL_NONE;forge_active(g)->level=5;forge_move(g,0,true);assert(!forge_trade(g,0));assert(forge_settle(g,false));assert(g->state.day==2);expect(&g->state.coins,"1002429");dispose(g);
    g=fresh();assert(forge_settle(g,true));assert(g->state.anger==1&&g->state.day==2);assert(forge_settle(g,true));assert(g->state.anger==2&&g->state.day==3);assert(forge_settle(g,true));assert(g->state.phase==PHASE_LOST);dispose(g);
    g=fresh();g->state.day=21;assert(forge_settle(g,true));assert(g->state.phase==PHASE_CLEAR);assert(forge_continue(g));assert(g->state.day==22&&g->state.endless);value=forge_quota(&g->state);expect(&value,"278400");dispose(g);
    for(int id=0;id<R_COUNT;id++){
        g=fresh();g->state.expanded=true;equip(g,id);sword=forge_active(g);sword->heated=true;sword->level=10;
        const int sequence[]={RESULT_HOLD,RESULT_HOLD,RESULT_DAMAGE,RESULT_DAMAGE,RESULT_SUCCESS,RESULT_GREAT};
        for(unsigned k=0;k<sizeof(sequence)/sizeof(sequence[0]);k++){assert(forge_strike(g,sequence[k]));assert(sword->level<=99);forge_tickets(&g->state,sword,odds);unsigned sum=0;for(unsigned j=0;j<5;j++)sum+=odds[j];assert(sum==1000);}
        sword->charge=0;sword->frenzy_ms=0;assert(forge_strike(g,RESULT_BREAK));dispose(g);
    }
    printf("PASS: 108-digit money, public value/ticket fixtures, 98 relic paths, protection, inheritance, rewind, currencies, 21-day clear and endless. state=%zu bytes\n",sizeof(forge_state_t));
    return 0;
}
