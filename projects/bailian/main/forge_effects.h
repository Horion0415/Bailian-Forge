// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
#include "forge_frenzy.h"
#include "forge_money.h"
enum { FORGE_FX_PARTICLES=64, FORGE_FX_GLOW_W=96, FORGE_FX_GLOW_H=48 };
typedef struct {
    float x,y,vx,vy,life,total,size,spin;
    uint8_t kind;
} forge_fx_particle_t;
typedef struct {
    uint32_t clock_ms,rng;
    uint16_t age_ms,duration_ms,frenzy_pulse_ms;
    uint8_t result,family,old_level,new_level;
    bool active;
    forge_frenzy_t frenzy;
    float heat,shake;
    forge_money_t value;
    uint8_t glow[FORGE_FX_GLOW_W*FORGE_FX_GLOW_H];
    forge_fx_particle_t particles[FORGE_FX_PARTICLES];
} forge_fx_t;
void forge_fx_reset(game_handle_t g);
void forge_fx_update(game_handle_t g,uint32_t ms);
void forge_fx_camera(game_handle_t g,int *x,int *y);
float forge_fx_hammer_angle(game_handle_t g);
void forge_fx_ambient(game_handle_t g,int dx,int dy);
void forge_fx_impact(game_handle_t g,int dx,int dy);
void forge_fx_labels(game_handle_t g);
bool forge_fx_break_visible(game_handle_t g);
unsigned forge_fx_particle_count(game_handle_t g);
