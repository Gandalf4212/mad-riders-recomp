# Mad Riders Recomp: status notes

Static recompilation of **Mad Riders** (Xbox 360 XBLA, Ubisoft 2012) to native Windows
using the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

## Game binary
| Field | Value |
|---|---|
| Title ID | 58411240 |
| Media ID | 17ADE555 |
| Version | 0.0.0.2 |
| XEX build time | 2012-03-07 22:50:48 UTC |
| `default.xex` size | 21,446,656 bytes |
| Image base / size | 0x82000000 / 0x15E0000 |
| Code base / size | 0x823F0000 / 0xFB1C2C |

No title update (.xexp) was in the package.

## Status
- [x] Package extracted (STFS LIVE, content type 000D0000 = XBLA)
- [x] ReXGlue project scaffolded (`rexglue init`, SDK v0.10.0-dev)
- [x] Codegen passes analysis with **0 errors** (515 generated files, ~266 MB)
- [ ] Build the SDK + project on Windows
- [ ] First boot: fill in missing kernel imports / stubs
- [ ] Graphics, audio, input

## Overrides applied (`config/overrides.toml`)
| Address | Why |
|---|---|
| 0x825B1C60 | Target of thunk `sub_825B1CD8` (`b 0x825B1C60`); separate function that follows the 4-instruction `sub_825B1C50` |
| 0x82C95D00 | Target of thunk `sub_82C95FD8` (`b 0x82C95D00`); separate function that follows the 26-instruction `sub_82C95C98` |

## Earlier XenonRecomp attempt (for reference)
XenonRecomp alone produced ~9,400 unsupported-instruction warnings and 66 broken jump
tables. ReXGlue's analyzer handles all of these, so the XenonRecomp config was dropped.
Helper addresses found then, in case they're needed:
`savegprlr_14 0x82E3D9B0`, `restgprlr_14 0x82E3DA00`, `savefpr_14 0x82E4F170`,
`restfpr_14 0x82E4F1BC`, `savevmx_14 0x831F3AC0`, `restvmx_14 0x831F3D58`,
`savevmx_64 0x831F3B54`, `restvmx_64 0x831F3DEC`, `setjmp 0x831F37F0`, `longjmp 0x82E477B0`.

## Building on Windows
Prereqs: CMake 3.25+, Ninja, Visual Studio 2022 with "Desktop development with C++"
plus **C++ Clang Compiler for Windows (20.x+)** and **MSBuild support for LLVM (clang-cl)**.

1. Build and install the SDK once:
   ```
   git clone --recursive https://github.com/rexglue/rexglue-sdk.git
   cd rexglue-sdk
   cmake --preset win-amd64
   cmake --build out/build/win-amd64 --config Release --target install
   ```
2. In this repo, copy `default.xex` (and the rest of the extracted game) into `assets/`.
3. Generate code and build:
   ```
   rexglue codegen mad_riders_manifest.toml
   cmake --preset win-amd64
   cmake --build --preset win-amd64-release
   ```
Run these from a "x64 Native Tools Command Prompt for VS 2022".
