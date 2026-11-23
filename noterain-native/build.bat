@echo off
rem NoteRain Native - one-shot MinGW-w64 build.
rem Requires g++ 13+ and windres on PATH (MSYS2 mingw64 recommended).
setlocal
if "%MSYS2%"=="" set "PATH=C:\msys64\mingw64\bin;%PATH%"

if not exist build mkdir build

echo [1/2] Compiling resources...
windres src\app.rc -O coff -o build\app.res
if errorlevel 1 (
  echo Resource build failed.
  exit /b 1
)

echo [2/2] Compiling sources...
g++ -std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -municode -mwindows ^
  -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE -Isrc ^
  src\main.cpp src\app\app.cpp src\app\draw.cpp src\core\smf.cpp ^
  src\audio\synth.cpp src\audio\wasapi.cpp src\midi\midiio.cpp ^
  src\ui\canvas.cpp src\ui\theme.cpp src\i18n.cpp ^
  build\app.res ^
  -o build\NoteRain.exe ^
  -ld2d1 -ldwrite -lole32 -lwinmm -lcomdlg32 -lshell32 -ldwmapi -luuid ^
  -static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows

if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

echo Built build\NoteRain.exe
