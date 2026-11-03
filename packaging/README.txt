Mad Riders - native Windows build (ReXGlue static recompilation)

Game files are NOT included. You need your own Mad Riders Xbox Live Arcade
package: the Xbox 360 LIVE file from your console's storage, in
58411240\000D0000\ (a long name with no extension, e.g. 0CE0DC5A...).

Setup
  Run mad_riders.exe. On the first start it looks for the package in this
  folder (you can copy the whole 58411240 folder here). If it isn't found, it
  asks you to select the package file. It then extracts the game into an
  "assets" folder next to the exe (about 1.5 GB); after that the package isn't
  needed any more. To start over, delete the "assets" folder.

  If Windows says a DLL (e.g. VCRUNTIME140.dll) is missing, install the
  Microsoft Visual C++ Redistributable 2015-2022 (x64):
  https://aka.ms/vs/17/release/vc_redist.x64.exe

Keys
  F1 / Esc: settings (display mode, window size, renderer, render resolution,
  anti-aliasing, screen sync, frame rate, performance overlay,
  aspect, texture filtering, audio buffer, keyboard on/off, key rebinding).
  F11: fullscreen. Settings are saved in mad_riders.toml next to the exe.
  Keyboard: arrows = steer/lean, Z = accelerate, X = brake, Space = boost,
  Enter = start, Backspace = back. A controller works too.

Renderer
  NativeX (default) draws the game natively with Vulkan (NativeXRenderer).
  Xenos emulates the console's GPU (ReXGlue). Switch in F1 > Renderer
  (next launch), or native_x_renderer = true/false in mad_riders.toml.
  NativeX compiles shaders the first time they are needed (short hitches
  on a first visit to a track); they are kept for later runs.

DLC
  F1 menu > Import DLC, then pick the DLC package (the file in
  58411240\00000002\). Or copy the package / 58411240 folder next to the exe:
  it is installed at the next start. Restart the game after a menu import.

Unofficial fan project, not affiliated with or endorsed by Ubisoft, Techland
or Microsoft. The exe contains code machine-translated from the game's
executable; all game data comes from your own package.
ReXGlue SDK: BSD-3-Clause (portions from Xenia) - see licenses\.
