cmake_minimum_required(VERSION 3.20)
project(NoteRainNative LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_executable(NoteRainNative WIN32
  src/main.cpp
  src/app/app.cpp
  src/app/draw.cpp
  src/core/smf.cpp
  src/audio/synth.cpp
  src/audio/wasapi.cpp
  src/midi/midiio.cpp
  src/ui/canvas.cpp
  src/ui/theme.cpp
  src/i18n.cpp
  src/app.rc
)

target_include_directories(NoteRainNative PRIVATE src)

target_compile_definitions(NoteRainNative PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

target_link_libraries(NoteRainNative PRIVATE
  d2d1
  dwrite
  ole32
  winmm
  comdlg32
  shell32
  dwmapi
  user32
  gdi32
)

if(MINGW)
  target_link_options(NoteRainNative PRIVATE
    -municode
    -mwindows
    -static
    -static-libgcc
    -static-libstdc++
    -Wl,--gc-sections
  )
  target_compile_options(NoteRainNative PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
    -O2
  )
endif()

# Copy the built binary next to a friendly name for convenience.
add_custom_command(TARGET NoteRainNative POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:NoteRainNative>
          ${CMAKE_BINARY_DIR}/NoteRain.exe
  COMMENT "Copying NoteRain.exe"
)
