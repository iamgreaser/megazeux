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

void update_one_player_input_state(struct player *player, int player_id)
{
  struct player_input *input = &player->input.s;

  // TODO: make these bindable per-player in the config
  input->shoot = get_key_status(keycode_internal_wrt_numlock, IKEY_SPACE);
  input->up = get_key_status(keycode_internal_wrt_numlock, IKEY_UP);
  input->down = get_key_status(keycode_internal_wrt_numlock, IKEY_DOWN);
  input->right = get_key_status(keycode_internal_wrt_numlock, IKEY_RIGHT);
  input->left = get_key_status(keycode_internal_wrt_numlock, IKEY_LEFT);
  input->bomb = get_key_status(keycode_internal_wrt_numlock, IKEY_DELETE);
}
