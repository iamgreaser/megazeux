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
struct key_to_player_input
{
  enum keycode up;
  enum keycode down;
  enum keycode right;
  enum keycode left;
  enum keycode shoot;
  enum keycode bomb;
};

union key_to_player_input_union
{
  struct key_to_player_input s;
  enum keycode a[NUM_PINP];
};

union key_to_player_input_union player_input_map[NUM_PLAYERS];

// TODO:
// - add config, and use from input.c/h:
//     enum keycode find_keycode(const char *name)

static enum keycode get_key_for_player_input(
 struct player *player, int player_id, int pinp)
{
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


void update_one_player_input_state(struct player *player, int player_id)
{
  int i;

  // TODO: make these bindable per-player in the config
  for(i = 0; i < NUM_PINP; i++)
  {
    enum keycode key = get_key_for_player_input(player, player_id, i);
    if(key != IKEY_UNKNOWN)
    {
      player->input.a[i] = get_key_status(keycode_internal_wrt_numlock, key);
    }
    else
    {
      player->input.a[i] = false;
    }
  }
}
