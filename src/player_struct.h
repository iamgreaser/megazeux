/* MegaZeux
 *
 * Copyright (C) 2004 Gilead Kutnick <exophase@adelphia.net>
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

#ifndef __PLAYER_STRUCT_H
#define __PLAYER_STRUCT_H

#include "compat.h"

__M_BEGIN_DECLS

#ifdef CONFIG_MULTIPLAYER
// TODO: A lot of things, check doc/multiplayer_support.txt
#define NUM_PLAYERS 16
#else
#define NUM_PLAYERS 1
#endif

enum player_input_bits
{
  PINP_UP        =   0,
  PINP_DOWN      =   1,
  PINP_RIGHT     =   2,
  PINP_LEFT      =   3,
  PINP_SHOOT     =   4,
  PINP_BOMB      =   5,
  PINP_UNUSED006 =   6, // TODO: move Ins/F5 to here
  PINP_UNUSED007 =   7,

  PINP_UNUSED008 =   8,
  PINP_UNUSED009 =   9,
  PINP_UNUSED010 =  10,
  PINP_UNUSED011 =  11,
  PINP_UNUSED012 =  12,
  PINP_UNUSED013 =  13,
  PINP_UNUSED014 =  14,
  PINP_UNUSED015 =  15,

  NUM_PINP,
};

struct player_input
{
  boolean up;
  boolean down;
  boolean right;
  boolean left;
  boolean shoot;
  boolean bomb;
};

union player_input_union
{
  struct player_input s;
  boolean a[16];
};

struct player
{
  int x;
  int y;
  int shoot_cooldown;

  // Has this player separated from the primary player?
  boolean separated;

  // Did the player just move?
  boolean moved;

  // Was the player on an entrance before the board scan?
  boolean was_on_entrance;

  // For use in repeat delays for player movement
  int key_up_delay;
  int key_down_delay;
  int key_right_delay;
  int key_left_delay;

  // Player inputs
  union player_input_union input;
};

__M_END_DECLS

#endif // __PLAYER_STRUCT_H
