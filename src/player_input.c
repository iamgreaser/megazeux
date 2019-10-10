/* MegaZeux
 *
 * Copyright (C) 2019 Ben Russell <thematrixeatsyou@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "player_input.h"

#include "event.h"
#include "keysym.h"
#include "player_struct.h"

// TODO: Do a many-to-many mapping for each player input somehow
static enum keycode player_input_map[NUM_PLAYERS][NUM_PINP];

// TODO:
// - add config, and use from input.c/h:
//     enum keycode find_keycode(const char *name)

static enum keycode get_default_key_for_player_input(int player_id, int pinp)
{
  // TODO: Add hook into configure.c and allow adding new binds
  if(false && player_id != 0)
  {
    return IKEY_UNKNOWN;
  }

  switch((enum player_input_bits)pinp)
  {
    case PINP_SHOOT: return IKEY_SPACE;
    case PINP_UP: return IKEY_UP;
    case PINP_DOWN: return IKEY_DOWN;
    case PINP_RIGHT: return IKEY_RIGHT;
    case PINP_LEFT: return IKEY_LEFT;
    case PINP_BOMB: return IKEY_DELETE;
    default: return IKEY_UNKNOWN;
  }
}

static enum keycode get_key_for_player_input(int player_id, int pinp)
{
  return player_input_map[player_id][pinp];
}

void init_player_input(void)
{
  int player_id, pinp;

  for(player_id = 0; player_id < NUM_PLAYERS; player_id++)
  {
    for(pinp = 0; pinp < NUM_PINP; pinp++)
    {
      player_input_map[player_id][pinp] = (
       get_default_key_for_player_input(player_id, pinp));
    }
  }
}

void update_one_player_input_state(struct player *player, int player_id)
{
  int pinp;

  // TODO: make these bindable per-player in the config
  for(pinp = 0; pinp < NUM_PINP; pinp++)
  {
    enum keycode key = get_key_for_player_input(player_id, pinp);
    if(key != IKEY_UNKNOWN)
    {
      player->input.a[pinp] = get_key_status(
       keycode_internal_wrt_numlock, key);
    }
    else
    {
      player->input.a[pinp] = false;
    }
  }
}
