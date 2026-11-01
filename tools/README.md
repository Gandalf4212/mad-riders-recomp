# Analysis tools

Run from this folder. Need the game's `default.xex` (not committed).

- `xexdump.cpp`: dumps the decrypted, decompressed image to `image.bin`. Build against
  [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)'s XenonUtils:
  `clang++ -std=c++20 -I<XenonRecomp>/XenonUtils -I<XenonRecomp>/thirdparty xexdump.cpp <build>/XenonUtils/libXenonUtils.a -o xexdump`,
  then `./xexdump ../assets/default.xex image.bin`
- `scan_jt.py`: finds absolute jump tables (`lis/addi/lwzx/mtctr/bctr`, bounded by `cmplwi`) -> `jt.json`
- `cmp_jt.py`: checks each table against the switches in `generated/`; wrong ones go to `jt_bad.json`
  and need a `[[switch_tables]]` entry in `config/overrides.toml`
- `scan_ptr_funcs.py`: code addresses stored in `.rdata`/`.data` (vtables, callback tables) that aren't
  functions -> `ptr_funcs.json`. Skips SEH scope-table targets (handlers inside functions) and nop padding by hand
  when adding them to `config/overrides.toml`.
- `scan_code_ptrs.py`: function addresses built in code (`lis`/`addi`) that aren't functions -> `code_ptr_funcs.json`;
  drop jump-table data and in-function computed jumps (`mtctr`+`bctr`) before adding them.
