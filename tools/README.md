# Analysis tools

Run from this folder. Need the game's `default.xex` (not committed).

- `xexdump.cpp`: dumps the decrypted, decompressed image to `image.bin`. Build against
  [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)'s XenonUtils:
  `clang++ -std=c++20 -I<XenonRecomp>/XenonUtils -I<XenonRecomp>/thirdparty xexdump.cpp <build>/XenonUtils/libXenonUtils.a -o xexdump`,
  then `./xexdump ../assets/default.xex image.bin`
- `scan_jt.py`: finds absolute jump tables (`lis/addi/lwzx/mtctr/bctr`, bounded by `cmplwi`) -> `jt.json`
- `cmp_jt.py`: checks each table against the switches in `generated/`; wrong ones go to `jt_bad.json`
  and need a `[[switch_tables]]` entry in `config/overrides.toml`
