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


#ifndef __PLAYER_INPUT_H
#define __PLAYER_INPUT_H

#include "compat.h"

__M_BEGIN_DECLS

#include "player_struct.h"
#include "player_input_struct.h"

void update_one_player_input_state(struct player *player, int player_id);

__M_END_DECLS

#endif // __PLAYER_INPUT_H
