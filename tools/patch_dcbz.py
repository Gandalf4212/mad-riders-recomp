# Post-codegen fix: on the Xbox 360 a plain dcbz clears a whole 128-byte cache line
# (Xenon has no 32-byte short-line mode; Xenia does the same). ReXGlue v0.10.0
# emits a 32-byte clear, which leaves stale data in the WMV decoder's coefficient
# buffers (sub_831C1A98, sub_831C22F8) and makes every video grainy.
# Run after `rexglue codegen`: python tools/patch_dcbz.py generated/default
import glob, re, sys
pat = re.compile(r'(\t// dcbz [^\n]*\n\tea = \([^\n]*\) & )~31;\n\tmemset\(\(void\*\)REX_RAW_ADDR\(ea\), 0, 32\);')
total = 0
for f in sorted(glob.glob(sys.argv[1] + '/*.cpp')):
    s = open(f, newline='').read()
    s2, n = pat.subn(r'\1~127;\n\tmemset((void*)REX_RAW_ADDR(ea), 0, 128);', s)
    if n:
        open(f, 'w', newline='').write(s2); total += n
print(f'patched {total} dcbz sites')
