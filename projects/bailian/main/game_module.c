// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include "game.h"
#include "game_config.h"
#include "mosaico_game_module.h"
#include "mosaico_raylib_fast.h"
#include "host_asset_runtime.h"

typedef struct {
    game_handle_t game;
    int32_t pointer_track;
    bool pointer_captured;
    uint32_t audio_sequence;
    uint32_t audio_floor;
    struct { uint32_t sequence; uint8_t effect; } audio_events[8];
} host_state_t;
static const char *const audio_clips[]={NULL,"hit","great","damage","break","bell","select","heat","frenzy"};

static int initialize(void *value, const char *asset_root)
{
    mosaico_host_assets_set_root(asset_root);
    host_state_t *state = value;
    const game_config_t config = {.width=GAME_WIDTH,.height=GAME_HEIGHT,.seed=0xb411a};
    if (!game_create(&config, &state->game)) return -1;
    InitWindow(GAME_WIDTH, GAME_HEIGHT, GAME_NAME);
    SetTargetFPS(GAME_TICK_HZ);
    if (!game_load_assets(state->game)) { game_delete(state->game); state->game=NULL; return -1; }
    return 0;
}

static void shutdown(void *value)
{
    host_state_t *state = value;
    game_delete(state->game);
    state->game = NULL;
}

static void input(void *value, const mosaico_host_input_v1_t *event)
{
    host_state_t *state = value;
    if (!event) return;
    if (event->type == MOSAICO_HOST_INPUT_POINTER) {
        if (!state->pointer_captured && event->pressed) {
            state->pointer_track=event->track_id;
            state->pointer_captured=true;
        }
        if (state->pointer_captured && state->pointer_track==event->track_id) {
            game_set_pointer(state->game,event->x,event->y,event->pressed);
            if (!event->pressed) state->pointer_captured=false;
        }
    } else if (event->type == MOSAICO_HOST_INPUT_CONTROL) {
        switch (event->code) {
        case MOSAICO_HOST_CONTROL_PAUSE: game_set_paused(state->game, true);
            break;
        case MOSAICO_HOST_CONTROL_RESUME: game_set_paused(state->game, false);
            break;
        case MOSAICO_HOST_CONTROL_RESET: game_reset(state->game);
            break;
        default: break;
        }
    } else if (event->type == MOSAICO_HOST_INPUT_ACTION && event->pressed) {
        if (event->code == GAME_ACTION_RESET) game_reset(state->game);
        else if (event->code == GAME_ACTION_PAUSE)
            game_set_paused(state->game, !game_read(state->game).paused);
        else game_action(state->game,event->code==6?501:event->code);
    }
}

static void update(void *value)
{
    host_state_t *state=value;
    game_update(state->game);
    int effect=game_take_feedback(state->game);
    if(game_is_muted(state->game)){
        state->audio_floor=state->audio_sequence+1;
        /* Entry and contact can occupy the two feedback slots in one frame. */
        (void)game_take_feedback(state->game);
    }
    if(!game_is_muted(state->game)&&effect>0&&effect<9){
        uint32_t sequence=++state->audio_sequence;
        unsigned slot=(sequence-1U)%8U;
        state->audio_events[slot].sequence=sequence;
        state->audio_events[slot].effect=(uint8_t)effect;
    }
}

static int render(void *value)
{
    return game_render(((host_state_t *)value)->game) ? 0 : -1;
}

static uint32_t state_hash(const void *value)
{
    return game_hash(((const host_state_t *)value)->game);
}

static int state_json(const void *value, char *output, size_t capacity)
{
    const host_state_t *state=value;
    int length=game_write_json(state->game,output,capacity);
    if(length<1)return -1;
    size_t used=(size_t)length-1;
    int n=snprintf(output+used,capacity-used,",\"audio_events\":[");
    if(n<0||(size_t)n>=capacity-used)return -1;
    used+=(size_t)n;
    uint32_t first=state->audio_sequence>8?state->audio_sequence-7:1;
    if(first<state->audio_floor)first=state->audio_floor;
    if(game_is_muted(state->game))first=state->audio_sequence+1;
    for(uint32_t sequence=first;sequence<=state->audio_sequence;sequence++){
        unsigned slot=(sequence-1U)%8U;
        n=snprintf(output+used,capacity-used,"%s{\"seq\":%lu,\"clip\":\"%s\",\"volume\":0.8}",
            sequence==first?"":",",(unsigned long)sequence,audio_clips[state->audio_events[slot].effect]);
        if(n<0||(size_t)n>=capacity-used)return -1;
        used+=(size_t)n;
    }
    n=snprintf(output+used,capacity-used,"]}");
    return n<0||(size_t)n>=capacity-used?-1:(int)(used+(size_t)n);
}

static const mosaico_game_module_v1_t s_module = {
    .descriptor = {MOSAICO_HOST_GAME_ABI_V1, GAME_NAME, GAME_NAME,
                   GAME_WIDTH, GAME_HEIGHT, GAME_TICK_HZ, 1},
    .state_size = sizeof(host_state_t),
    .initialize = initialize, .shutdown = shutdown, .input = input,
    .update = update, .render = render,
    .state_hash = state_hash, .state_json = state_json,
};

const mosaico_game_module_v1_t *mosaico_game_module_v1(void)
{
    return &s_module;
}
