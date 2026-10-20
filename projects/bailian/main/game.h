// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct game_t *game_handle_t;
typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t seed;
} game_config_t;

typedef struct {
    uint32_t tick;
    int32_t pointer_x;
    int32_t pointer_y;
    bool pointer_down;
    bool paused;
    uint16_t day;
    uint8_t phase,screen,anger,level,charge,last_result,relic_count;
    uint16_t blade_count,frenzy_ms;
    uint32_t active_id;
} game_snapshot_t;

/* Portable C shared by Host and device. One loop owns each instance and calls
 * these methods; input adapters enqueue or deliver events on that same loop. */
bool game_create(const game_config_t *config, game_handle_t *ret_handle);
void game_delete(game_handle_t handle);
void game_reset(game_handle_t handle);
void game_set_paused(game_handle_t handle, bool paused);
/* Audio preference is independent of run saves, undo and new-game state. */
void game_set_muted(game_handle_t handle,bool muted);
bool game_is_muted(game_handle_t handle);
void game_set_pointer(game_handle_t handle, int32_t x, int32_t y, bool pressed);
void game_update(game_handle_t handle);
void game_update_elapsed(game_handle_t handle,uint32_t elapsed_ms);
bool game_render(game_handle_t handle);
game_snapshot_t game_read(game_handle_t handle);
bool game_load_assets(game_handle_t handle);
void game_action(game_handle_t handle,int action);
int game_write_json(game_handle_t handle,char *output,size_t capacity);
uint32_t game_hash(game_handle_t handle);
size_t game_save_size(void);
bool game_save_write(game_handle_t handle,void *buffer,size_t capacity);
bool game_save_read(game_handle_t handle,const void *buffer,size_t size);
uint32_t game_revision(game_handle_t handle);
int game_take_feedback(game_handle_t handle);
