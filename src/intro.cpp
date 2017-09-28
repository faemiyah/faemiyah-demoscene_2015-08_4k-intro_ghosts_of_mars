/// \file
/// Main intro code.

//######################################
// Define ##############################
//######################################

#if !defined(DISPLAY_MODE)
/// Screen mode.
///
/// Negative values windowed.
/// Positive values fullscreen.
///
/// Only has effect in release mode.
#define DISPLAY_MODE -720
#endif

/// \cond
#if (0 > (DISPLAY_MODE))
#define SCREEN_F 0
#define SCREEN_H (-(DISPLAY_MODE))
#elif (0 < (DISPLAY_MODE))
#define SCREEN_F 1
#define SCREEN_H (DISPLAY_MODE)
#else
#error "invalid display mode (pre)"
#endif
#if ((800 == SCREEN_H) || (1200 == SCREEN_H))
#define SCREEN_W ((SCREEN_H / 10) * 16)
#else
#define SCREEN_W (((SCREEN_H * 16) / 9) - (((SCREEN_H * 16) / 9) % 4))
#endif
/// \endcond

/// Audio channels.
#define AUDIO_CHANNELS 1

/// Audio sample byte count.
#define AUDIO_SAMPLE_BYTES 2

/// Audio samplerate.
#define AUDIO_SAMPLERATE 44100

/// Audio byterate.
#define AUDIO_BYTERATE (AUDIO_CHANNELS * AUDIO_SAMPLERATE * AUDIO_SAMPLE_BYTES)

/// Intro length in seconds.
#define INTRO_LENGTH_SECONDS 146

/// Intro length (in bytes of audio).
#define INTRO_LENGTH (INTRO_LENGTH_SECONDS * AUDIO_BYTERATE)

/// Display frameskip or not?
#define DISPLAY_FRAMESKIP 0

/// \cond
#define STARTING_POS_X 0.0f
#define STARTING_POS_Y -2.0f
#define STARTING_POS_Z 63.0f
#define STARTING_FW_X 0.0f
#define STARTING_FW_Y 0.0f
#define STARTING_FW_Z -1.0f
#define STARTING_UP_X 0.0f
#define STARTING_UP_Y 1.0f
#define STARTING_UP_Z 0.0f
/// \endcond

//######################################
// Include #############################
//######################################

#include "dnload.h"
#include "intro.hpp"

#include <cfloat>

#if defined(USE_LD)
#include "glsl_program.hpp"
#include <iostream>
#endif

#if defined(USE_LD)
#include "image_png.hpp"
#endif

//######################################
// Global data #########################
//######################################

/// Current tick count.
static unsigned g_curr_ticks = 0;

/// Cell count.
static const unsigned CELL_COUNT = 128;

/// Cell data.
static int g_cell_data[CELL_COUNT * 2];

/// Image side length.
static const unsigned IMAGE_SIDE = 2048;

/// Image size (in texels).
static const unsigned IMAGE_SIZE = IMAGE_SIDE * IMAGE_SIDE;

/// Preprocess image data.
static float g_image_preprocess[IMAGE_SIZE];

/// Actual image data.
static uint16_t g_image_data[IMAGE_SIZE];

/// Volume side length.
static const unsigned VOLUME_SIDE = 32;

/// Volume size (in units).
static const unsigned VOLUME_SIZE = VOLUME_SIDE * VOLUME_SIDE * VOLUME_SIDE * 3;

/// Actual volume data.
static uint16_t g_volume_data[VOLUME_SIZE];

#if defined(USE_LD)

/// Audio buffer for output.
static uint8_t g_audio_buffer[INTRO_LENGTH * 9 / 8];

/// \cond
static float g_pos_x = STARTING_POS_X;
static float g_pos_y = STARTING_POS_Y;
static float g_pos_z = STARTING_POS_Z;
static float g_fw_x = STARTING_FW_X;
static float g_fw_y = STARTING_FW_Y;
static float g_fw_z = STARTING_FW_Z;
static float g_up_x = STARTING_UP_X;
static float g_up_y = STARTING_UP_Y;
static float g_up_z = STARTING_UP_Z;
/// \endcond

/// Developer mode global toggle.
static uint8_t g_flag_developer = 0;

/// Developer mode freelook status.
static bool g_freelook = true;

#endif

//######################################
// Random ##############################
//######################################

/// More random random.
///
/// It's better to discard a few bottom-most bits to achieve better randomness.
///
/// \param op Modulator for random.
/// \return Random value in range [0, op[.
static int irand(int op)
{
  return (dnload_rand() >> 4) % op;
}

#if defined(USE_LD)

/// Random float value.
///
/// \param op Given maximum value.
/// \return Random value between 0 and given value.
static float frand(float op)
{
  return static_cast<float>(irand(65536)) * ((1.0f / 65535.0f) * op);
}

#endif

//######################################
// Music ###############################
//######################################

#include "synth.c"

/// Update audio stream.
///
/// \param userdata Not used.
/// \param stream Target stream.
/// \param len Number of bytes to write.
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
  (void)userdata;

  render(reinterpret_cast<int16_t*>(stream), static_cast<unsigned>(len) / AUDIO_SAMPLE_BYTES);
  g_curr_ticks += static_cast<unsigned>(len);
}

/// SDL audio specification struct.
static SDL_AudioSpec audio_spec =
{
  AUDIO_SAMPLERATE,
  AUDIO_S16SYS,
  AUDIO_CHANNELS,
  0,
#if defined(USE_LD)
  2048,
#else
  256, // ~172.3Hz, lower values seem to cause underruns
#endif
  0,
  0,
  audio_callback,
  NULL
};

//######################################
// Shaders #############################
//######################################

#include "quad.frag.glsl.hpp" // g_shader_fragment_quad

/// Fixed uniform location.
static const GLint g_uniform_texture = 0;

/// Fixed uniform location.
static const GLint g_uniform_volume = 1;

/// Fixed uniform location.
static const GLint g_uniform_position = 2;

#if defined(USE_LD)
/// Fixed uniform location.
static const GLint g_uniform_m = 19;

/// Fixed uniform location.
static const GLint g_uniform_scales = 22;

/// Fixed uniform location.
static const GLint g_uniform_aspect = 25;
#endif

/// \cond
GLuint g_program_fragment;
/// \endcond

#if !defined(USE_LD)

/// Create a program pipeline.
///
/// \return Program pipeline (already bound).
static GLuint pipeline_create()
{
  GLuint ret;

  dnload_glGenProgramPipelines(1, &ret);
  dnload_glBindProgramPipeline(ret);

  return ret;
}

/// Create a shader.
///
/// \param type Shader type.
/// \param source Shader content.
/// \return Shader program ID.
static GLuint program_attach(GLenum type, const char *source, GLuint pipeline, GLbitfield mask)
{
  GLuint ret = dnload_glCreateShaderProgramv(type, 1, &source);

  dnload_glUseProgramStages(pipeline, mask, ret);

  return ret;
}

#endif

//######################################
// Uniform data ########################
//######################################

#if defined(USE_LD)
/// Generate a ZXY rotation matrix.
///
/// \param rx Rotation x.
/// \param ry Rotation y.
/// \param rz Rotation z.
/// \param out_matrix Matrix to write into.
static void generate_rotation_matrix_zxy(float rx, float ry, float rz, float *out_matrix)
{
  float sx = sinf(rx);
  float sy = sinf(ry);
  float sz = sinf(rz);
  float cx = cosf(rx);
  float cy = cosf(ry);
  float cz = cosf(rz);

  out_matrix[0] = sx * sy * sz + cy * cz;
  out_matrix[1] = sz * cx;
  out_matrix[2] = sx * sz * cy - sy * cz;
  out_matrix[3] = sx * sy * cz - sz * cy;
  out_matrix[4] = cx * cz;
  out_matrix[5] = sx * cy * cz + sy * sz;
  out_matrix[6] = sy * cx;
  out_matrix[7] = -sx;
  out_matrix[8] = cx * cy;
}

/// Position uniforms.
///
/// 0: X position 1.
/// 1: Y position 1.
/// 2: Z position 1.
/// 3: Spline weight X.
/// 4: Spline weight Y.
/// 5: Spline weight Z.
/// 6: X position 2.
/// 7: Y position 2.
/// 8: Z position 2.
/// 9: X forward 1.
/// 10: Y forward 1.
/// 11: Z forward 1.
/// 12: X forward 2.
/// 13: Y forward 2.
/// 14: Z forward 2.
/// 15: X up 1.
/// 16: Y up 1.
/// 17: Z up 1.
/// 18: X up 2.
/// 19: Y up 2.
/// 20: Z up 2.
/// 21: ticks.
/// 22: Current LERP status.
/// 23: Current world.
/// 24: Destruction X.
/// 25: Destruction Y.
/// 26: Destruction Z.
/// 27: Destruction strength.
static float g_uniform_array_position[10 * 3] =
{
  STARTING_POS_X, STARTING_POS_Y, STARTING_POS_Z,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
  STARTING_FW_X, STARTING_FW_Y, STARTING_FW_Z,
  0.0f, 0.0f, 0.0f,
  STARTING_UP_X, STARTING_UP_Y, STARTING_UP_Z,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 68.0f,
  -0.2f, 0.0f, 0.0f,
};

/// Noise uniform array.
///
/// 0-3: Noise scale.
/// 4-7: Noise strenght.
/// 8-11: Mandelbox parameters.
static float g_uniform_array_noise[12] =
{
  0.007f, 2.61f, 2.11f, 2.11f,
  2.61f, 1.77f, 0.11f, 0.11f,
  9.0f, 0.824f, -2.259f, 1.0f,
  //9.0f, 0.856f, -2.277f, 1.0f,
  //9.0f, 0.618f, -2.266f, 1.0f,
  //0.0f, 0.0f, 0.0f, 0.0f,
};

/// Matrix uniform array.
///
/// One 3x3 matrix.
static float g_uniform_array_matrix[9] =
{
  0.42f, -0.7f, 0.58f,
  0.53f, 0.71f, 0.46f,
  -0.74f, 0.12f, 0.67f
};

#endif

/// 'Detail' of direction time.
#define DIRECTION_GRANULARITY 16

/// Tick count at first impact.
#define IMPACT_FIRST_START (81 * AUDIO_BYTERATE)

// Tick count at end of first impact.
#define IMPACT_FIRST_END (113 * AUDIO_BYTERATE)

/// Tick count at second impact.
#define IMPACT_SECOND (132 * AUDIO_BYTERATE)

/// Destruction moves due to cinematic purposes."
#define DESTRUCTION_INCREMENT 22.0f

/// Direction.
///
/// Divided into series of 15 numbers signifying:
/// 0: X position 1.
/// 1: Y position 1.
/// 2: Z position 1.
/// 3: Spline weight X.
/// 4: Spline weight Y.
/// 5: Spline weight Z.
/// 6: X position 2.
/// 7: Y position 2.
/// 8: Z position 2.
/// 9: Random seed for forward vector 1.
/// 10: Random seed for forward vector 2.
/// 11: Random seed for up vector 1.
/// 12: Random seed for up vector 2.
/// 13: State, bit 1 = portal on, bit 2 = world.
/// 14: Duration in (1 / DIRECTION_GRANULARITY) of a second.
static const uint8_t g_direction[] =
{
// X1   Y1   Z1  SWx  SWy  SWz   X2   Y2   Z2  FW1  FW1  UP1  UP2  POR  DUR
  147, 130,  27, 177, 120,  10, 255, 124,   0, 191, 105,  53, 246,   0, 131, // aavikkoa
  124, 118, 255, 102, 122, 229,  95, 134, 135,   4, 181,  72, 189,   0, 131, // Nousu vuorta pitkin 71,  136, 113,  4, 115,  72, 246,
  102, 143, 151, 124, 146, 165, 152, 144, 174, 131, 150,  94, 192,   1, 129,
  160, 140, 164, 179, 136, 161, 163, 133, 157, 150, 251, 192,  72,   1, 245, // porttaa liin

  148, 127, 160, 140, 124, 147, 153, 127, 147, 151, 241, 101,  72,   2, 146, // poksia
  160, 140, 157, 158, 141, 156, 155, 141, 157,  78,  68,   9,  69,   2, 130,
  159, 127, 146, 153, 128, 146, 142, 128, 147, 183,   7, 120,  72,   2, 255,
  
  135, 128, 146, 128, 129, 144, 122, 130, 144, 244, 238, 189, 189,   2, 255, // decay start
  154, 124, 145, 158, 128, 151, 157, 128, 159, 244,  29, 189, 148,   2, 241, // decay
  166, 131, 166, 169, 133, 165, 162, 132, 155,  29,  48,   0,  60,   3, 178, // escape

  136, 148, 175, 105, 147, 167,  95, 138, 149,  33, 115, 196, 110,   1, 162, // hopelessness
  104, 146, 132,  85, 142, 135,  70, 139, 139,  17, 134, 113, 189,   0, 255,

  104, 146, 132,  85, 142, 135,  70, 139, 139,  17, 134, 113, 189,   0, 255, // piisin pitää loppuu
};

/// Image texture.
static GLuint g_image;

/// Volumetric texture.
static GLuint g_volume;

/// Randomize a direction.
///
/// \param op Random seed.
/// \param dst Destination pointer.
static void rand_dir(uint8_t op, float *dst)
{
  dnload_srand(static_cast<unsigned>(op));
  dst[0] = static_cast<float>(irand(256) - 128);
  dst[1] = static_cast<float>(irand(256) - 128);
  dst[2] = static_cast<float>(irand(256) - 128);
}

/// Draw the world.
///
/// \param ticks Tick count.
static void draw(unsigned ticks)
{
#if defined(USE_LD)
  if(g_flag_developer && g_freelook)
  {
    g_uniform_array_position[0] = g_pos_x;
    g_uniform_array_position[1] = g_pos_y;
    g_uniform_array_position[2] = g_pos_z;
    g_uniform_array_position[9] = g_fw_x;
    g_uniform_array_position[10] = g_fw_y;
    g_uniform_array_position[11] = g_fw_z;
    g_uniform_array_position[15] = g_up_x;
    g_uniform_array_position[16] = g_up_y;
    g_uniform_array_position[17] = g_up_z;

    g_uniform_array_position[21] = static_cast<float>(ticks);
    g_uniform_array_position[22] = 0.0f;

    glProgramUniform3fv(g_program_fragment, g_uniform_position, 10, g_uniform_array_position);
  }
  else
#endif
  {
    float uniform_array_position[30];
    const uint8_t* direction = g_direction;

    // Lazy manual control of destruction.
    uniform_array_position[24] = 0.0f;
    uniform_array_position[25] = 0.0f;
    uniform_array_position[26] = 0.0f;
    uniform_array_position[27] = 23.0f;
    uniform_array_position[28] = -0.2f;
    if((IMPACT_FIRST_START < ticks) && (IMPACT_FIRST_END > ticks))
    {
      uniform_array_position[26] = 84.0f + 7.0f * DESTRUCTION_INCREMENT;
      uniform_array_position[28] = static_cast<float>(ticks - IMPACT_FIRST_START) *
        (0.000044f / static_cast<float>(AUDIO_CHANNELS));
      //std::cout << "impact: " << uniform_array_position[27] << std::endl;
    }
    if(IMPACT_SECOND < ticks)
    {
      uniform_array_position[25] = -22.0f;
      uniform_array_position[26] = 0.0f + 11.0f * DESTRUCTION_INCREMENT;
      uniform_array_position[28] = static_cast<float>(ticks - IMPACT_SECOND) *
        (0.00022f / static_cast<float>(AUDIO_CHANNELS));
    }

    for(;;)
    {
      unsigned duration = static_cast<unsigned>(direction[14]) * (AUDIO_BYTERATE / DIRECTION_GRANULARITY);

      if(ticks < duration)
      {
        float fticks = static_cast<float>(ticks);
        float fduration = static_cast<float>(duration);
        float mul = fticks / fduration;

        for(unsigned ii = 0; (9 > ii); ++ii)
        {
          uniform_array_position[ii] = static_cast<float>(direction[ii] - 128);
        }
        rand_dir(direction[9], uniform_array_position + 9);
        rand_dir(direction[10], uniform_array_position + 12);
        rand_dir(direction[11], uniform_array_position + 15);
        rand_dir(direction[12], uniform_array_position + 18);

        uniform_array_position[21] = fticks;
        uniform_array_position[22] = mul;
        uniform_array_position[23] = static_cast<float>(direction[13]);

        //printf("lerp: %f\n", uniform_array_position[22]);

        break;
      }

      uniform_array_position[26] -= DESTRUCTION_INCREMENT;
      uniform_array_position[27] = 1.0f;

      ticks -= duration;
      direction += 15;
    }

    dnload_glProgramUniform3fv(g_program_fragment, g_uniform_position, 10, uniform_array_position);
  }

#if defined(USE_LD)
  glProgramUniform4fv(g_program_fragment, g_uniform_scales, 3, g_uniform_array_noise);
  glProgramUniformMatrix3fv(g_program_fragment, g_uniform_m, 1, static_cast<GLboolean>(0),
      g_uniform_array_matrix);
#endif

  dnload_glProgramUniform1i(g_program_fragment, g_uniform_texture, 0);
  dnload_glProgramUniform1i(g_program_fragment, g_uniform_volume, 1);

  dnload_glRects(-1, -1, 1, 1);
}

//######################################
// Main ################################
//######################################

#if defined(USE_LD)
int intro(unsigned screen_w, unsigned screen_h, uint8_t flag_developer, uint8_t flag_fullscreen,
    uint8_t flag_record)
{
#else
/// \cond
#define screen_w SCREEN_W
#define screen_h SCREEN_H
#define flag_fullscreen SCREEN_F
#define flag_developer 0
/// \endcond
void _start()
{
#endif
  dnload();
  dnload_SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window *window = dnload_SDL_CreateWindow(NULL, 0, 0, static_cast<int>(screen_w),
      static_cast<int>(screen_h), SDL_WINDOW_OPENGL | (flag_fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
  dnload_SDL_GL_CreateContext(window);
  dnload_SDL_ShowCursor(flag_developer);
#if defined(USE_LD)
  {
    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
      SDL_Quit();
      std::cerr  << "glewInit(): " << glewGetErrorString(err) << std::endl;
      exit(1);
    }
  }
#endif

  // Generate texture.
  {
    dnload_srand(15);

    for(unsigned ii = 0; ((CELL_COUNT * 2) > ii); ++ii)
    {
      g_cell_data[ii] = irand(IMAGE_SIDE);
    }
    for(unsigned ii = 0; (VOLUME_SIZE > ii); ++ii)
    {
      g_volume_data[ii] = static_cast<uint16_t>(irand(65536));
    }

    float largest = 0.0f;
    {
      float *dst = g_image_preprocess;

      for(int jj = 0; (static_cast<int>(IMAGE_SIDE) > jj); ++jj)
      {
        for(int ii = 0; (static_cast<int>(IMAGE_SIDE) > ii); ++ii)
        {
          static const int MAX_CELL_DIST = static_cast<int>(IMAGE_SIDE / 2);
          float closest = FLT_MAX;

          for(unsigned kk = 0; ((CELL_COUNT * 2) > kk); kk += 2)
          {
            const int xpos = g_cell_data[kk + 0];
            const int ypos = g_cell_data[kk + 1];
            int xdiff = std::abs(xpos - ii);
            int ydiff = std::abs(ypos - jj);

            if(MAX_CELL_DIST <= xdiff)
            {
              xdiff = MAX_CELL_DIST - (xdiff - MAX_CELL_DIST);
            }
            if(MAX_CELL_DIST <= ydiff)
            {
              ydiff = MAX_CELL_DIST - (ydiff - MAX_CELL_DIST);
            }

            const float fxdiff = static_cast<float>(xdiff);
            const float fydiff = static_cast<float>(ydiff);
            const float fdist = (fxdiff * fxdiff) + (fydiff * fydiff);

            if(fdist < closest)
            {
              closest = fdist;
            }
          }

          if(largest < closest)
          {
            largest = closest;
          }
          *dst = closest;
          ++dst;
        }
      }
    }

#if 1
    {
      const float mul = 65535.0f / largest;

      for(unsigned ii = 0; (IMAGE_SIDE * IMAGE_SIDE > ii); ++ii)
      {
        g_image_data[ii] = static_cast<uint16_t>(65535 - g_image_preprocess[ii] * mul);
      }
    }
#else
    {
      const float mul = 255.0f / largest;

      for(unsigned ii = 0; (IMAGE_SIDE * IMAGE_SIDE > ii); ++ii)
      {
        g_image_data[ii] = 255 - static_cast<uint8_t>(g_image_preprocess[ii] * mul);
      }
    }
    gfx::image_png_save(std::string("lol.png"), IMAGE_SIDE, IMAGE_SIDE, 8, g_image_data);
#endif

    dnload_glActiveTexture(GL_TEXTURE0);
    dnload_glGenTextures(1, &g_image);
    dnload_glBindTexture(GL_TEXTURE_2D, g_image);
    dnload_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#if defined(USE_LD)
    dnload_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    dnload_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dnload_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif
#if 1
    dnload_glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, IMAGE_SIDE, IMAGE_SIDE, 0, GL_RED, GL_UNSIGNED_SHORT,
        g_image_data);
#else
    dnload_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, IMAGE_SIDE, IMAGE_SIDE, 0, GL_RED, GL_UNSIGNED_BYTE,
        g_image_data);
#endif
    dnload_glGenerateMipmap(GL_TEXTURE_2D);

    dnload_glActiveTexture(GL_TEXTURE1);
    dnload_glGenTextures(1, &g_volume);
    dnload_glBindTexture(GL_TEXTURE_3D, g_volume);
    dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#if defined(USE_LD)
    dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
#endif
    dnload_glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16_SNORM, VOLUME_SIDE, VOLUME_SIDE, VOLUME_SIDE, 0, GL_RGB,
        GL_UNSIGNED_SHORT, g_volume_data);
    dnload_glGenerateMipmap(GL_TEXTURE_3D);
  }

#if defined(USE_LD)
  GlslProgram program;
  program.addShader(GL_FRAGMENT_SHADER, g_shader_fragment_quad);
  if(!program.link(true))
  {
    SDL_Quit();
    std::cerr  << "pipeline creation failure" << std::endl;
    exit(1);
  }
  glBindProgramPipeline(program.getPipelineId());
  g_program_fragment = program.getPipelineId(GL_FRAGMENT_SHADER);
#else
  GLuint pipeline = pipeline_create();
  g_program_fragment = program_attach(GL_FRAGMENT_SHADER, g_shader_fragment_quad, pipeline, 2);
#endif

#if defined(USE_LD)
  {
    float fw = static_cast<float>(screen_w);
    float fh = static_cast<float>(screen_h);

    if(screen_w > screen_h)
    {
      glProgramUniform3f(g_program_fragment, g_uniform_aspect, fw / fh, 1.0f, 2.0f / fh);
    }
    else
    {
      glProgramUniform3f(g_program_fragment, g_uniform_aspect, 1.0f, fh / fw, 2.0f / fw);
    }
  }
#endif

#if defined(USE_LD)
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
#endif

#if defined(USE_LD)
  if(flag_record)
  {
    SDL_Event event;
    unsigned frame_idx = 0;

    // Music first.
    render(reinterpret_cast<int16_t*>(g_audio_buffer), INTRO_LENGTH / AUDIO_SAMPLE_BYTES);
    write_audio_callback(g_audio_buffer, INTRO_LENGTH);

    // video
    for(;;)
    {
      unsigned ticks = static_cast<unsigned>(static_cast<float>(frame_idx) *
          (static_cast<float>(AUDIO_BYTERATE) / 60.0f));

      if(ticks > INTRO_LENGTH)
      {
        break;
      }

      if(SDL_PollEvent(&event) && (event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE))
      {
        break;
      }

      draw(ticks);
      write_frame_callback(screen_w, screen_h, frame_idx);
      SDL_GL_SwapWindow(window);
      ++frame_idx;
    }

    SDL_Quit();
    return 0;
  }

  if(!flag_developer)
  {
    SDL_OpenAudio(&audio_spec, NULL);
    SDL_PauseAudio(0);
  }
  g_flag_developer = flag_developer;
#else
  dnload_SDL_OpenAudio(&audio_spec, NULL);
  dnload_SDL_PauseAudio(0);
#endif

#if defined(USE_LD)
  int start_ticks = static_cast<int>(SDL_GetTicks());
#if defined(DISPLAY_FRAMESKIP) && DISPLAY_FRAMESKIP
  static int last_ticks = start_ticks;
#endif
#endif

  for(;;)
  {
#if defined(USE_LD)
    const unsigned TIME_SKIP_SHORT = 5000;
    const unsigned TIME_SKIP_LONG = 30000;
    static float saved_pos[3];
    static float saved_fw[3];
    static float saved_up[3];
    static float current_time = 0.0f;
    static int8_t destruction = 0;
    static uint8_t mouse_look = 0;
    static int8_t move_speed = 0;
    static int8_t move_backward = 0;
    static int8_t move_down = 0;
    static int8_t move_forward = 0;
    static int8_t move_left = 0;
    static int8_t move_right = 0;
    static int8_t move_up = 0;
    static int8_t time_delta = 0;
    static int8_t scale_0 = 0;
    static int8_t scale_1 = 0;
    static int8_t scale_2 = 0;
    static int8_t scale_3 = 0;
    static int8_t weight_0 = 0;
    static int8_t weight_1 = 0;
    static int8_t weight_2 = 0;
    static int8_t weight_3 = 0;
    static bool world = false;
    int mouse_look_x = 0;
    int mouse_look_y = 0;
    bool quit = false;
#endif
    SDL_Event event;

#if defined(USE_LD)
    while(SDL_PollEvent(&event))
    {
      if(SDL_QUIT == event.type)
      {
        quit = true;
      }
      else if(SDL_KEYDOWN == event.type)
      {
        switch(event.key.keysym.sym)
        {
          case SDLK_a:
            move_left = 1;
            break;

          case SDLK_d:
            move_right = 1;
            break;

          case SDLK_e:
            move_up = 1;
            break;

          case SDLK_q:
            move_down = 1;
            break;

          case SDLK_s:
            move_backward = 1;
            break;

          case SDLK_w:
            move_forward = 1;
            break;

          case SDLK_f:
            g_freelook = !g_freelook;
            break;

          case SDLK_z:
            scale_0 = move_speed ? -1 : 1;
            break;

          case SDLK_x:
            scale_1 = move_speed ? -1 : 1;
            break;

          case SDLK_c:
            scale_2 = move_speed ? -1 : 1;
            break;

          case SDLK_v:
            scale_3 = move_speed ? -1 : 1;
            break;

          case SDLK_b:
            weight_0 = move_speed ? -1 : 1;
            break;

          case SDLK_n:
            weight_1 = move_speed ? -1 : 1;
            break;

          case SDLK_m:
            weight_2 = move_speed ? -1 : 1;
            break;

          case SDLK_COMMA:
            weight_3 = move_speed ? -1 : 1;
            break;

          case SDLK_MINUS:
          case SDLK_KP_2:
            destruction = move_speed ? -1 : 1;
            break;

          case SDLK_LSHIFT:
          case SDLK_RSHIFT:
            move_speed = 1;
            break;            

          case SDLK_LCTRL:
            world = !world;
            g_uniform_array_position[23] = 2.0f * static_cast<float>(world) + 1.0f;
            break;

          case SDLK_LALT:
          case SDLK_KP_1:
            time_delta = -1;
            g_freelook = false;
            break;

          case SDLK_MODE:
          case SDLK_KP_3:
            time_delta = 1;
            g_freelook = false;
            break;

          case SDLK_INSERT:
            {
              float rx = frand(static_cast<float>(M_PI * 2.0));
              float ry = frand(static_cast<float>(M_PI * 2.0));
              float rz = frand(static_cast<float>(M_PI * 2.0));

              generate_rotation_matrix_zxy(rx, ry, rz, g_uniform_array_matrix);

              printf("[ %1.2f ; %1.2f ; %1.2f ] =>\n[ %+1.2f ; %+1.2f ; %+1.2f\n"
                  "  %+1.2f ; %+1.2f ; %+1.2f\n  %+1.2f ; %+1.2f ; %+1.2f ]\n",
                  rx, ry, rz,
                  g_uniform_array_matrix[0], g_uniform_array_matrix[3], g_uniform_array_matrix[6],
                  g_uniform_array_matrix[1], g_uniform_array_matrix[4], g_uniform_array_matrix[7],
                  g_uniform_array_matrix[2], g_uniform_array_matrix[5], g_uniform_array_matrix[8]);
            }
            break;

          case SDLK_SPACE:
            {
              int px = static_cast<int>(g_pos_x + 0.5f) + 128;
              int py = static_cast<int>(g_pos_y + 0.5f) + 128;
              int pz = static_cast<int>(g_pos_z + 0.5f) + 128;
              unsigned fseed = 0;
              unsigned useed = 0;
              float min_fdist = FLT_MAX;
              float min_udist = FLT_MAX;

              saved_pos[0] = g_pos_x;
              saved_pos[1] = g_pos_y;
              saved_pos[2] = g_pos_z;
              saved_fw[0] = g_fw_x;
              saved_fw[1] = g_fw_y;
              saved_fw[2] = g_fw_z;
              saved_up[0] = g_up_x;
              saved_up[1] = g_up_y;
              saved_up[2] = g_up_z;

              for(unsigned ii = 0; (256 > ii); ++ii)
              {
                dnload_srand(ii);

                float dx = static_cast<float>(irand(256) - 128);
                float dy = static_cast<float>(irand(256) - 128);
                float dz = static_cast<float>(irand(256) - 128);
                float len = sqrtf(dx * dx + dy * dy + dz * dz);

                dx /= len;
                dy /= len;
                dz /= len;

                // Squared distance is better than simple sum of distances.
                float fdist = powf(dx - g_fw_x, 2.0f) + powf(dy - g_fw_y, 2.0f) +
                  powf(dz - g_fw_z, 2.0f);
                float udist = powf(dx - g_up_x, 2.0f) + powf(dy - g_up_y, 2.0f) +
                  powf(dz - g_up_z, 2.0f);

                if(fdist < min_fdist)
                {
                  min_fdist = fdist;
                  fseed = ii;
                }
                if(udist < min_udist)
                {
                  min_udist = udist;
                  useed = ii;
                }
              }

              printf("Position: [ %1.1f ; %1.1f ; %1.1f ] -> %i, %i, %i, Forward: %u, Up: %u\n",
                  g_pos_x * 3.0f, g_pos_y * 3.0f, g_pos_z * 3.0f,
                  px, py, pz, fseed, useed);

              g_pos_x = static_cast<float>(px) - 128.0f;
              g_pos_y = static_cast<float>(py) - 128.0f;
              g_pos_z = static_cast<float>(pz) - 128.0f;
              dnload_srand(fseed);
              g_fw_x = static_cast<float>(irand(256) - 128);
              g_fw_y = static_cast<float>(irand(256) - 128);
              g_fw_z = static_cast<float>(irand(256) - 128);
              dnload_srand(useed);
              g_up_x = static_cast<float>(irand(256) - 128);
              g_up_y = static_cast<float>(irand(256) - 128);
              g_up_z = static_cast<float>(irand(256) - 128);
            }
            break;

          case SDLK_LEFT:
            start_ticks += TIME_SKIP_SHORT;
            break;

          case SDLK_RIGHT:
            start_ticks -= TIME_SKIP_SHORT;
            break;

          case SDLK_DOWN:
            start_ticks += TIME_SKIP_LONG;
            break;

          case SDLK_UP:
            start_ticks -= TIME_SKIP_LONG;
            break;

          case SDLK_ESCAPE:
            quit = true;
            break;

          default:
            break;
        }
      }
      else if(SDL_KEYUP == event.type)
      {
        switch(event.key.keysym.sym)
        {
          case SDLK_a:
            move_left = 0;
            break;

          case SDLK_d:
            move_right = 0;
            break;

          case SDLK_e:
            move_up = 0;
            break;

          case SDLK_q:
            move_down = 0;
            break;

          case SDLK_s:
            move_backward = 0;
            break;

          case SDLK_w:
            move_forward = 0;
            break;

          case SDLK_z:
            scale_0 = 0;
            break;

          case SDLK_x:
            scale_1 = 0;
            break;

          case SDLK_c:
            scale_2 = 0;
            break;

          case SDLK_v:
            scale_3 = 0;
            break;

          case SDLK_b:
            weight_0 = 0;
            break;

          case SDLK_n:
            weight_1 = 0;
            break;

          case SDLK_m:
            weight_2 = 0;
            break;

          case SDLK_COMMA:
            weight_3 = 0;
            break;

          case SDLK_MINUS:
          case SDLK_KP_2:
            destruction = 0;
            break;

          case SDLK_LSHIFT:
          case SDLK_RSHIFT:
            move_speed = 0;
            break;            

          case SDLK_LALT:
          case SDLK_MODE:
          case SDLK_KP_3:
          case SDLK_KP_1:
            time_delta = 0;
            break;

          case SDLK_SPACE:
            g_pos_x = saved_pos[0];
            g_pos_y = saved_pos[1];
            g_pos_z = saved_pos[2];
            g_fw_x = saved_fw[0];
            g_fw_y = saved_fw[1];
            g_fw_z = saved_fw[2];
            g_up_x = saved_up[0];
            g_up_y = saved_up[1];
            g_up_z = saved_up[2];
            break;

          default:
            break;
        }
      }
      else if(SDL_MOUSEBUTTONDOWN == event.type)
      {
        if(1 == event.button.button)
        {
          mouse_look = 1;
        }
      }
      else if(SDL_MOUSEBUTTONUP == event.type)
      {
        if(1 == event.button.button)
        {
          mouse_look = 0;
        }
      }
      else if(SDL_MOUSEMOTION == event.type)
      {
        if(0 != mouse_look)
        {
          mouse_look_x += event.motion.xrel;
          mouse_look_y += event.motion.yrel;
        }
      }
    }

    if(g_flag_developer)
    {
      float current_move_speed = 1.0f / (move_speed ? 5.0f : 60.0f);
      float uplen = sqrtf(g_up_x * g_up_x + g_up_y * g_up_y + g_up_z * g_up_z);
      float fwlen = sqrtf(g_fw_x * g_fw_x + g_fw_y * g_fw_y + g_fw_z * g_fw_z);
      float rt_x;
      float rt_y;
      float rt_z;
      float movement_rt = static_cast<float>(move_right - move_left) * current_move_speed;
      float movement_up = static_cast<float>(move_up - move_down) * current_move_speed;
      float movement_fw = static_cast<float>(move_forward - move_backward) * current_move_speed;

      g_up_x /= uplen;
      g_up_y /= uplen;
      g_up_z /= uplen;

      g_fw_x /= fwlen;
      g_fw_y /= fwlen;
      g_fw_z /= fwlen;

      rt_x = g_fw_y * g_up_z - g_fw_z * g_up_y;
      rt_y = g_fw_z * g_up_x - g_fw_x * g_up_z;
      rt_z = g_fw_x * g_up_y - g_fw_y * g_up_x;

      if(0 != mouse_look_x)
      {
        float angle = static_cast<float>(mouse_look_x) / static_cast<float>(screen_h / 4) * 0.25f;
        float ca = cosf(angle);
        float sa = sinf(angle);
        float new_rt_x = ca * rt_x + sa * g_fw_x;
        float new_rt_y = ca * rt_y + sa * g_fw_y;
        float new_rt_z = ca * rt_z + sa * g_fw_z;
        float new_fw_x = ca * g_fw_x - sa * rt_x;
        float new_fw_y = ca * g_fw_y - sa * rt_y;
        float new_fw_z = ca * g_fw_z - sa * rt_z;

        rt_x = new_rt_x;          
        rt_y = new_rt_y;
        rt_z = new_rt_z;
        g_fw_x = new_fw_x;
        g_fw_y = new_fw_y;
        g_fw_z = new_fw_z;
      }
      if(0 != mouse_look_y)
      {
        float angle = static_cast<float>(mouse_look_y) / static_cast<float>(screen_h / 4) * 0.25f;
        float ca = cosf(angle);
        float sa = sinf(angle);
        float new_fw_x = ca * g_fw_x + sa * g_up_x;
        float new_fw_y = ca * g_fw_y + sa * g_up_y;
        float new_fw_z = ca * g_fw_z + sa * g_up_z;
        float new_up_x = ca * g_up_x - sa * g_fw_x;
        float new_up_y = ca * g_up_y - sa * g_fw_y;
        float new_up_z = ca * g_up_z - sa * g_fw_z;

        g_fw_x = new_fw_x;
        g_fw_y = new_fw_y;
        g_fw_z = new_fw_z;
        g_up_x = new_up_x;
        g_up_y = new_up_y;
        g_up_z = new_up_z;
      }

      if(scale_0 || scale_1 || scale_2 || scale_3)
      {
        g_uniform_array_noise[0] += static_cast<float>(scale_0) * 0.001f;
        g_uniform_array_noise[1] += static_cast<float>(scale_1) * 0.001f;
        g_uniform_array_noise[2] += static_cast<float>(scale_2) * 0.001f;
        g_uniform_array_noise[3] += static_cast<float>(scale_3) * 0.001f;
        printf("Scale: %1.3f ; %1.3f ; %1.3f ; %1.3f\n",
            g_uniform_array_noise[0], g_uniform_array_noise[1],
            g_uniform_array_noise[2], g_uniform_array_noise[3]);
      }

      if(weight_0 || weight_1 || weight_2 || weight_3)
      {
        g_uniform_array_noise[4] += static_cast<float>(weight_0) * 0.001f;
        g_uniform_array_noise[5] += static_cast<float>(weight_1) * 0.001f;
        g_uniform_array_noise[6] += static_cast<float>(weight_2) * 0.001f;
        g_uniform_array_noise[7] += static_cast<float>(weight_3) * 0.001f;
        printf("Weight: %1.3f ; %1.3f ; %1.3f ; %1.3f\n",
            g_uniform_array_noise[4], g_uniform_array_noise[5],
            g_uniform_array_noise[6], g_uniform_array_noise[7]);
      }

      if(destruction)
      {
        g_uniform_array_position[27] += static_cast<float>(destruction) * 0.1f;
        printf("Destruction: [ %1.1f ; %1.1f ; %1.1f ] : %1.1f\n",
            g_uniform_array_position[24],
            g_uniform_array_position[25],
            g_uniform_array_position[26],
            g_uniform_array_position[27]);
      }

      g_pos_x += movement_rt * rt_x + movement_up * g_up_x + movement_fw * g_fw_x;
      g_pos_y += movement_rt * rt_y + movement_up * g_up_y + movement_fw * g_fw_y;
      g_pos_z += movement_rt * rt_z + movement_up * g_up_z + movement_fw * g_fw_z;

      // Developer time.
      float corrected_time_delta = static_cast<float>(time_delta * (move_speed ? 10 : 1));
      current_time += static_cast<float>(AUDIO_BYTERATE) / 60.0f * corrected_time_delta;
      g_curr_ticks = static_cast<unsigned>(fminf(fmaxf(current_time, 0.0f),
            static_cast<float>(INTRO_LENGTH) - 2.0f));
    }
    else
    {
      int corrected_ticks = static_cast<int>(SDL_GetTicks()) - start_ticks;

      if(0 > corrected_ticks)
      {
        start_ticks += corrected_ticks;
        corrected_ticks = 0;
      }

#if defined(DISPLAY_FRAMESKIP) && DISPLAY_FRAMESKIP
      if(corrected_ticks - last_ticks >= 18)
      {
        printf("frameskip: %u\n", corrected_ticks - last_ticks);
      }
      last_ticks = corrected_ticks;
#endif

      float seconds_elapsed = static_cast<float>(corrected_ticks) / 1000.0f;

      g_curr_ticks = static_cast<unsigned>(seconds_elapsed * static_cast<float>(AUDIO_BYTERATE));

      //printf("start: %i, corrected: %i, seconds: %1.1f, curr: %u\n", start_ticks, corrected_ticks, seconds_elapsed, curr_ticks);
    }

    if((g_curr_ticks >= INTRO_LENGTH) || quit)
    {
      break;
    }
#else
    dnload_SDL_PollEvent(&event);
    
    if((g_curr_ticks >= INTRO_LENGTH) || (event.type == SDL_KEYDOWN))
    {
      break;
    }
#endif

    draw(g_curr_ticks);
    dnload_SDL_GL_SwapWindow(window);
  }

#if defined(DNLOAD_GLESV2)
  egl_quit(egl_display);
#endif
  dnload_SDL_Quit();

#if defined(USE_LD)
  return 0;
#else
  asm_exit();
#endif
}

//######################################
// End #################################
//######################################

