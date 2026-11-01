# Function addresses built in code (lis rX,hi ; addi/ori rY,rX,lo), e.g. thread entry points
# and callbacks, that the analyzer didn't make into functions.
import re, json, sys
from img import u32
TEXT_LO, TEXT_HI = 0x823F0000, 0x823F0000 + 0xFB1C2C
init = open('../generated/default/mad_riders_init.cpp').read()
funcs = set(int(x, 16) for x in re.findall(r'\{ 0x([0-9A-F]{8}), sub_', init))
jt = set()
for t in json.load(open('jt.json')): jt.update(t['labels'])
def s16(x): return x - 0x10000 if x & 0x8000 else x
def ends_block(i): return i in (0x4E800020, 0x4E800420, 0) or ((i >> 26) == 18 and not i & 1)
found = {}
for a in range(TEXT_LO, TEXT_HI, 4):
    i = u32(a)
    if (i >> 26) != 15 or ((i >> 16) & 31) != 0: continue   # lis rD,hi
    rd, hi = (i >> 21) & 31, i & 0xFFFF
    for k in range(1, 6):
        j = u32(a + 4 * k); op = j >> 26
        if op in (14, 24) and ((j >> 16) & 31) == rd:          # addi / ori rY,rD,lo
            v = ((hi << 16) + (s16(j & 0xFFFF) if op == 14 else (j & 0xFFFF))) & 0xFFFFFFFF
            if TEXT_LO <= v < TEXT_HI and v % 4 == 0 and v not in funcs and v not in jt \
               and ends_block(u32(v - 4)) and u32(v) not in (0, 0x60000000):
                found.setdefault(v, []).append(a)
            break
print(len(found), "candidates", file=sys.stderr)
json.dump([{"address": v, "refs": r[:4]} for v, r in sorted(found.items())], open('code_ptr_funcs.json', 'w'))
for v, r in sorted(found.items()): print("%08X refs %d first %08X  first insn %08X" % (v, len(r), r[0], u32(v)))
