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

#ifndef __DEMO_STRUCT_H
#define __DEMO_STRUCT_H

#include "compat.h"

__M_BEGIN_DECLS

#ifdef SKIP_SDL
struct buffered_status {int dummy_do_not_use;};
#else
#include "event.h"
#endif

#include <stdint.h>

#define DEMO_FRAME_IS_FIRST 0x00000001

struct demo_frame
{
  uint32_t flags;

  uint64_t timestamp_usec;
  struct buffered_status buffer;

  // Integrity checks
  unsigned long long random_seed;

  // Anything which cannot be predicted and thus must be faked
  uint32_t mod_order;
  uint32_t mod_position;
  uint32_t audio_length;
};

struct demo_runtime
{
  struct demo_frame current_frame;
  struct demo_frame previous_frame;

  FILE *fp;

  // Set one of these if we're playing or recording a demo
  // Both cannot be set, but both can be clear
  bool playing;
  bool recording;
};

__M_END_DECLS

#endif

