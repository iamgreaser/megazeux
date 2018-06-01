/* MegaZeux
 *
 * Copyright (C) 2018 GreaseMonkey
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

#ifndef __DEMO_H
#define __DEMO_H

#include "compat.h"

__M_BEGIN_DECLS

#include "demo_struct.h"
#include "world_struct.h"

int demo_is_playing(struct world *mzx_world);
int demo_is_recording(struct world *mzx_world);
int demo_is_active(struct world *mzx_world);

int demo_init(struct world *mzx_world);
int demo_deinit(struct world *mzx_world);
int demo_start_frame(struct world *mzx_world);
int demo_end_frame(struct world *mzx_world);
int demo_record_init(struct world *mzx_world, const char *file_name);
int demo_record_start_frame(struct world *mzx_world);
int demo_record_end_frame(struct world *mzx_world);
int demo_play_init(struct world *mzx_world, const char *file_name);
int demo_play_start_frame(struct world *mzx_world);
int demo_play_end_frame(struct world *mzx_world);

__M_END_DECLS

#endif

