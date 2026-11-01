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
- [x] Generated code committed; GitHub Actions Windows build
- [x] First-run setup: finds or asks for the package, extracts it, checks xex SHA-1
- [x] F1/Esc settings menu, key rebinding (adapted from afterburner-climax-recomp)
- [ ] First boot: fill in missing kernel imports / stubs
- [ ] Graphics, audio, input

## Overrides applied (`config/overrides.toml`)
| Address | Why |
|---|---|
| 0x825B1C60 | Target of thunk `sub_825B1CD8` (`b 0x825B1C60`); separate function that follows the 4-instruction `sub_825B1C50` |
| 0x826C1D54 (switch) | Jump table in sub_826C1A98 read with the wrong index register (r11, 1 entry; really r9, 79 entries). Crashed with an illegal instruction on the first run. All 196 absolute jump tables checked with `tools/cmp_jt.py`. |
| 58 + 323 functions | Reached only through pointers, so the analyzer folded them into the code before them: 56 from vtables/callback tables in .rdata/.data (`tools/scan_ptr_funcs.py`, e.g. `0x830B3070`, which crashed the second run with "call to unregistered function") and 323 whose address is built in code and stored or passed as a callback (`tools/scan_code_ptrs.py`). |
| 0x82C95D00 | Target of thunk `sub_82C95FD8` (`b 0x82C95D00`); separate function that follows the 26-instruction `sub_82C95C98` |

## Earlier XenonRecomp attempt (for reference)
XenonRecomp alone produced ~9,400 unsupported-instruction warnings and 66 broken jump
tables. ReXGlue's analyzer handles all of these, so the XenonRecomp config was dropped.
Helper addresses found then, in case they're needed:
`savegprlr_14 0x82E3D9B0`, `restgprlr_14 0x82E3DA00`, `savefpr_14 0x82E4F170`,
`restfpr_14 0x82E4F1BC`, `savevmx_14 0x831F3AC0`, `restvmx_14 0x831F3D58`,
`savevmx_64 0x831F3B54`, `restvmx_64 0x831F3DEC`, `setjmp 0x831F37F0`, `longjmp 0x82E477B0`.

## Building
See README.md.
