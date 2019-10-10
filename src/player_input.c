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
#include "util.h"

struct player_input_name_map
{
  const char *name;
  enum player_input_bits value;
};

static const struct player_input_name_map player_input_names[] =
{
  { "bomb",  PINP_BOMB,  },
  { "down",  PINP_DOWN,  },
  { "left",  PINP_LEFT,  },
  { "right", PINP_RIGHT, },
  { "shoot", PINP_SHOOT, },
  { "up",    PINP_UP,    },
};


// TODO: Do a many-to-many mapping for each player input somehow
static enum keycode player_input_map[NUM_PLAYERS][NUM_PINP];

/*
 * Convert a player input action name into its player input index.
 */
static boolean find_player_input_action(
 char **name, enum player_input_bits *pinp)
{
  int top = ARRAY_SIZE(player_input_names) - 1;
  int bottom = 0;

  while(bottom <= top)
  {
    int middle = (bottom + top) / 2;
    int cmpval = strcasecmp(*name, player_input_names[middle].name);

    if(cmpval > 0)
    {
      bottom = middle + 1;
    }
    else if(cmpval < 0)
    {
      top = middle - 1;
    }
    else
    {
      *pinp = player_input_names[middle].value;
      return true;
    }
  }

  return false;
}


/*
 * Parse each part of a player!.* = * configuration line.
 */
static boolean player_input_player_num(
 char **name, unsigned int *player_id)
{
  int next = 0;
  if(!strncasecmp(*name, "player", 6))
  {
    *name += 6;

    if(sscanf(*name, "%u%n", player_id, &next) == 1)
    {
      if((*player_id < NUM_PLAYERS))
      {
        *name += next;
        return true;
      }
    }
  }
  return false;
}

static boolean player_input_parse_bind_value(
 const char *value, enum keycode *binding)
{
  char *next;
  Uint32 key_value;

  key_value = strtoul(value, &next, 10);
  if((key_value >= 1) && (key_value <= IKEY_LAST) && (!next[0]))
  {
    *binding = key_value;
    return true;
  }
  if(!strncmp(value, "key.", 4))
  {
    key_value = find_keycode(value + 4);
    if(key_value)
    {
      *binding = key_value;
      return true;
    }
  }
  return false;
}


/*
 * Configuration:
 * Given a player!.* = * line, attempt to bind stuff to an action.
 */
// TODO: Support unbinding of actions.
// TODO: Prevent per-game configs from clobbering one another.
void player_input_bind_set(char *name, char *value)
{
  unsigned int player_id;
  enum player_input_bits pinp;

  if(player_input_player_num(&name, &player_id)
   && (*(name++) == '.')
   && find_player_input_action(&name, &pinp))
  {
    enum keycode key;

    if(player_input_parse_bind_value(value, &key))
    {
      player_input_map[player_id][pinp] = key;
    }
  }
}


/*
 * Set up sensible defaults for player inputs prior to loading configs.
 */
static enum keycode get_default_key_for_player_input(int player_id, int pinp)
{
  if(player_id != 0)
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


/*
 * Update the input state for each player.
 */
static enum keycode get_key_for_player_input(int player_id, int pinp)
{
  return player_input_map[player_id][pinp];
}

void update_one_player_input_state(struct player *player, int player_id)
{
  int pinp;

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
