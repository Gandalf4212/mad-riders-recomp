# Mad Riders Recompiled

Static recompilation of **Mad Riders** (Xbox 360 XBLA, Techland / Ubisoft 2012; title ID 58411240,
`default.xex` 0.0.0.2) to native Windows, using the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) v0.10.0.

**No game data is included.** You need your own Mad Riders XBLA package (the Xbox 360 LIVE file from
your console's storage, `58411240\000D0000\0CE0DC5A...`).

> Early work in progress: the code is recompiled and builds, but the game is not expected to run yet.
> See [NOTES.md](NOTES.md) for status.

## Running
Download `mad_riders-win-amd64` from the latest successful run under the **Actions** tab (or a Release zip),
unzip it and run `mad_riders.exe`. On the first start it finds your package next to the exe (copy the whole
`58411240` folder there) or asks you to select the package file, then extracts it into an `assets` folder.
It checks that `default.xex` is exactly the version this build was made from
(21,446,656 bytes, SHA-1 `90349e4daf4812f27faf5c0a9e01edb5ff7d5219`).

F1 / Esc opens the settings menu; F11 toggles fullscreen. Settings are saved in `mad_riders.toml`.

**Renderer**: [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) (native Vulkan) is the default; the
ReXGlue SDK's Xenos GPU emulation is the alternative (F1 > Renderer, or `native_x_renderer = false`). Details in
[`docs/native-x-renderer.md`](docs/native-x-renderer.md).

**DLC** (e.g. the Zambia pack): use **Import DLC** in the F1 menu and pick the package file (from
`58411240\00000002\`), or put the package (or the whole `58411240` folder) next to the exe and it is
installed at the next launch. It is extracted into ReXGlue's content folder
(`<user data>\0000000000000000\58411240\00000002\`); restart the game after importing from the menu.

## Building
GitHub Actions builds on every push to `main` (`.github/workflows/build-windows.yml`): it builds the stock
ReXGlue SDK once (cached), then the game from the committed `generated/` code. No game files are needed to build.

Locally (Windows, VS 2022 with Clang 20+, CMake 3.25+, Ninja), with the SDK installed:
```
cmake -S . -B out -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_PREFIX_PATH=<sdk-install>
cmake --build out
```
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

## Disclaimer
Unofficial fan project, not affiliated with or endorsed by Ubisoft, Techland or Microsoft. This repository and its
builds contain no game data; `generated/` and the exe contain code machine-translated from the game's executable.
The project's own code is MIT-licensed (see [LICENSE](LICENSE)); see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
