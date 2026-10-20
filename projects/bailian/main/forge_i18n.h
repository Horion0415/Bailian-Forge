// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "game.h"

/* Keys are the original Chinese strings. Unknown/ASCII strings pass through.
 * Only presentation changes: no gameplay state or RNG access is performed. */
const char *forge_tr(game_handle_t game,const char *text);
