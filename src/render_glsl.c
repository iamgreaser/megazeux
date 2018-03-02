/* MegaZeux
 *
 * Copyright (C) 2008 Joel Bouchard Lamontagne <logicow@gmail.com>
 * Copyright (C) 2006-2007 Gilead Kutnick <exophase@adelphia.net>
 * Copyright (C) 2009 Alistair John Strachan <alistair@devzero.co.uk>
 * Copyright (C) 2017 Dr Lancer-X <drlancer@megazeux.org>
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "platform.h"
#include "render.h"
#include "render_layer.h"
#include "renderers.h"
#include "util.h"
#include "data.h"

#ifdef CONFIG_SDL
#include "render_sdl.h"
#endif

#ifdef CONFIG_EGL
#include <GLES2/gl2.h>
#include "render_egl.h"
typedef GLenum GLiftype;
#else
typedef GLint GLiftype;
#endif

#include "render_gl.h"

#define CHARSET_COLS 32
#define CHARSET_ROWS (FULL_CHARSET_SIZE / CHARSET_COLS)
#define BG_WIDTH 128
#define BG_HEIGHT 32

static inline int next_power_of_two(int v)
{
  for (int i = 1; i <= 65536; i *= 2)
    if (i >= v) return i;
  debug("Couldn't find power of two for %d\n", v);
  return v;
}

enum
{
  ATTRIB_POSITION,
  ATTRIB_TEXCOORD,
  ATTRIB_COLOR,
};

static struct
{
  void (GL_APIENTRY *glAttachShader)(GLuint program, GLuint shader);
  void (GL_APIENTRY *glBindAttribLocation)(GLuint program, GLuint index,
   const char *name);
  void (GL_APIENTRY *glBindTexture)(GLenum target, GLuint texture);
  void (GL_APIENTRY *glBlendFunc)(GLenum sfactor, GLenum dfactor);
  void (GL_APIENTRY *glClear)(GLbitfield mask);
  void (GL_APIENTRY *glCompileShader)(GLuint shader);
  void (GL_APIENTRY *glCopyTexImage2D)(GLenum target, GLint level,
   GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height,
   GLint border);
  GLenum (GL_APIENTRY *glCreateProgram)(void);
  GLenum (GL_APIENTRY *glCreateShader)(GLenum type);
  void (GL_APIENTRY *glDisable)(GLenum cap);
  void (GL_APIENTRY *glDisableVertexAttribArray)(GLuint index);
  void (GL_APIENTRY *glDrawArrays)(GLenum mode, GLint first, GLsizei count);
  void (GL_APIENTRY *glEnable)(GLenum cap);
  void (GL_APIENTRY *glEnableVertexAttribArray)(GLuint index);
  void (GL_APIENTRY *glGenTextures)(GLsizei n, GLuint *textures);
  GLenum (GL_APIENTRY *glGetError)(void);
  void (GL_APIENTRY *glGetProgramInfoLog)(GLuint program, GLsizei bufsize,
   GLsizei *len, char *infolog);
  void (GL_APIENTRY *glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
  void (GL_APIENTRY *glGetShaderInfoLog)(GLuint shader, GLsizei bufsize,
   GLsizei *len, char *infolog);
  const GLubyte* (GL_APIENTRY *glGetString)(GLenum name);
  void (GL_APIENTRY *glLinkProgram)(GLuint program);
  void (GL_APIENTRY *glShaderSource)(GLuint shader, GLsizei count,
   const char **strings, const GLint *length);
  void (GL_APIENTRY *glTexImage2D)(GLenum target, GLint level,
   GLiftype internalformat, GLsizei width, GLsizei height, GLint border,
   GLenum format, GLenum type, const GLvoid *pixels);
  void (GL_APIENTRY *glTexParameterf)(GLenum target, GLenum pname,
   GLfloat param);
  void (GL_APIENTRY *glTexSubImage2D)(GLenum target, GLint level,
   GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
   GLenum format, GLenum type, const void *pixels);
  void (GL_APIENTRY *glUseProgram)(GLuint program);
  void (GL_APIENTRY *glVertexAttribPointer)(GLuint index, GLint size,
   GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
  void (GL_APIENTRY *glViewport)(GLint x, GLint y, GLsizei width,
   GLsizei height);
  void (GL_APIENTRY *glDeleteShader)(GLuint shader);
  void (GL_APIENTRY *glDetachShader)(GLuint program, GLuint shader);
  void (GL_APIENTRY *glDeleteProgram)(GLuint program);
  void (GL_APIENTRY *glGetAttachedShaders)(GLuint program, GLsizei maxCount,
   GLsizei *count, GLuint *shaders);
}
glsl;

static const struct dso_syms_map glsl_syms_map[] =
{
  { "glAttachShader",             (fn_ptr *)&glsl.glAttachShader },
  { "glBindAttribLocation",       (fn_ptr *)&glsl.glBindAttribLocation },
  { "glBindTexture",              (fn_ptr *)&glsl.glBindTexture },
  { "glBlendFunc",                (fn_ptr *)&glsl.glBlendFunc },
  { "glClear",                    (fn_ptr *)&glsl.glClear },
  { "glCompileShader",            (fn_ptr *)&glsl.glCompileShader },
  { "glCopyTexImage2D",           (fn_ptr *)&glsl.glCopyTexImage2D },
  { "glCreateProgram",            (fn_ptr *)&glsl.glCreateProgram },
  { "glCreateShader",             (fn_ptr *)&glsl.glCreateShader },
  { "glDeleteProgram",            (fn_ptr *)&glsl.glDeleteProgram },
  { "glDeleteShader",             (fn_ptr *)&glsl.glDeleteShader },
  { "glDetachShader",             (fn_ptr *)&glsl.glDetachShader },
  { "glDisable",                  (fn_ptr *)&glsl.glDisable },
  { "glDisableVertexAttribArray", (fn_ptr *)&glsl.glDisableVertexAttribArray },
  { "glDrawArrays",               (fn_ptr *)&glsl.glDrawArrays },
  { "glEnable",                   (fn_ptr *)&glsl.glEnable },
  { "glEnableVertexAttribArray",  (fn_ptr *)&glsl.glEnableVertexAttribArray },
  { "glGenTextures",              (fn_ptr *)&glsl.glGenTextures },
  { "glGetAttachedShaders",       (fn_ptr *)&glsl.glGetAttachedShaders },
  { "glGetError",                 (fn_ptr *)&glsl.glGetError },
  { "glGetProgramInfoLog",        (fn_ptr *)&glsl.glGetProgramInfoLog },
  { "glGetShaderInfoLog",         (fn_ptr *)&glsl.glGetShaderInfoLog },
  { "glGetShaderiv",              (fn_ptr *)&glsl.glGetShaderiv },
  { "glGetString",                (fn_ptr *)&glsl.glGetString },
  { "glLinkProgram",              (fn_ptr *)&glsl.glLinkProgram },
  { "glShaderSource",             (fn_ptr *)&glsl.glShaderSource },
  { "glTexImage2D",               (fn_ptr *)&glsl.glTexImage2D },
  { "glTexParameterf",            (fn_ptr *)&glsl.glTexParameterf },
  { "glTexSubImage2D",            (fn_ptr *)&glsl.glTexSubImage2D },
  { "glUseProgram",               (fn_ptr *)&glsl.glUseProgram },
  { "glVertexAttribPointer",      (fn_ptr *)&glsl.glVertexAttribPointer },
  { "glViewport",                 (fn_ptr *)&glsl.glViewport },
  { NULL, NULL}
};

#define gl_check_error() gl_error(__FILE__, __LINE__, glsl.glGetError)

struct glsl_render_data
{
#ifdef CONFIG_EGL
  struct egl_render_data egl;
#endif
#ifdef CONFIG_SDL
  struct sdl_render_data sdl;
#endif
  Uint32 *pixels;
  Uint32 charset_texture[CHAR_H * FULL_CHARSET_SIZE * CHAR_W];
  Uint32 background_texture[BG_WIDTH * BG_HEIGHT];
  GLuint texture_number[3];
  GLubyte palette[3 * FULL_PAL_SIZE];
  Uint8 remap_texture;
  Uint8 remap_char[FULL_CHARSET_SIZE];
  GLuint scaler_program;
  GLuint tilemap_program;
  GLuint tilemap_smzx_program;
  GLuint mouse_program;
  GLuint cursor_program;
  struct config_info *conf;
};

static char *source_cache[SHADERS_CURSOR_FRAG - SHADERS_SCALER_VERT + 1];

#define INFO_MAX 1000

static GLint glsl_verify_compile(struct glsl_render_data *render_data,
 GLenum shader)
{
  GLint compile_status;
  char buffer[INFO_MAX];
  int len = 0;

  glsl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
  glsl.glGetShaderInfoLog(shader, INFO_MAX - 1, &len, buffer);
  buffer[len] = 0;

  if(len > 0)
    warn("%s", buffer);

  if(compile_status == GL_FALSE)
    warn("Shader compilation failed!\n");

  return compile_status;
}

static void glsl_verify_link(struct glsl_render_data *render_data,
 GLenum program)
{
  char buffer[INFO_MAX];
  int len = 0;

  glsl.glGetProgramInfoLog(program, INFO_MAX - 1, &len, buffer);
  buffer[len] = 0;

  if(len > 0)
    warn("%s", buffer);
}

static char *glsl_load_string(const char *filename)
{
  char *buffer = NULL;
  unsigned long size;
  FILE *f;

  f = fopen_unsafe(filename, "rb");
  if(!f)
    goto err_out;

  size = ftell_and_rewind(f);
  if(!size)
    goto err_close;

  buffer = cmalloc(size + 1);
  buffer[size] = '\0';

  if(fread(buffer, size, 1, f) != 1)
  {
    free(buffer);
    buffer = NULL;
  }

err_close:
  fclose(f);
err_out:
  return buffer;
}

static GLuint glsl_load_shader(struct graphics_data *graphics,
 enum resource_id res, GLenum type)
{
  struct glsl_render_data *render_data = graphics->render_data;
  int index = res - SHADERS_SCALER_VERT;
  int loaded_config = 0;
  bool is_user_scaler;
  GLint compile_status;
  GLenum shader;

  assert(res >= SHADERS_SCALER_VERT && res <= SHADERS_CURSOR_FRAG);

  is_user_scaler = (res == SHADERS_SCALER_VERT || res == SHADERS_SCALER_FRAG) &&
   graphics->gl_scaling_shader[0];

  // If we've already seen this shader, it's loaded, and we don't
  // need to do any more file I/O.

  if(!source_cache[index])
  {
    if(is_user_scaler)
    {
      // Try to load these from config before loading the default.

      char *path = cmalloc(MAX_PATH);

      strcpy(path, mzx_res_get_by_id(SHADERS_SCALER_DIRECTORY));

      switch(res)
      {
        case SHADERS_SCALER_VERT:
          sprintf(path + strlen(path), "%s%s.vert",
           DIR_SEPARATOR, graphics->gl_scaling_shader);
          break;

        case SHADERS_SCALER_FRAG:
          sprintf(path + strlen(path), "%s%s.frag",
           DIR_SEPARATOR, graphics->gl_scaling_shader);
          break;

        default:
          break;
      }

      source_cache[index] = glsl_load_string(path);

      if(source_cache[index])
      {
        loaded_config = 1;
      }
      else
      {
        debug("Failed to find shader '%s'; falling back to default\n", path);
        loaded_config = 0;
      }

      free(path);
    }

    if(!loaded_config)
    {
      is_user_scaler = false;

      if(res == SHADERS_SCALER_FRAG)
        graphics->gl_scaling_shader[0] = 0;

      source_cache[index] = glsl_load_string(mzx_res_get_by_id(res));
      if(!source_cache[index])
      {
        warn("Failed to find required shader '%s'\n", mzx_res_get_by_id(res));
        return 0;
      }
    }
  }

  shader = glsl.glCreateShader(type);

#ifdef CONFIG_EGL
  {
    const GLchar *sources[2];
    GLint lengths[2];

    sources[0] = "precision mediump float;";
    sources[1] = source_cache[index];

    lengths[0] = (GLint)strlen(sources[0]);
    lengths[1] = (GLint)strlen(source_cache[index]);

    glsl.glShaderSource(shader, 2, sources, lengths);
  }
#else
  {
    GLint length = (GLint)strlen(source_cache[index]);
    glsl.glShaderSource(shader, 1, (const char **)&source_cache[index], &length);
  }
#endif

  glsl.glCompileShader(shader);
  compile_status = glsl_verify_compile(render_data, shader);

  if(compile_status == GL_FALSE)
  {
    if(is_user_scaler)
    {
      // Attempt the default shader
      graphics->gl_scaling_shader[0] = 0;
      free(source_cache[index]);
      source_cache[index] = NULL;

      return glsl_load_shader(graphics, res, type);
    }
    else
    {
      warn("Required shader '%s' failed to compile.\n", mzx_res_get_by_id(res));
      return 0;
    }
  }

  return shader;
}

static GLuint glsl_load_program(struct graphics_data *graphics,
 enum resource_id v_res, enum resource_id f_res)
{
  GLenum vertex, fragment;
  GLuint program = 0;
  char *path;

  path = cmalloc(MAX_PATH);

  vertex = glsl_load_shader(graphics, v_res, GL_VERTEX_SHADER);
  if(!vertex)
    goto err_free_path;

  fragment = glsl_load_shader(graphics, f_res, GL_FRAGMENT_SHADER);
  if(!fragment)
    goto err_free_path;

  program = glsl.glCreateProgram();

  glsl.glAttachShader(program, vertex);
  glsl.glAttachShader(program, fragment);

err_free_path:
  free(path);
  return program;
}

static void glsl_delete_shaders(GLuint program)
{
  GLuint shaders[16];
  GLsizei shader_count, i;
  glsl.glGetAttachedShaders(program, 16, &shader_count, shaders);
  gl_check_error();
  for (i = 0; i < shader_count; i++) {
    glsl.glDetachShader(program, shaders[i]);
    gl_check_error();
    glsl.glDeleteShader(shaders[i]);
    gl_check_error();
  }
}

static void glsl_load_shaders(struct graphics_data *graphics)
{
  struct glsl_render_data *render_data = graphics->render_data;

  render_data->scaler_program = glsl_load_program(graphics,
   SHADERS_SCALER_VERT, SHADERS_SCALER_FRAG);
  if(render_data->scaler_program)
  {
    glsl.glBindAttribLocation(render_data->scaler_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->scaler_program,
     ATTRIB_TEXCOORD, "Texcoord");
    glsl.glLinkProgram(render_data->scaler_program);
    glsl_verify_link(render_data, render_data->scaler_program);
    glsl_delete_shaders(render_data->scaler_program);
  }

  render_data->tilemap_program = glsl_load_program(graphics,
   SHADERS_TILEMAP_VERT, SHADERS_TILEMAP_FRAG);
  if(render_data->tilemap_program)
  {
    glsl.glBindAttribLocation(render_data->tilemap_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->tilemap_program,
     ATTRIB_TEXCOORD, "Texcoord");
    glsl.glLinkProgram(render_data->tilemap_program);
    glsl_verify_link(render_data, render_data->tilemap_program);
    glsl_delete_shaders(render_data->tilemap_program);
  }

  render_data->tilemap_smzx_program = glsl_load_program(graphics,
   SHADERS_TILEMAP_VERT, SHADERS_TILEMAP_SMZX_FRAG);
  if(render_data->tilemap_smzx_program)
  {
    glsl.glBindAttribLocation(render_data->tilemap_smzx_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->tilemap_smzx_program,
     ATTRIB_TEXCOORD, "Texcoord");
    glsl.glLinkProgram(render_data->tilemap_smzx_program);
    glsl_verify_link(render_data, render_data->tilemap_smzx_program);
    glsl_delete_shaders(render_data->tilemap_smzx_program);
  }
  /*
  render_data->tilemap_smzx3_program = glsl_load_program(graphics,
   SHADERS_TILEMAP_VERT, SHADERS_TILEMAP_SMZX3_FRAG);
  if(render_data->tilemap_smzx3_program)
  {
    glsl.glBindAttribLocation(render_data->tilemap_smzx3_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->tilemap_smzx3_program,
     ATTRIB_TEXCOORD, "Texcoord");
    glsl.glLinkProgram(render_data->tilemap_smzx3_program);
    glsl_verify_link(render_data, render_data->tilemap_smzx3_program);
  }
  */
  render_data->mouse_program = glsl_load_program(graphics,
   SHADERS_MOUSE_VERT, SHADERS_MOUSE_FRAG);
  if(render_data->mouse_program)
  {
    glsl.glBindAttribLocation(render_data->mouse_program,
     ATTRIB_POSITION, "Position");
    glsl.glLinkProgram(render_data->mouse_program);
    glsl_verify_link(render_data, render_data->mouse_program);
    glsl_delete_shaders(render_data->mouse_program);
  }

  render_data->cursor_program  = glsl_load_program(graphics,
   SHADERS_CURSOR_VERT, SHADERS_CURSOR_FRAG);
  if(render_data->cursor_program)
  {
    glsl.glBindAttribLocation(render_data->cursor_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->cursor_program,
     ATTRIB_COLOR, "Color");
    glsl.glLinkProgram(render_data->cursor_program);
    glsl_verify_link(render_data, render_data->cursor_program);
    glsl_delete_shaders(render_data->cursor_program);
  }
}

static bool glsl_init_video(struct graphics_data *graphics,
 struct config_info *conf)
{
  struct glsl_render_data *render_data;

  render_data = cmalloc(sizeof(struct glsl_render_data));
  if(!render_data)
    return false;

  if(!GL_LoadLibrary(GL_LIB_PROGRAMMABLE))
    goto err_free;

  memset(render_data, 0, sizeof(struct glsl_render_data));
  graphics->render_data = render_data;

  graphics->gl_vsync = conf->gl_vsync;
  graphics->allow_resize = conf->allow_resize;
  graphics->gl_filter_method = conf->gl_filter_method;
  graphics->gl_scaling_shader = conf->gl_scaling_shader;
  graphics->ratio = conf->video_ratio;
  graphics->bits_per_pixel = 32;

  // OpenGL only supports 16/32bit colour
  if(conf->force_bpp == 16 || conf->force_bpp == 32)
    graphics->bits_per_pixel = conf->force_bpp;

  render_data->pixels = cmalloc(sizeof(Uint32) * GL_POWER_2_WIDTH *
   GL_POWER_2_HEIGHT);
  render_data->conf = conf;
  if(!render_data->pixels)
    goto err_free;

  if(!set_video_mode())
    goto err_free;

  return true;

err_free:
#if SDL_VERSION_ATLEAST(2,0,0)
  if(render_data->sdl.context)
  {
    SDL_GL_DeleteContext(render_data->sdl.context);
    render_data->sdl.context = NULL;
  }
  if(render_data->sdl.window)
  {
    SDL_DestroyWindow(render_data->sdl.window);
    render_data->sdl.window = NULL;
  }
#endif // SDL_VERSION_ATLEAST(2,0,0)
  free(render_data);
  return false;
}

static void glsl_free_video(struct graphics_data *graphics)
{
  int i;

  for(i = 0; i < SHADERS_CURSOR_FRAG - SHADERS_SCALER_VERT + 1; i++)
    if(source_cache[i])
      free((void *)source_cache[i]);

  free(graphics->render_data);
  graphics->render_data = NULL;
}

static void glsl_remap_charsets(struct graphics_data *graphics)
{
  struct glsl_render_data *render_data = graphics->render_data;
  render_data->remap_texture = true;
}

// FIXME: Many magic numbers
static void glsl_resize_screen(struct graphics_data *graphics,
 int width, int height)
{
  struct glsl_render_data *render_data = graphics->render_data;

  glsl.glViewport(0, 0, width, height);
  gl_check_error();

  glsl.glGenTextures(3, render_data->texture_number);
  gl_check_error();

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[0]);
  gl_check_error();

  gl_set_filter_method(CONFIG_GL_FILTER_LINEAR, glsl.glTexParameterf);

  glsl.glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

  memset(render_data->pixels, 255,
   sizeof(Uint32) * GL_POWER_2_WIDTH * GL_POWER_2_HEIGHT);

  //Uint32 *big_array = cmalloc(sizeof(Uint32) * 512 * 1024);
  //memset(big_array, 255, sizeof(Uint32) * 512 * 1024);

  glsl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GL_POWER_2_WIDTH,
   GL_POWER_2_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
   render_data->pixels);
  gl_check_error();

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  gl_set_filter_method(CONFIG_GL_FILTER_NEAREST, glsl.glTexParameterf);
  glsl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 1024, 0, GL_RGBA,
   GL_UNSIGNED_BYTE, NULL);
  gl_check_error();

  //free(big_array);

  glsl_remap_charsets(graphics);

  glsl_load_shaders(graphics);
}

static bool glsl_set_video_mode(struct graphics_data *graphics,
 int width, int height, int depth, bool fullscreen, bool resize)
{
  gl_set_attributes(graphics);

  if(!gl_set_video_mode(graphics, width, height, depth, fullscreen, resize))
    return false;
  
  gl_set_attributes(graphics);

  if(!gl_load_syms(glsl_syms_map))
    return false;

  // We need a specific version of OpenGL; desktop GL must be 2.0.
  // All OpenGL ES 2.0 implementations are supported, so don't do
  // the check with EGL configurations (EGL implies OpenGL ES).
#ifndef CONFIG_EGL
  {
    static bool initialized = false;

    if(!initialized)
    {
      const char *version;
      double version_float;

      version = (const char *)glsl.glGetString(GL_VERSION);
      if(!version)
        return false;

      version_float = atof(version);
      if(version_float < 2.0)
      {
        warn("Need >= OpenGL 2.0, got OpenGL %.1f.\n", version_float);
        return false;
      }
      
      initialized = true;
    }
  }
#endif

  glsl_resize_screen(graphics, width, height);
  return true;
}

static bool glsl_auto_set_video_mode(struct graphics_data *graphics,
 int width, int height, int depth, bool fullscreen, bool resize)
{
  const char *renderer;
  struct glsl_render_data *render_data = graphics->render_data;

  if(glsl_set_video_mode(graphics, width, height, depth, fullscreen, resize))
  {

    // Driver blacklist. If the renderer is on the blacklist we want to fall
    // back on software as these renderers have disastrous GLSL performance.

    renderer = (const char *)glsl.glGetString(GL_RENDERER);
    if(strstr(renderer, "llvmpipe"))
    {
      warn("Detected MESA software renderer. Disabling glsl.\n");
      return false;
    }

    // Overwrite the original video_output now that we know glsl works
    strcpy(render_data->conf->video_output, "glsl");

    return true;
  }
  return false;
}

static void glsl_remap_char(struct graphics_data *graphics, Uint16 chr)
{
  struct glsl_render_data *render_data = graphics->render_data;
  render_data->remap_char[chr] = true;
}

static void glsl_remap_charbyte(struct graphics_data *graphics,
 Uint16 chr, Uint8 byte)
{
  glsl_remap_char(graphics, chr);
}

static int *glsl_char_bitmask_to_texture(signed char *c, int *p)
{
  // This tests the 7th bit, if 0, result is 0.
  // If 1, result is 255 (because of sign extended bit shift!).
  // Note the use of char constants to force 8bit calculation.
  *(p++) = *c << 24 >> 31;
  *(p++) = *c << 25 >> 31;
  *(p++) = *c << 26 >> 31;
  *(p++) = *c << 27 >> 31;
  *(p++) = *c << 28 >> 31;
  *(p++) = *c << 29 >> 31;
  *(p++) = *c << 30 >> 31;
  *(p++) = *c << 31 >> 31;
  return p;
}

static inline void glsl_do_remap_charsets(struct graphics_data *graphics)
{
  struct glsl_render_data *render_data = graphics->render_data;
  signed char *c = (signed char *)graphics->charset;
  int *p = (int *)render_data->charset_texture;
  unsigned int i, j, k;

  for(i = 0; i < CHARSET_ROWS / 2; i++, c += -CHAR_H + (CHARSET_COLS * 2) * CHAR_H)
    for(j = 0; j < CHAR_H; j++, c += -(CHARSET_COLS * 2) * CHAR_H + 1)
      for(k = 0; k < CHARSET_COLS * 2; k++, c += CHAR_H)
        p = glsl_char_bitmask_to_texture(c, p);

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CHARSET_COLS * CHAR_W * 2, CHARSET_ROWS / 2 * CHAR_H, GL_RGBA,
    GL_UNSIGNED_BYTE, render_data->charset_texture);
  gl_check_error();
}

static inline void glsl_do_remap_char(struct graphics_data *graphics,
 Uint16 chr)
{
  struct glsl_render_data *render_data = graphics->render_data;
  signed char *c = (signed char *)graphics->charset;
  int *p = (int *)render_data->charset_texture;
  unsigned int i;

  c += chr * 14;

  for(i = 0; i < 14; i++, c++)
    p = glsl_char_bitmask_to_texture(c, p);

  //glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, chr % 32 * 8, chr / 32 * 14, 8, 14,GL_RGBA, GL_UNSIGNED_BYTE, render_data->charset_texture);
  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, chr % (CHARSET_COLS * 2) * CHAR_W, chr / (CHARSET_COLS * 2) * CHAR_H, CHAR_W, CHAR_H,
   GL_RGBA, GL_UNSIGNED_BYTE, render_data->charset_texture);
  gl_check_error();
}

static void glsl_update_colors(struct graphics_data *graphics,
 struct rgb_color *palette, Uint32 count)
{
  struct glsl_render_data *render_data = graphics->render_data;
  Uint32 i;
  for(i = 0; i < count; i++)
  {
    graphics->flat_intensity_palette[i] = (0xFF << 24) | (palette[i].b << 16) |
     (palette[i].g << 8) | palette[i].r;
    render_data->palette[i*3  ] = (GLubyte)palette[i].r;
    render_data->palette[i*3+1] = (GLubyte)palette[i].g;
    render_data->palette[i*3+2] = (GLubyte)palette[i].b;
  }
}

static void glsl_render_graph(struct graphics_data *graphics)
{
  struct glsl_render_data *render_data = graphics->render_data;
  struct char_element *src = graphics->text_video;
  Uint32 *colorptr, *dest, i, j;
  int width, height;

  static const float tex_coord_array_single[2 * 4] = {
    0.0f,          0.0f,
    0.0f,          25.0f,
    25.0f / 80.0f, 0.0f,
    25.0f / 80.0f, 25.0f,
  };

  get_context_width_height(graphics, &width, &height);
  if(width < 640 || height < 350)
  {
    if(width >= 1024)
      width = 1024;
    if(height >= 512)
      height = 512;
  }
  else
  {
    width = 640;
    height = 350;
  }

  glsl.glViewport(0, 0, width, height);
  gl_check_error();

  if(graphics->screen_mode == 0)
    glsl.glUseProgram(render_data->tilemap_program);
  else
    glsl.glUseProgram(render_data->tilemap_smzx_program);
  gl_check_error();

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  if(render_data->remap_texture)
  {
    glsl_do_remap_charsets(graphics);
    render_data->remap_texture = false;
    memset(render_data->remap_char, false, sizeof(Uint8) * FULL_CHARSET_SIZE);
  }
  else
  {
    for(i = 0; i < FULL_CHARSET_SIZE; i++)
    {
      if(render_data->remap_char[i])
      {
        glsl_do_remap_char(graphics, i);
        render_data->remap_char[i] = false;
      }
    }
  }

  dest = render_data->background_texture;

  for(i = 0; i < SCREEN_W * SCREEN_H; i++, dest++, src++) {
    *dest = (src->char_value<<18) + (src->bg_color<<9) + src->fg_color;
  }

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 901, SCREEN_W, SCREEN_H, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  colorptr = graphics->flat_intensity_palette;
  dest = render_data->background_texture;

  for(i = 0; i < graphics->protected_pal_position + 16; i++, dest++, colorptr++)
    *dest = *colorptr;

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 896, FULL_PAL_SIZE, 1, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  dest = render_data->background_texture;
  for (i = 0; i < 4; i++)
    for(j = 0; j < SMZX_PAL_SIZE; j++, dest++)
      *dest = graphics->smzx_indices[j * 4 + i];

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 897, SMZX_PAL_SIZE, 4, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  glsl.glEnableVertexAttribArray(ATTRIB_POSITION);
  glsl.glEnableVertexAttribArray(ATTRIB_TEXCOORD);

  glsl.glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0,
   vertex_array_single);
  gl_check_error();

  glsl.glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0,
   tex_coord_array_single);
  gl_check_error();

  glsl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl_check_error();

  glsl.glDisableVertexAttribArray(ATTRIB_POSITION);
  glsl.glDisableVertexAttribArray(ATTRIB_TEXCOORD);
}


static void glsl_render_layer(struct graphics_data *graphics, struct video_layer *layer)
{
  struct glsl_render_data *render_data = graphics->render_data;
  struct char_element *src = layer->data;
  Uint32 *colorptr, *dest, i, j;
  int width, height;
  Uint32 char_value, fg_color, bg_color;

  float v_left = 1.0f * (layer->x) / 640.0f * 2.0f - 1.0f;
  float v_right = 1.0f * (layer->x + (int)layer->w * CHAR_W) / 640.0f * 2.0f - 1.0f;
  float v_top = 1.0f * (layer->y) / 350.0f * 2.0f - 1.0f;  
  float v_bottom = 1.0f * (layer->y + (int)layer->h * CHAR_H) / 350.0f * 2.0f - 1.0f;

  float vertex_array_single[2 * 4] = {
    v_left, -v_top,
    v_left, -v_bottom,
    v_right, -v_top,
    v_right, -v_bottom,
  };
  
  float t_left = 1.0f * (layer->x) / 640.0f * 25.0f / 80.0f;
  float t_right = 1.0f * (layer->x + (int)layer->w * CHAR_W) / 640.0f * 25.0f / 80.0f;
  float t_top = 1.0f * (layer->y) / 350.0f * 25.0f;
  float t_bottom = 1.0f * (layer->y + (int)layer->h * CHAR_H)  / 350.0f * 25.0f;

  float tex_coord_array_single[2 * 4] = {
    t_left - t_left, t_top - t_top,
    t_left - t_left, t_bottom - t_top,
    t_right - t_left, t_top - t_top,
    t_right - t_left, t_bottom - t_top,
  };

  get_context_width_height(graphics, &width, &height);
  if(width < 640 || height < 350)
  {
    if(width >= 1024)
      width = 1024;
    if(height >= 512)
      height = 512;
  }
  else
  {
    width = 640;
    height = 350;
  }

  glsl.glViewport(0, 0, width, height);
  gl_check_error();

  if(layer->mode == 0)
    glsl.glUseProgram(render_data->tilemap_program);
  else
    glsl.glUseProgram(render_data->tilemap_smzx_program);
  gl_check_error();

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  if(render_data->remap_texture)
  {
    glsl_do_remap_charsets(graphics);
    render_data->remap_texture = false;
    memset(render_data->remap_char, false, sizeof(Uint8) * FULL_CHARSET_SIZE);
  }
  else
  {
    for(i = 0; i < FULL_CHARSET_SIZE; i++)
    {
      if(render_data->remap_char[i])
      {
        glsl_do_remap_char(graphics, i);
        render_data->remap_char[i] = false;
      }
    }
  }

  dest = render_data->background_texture;

  for(i = 0; i < layer->w * layer->h; i++, dest++, src++) {
    char_value = src->char_value;
    bg_color = src->bg_color;
    fg_color = src->fg_color;
    if (char_value != 0xFFFF) {
      if (char_value < PROTECTED_CHARSET_POSITION) {
        char_value = (char_value + layer->offset) % PROTECTED_CHARSET_POSITION;
      }
      if (bg_color >= 16) bg_color = (bg_color & 0xF) + graphics->protected_pal_position;
      if (fg_color >= 16) fg_color = (fg_color & 0xF) + graphics->protected_pal_position;
    } else {
      bg_color = FULL_PAL_SIZE;
      fg_color = FULL_PAL_SIZE;
    }
    *dest = (char_value<<18) | (bg_color<<9) | fg_color;
  }

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 901, layer->w, layer->h, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  colorptr = graphics->flat_intensity_palette;
  dest = render_data->background_texture;

  for(i = 0; i < graphics->protected_pal_position + 16; i++, dest++, colorptr++)
  {
    *dest = *colorptr;
  }
  
  if (layer->transparent_col != -1)
    render_data->background_texture[layer->transparent_col] = 0x00000000;
  render_data->background_texture[FULL_PAL_SIZE] = 0x00000000;

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 896, FULL_PAL_SIZE + 1, 1, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  dest = render_data->background_texture;
  for (i = 0; i < 4; i++)
    for(j = 0; j < SMZX_PAL_SIZE; j++, dest++)
      *dest = graphics->smzx_indices[j * 4 + i];

  glsl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 897, SMZX_PAL_SIZE, 4, GL_RGBA,
   GL_UNSIGNED_BYTE, render_data->background_texture);
  gl_check_error();

  glsl.glEnableVertexAttribArray(ATTRIB_POSITION);
  glsl.glEnableVertexAttribArray(ATTRIB_TEXCOORD);

  glsl.glEnable(GL_BLEND);
  glsl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glsl.glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0,
   vertex_array_single);
  gl_check_error();

  glsl.glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0,
   tex_coord_array_single);
  gl_check_error();

  glsl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl_check_error();

  glsl.glDisableVertexAttribArray(ATTRIB_POSITION);
  glsl.glDisableVertexAttribArray(ATTRIB_TEXCOORD);
  glsl.glDisable(GL_BLEND);
}

static void glsl_render_cursor(struct graphics_data *graphics,
 Uint32 x, Uint32 y, Uint16 color, Uint8 lines, Uint8 offset)
{
  struct glsl_render_data *render_data = graphics->render_data;
  GLubyte *pal_base = &render_data->palette[color * 3];

  const float vertex_array[2 * 4] = {
    (x * 8)*2.0f/640.0f-1.0f,     (y * 14 + offset)*-2.0f/350.0f+1.0f,
    (x * 8)*2.0f/640.0f-1.0f,     (y * 14 + lines + offset)*-2.0f/350.0f+1.0f,
    (x * 8 + 8)*2.0f/640.0f-1.0f, (y * 14 + offset)*-2.0f/350.0f+1.0f,
    (x * 8 + 8)*2.0f/640.0f-1.0f, (y * 14 + lines + offset)*-2.0f/350.0f+1.0f
  };

  const float color_array[4 * 4] = {
    pal_base[0]/255.0f, pal_base[1]/255.0f, pal_base[2]/255.0f,
    pal_base[0]/255.0f, pal_base[1]/255.0f, pal_base[2]/255.0f,
    pal_base[0]/255.0f, pal_base[1]/255.0f, pal_base[2]/255.0f,
    pal_base[0]/255.0f, pal_base[1]/255.0f, pal_base[2]/255.0f,
  };

  glsl.glDisable(GL_TEXTURE_2D);

  glsl.glUseProgram(render_data->cursor_program);
  gl_check_error();

  glsl.glEnableVertexAttribArray(ATTRIB_POSITION);
  glsl.glEnableVertexAttribArray(ATTRIB_COLOR);

  glsl.glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0,
   vertex_array);
  gl_check_error();

  glsl.glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, 0,
   color_array);
  gl_check_error();

  glsl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl_check_error();

  glsl.glDisableVertexAttribArray(ATTRIB_POSITION);
  glsl.glDisableVertexAttribArray(ATTRIB_COLOR);
}

static void glsl_render_mouse(struct graphics_data *graphics,
 Uint32 x, Uint32 y, Uint8 w, Uint8 h)
{
  struct glsl_render_data *render_data = graphics->render_data;

  const float vertex_array[2 * 4] = {
     x*2.0f/640.0f-1.0f,       y*-2.0f/350.0f+1.0f,
     x*2.0f/640.0f-1.0f,      (y + h)*-2.0f/350.0f+1.0f,
    (x + w)*2.0f/640.0f-1.0f,  y*-2.0f/350.0f+1.0f,
    (x + w)*2.0f/640.0f-1.0f, (y + h)*-2.0f/350.0f+1.0f
  };

  glsl.glEnable(GL_BLEND);
  glsl.glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

  glsl.glUseProgram(render_data->mouse_program);
  gl_check_error();

  glsl.glEnableVertexAttribArray(ATTRIB_POSITION);

  glsl.glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0,
   vertex_array);
  gl_check_error();

  glsl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl_check_error();

  glsl.glDisableVertexAttribArray(ATTRIB_POSITION);

  glsl.glDisable(GL_BLEND);
}

static void glsl_sync_screen(struct graphics_data *graphics)
{
  struct glsl_render_data *render_data = graphics->render_data;
  int v_width, v_height, width, height;

  get_context_width_height(graphics, &width, &height);
  fix_viewport_ratio(width, height, &v_width, &v_height, graphics->ratio);
  glsl.glViewport((width - v_width) >> 1, (height - v_height) >> 1,
   v_width, v_height);

  if(width < 640 || height < 350)
  {
    width = (width < 1024) ? width : 1024;
    height = (height < 512) ? height : 512;
  }
  else
  {
    width = 640;
    height = 350;
  }

  glsl.glUseProgram(render_data->scaler_program);
  gl_check_error();

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[0]);
  gl_check_error();

  glsl.glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 1024, 512, 0);
  gl_check_error();

  glsl.glClear(GL_COLOR_BUFFER_BIT);
  gl_check_error();

  glsl.glEnableVertexAttribArray(ATTRIB_POSITION);
  glsl.glEnableVertexAttribArray(ATTRIB_TEXCOORD);
 
  glsl.glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0,
   vertex_array_single);
  gl_check_error();

  {
    const float tex_coord_array_single[2 * 4] = {
       0.0f,            height / 512.0f,
       0.0f,            0.0f,
       width / 1024.0f, height / 512.0f,
       width / 1024.0f, 0.0f,
    };

    glsl.glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0,
     tex_coord_array_single);
    gl_check_error();
  }

  glsl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl_check_error();

  glsl.glDisableVertexAttribArray(ATTRIB_POSITION);
  glsl.glDisableVertexAttribArray(ATTRIB_TEXCOORD);

  glsl.glBindTexture(GL_TEXTURE_2D, render_data->texture_number[1]);
  gl_check_error();

  gl_swap_buffers(graphics);

  glsl.glClear(GL_COLOR_BUFFER_BIT);
  gl_check_error();
}

static void glsl_switch_shader(struct graphics_data *graphics,
 const char *name)
{
  struct glsl_render_data *render_data = graphics->render_data;
  int res = SHADERS_SCALER_VERT - SHADERS_SCALER_VERT;
  if(source_cache[res]) free(source_cache[res]);
  source_cache[res] = NULL;

  res = SHADERS_SCALER_FRAG - SHADERS_SCALER_VERT;
  if(source_cache[res]) free(source_cache[res]);
  source_cache[res] = NULL;

  strncpy(graphics->gl_scaling_shader, name, 32);
  graphics->gl_scaling_shader[31] = 0;

  glsl.glDeleteProgram(render_data->scaler_program);
  gl_check_error();
  render_data->scaler_program = glsl_load_program(graphics,
   SHADERS_SCALER_VERT, SHADERS_SCALER_FRAG);
  if(render_data->scaler_program)
  {
    glsl.glBindAttribLocation(render_data->scaler_program,
     ATTRIB_POSITION, "Position");
    glsl.glBindAttribLocation(render_data->scaler_program,
     ATTRIB_TEXCOORD, "Texcoord");
    glsl.glLinkProgram(render_data->scaler_program);
    glsl_verify_link(render_data, render_data->scaler_program);
    glsl_delete_shaders(render_data->scaler_program);
  }
}

void render_glsl_register(struct renderer *renderer)
{
  memset(renderer, 0, sizeof(struct renderer));
  renderer->init_video = glsl_init_video;
  renderer->free_video = glsl_free_video;
  renderer->check_video_mode = gl_check_video_mode;
  renderer->set_video_mode = glsl_set_video_mode;
  renderer->update_colors = glsl_update_colors;
  renderer->resize_screen = resize_screen_standard;
  renderer->remap_charsets = glsl_remap_charsets;
  renderer->remap_char = glsl_remap_char;
  renderer->remap_charbyte = glsl_remap_charbyte;
  renderer->get_screen_coords = get_screen_coords_scaled;
  renderer->set_screen_coords = set_screen_coords_scaled;
  renderer->render_graph = glsl_render_graph;
  renderer->render_layer = glsl_render_layer;
  renderer->render_cursor = glsl_render_cursor;
  renderer->render_mouse = glsl_render_mouse;
  renderer->sync_screen = glsl_sync_screen;
  renderer->switch_shader = glsl_switch_shader;
}

void render_auto_glsl_register(struct renderer *renderer)
{
  render_glsl_register(renderer);
  renderer->set_video_mode = glsl_auto_set_video_mode;
}
