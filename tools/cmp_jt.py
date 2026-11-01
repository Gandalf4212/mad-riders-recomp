import json, re, glob
j = json.load(open('jt.json'))
sw = []   # (reg, frozenset(targets), ncases, file)
pat = re.compile(r'switch \(ctx\.r(\d+)\.u32\) \{(.*?)\n\t\}', re.S)
for f in glob.glob('../generated/default/mad_riders_recomp.*.cpp'):
    s = open(f).read()
    for m in pat.finditer(s):
        body = m.group(2)
        tg = [int(x, 16) for x in re.findall(r'goto loc_([0-9A-F]+);', body)]
        sw.append((int(m.group(1)), tg))
print("switches in generated code:", len(sw))
bykey = {}
for r, tg in sw:
    bykey.setdefault(frozenset(tg), []).append((r, tg))
ok = bad = 0; badlist = []
for t in j:
    k = frozenset(t['labels'])
    hit = [x for x in bykey.get(k, []) if x[0] == t['register'] and x[1] == t['labels']]
    if hit: ok += 1
    else: bad += 1; badlist.append(t)
print("tables generated correctly:", ok, " wrong/missing:", bad)
for t in badlist[:12]:
    print(hex(t['address']), 'r%d' % t['register'], len(t['labels']))
json.dump(badlist, open('jt_bad.json', 'w'))
