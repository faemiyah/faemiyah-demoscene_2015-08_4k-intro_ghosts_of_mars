cmake_minimum_required(VERSION 2.8)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project(ghosts_of_mars)

enable_language(C)
enable_language(CXX)

include("defaults.cmake")

find_boost("filesystem" "program_options" "system" "thread" "wave")
find_glew()
find_opengl()
find_png()
find_sdl2()
find_threads()

output_flags("DEBUG" on)

add_definitions(-DUSE_LD)

include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(ghosts_of_mars
  "src/bsd_rand.c"
  "src/bsd_rand.h"
  "src/glsl_program.cpp"
  "src/glsl_program.hpp"
  "src/glsl_shader.cpp"
  "src/glsl_shader.hpp"
  "src/glsl_wave.cpp"
  "src/glsl_wave.hpp"
  "src/image_png.cpp"
  "src/image_png.hpp"
  "src/intro.cpp"
  "src/intro.hpp"
  "src/main.cpp")
if(MSVC)
  target_link_libraries(ghosts_of_mars "WINMM")
  target_link_libraries(ghosts_of_mars ${OPENGL_gl_LIBRARY})
  target_link_libraries(ghosts_of_mars debug "${GLEW_LIBRARY_DEBUG}" optimized "${GLEW_LIBRARY}")
  target_link_libraries(ghosts_of_mars debug "${PNG_LIBRARY_DEBUG}" optimized "${PNG_LIBRARY}")
  target_link_libraries(ghosts_of_mars debug "${SDL2_LIBRARY_DEBUG}" optimized "${SDL2_LIBRARY}")
  target_link_libraries(ghosts_of_mars debug "${ZLIB_LIBRARY_DEBUG}" optimized "${ZLIB_LIBRARY}")
else()
  target_link_libraries(ghosts_of_mars "${BOOST_FILESYSTEM_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${BOOST_PROGRAM_OPTIONS_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${BOOST_SYSTEM_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${BOOST_THREAD_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${BOOST_WAVE_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${GLEW_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${PNG_LIBRARY}")
  target_link_libraries(ghosts_of_mars "${SDL2_LIBRARY}")
endif()
target_link_libraries(ghosts_of_mars "${THREADS_LIBRARY}")

# TODO: Remove when Wave works with C++11.
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set_property(SOURCE "src/glsl_wave.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++03")
endif()
