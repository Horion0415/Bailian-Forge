// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"
enum { FORGE_SWING_NORMAL_MS=200, FORGE_SWING_FRENZY_MS=140,
       FORGE_SWING_MIN_MS=100, FORGE_SWING_BUFFER_MS=80,
       FORGE_SWING_LEGACY_MS=640, FORGE_SWING_LEGACY_CONTACT_MS=160 };
typedef struct {
    uint32_t blade_id;
    uint16_t cycle_ms,contact_ms;
    bool queued,frenzy_sound;
} forge_swing_t;
void forge_swing_reset(game_handle_t g);
void forge_swing_cancel(game_handle_t g,bool stop_current);
void forge_swing_request(game_handle_t g);
void forge_swing_update(game_handle_t g,uint32_t ms);
