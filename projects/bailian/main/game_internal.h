// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
#include "forge_data.h"
#include "forge_money.h"
#include "mosaico_game_2d.h"
#include "forge_effects.h"
#include "forge_heating.h"
#include "forge_tribute.h"
#include "forge_sword.h"
#include "forge_swing.h"
#include "forge_i18n.h"
enum { FORGE_BLADES=96, FORGE_SCALE=1000000000, FORGE_PAGE_SIZE=6 };
enum { PHASE_GIFT,PHASE_PLAY,PHASE_CLEAR,PHASE_LOST };
enum { SCREEN_FORGE,SCREEN_RACK,SCREEN_RELICS,SCREEN_TRIBUTE,SCREEN_MERCHANT,SCREEN_ITEMS,SCREEN_BOOK,SCREEN_PAUSE };
enum { RESULT_BREAK,RESULT_DAMAGE,RESULT_HOLD,RESULT_SUCCESS,RESULT_GREAT,RESULT_NONE,RESULT_REFINE };
enum { PLACE_FREE,PLACE_RACK,PLACE_ANVIL,PLACE_TRIBUTE };
enum { ITEM_SCALE,ITEM_CLOCK,ITEM_CHALK,ITEM_POWDER,ITEM_STAR,ITEM_INCENSE,ITEM_DELIVERY,ITEM_COUNT };
enum { MODAL_NONE,MODAL_SHORT,MODAL_NEW,MODAL_RELIC,MODAL_FAMILY,MODAL_TRADE,MODAL_DISCARD,MODAL_ITEM,MODAL_OFFER };
enum { FORGE_ENERGY_UNIT=32,FORGE_ENERGY_COST=640*FORGE_ENERGY_UNIT,FORGE_ENERGY_MAX=2*FORGE_ENERGY_COST };
enum { FB_NONE,FB_HIT,FB_GREAT,FB_DAMAGE,FB_BREAK,FB_BELL,FB_SELECT,FB_HEAT,FB_FRENZY };
typedef struct {
    forge_money_t base,flat,perm,peak;
    uint32_t id;
    uint16_t hits,damages,holds,successes,same,success_streak,hold_streak;
    uint16_t scars,charm,success_tickets,great_tickets,king,scholar;
    int16_t frenzy_ms;
    uint8_t family,place,level,charge,wind,heart,spine_count,milestone,last;
    bool heated,spine_ready,corner_used,phoenix_used;
} forge_blade_t;
typedef struct {
    uint8_t kind,id,family,level,stock,bought;
} forge_offer_t;
typedef struct {
    forge_money_t coins,scrap,quota_credit,peak,earned,tribute;
    forge_blade_t blades[FORGE_BLADES];
    uint64_t acquired[2],found[2];
    uint32_t rng,next_id,revision,hits,breaks,greats,paid,failed,extended_paid;
    uint32_t elapsed_ms;
    uint16_t day,credits,unlocks,achievements,backlog,extra_supply,rerolls,one_great;
    uint16_t inherit_level,inherit_charge,inherit_molar,quest_level;
    uint16_t deck[5],items[ITEM_COUNT];
    int16_t active;
    int8_t owned[8],offers[3];
    forge_offer_t merchant[10];
    uint8_t owned_count,merchant_count,phase,anger,max_level,clear_best;
    bool expanded,endless,cleared,incense_used,wire_used,hook_used,quest_used,clear_paid;
    uint8_t undertaker_count;
    bool scale,chalk,powder;
    uint8_t legacy_padding[5]; /* Explicitly retain v1's final alignment bytes. */
    /* Appended v2 payload: exact minimum levels of supplies awaiting a slot. */
    uint16_t pending_levels[100];
    uint16_t frenzy_energy[FORGE_BLADES];
    uint8_t frenzy_fraction[FORGE_BLADES];
} forge_state_t;
_Static_assert(offsetof(forge_state_t,pending_levels)==23544,"v1 save prefix must remain compatible");
struct game_t {
    game_config_t config;
    forge_state_t state,*undo;
    bool undo_valid,paused,pointer_down,assets_loaded,pending_strike;
    int32_t pointer_x,pointer_y;
    uint32_t tick,ms_remainder;
    uint16_t cooldown_ms,heat_ms,animation_ms,notice_ms,feedback;
    uint8_t screen,page,selected,shop_page,modal;
    int16_t detail_relic;
    uint8_t detail_item,detail_offer,book_tab;
    bool detail_readonly;
    uint8_t last_result,visual_family;
    char notice[160];
    char text_buffer[256];
    forge_fx_t fx;
    forge_heating_t heating;
    forge_tribute_fx_t tribute;
    forge_sword_shape_t shape;
    forge_swing_t swing;
    MosaicoAtlas room,hammer,swords,relic_atlas;
    mosaico_asset_view_t font;
    uint8_t *save_buffer;
    bool audio_muted;
    uint8_t language;
};
bool forge_has(const forge_state_t *s,int id);
forge_blade_t *forge_active(game_handle_t g);
uint32_t forge_random(forge_state_t *s);
void forge_notice(game_handle_t g,const char *text);
void forge_changed(game_handle_t g,bool invalidate_undo);
void forge_remember(game_handle_t g,forge_blade_t *blade);
void forge_claim_supply(game_handle_t g);
void forge_energy_update(forge_state_t *state,forge_blade_t *blade,unsigned elapsed_ms);
unsigned forge_energy_percent(const forge_state_t *state,const forge_blade_t *blade);
forge_money_t forge_value(const forge_state_t *s,const forge_blade_t *b);
forge_money_t forge_quota(const forge_state_t *s);
forge_money_t forge_base_quota(unsigned day);
forge_money_t forge_tribute(const forge_state_t *s);
void forge_tickets(const forge_state_t *s,const forge_blade_t *b,uint16_t out[5]);
void forge_new(game_handle_t g,uint32_t seed,bool preserve_profile);
bool forge_choose_gift(game_handle_t g,int slot);
bool forge_select(game_handle_t g,int index);
bool forge_move(game_handle_t g,int index,bool put);
bool forge_trade(game_handle_t g,int index);
bool forge_buy_relic(game_handle_t g,int slot);
bool forge_discard(game_handle_t g,int id);
bool forge_reroll(game_handle_t g);
forge_money_t forge_reroll_price(const forge_state_t *s);
forge_money_t forge_merchant_price(const forge_state_t *s,unsigned slot);
bool forge_buy_merchant(game_handle_t g,unsigned slot);
bool forge_use_item(game_handle_t g,int item);
bool forge_rewind(game_handle_t g);
bool forge_settle(game_handle_t g,bool accept_short);
bool forge_continue(game_handle_t g);
bool forge_family(game_handle_t g,int family);
bool forge_strike(game_handle_t g,int forced_result);
void forge_render(game_handle_t g);
void forge_pointer(game_handle_t g,int x,int y);
void forge_particles(game_handle_t g,int kind);
void forge_text(game_handle_t g,const char *text,int x,int y,int size,Color color);
void forge_audio_button(game_handle_t g,int x,int y,int width);
void forge_language_button(game_handle_t g,int x,int y,int width);
int forge_text_width(game_handle_t g,const char *text,int size);
void forge_text_fit(game_handle_t g,const char *text,int x,int y,int width,int size,Color color);
void forge_text_center(game_handle_t g,const char *text,int x,int y,int width,int size,Color color);
