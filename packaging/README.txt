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
  F1 / Esc: settings (display mode, window size, render resolution,
  anti-aliasing, screen sync, frame rate, performance overlay,
  aspect, texture filtering, audio buffer, keyboard on/off, log file,
  key rebinding).
  F11: fullscreen. Settings are saved in mad_riders.toml next to the exe.
  Keyboard: arrows = steer/lean, Z = accelerate, X = brake, Space = boost,
  Enter = start, Backspace = back. A controller works too.

Renderer and shader cache
  The game is drawn natively with Vulkan (NativeXRenderer).
  The first launch compiles every shader the game ships into shaders.cache
  next to the exe, with a progress screen (once). Later launches load it in
  a few seconds. Shader variants first used while playing are added to it.
  Delete shaders.cache to build it again.

Bug reports
  Logging is off by default. Turn on F1 > Log file (or start with
  --recomp_log=true), reproduce the problem and send logs\mad_riders_NNN.log.
  A crash writes mad_riders_crash.txt and mad_riders_crash.dmp next to the exe.

DLC
  F1 menu > Import DLC, then pick the DLC package (the file in
  58411240\00000002\). Or copy the package / 58411240 folder next to the exe:
  it is installed at the next start. Restart the game after a menu import.

Unofficial fan project, not affiliated with or endorsed by Ubisoft, Techland
or Microsoft. The exe contains code machine-translated from the game's
executable; all game data comes from your own package.
ReXGlue SDK: BSD-3-Clause (portions from Xenia) - see licenses\.
