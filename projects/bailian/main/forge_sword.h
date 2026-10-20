// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
#include "raylib.h"
enum { FORGE_SWORD_MORPH_MS=600 };
typedef struct {
    uint32_t blade_id;
    uint16_t age_ms;
    uint8_t family,level,from_family,from_level;
    bool morphing;
} forge_sword_shape_t;
unsigned forge_sword_stage(unsigned level);
const char *forge_sword_stage_name(unsigned level);
void forge_sword_reset(game_handle_t g);
void forge_sword_update(game_handle_t g,uint32_t ms);
void forge_sword_draw(game_handle_t g,unsigned family,unsigned level,Rectangle dest,
    Vector2 origin,float rotation,Color tint,bool animate);
void forge_sword_draw_piece(game_handle_t g,unsigned family,unsigned level,Rectangle window,Rectangle dest,Color tint);
