// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
#include "forge_money.h"
enum { FORGE_TRIBUTE_DURATION_MS=2300, FORGE_TRIBUTE_RESPONSE_MS=650,
       FORGE_TRIBUTE_COINS_MS=1200, FORGE_TRIBUTE_SKIP_MS=1400,
       FORGE_TRIBUTE_SAMPLES=6 };
typedef struct {
    forge_money_t amount,need,reward;
    uint16_t age_ms,from_day,to_day,credit_gain;
    uint8_t count,sample_count,families[FORGE_TRIBUTE_SAMPLES],levels[FORGE_TRIBUTE_SAMPLES];
    uint8_t anger_before,anger_after,phase_after;
    bool active,paid;
} forge_tribute_fx_t;
void forge_tribute_reset(game_handle_t g);
bool forge_tribute_start(game_handle_t g,bool accept_short);
void forge_tribute_update(game_handle_t g,uint32_t ms);
unsigned forge_tribute_stage(game_handle_t g);
void forge_tribute_draw(game_handle_t g);
void forge_tribute_pointer(game_handle_t g,int x,int y);
