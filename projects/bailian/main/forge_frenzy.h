// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
/* Transient presentation state. Never serialized and never uses model RNG. */
typedef struct {
    uint32_t blade_id;
    uint16_t age_ms,total_ms;
    bool active,hit;
} forge_frenzy_t;
void forge_frenzy_strike(game_handle_t g,bool inside,bool entering);
void forge_frenzy_update(game_handle_t g,uint32_t ms);
void forge_frenzy_back(game_handle_t g,int dx,int dy);
void forge_frenzy_front(game_handle_t g,int dx,int dy);
void forge_frenzy_label(game_handle_t g);
