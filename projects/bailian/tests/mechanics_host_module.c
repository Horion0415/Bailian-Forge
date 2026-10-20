/* Non-shipping UI states. All input/update/rendering uses the production game. */
#include "game_internal.h"
#include "mosaico_game_module.h"
#include "mosaico_raylib_fast.h"
#include "host_asset_runtime.h"
#include <assert.h>
#include <string.h>
typedef struct {game_handle_t game;} fixture_t;
static int initialize(void *ptr,const char *root){
    fixture_t *f=ptr;game_config_t config={480,480,123};mosaico_host_assets_set_root(root);
    if(!game_create(&config,&f->game))return -1;
    InitWindow(480,480,"Mechanics fixture");return game_load_assets(f->game)?0:-1;
}
static void shutdown(void *ptr){game_delete(((fixture_t*)ptr)->game);}
static void input(void *ptr,const mosaico_host_input_v1_t *event){
    game_handle_t g=((fixture_t*)ptr)->game;
    if(event->type==MOSAICO_HOST_INPUT_POINTER){game_set_pointer(g,event->x,event->y,event->pressed);return;}
    if(event->type!=MOSAICO_HOST_INPUT_ACTION||!event->pressed)return;
    if(event->code>=4000&&event->code<4010){
        game_reset(g);g->state.phase=PHASE_PLAY;g->notice_ms=0;g->state.day=6;
        forge_blade_t *b=forge_active(g);b->heated=true;b->level=8;forge_remember(g,b);
        if(event->code==4000){b->frenzy_ms=4000;g->state.frenzy_energy[0]=FORGE_ENERGY_COST/2;}
        else if(event->code==4001){g->modal=MODAL_FAMILY;g->detail_item=6;}
        else if(event->code==4002){g->screen=SCREEN_ITEMS;g->state.items[ITEM_CHALK]=2;g->detail_item=ITEM_CHALK;g->modal=MODAL_ITEM;}
        else if(event->code==4003){g->screen=SCREEN_MERCHANT;g->state.merchant_count=1;g->state.coins=forge_money_u64(10000);g->state.merchant[0]=(forge_offer_t){.kind=2,.id=ITEM_STAR,.stock=3};g->detail_offer=0;g->modal=MODAL_OFFER;}
        else if(event->code==4004){g->screen=SCREEN_BOOK;g->book_tab=1;g->state.unlocks=2|16|1024;g->state.achievements=4;}
        else if(event->code==4005){g->screen=SCREEN_BOOK;g->book_tab=2;g->state.found[0]=63;}
        else if(event->code==4006){g->state.scale=g->state.chalk=true;}
        else if(event->code==4007){g->screen=SCREEN_BOOK;g->book_tab=0;}
    }else game_action(g,event->code);
}
static void update(void *ptr){game_update(((fixture_t*)ptr)->game);}
static int render(void *ptr){return game_render(((fixture_t*)ptr)->game)?0:-1;}
static uint32_t hash(const void *ptr){return game_hash(((const fixture_t*)ptr)->game);}
static int json(const void *ptr,char *out,size_t size){return game_write_json(((const fixture_t*)ptr)->game,out,size);}
static const mosaico_game_module_v1_t module={
    .descriptor={MOSAICO_HOST_GAME_ABI_V1,"mechanics-fixture","Native mechanics",480,480,30,1},
    .state_size=sizeof(fixture_t),.initialize=initialize,.shutdown=shutdown,.input=input,.update=update,
    .render=render,.state_hash=hash,.state_json=json};
const mosaico_game_module_v1_t *mosaico_game_module_v1(void){return &module;}
