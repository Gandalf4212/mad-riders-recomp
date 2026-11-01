# Finds absolute jump tables in .text:
#   cmplwi crF,rI,N ; bgt crF,default ; ... lis rT,hi ; [rlwinm rO,rI,2,0,29] ; addi rT,rT,lo ;
#   lwzx rS,rT,rO ; mtctr rS ; bctr     -> table at hi<<16+lo, N+1 absolute entries
import struct, sys
from img import D, B, u32
TEXT_LO, TEXT_HI = 0x823F0000, 0x823F0000 + 0xFB1C2C
def s16(x): return x - 0x10000 if x & 0x8000 else x
def op(i): return i >> 26
res, skipped = [], []
for a in range(TEXT_LO, TEXT_HI, 4):
    if u32(a) != 0x4E800420: continue          # bctr
    m = u32(a - 4)
    if (m & 0xFC1FFFFF) != 0x7C0903A6: continue  # mtctr rS
    rS = (m >> 21) & 31
    l = u32(a - 8)
    if op(l) != 31 or ((l >> 1) & 0x3FF) != 23 or ((l >> 21) & 31) != rS:
        skipped.append((a, 'not lwzx')); continue
    rA, rB = (l >> 16) & 31, (l >> 11) & 31
    hi = lo = None; rT = rO = rI = None
    for k in range(3, 14):
        i = u32(a - 4 * k)
        o = op(i); d = (i >> 21) & 31; ra = (i >> 16) & 31
        if o == 14 and lo is None and d == ra and d in (rA, rB):
            lo = s16(i & 0xFFFF); rT = d; rO = rB if d == rA else rA
        elif o == 15 and ra == 0 and hi is None and d == rT:
            hi = i & 0xFFFF
        elif o == 21 and d == rO and rI is None:
            sh, mb, me = (i >> 11) & 31, (i >> 6) & 31, (i >> 1) & 31
            if sh == 2 and mb == 0 and me == 29: rI = (i >> 21) & 31; rI = ra  # rlwinm rO,rI,...: rS field is bits 21-25
        if hi is not None and rI is not None: break
    # fix: for rlwinm the source register is in bits 21-25 (rS), dest in 16-20 (rA)
    rI = None
    for k in range(3, 14):
        i = u32(a - 4 * k)
        if op(i) == 21 and ((i >> 16) & 31) == rO and ((i >> 11) & 31) == 2 and ((i >> 6) & 31) == 0 and ((i >> 1) & 31) == 29:
            rI = (i >> 21) & 31; break
    if hi is None or lo is None or rI is None:
        skipped.append((a, 'pattern')); continue
    table = ((hi << 16) + lo) & 0xFFFFFFFF
    n = None
    for k in range(3, 24):
        i = u32(a - 4 * k)
        if op(i) == 10 and ((i >> 16) & 31) == rI and ((i >> 21) & 1) == 0:
            n = (i & 0xFFFF) + 1; break
    if n is None or n > 4096:
        skipped.append((a, 'no bound')); continue
    labels = [u32(table + 4 * j) for j in range(n)]
    if not all(TEXT_LO <= x < TEXT_HI and abs(x - a) < 0x40000 for x in labels):
        skipped.append((a, 'labels out of range')); continue
    res.append((a, rI, table, labels))
print(f"absolute tables: {len(res)}, other bctr patterns: {len(skipped)}", file=sys.stderr)
from collections import Counter
print(Counter(r for _, r in skipped), file=sys.stderr)
import json
json.dump([{"address": a, "register": r, "table": t, "labels": l} for a, r, t, l in res], open('jt.json', 'w'))
