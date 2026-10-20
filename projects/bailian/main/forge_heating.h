// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
#include "raylib.h"
enum { FORGE_HEAT_DURATION_MS=1200, FORGE_HEAT_INSERT_MS=220,
       FORGE_HEAT_RETURN_MS=280, FORGE_HEAT_READY_MS=1100 };
typedef enum { HEAT_NONE,HEAT_COLD,HEAT_INSERT,HEAT_WARM,HEAT_RETURN,HEAT_READY,HEAT_HOT } forge_heat_phase_t;
typedef struct {uint32_t blade_id;uint16_t ready_ms;} forge_heating_t;
typedef struct {
    Rectangle sword;
    Color tint;
    float progress,travel,hammer_drop,value_alpha,hud_alpha,charge_alpha;
    forge_heat_phase_t phase;
} forge_heat_view_t;
void forge_heat_reset(game_handle_t g);
void forge_heat_start(game_handle_t g);
void forge_heat_update(game_handle_t g,uint32_t ms);
forge_heat_phase_t forge_heat_phase(game_handle_t g);
void forge_heat_read(game_handle_t g,forge_heat_view_t *view);
void forge_heat_draw_light(game_handle_t g,const forge_heat_view_t *view,int dx,int dy);
void forge_heat_draw_hud(game_handle_t g,const forge_heat_view_t *view);
bool forge_heat_draw_footer(game_handle_t g,const forge_heat_view_t *view);
