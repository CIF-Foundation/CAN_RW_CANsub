"""Generates ../test_cansub_codec.c from the Python models (models.py = the build-doc algorithms,
bt.py = bit timing) and the CSS test vectors in hil_test/codec_ref_tv.py.
Run from anywhere:  python3 gen/gen_ctest.py   then  make test."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, HERE)
from models import *
from bt import lv_bit_timing
src=open(os.path.join(REPO, "hil_test", "codec_ref_tv.py")).read()
TV=eval(src[src.index("TV={")+3:src.index("}\n",src.index("TV={"))+1])
def carr(b): return "{"+",".join("0x%02X"%x for x in b)+"}" if b else "{0}"
out=[]
out.append('''/* test_cansub_codec.c - GENERATED test vectors (CSS TV-01..TV-20 and the build-doc tables).
 * Expected values come from the Python models that produced LABVIEW_NATIVE_CLIENT_BUILD.md.
 * Build and run: make test   ->   prints "ALL PASS" and exits 0. */
#include "cansub_codec.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\\n"); } } while (0)

static uint8_t hstate[CSB_HDLC_STATE_SIZE], wstate[CSB_WS_STATE_SIZE], wstate2[CSB_WS_STATE_SIZE];
static uint8_t out[1 << 17]; static int32_t lens[1 << 14]; static uint8_t ops[1 << 14];
static double t_dev[8192]; static uint32_t ids[8192]; static uint16_t fl[8192];
static uint8_t dlcs[8192], dlen[8192], data[8192 * 64];
''')
crc_cases=[(b"123456789",0xCBF43926),(b"",0),(bytes(H("1C AE 8C 13 E0 00 01 07 FF 00")),0x984FD1B8),(b"\x00",0xD202EF8D),(b"\xff",0xFF000000),(bytes(H("00 00 00 00 00 00 08 00 01 7E 7E 7E 7E 7D 7D 7D 7D")),0x67499708),(bytes(H("00 00 00 00 2A D1 01 00 01 01")),0x27887E7D)]
out.append("static void test_crc(void) {")
for i,(d,c) in enumerate(crc_cases):
    assert zlib.crc32(d)==c
    out.append(f"  {{ static const uint8_t d[] = {carr(d)}; CHECK(csb_crc32(d, {len(d)}) == 0x{c:08X}u, \"crc case {i}\"); }}")
out.append("}")
enc=[H("1C AE 8C 13 E0 00 01 07 FF 00"),H("00 00 00 00 00 00 08 00 01 7E 7E 7E 7E 7D 7D 7D 7D"),H("00 00 00 00 2A D1 01 00 01 01"),[],H("7E 7D")]
out.append("static void test_hdlc_encode(void) { uint8_t f[256]; int32_t n;")
for i,p in enumerate(enc):
    f=HDLC_Encode(p)
    out.append(f"  {{ static const uint8_t p[] = {carr(p)}; static const uint8_t e[] = {carr(f)}; n = csb_hdlc_encode(p, {len(p)}, f, sizeof f); CHECK(n == {len(f)} && memcmp(f, e, {len(f)}) == 0, \"hdlc encode case {i}\"); }}")
out.append("}")
out.append("static void test_tv(void) { int32_t np, nf, de; uint32_t ce = 0;")
for k in range(1,21):
    st=([],False,False,0); frames=[]; detot=0
    for c in TV[k]:
        ps,st=HDLC_Decode(H(c),st)
        for p in ps:
            f,d=CAN_Deserialize(p); frames+=f; detot+=d
    out.append(f"  /* TV-{k:02d} */ csb_hdlc_state_init(hstate, sizeof hstate); {{ int32_t total = 0, detotal = 0;")
    for ci,c in enumerate(TV[k]):
        b=H(c)
        out.append(f"    {{ static const uint8_t in[] = {carr(b)}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, {len(b)}, out, sizeof out, lens, 1<<14, &np, &ce) == 0, \"TV-{k:02d} decode\");")
        out.append(f"      int32_t off = 0; for (int32_t i = 0; i < np; i++) {{ CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, \"TV-{k:02d} deser\"); total += nf; detotal += de; off += lens[i]; }} }}")
    out.append(f"    CHECK(total == {len(frames)}, \"TV-{k:02d} frames %d\", total); CHECK(detotal == {detot}, \"TV-{k:02d} decode errors\"); CHECK(ce == {st[3]}u, \"TV-{k:02d} crc errors %u\", ce);")
    for i,x in enumerate(frames):
        if x.get("error"):
            fl_=0x40|((x["errtype"])<<8)
            out.append(f"    CHECK(fl[{i}] == 0x{fl_:04X} && ids[{i}] == 0, \"TV-{k:02d} frame {i} error flags %04X\", fl[{i}]);")
        else:
            fl_=(1 if x["ide"] else 0)|(2 if x["fdf"] else 0)|(4 if x["rtr"] else 0)|(8 if x["brs"] else 0)|(16 if x["esi"] else 0)|(32 if x["tx_ack"] else 0)
            out.append(f"    {{ static const uint8_t e[] = {carr(x['data'])}; CHECK(ids[{i}] == 0x{x['id']:X}u && fl[{i}] == 0x{fl_:04X} && dlcs[{i}] == {x['dlc']} && dlen[{i}] == {len(x['data'])} && memcmp(data + {i}*64, e, {len(x['data'])}) == 0, \"TV-{k:02d} frame {i}\"); }}")
        out.append(f"    CHECK(fabs(t_dev[{i}] - {x['t_dev']!r}) < 1e-7, \"TV-{k:02d} t_dev {i} %.6f\", t_dev[{i}]);")
    out.append("  }")
out.append("}")
ser=[(0x7FF,0,0,0,"00"),(0x1FFFFFFF,1,0,0,"00"),(0,0,0,0,""),(0x123,0,0,0,"11 22 33 44 55 66 77 88"),(0x18FEF100,1,0,0,"01 02 03"),(0x800,0,0,0,"AA"),(0x123,0,0,1,"01"),(0x123,0,0,0,"01 02 03 04 05 06 07 08 09"),(1,0,1,0,"00"),(0x123,0,1,1,"11 22 33 44 55 66 77 88"),(0x123,0,1,1,"01 02 03 04 05 06 07 08 09 0A"),(0x18FEF100,1,1,0," ".join("%02X"%i for i in range(20))),(0x7FF,0,1,0,""),(1,0,1,1," ".join("%02X"%i for i in range(64)))]
out.append("static void test_serialize(void) { uint8_t p[80]; int32_t n;")
for i,(cid,e,f,b,d) in enumerate(ser):
    exp=CAN_Serialize_FD(cid,bool(e),bool(f),bool(b),H(d))
    out.append(f"  {{ static const uint8_t d[] = {carr(H(d))}; static const uint8_t e[] = {carr(exp)}; n = csb_can_serialize(0x{cid:X}u, {e}, {f}, {b}, d, {len(H(d))}, p, sizeof p); CHECK(n == {len(exp)} && memcmp(p, e, {len(exp)}) == 0, \"serialize case {i} n=%d\", n); }}")
out.append("}")
wsb=[("01 02 03 04 05",2),("03 E8",8),("",10),("AA BB",10)]
out.append("static void test_ws_build(void) { uint8_t f[300]; int32_t n;")
for i,(d,op) in enumerate(wsb):
    exp=WS_BuildFrame(H(d),op)
    out.append(f"  {{ static const uint8_t d[] = {carr(H(d))}; static const uint8_t e[] = {carr(exp)}; n = csb_ws_build_frame(d, {len(H(d))}, {op}, 0x12345678u, f, sizeof f); CHECK(n == {len(exp)} && memcmp(f, e, {len(exp)}) == 0, \"ws build case {i}\"); }}")
for L in (125,126,200):
    exp=WS_BuildFrame([0]*L,2)
    out.append(f"  {{ static uint8_t d[{L}]; static const uint8_t e[] = {carr(exp)}; memset(d, 0, sizeof d); n = csb_ws_build_frame(d, {L}, 2, 0x12345678u, f, sizeof f); CHECK(n == {len(exp)} && memcmp(f, e, {len(exp)}) == 0, \"ws build {L} bytes\"); }}")
out.append("}")
Z=" ".join(["00"]*200)
wsp=[["82 05 01 02 03 04 05"],["82 05 01","02 03 04 05"],["89 00"],["82 02 AA BB 89 00 82 01"],["88 02 03 E8"],["82 7E 00 C8 "+Z],["82 7E","00 C8",Z],["82"]]
out.append("static void test_ws_parse(void) { int32_t nf;")
for i,calls in enumerate(wsp):
    buf=[]; allf=[]
    for c in calls:
        f,buf=WS_ParseFrames(H(c),buf); allf+=f
    out.append(f"  csb_ws_state_init(wstate, sizeof wstate); {{ int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;")
    for c in calls:
        b=H(c)
        out.append(f"    {{ static const uint8_t in[] = {carr(b)}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, {len(b)}, ops, lens, 1<<14, out, sizeof out, &nf) == 0, \"ws parse {i}\");")
        out.append(f"      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) {{ got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; }} }}")
    out.append(f"    CHECK(total == {len(allf)}, \"ws parse {i} frames %d\", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;")
    pos=0
    for j,(op,p) in enumerate(allf):
        out.append(f"    {{ static const uint8_t e[] = {carr(p)}; CHECK(got_ops[{j}] == {op} && got_lens[{j}] == {len(p)} && memcmp(got + {pos}, e, {len(p)}) == 0, \"ws parse {i} frame {j}\"); }}")
        pos+=len(p)
    out.append("  }")
out.append("}")
btc=[(500000,80,2000000,80),(500000,80,1000000,80),(250000,80,2000000,80),(500000,87.5,2000000,75),(500000,80,4000000,80),(500000,80,5000000,80),(1000000,80,5000000,75),(1000000,80,8000000,80),(125000,87.5,500000,80),(250000,80,250000,80),(500000,80,250000,80),(500000,40,2000000,80),(125000,80,5000000,80)]
out.append("static void test_bit_timing(void) { uint16_t t[8]; double a, b; int32_t rc;")
for i,(n,a,d,b) in enumerate(btc):
    m,err=lv_bit_timing(n,a,d,b)
    if m:
        v=[m['brp'],m['seg1'],m['seg2'],m['sjw'],m['dbrp'],m['dseg1'],m['dseg2'],m['dsjw']]
        out.append(f"  rc = csb_bit_timing({n}u, {a}, {d}u, {b}, t, &a, &b); CHECK(rc == 0 && t[0]=={v[0]} && t[1]=={v[1]} && t[2]=={v[2]} && t[3]=={v[3]} && t[4]=={v[4]} && t[5]=={v[5]} && t[6]=={v[6]} && t[7]=={v[7]} && fabs(a-{m['sp']})<1e-9 && fabs(b-{m['dsp']})<1e-9, \"bit timing case {i} rc=%d\", rc);")
    else:
        code = -2 if err=="invalid input" else -3
        out.append(f"  rc = csb_bit_timing({n}u, {a}, {d}u, {b}, t, &a, &b); CHECK(rc == {code}, \"bit timing case {i} rc=%d\", rc);")
out.append("}")
out.append(open(os.path.join(HERE, "helpers_test.c")).read())
open(os.path.join(HERE, "..", "test_cansub_codec.c"), "w").write("\n".join(out))
print("generated")
