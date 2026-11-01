# Code addresses stored in .rdata/.data (vtables, callback tables) that the
# analyzer didn't make into functions. A real target directly follows the end of
# the previous code (blr / unconditional b / bctr / padding).
import re, bisect, json, struct, sys
from img import D, B, u32
TEXT_LO, TEXT_HI = 0x823F0000, 0x823F0000 + 0xFB1C2C
SECTIONS = [(0x82000400, 0x82000400 + 0x397a94), (0x833B0000, 0x833B0000 + 0x1fa4c0)]
init = open('../generated/default/mad_riders_init.cpp').read()
funcs = set(int(x, 16) for x in re.findall(r'\{ 0x([0-9A-F]{8}), sub_', init))
fa = sorted(funcs)
# Jump-table labels are legitimately mid-function: skip them.
jt_labels = set()
for t in json.load(open('jt.json')): jt_labels.update(t['labels'])
def ends_block(i):
    if i in (0x4E800020, 0x4E800420, 0): return True           # blr, bctr, padding
    if (i >> 26) == 18 and (i & 1) == 0: return True            # b (no link)
    return False
refs = {}
for lo, hi in SECTIONS:
    for a in range(lo, hi - 3, 4):
        v = u32(a)
        if TEXT_LO <= v < TEXT_HI and v % 4 == 0 and v not in funcs and v not in jt_labels:
            refs.setdefault(v, []).append(a)
cands = []
for v, where in sorted(refs.items()):
    if not ends_block(u32(v - 4)): continue
    if u32(v) == 0: continue
    cands.append((v, where))
print(f"code pointers in data not at a function: {len(refs)}; after the block-end test: {len(cands)}", file=sys.stderr)
json.dump([{"address": v, "refs": w[:4]} for v, w in cands], open('ptr_funcs.json', 'w'))
for v, w in cands[:15]:
    i = bisect.bisect_right(fa, v)
    print("%08X  inside sub_%08X  refs %d (first at %08X)" % (v, fa[i - 1], len(w), w[0]))
