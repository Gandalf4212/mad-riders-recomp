# Mad Riders Recompiled

Static recompilation of **Mad Riders** (Xbox 360 XBLA, Techland / Ubisoft 2012; title ID 58411240,
`default.xex` 0.0.0.2) to native Windows, using the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) v0.10.0.

**No game data is included.** You need your own Mad Riders XBLA package (the Xbox 360 LIVE file from
your console's storage, `58411240\000D0000\0CE0DC5A...`).

The game runs: menus, races and the DLC tracks have been played on Windows. See [NOTES.md](NOTES.md) for details and
known issues.

## What it includes
- **Two renderers.** [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) (default) draws the game's
  Direct3D calls natively with Vulkan: about 3x the frame rate of GPU emulation at the same resolution. The ReXGlue
  SDK's Xenos GPU emulation is the alternative and the fallback (F1 > Renderer, next launch).
- **Frame rate**: 30 (original), 60, 120 or uncapped.
- **Resolution**: NativeX renders at your screen's size by default, or at 1x-3x of the original 1280x720 (above the
  screen's size it scales down: smoother edges). Xenos: 1x-3x plus optional FXAA.
- **Display**: borderless fullscreen (F11) or a window of 1280x720-2560x1440; keep 16:9 or stretch; screen sync
  (fast without tearing, V-Sync, or immediate for G-Sync/FreeSync); anisotropic texture filtering.
- **Performance overlay**: FPS, frame time and renderer; CPU and GPU use with a frame-time graph.
- **Controls**: controller, or keyboard as controller 1 with rebindable keys.
- **DLC**: import add-on packages from the menu (or drop them next to the exe).
- **Setup from your own package**: the exe finds or asks for your XBLA package and extracts it; the full game is
  unlocked (`recomp_full_game = false` for the trial).
- **Fixes over a plain recompilation**: the videos' 128-byte `dcbz` (no more grainy video), and more in
  [NOTES.md](NOTES.md).

Every setting is in the F1 / Esc menu and in `mad_riders.toml` next to the exe (listed with comments; command-line
`--name=value` overrides it).

## Running
Download `mad_riders-win-amd64` from the latest successful run under the **Actions** tab (or a Release zip),
unzip it and run `mad_riders.exe`. On the first start it finds your package next to the exe (copy the whole
`58411240` folder there) or asks you to select the package file, then extracts it into an `assets` folder.
It checks that `default.xex` is exactly the version this build was made from
(21,446,656 bytes, SHA-1 `90349e4daf4812f27faf5c0a9e01edb5ff7d5219`).

F1 / Esc opens the settings menu; F11 toggles fullscreen. Settings are saved in `mad_riders.toml`.
NativeXRenderer compiles its shaders the first time each one is needed (short hitches on a first visit to a track);
they are kept for later runs. Its integration is described in [`docs/native-x-renderer.md`](docs/native-x-renderer.md).

**DLC** (e.g. the Zambia pack): use **Import DLC** in the F1 menu and pick the package file (from
`58411240\00000002\`), or put the package (or the whole `58411240` folder) next to the exe and it is
installed at the next launch. It is extracted into ReXGlue's content folder
(`<user data>\0000000000000000\58411240\00000002\`); restart the game after importing from the menu.

## Building
GitHub Actions builds on every push to `main` (`.github/workflows/build-windows.yml`): it checks out the ReXGlue SDK
v0.10.0, applies NativeXRenderer's SDK changes (`patches/rexglue-sdk/`), and builds it with the game from the
committed `generated/` code and the `thirdparty/native_x_renderer` submodule. No game files are needed to build.

Locally (Windows, VS 2022 with Clang 20+, CMake 3.25+, Ninja), with the patched SDK source tree:
```
git submodule update --init --recursive
cmake -S . -B out -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ^
  -DMADRIDERS_NATIVE_X_RENDERER=ON -DREXGLUE_USE_VULKAN=ON -DREXSDK_DIR=<patched rexglue-sdk>
cmake --build out
```
Without `-DMADRIDERS_NATIVE_X_RENDERER=ON`, an installed stock SDK (`-DCMAKE_PREFIX_PATH=<sdk-install>`) builds the
Xenos-only game.
To regenerate the recompiled code, put `default.xex` in `assets/` and run `rexglue codegen mad_riders_manifest.toml`
(or just build: with the xex present, codegen re-runs automatically when its inputs change).

## Layout
| Path | What |
|---|---|
| `mad_riders_manifest.toml` | ReXGlue project manifest (codegen input) |
| `config/overrides.toml` | Codegen fixes (function boundaries) |
| `generated/` | Recompiled C++ output of `rexglue codegen` (machine-translated game code) |
| `src/mad_riders_app.h` | App class: first-run setup, Xenos GPU plugin, full-game license, defaults, menu binds |
| `src/package_setup.*` | Finds or asks for the XBLA package, extracts it (STFS), checks the xex version |
| `src/game_menu.*`, `src/game_keys.h` | F1/Esc settings menu and Mad Riders key layout |
| `src/config_file.h` | Writes `mad_riders.toml` with every menu setting and its comment |
| `src/native_x_renderer_mad_riders.cpp` | NativeXRenderer hooks and the game's D3D profile |
| `src/perf_overlay.*`, `src/frame_rate.*` | Performance overlay; frame rate options |
| `src/dlc.*` | DLC import and install |
| `patches/rexglue-sdk/` | NativeXRenderer's changes to the SDK (from ElGuslml/rexglue-sdk) |
| `thirdparty/native_x_renderer` | NativeXRenderer (submodule) |

## Disclaimer
Unofficial fan project, not affiliated with or endorsed by Ubisoft, Techland or Microsoft. This repository and its
builds contain no game data; `generated/` and the exe contain code machine-translated from the game's executable.
The project's own code is MIT-licensed (see [LICENSE](LICENSE)); see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
