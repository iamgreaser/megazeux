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

// Demo recording and playback routines.

#include "const.h"
#include "demo.h"
#include "error.h"
#include "event.h"
#include "fsafeopen.h"
#include "util.h"

extern struct buffered_status *load_status_nonconst(void);

//
// ABSTRACTION LAYER
//

int demo_is_playing(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;

  return demo->playing;
}

int demo_is_recording(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;

  return demo->recording;
}

int demo_is_active(struct world *mzx_world)
{
  return (demo_is_playing(mzx_world) || demo_is_recording(mzx_world));
}

//
// GENERAL
//

static void demo_forced_input_callback(void *mzx_world_thunk)
{
  struct world *mzx_world = (struct world *)mzx_world_thunk;
  demo_end_frame(mzx_world);
  demo_start_frame(mzx_world);
}

static void demo_clear_frame(struct world *mzx_world, struct demo_frame *frame)
{
  memset(frame, 0, sizeof(*frame));
  frame->flags = (0
    | DEMO_FRAME_IS_FIRST
    );
  frame->timestamp_usec = 0;
  frame->random_seed = 0;
  frame->mod_order = 0;
  frame->mod_position = 0;
}

int demo_init(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;

  // Clear frames
  demo_clear_frame(mzx_world, &(demo->previous_frame));
  demo_clear_frame(mzx_world, &(demo->current_frame));

  // Clear file pointer
  demo->fp = NULL;

  // Clear playing / recording status
  demo->playing = false;
  demo->recording = false;

  return 0;
}

int demo_deinit(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;

  // Close file and clear pointer if necessary
  if(demo->fp != NULL)
  {
    fclose(demo->fp);
    demo->fp = NULL;
  }

  // Release input hooks
  set_forced_input(NULL, NULL, NULL);

  // Reinit to clear everything else
  demo_init(mzx_world);

  return 0;
}

int demo_start_frame(struct world *mzx_world)
{
  if(mzx_world->demo.playing)
  {
    return demo_play_start_frame(mzx_world);
  }

  else if(mzx_world->demo.recording)
  {
    return demo_record_start_frame(mzx_world);
  }

  else
  {
    return 0;
  }
}

int demo_end_frame(struct world *mzx_world)
{
  if(mzx_world->demo.playing)
  {
    return demo_play_end_frame(mzx_world);
  }

  else if(mzx_world->demo.recording)
  {
    return demo_record_end_frame(mzx_world);
  }

  else
  {
    return 0;
  }
}

//
// RECORDING
//

int demo_record_init(struct world *mzx_world, const char *file_name)
{
  struct demo_runtime *demo = &mzx_world->demo;

  // Enable recording and open file
  demo->recording = true;
  demo->fp = fopen_unsafe(file_name, "wb");

  // Magic header
  fputc('D', demo->fp);
  fputc('M', demo->fp);
  fputc('Z', demo->fp);
  fputc('x', demo->fp); // version: 'x' = experimental

  // TODO: more stuff

  unlink("__demo1.sav");

  return 0;
}

int demo_record_start_frame(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;
  const struct buffered_status *current_input = load_status();
  struct demo_frame *frame = &(demo->current_frame);

  // Copy the new status over the old status
  memcpy(
    &(frame->buffer),
    current_input,
    sizeof(frame->buffer));

  // TODO: fill other fields in

  // Fetch latest seed
  frame->random_seed = rng_get_seed();

  // Serialise the new status
  // Use RLE for encoding runs of no change
  // If any frames genuinely do not change, then mark them as unchanged
  {
    uint8_t *frame_old = (uint8_t *)&(demo->previous_frame);
    uint8_t *frame_new = (uint8_t *)frame;
    uint16_t rle_accum = 0;
    FILE *fp = demo->fp;
    size_t i;

    if(!memcmp(frame_old, frame_new, sizeof(*frame)))
    {
      fputc(0xFF, fp); // Mark this as a blank frame
    }
    else
    {
      for(i = 0; i < sizeof(*frame); i++)
      {
        if(rle_accum == 0x7EFF || frame_old[i] != frame_new[i] || i+1 == sizeof(*frame))
        {
          if(rle_accum >= 0x80)
          {
            fputc(((rle_accum>>8)|0x80)&0xFF, fp);
            fputc((rle_accum)&0xFF, fp);
          }
          else
          {
            fputc(rle_accum, fp);
          }

          fputc(frame_new[i], fp);
          rle_accum = 0;
        }
        else
        {
          rle_accum += 1;
        }
      }
      fputc(0, fp);
    }
  }

  // Copy the new frame over the old one
  memcpy(
   &(demo->previous_frame),
   &(demo->current_frame),
   sizeof(demo->current_frame));

  // Clear "is first frame" flag
  frame->flags &= ~DEMO_FRAME_IS_FIRST;

  // Set hook on input bump
  set_forced_input(demo_forced_input_callback, mzx_world, NULL);

  return 0;
}

int demo_record_end_frame(struct world *mzx_world)
{
  return 0;
}

//
// PLAYBACK
//

int demo_play_init(struct world *mzx_world, const char *file_name)
{
  struct demo_runtime *demo = &mzx_world->demo;
  char magic[4];

  // Enable playback and open file
  demo->fp = fopen_unsafe(file_name, "rb");

  if(demo->fp == NULL)
  {
    error_message(E_FILE_DOES_NOT_EXIST, 0, NULL);
    return 1;
  }

  // Read magic header
  magic[0] = fgetc(demo->fp);
  magic[1] = fgetc(demo->fp);
  magic[2] = fgetc(demo->fp);
  magic[3] = fgetc(demo->fp);
  if(magic[0] != 'D' || magic[1] != 'M' || magic[2] != 'Z' || magic[3] != 'x')
  {
    fclose(demo->fp);
    error_message(E_DEMO_FILE_INVALID, 0, NULL);
    return 1;
  }

  unlink("__demo1.sav");

  demo->playing = true;

  return 0;
}

int demo_play_start_frame(struct world *mzx_world)
{
  struct demo_runtime *demo = &mzx_world->demo;
  struct demo_frame *frame = &(demo->current_frame);
  unsigned long long got_seed;

  // Copy the new frame over the old one
  memcpy(
   &(demo->previous_frame),
   &(demo->current_frame),
   sizeof(demo->current_frame));

  // Deserialise the new status
  // Use RLE for encoding runs of no change
  {
    uint8_t *frame_new = (uint8_t *)frame;
    FILE *fp = demo->fp;
    uint16_t rle_accum = fgetc(fp);
    size_t i;

    // Check if this is a blank frame
    if(rle_accum != 0xFF)
    {
      // Not a blank frame, so decode it
      if(rle_accum >= 0x80)
      {
        rle_accum &= 0x7F;
        rle_accum <<= 8;
        rle_accum |= fgetc(fp);
      }

      for(i = 0; i < sizeof(*frame); i++)
      {
        if(rle_accum == 0)
        {
          frame_new[i] = fgetc(fp);
          rle_accum = fgetc(fp);
          if(rle_accum >= 0x80)
          {
            rle_accum &= 0x7F;
            rle_accum <<= 8;
            rle_accum |= fgetc(fp);
          }
        }
        else
        {
          rle_accum -= 1;
        }
      }
    }
  }

  // TODO: use other fields

  // Update seed
  got_seed = rng_get_seed();
  if(got_seed != frame->random_seed)
  {
    if((frame->flags & DEMO_FRAME_IS_FIRST) == 0)
    {
      debug("DESYNC: Mismatched seed - have %016llX, got %016llX\n", got_seed, frame->random_seed);
      // TODO: pop up an error message
    }
    rng_set_seed(frame->random_seed);
  }

  // Force the new input
  set_forced_input(demo_forced_input_callback, mzx_world, &(frame->buffer));

  return 0;
}

int demo_play_end_frame(struct world *mzx_world)
{
  // Nothing, really.

  return 0;
}
