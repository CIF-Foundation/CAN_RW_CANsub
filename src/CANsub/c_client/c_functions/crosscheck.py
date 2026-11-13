"""crosscheck.py - fuzz libcansub_codec.so (ctypes) against the Python models of the build doc
and against python-can's bit timing. Run on any Linux with the .so built (also on the RT target):
    python3 crosscheck.py [path/to/libcansub_codec.so]
Prints one line per function and 'CROSSCHECK PASS' at the end. python-can is optional."""
import ctypes as C, os, random, sys, zlib

lib = C.CDLL(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "libcansub_codec.so"))
u8p = C.POINTER(C.c_uint8)
def buf(n): return (C.c_uint8 * max(n, 1))()
def arr(b): a = (C.c_uint8 * max(len(b), 1))(*b); return a
ok_all = True
def report(name, bad, n):
    global ok_all
    ok_all &= bad == 0
    print(f"{name:28s} {n:6d} cases, {bad} mismatches")

# --- python reference (same as the build-doc models)
T = []
for i in range(256):
    c = i
    for _ in range(8): c = (c >> 1) ^ 0xEDB88320 if c & 1 else c >> 1
    T.append(c)
DLC = [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64]
def ref_hdlc_encode(p):
    raw = list(p) + list(zlib.crc32(bytes(p)).to_bytes(4, "big")); o = []
    for b in raw: o += [0x7D, 0x5E] if b == 0x7E else [0x7D, 0x5D] if b == 0x7D else [b]
    return [0x7E] + o + [0x7E]
def ref_ser(cid, ide, fdf, brs, d):
    if fdf:
        n = min(len(d), 64); k = next(i for i, v in enumerate(DLC) if v >= n)
        dd = list(d[:n]) + [0] * (DLC[k] - n); fl = 0x80 | (0x40 if brs else 0) | k
    else:
        n = min(len(d), 8); dd = list(d[:n]); fl = n
    idb = list(((cid | 0x80000000) & 0xFFFFFFFF).to_bytes(4, "big")) if ide else list((cid & 0x7FF).to_bytes(2, "big"))
    return [0] * 6 + [fl] + idb + dd

rnd = random.Random(2026)
lib.csb_crc32.restype = C.c_uint32
bad = 0
for _ in range(3000):
    d = bytes(rnd.randrange(256) for _ in range(rnd.randrange(300)))
    bad += lib.csb_crc32(arr(d), len(d)) != zlib.crc32(d)
report("csb_crc32 vs zlib", bad, 3000)

bad = 0
for _ in range(3000):
    d = [rnd.choice([0x7E, 0x7D, rnd.randrange(256)]) for _ in range(rnd.randrange(200))]
    f = buf(2 * (len(d) + 4) + 2); n = lib.csb_hdlc_encode(arr(d), len(d), f, len(f))
    bad += list(f[:n]) != ref_hdlc_encode(d)
report("csb_hdlc_encode", bad, 3000)

bad = 0
for _ in range(3000):
    fdf = rnd.random() < .5; brs = fdf and rnd.random() < .5; ide = rnd.random() < .3
    cid = rnd.randrange(1 << 32); d = [rnd.randrange(256) for _ in range(rnd.randrange(80))]
    p = buf(80); n = lib.csb_can_serialize(C.c_uint32(cid), ide, fdf, brs, arr(d), len(d), p, 80)
    bad += list(p[:n]) != ref_ser(cid, ide, fdf, brs, d)
report("csb_can_serialize", bad, 3000)

# HDLC decode + CAN deserialize on random streams split at random points
hs = buf(lib.csb_hdlc_state_size()); bad = 0; cases = 0
for _ in range(300):
    lib.csb_hdlc_state_init(hs, len(hs))
    frames = []; stream = []
    for _ in range(rnd.randrange(1, 20)):
        fdf = rnd.random() < .5; ide = rnd.random() < .3
        cid = rnd.randrange(1 << 29) if ide else rnd.randrange(0x800)
        d = [rnd.randrange(256) for _ in range(rnd.randrange(65 if fdf else 9))]
        frames.append((cid, ide, fdf, d)); stream += ref_hdlc_encode(ref_ser(cid, ide, fdf, False, d))
        if rnd.random() < .2: stream += [rnd.randrange(256) for _ in range(rnd.randrange(1, 8))] + [0x7E]  # garbage
    got = []; pos = 0
    while pos < len(stream):
        ch = stream[pos:pos + rnd.randrange(1, 50)]; pos += len(ch)
        out = buf(len(ch) + 4096); lens = (C.c_int32 * (len(ch) // 6 + 1))(); np = C.c_int32(); ce = C.c_uint32()
        assert lib.csb_hdlc_decode(hs, len(hs), arr(ch), len(ch), out, len(out), lens, len(lens), C.byref(np), C.byref(ce)) == 0
        off = 0
        for i in range(np.value):
            L = lens[i]; mf = L // 7 + 1
            td = (C.c_double * mf)(); ids = (C.c_uint32 * mf)(); fl = (C.c_uint16 * mf)(); dl = buf(mf); dn = buf(mf); dd = buf(mf * 64)
            nf = C.c_int32(); de = C.c_int32()
            assert lib.csb_can_deserialize(C.cast(C.byref(out, off), u8p), L, td, ids, fl, dl, dn, dd, mf, C.byref(nf), C.byref(de)) == 0
            for j in range(nf.value):
                got.append((ids[j], bool(fl[j] & 1), bool(fl[j] & 2), list(dd[j * 64:j * 64 + dn[j]])))
            off += L
    exp = [(c, i, f, d + [0] * ((DLC[next(k for k, v in enumerate(DLC) if v >= len(d))] - len(d)) if f else 0)) for c, i, f, d in frames]
    cases += len(frames); bad += got != exp
report("hdlc_decode+can_deserialize", bad, cases)

lib.csb_bit_timing.argtypes = [C.c_uint32, C.c_double, C.c_uint32, C.c_double, C.POINTER(C.c_uint16), C.POINTER(C.c_double), C.POINTER(C.c_double)]
try:
    import can, itertools
    bad = 0; cases = 0; skipped = 0
    for n, d, a, b in itertools.product([50_000, 100_000, 125_000, 250_000, 500_000, 800_000, 1_000_000],
                                        [500_000, 1_000_000, 2_000_000, 4_000_000, 5_000_000, 8_000_000], [70, 75, 80, 87.5], [70, 75, 80, 87.5]):
        t = (C.c_uint16 * 8)(); x = C.c_double(); y = C.c_double()
        rc = lib.csb_bit_timing(n, a, d, b, t, C.byref(x), C.byref(y))
        try:
            r = can.BitTimingFd.from_sample_point(f_clock=80_000_000, nom_bitrate=n, nom_sample_point=a, data_bitrate=d, data_sample_point=b)
            rr = (r.nom_brp, r.nom_tseg1, r.nom_tseg2, r.nom_sjw, r.data_brp, r.data_tseg1, r.data_tseg2, r.data_sjw)
        except Exception:
            rr = None
        cases += 1
        if rc == 0: bad += tuple(t) != rr
        elif rr is not None and rr[0] != rr[4]: skipped += 1          # python-can needs 2 prescalers
        elif rr is not None: bad += 1
    report("csb_bit_timing vs python-can", bad, cases)
    print(f"{'':28s} ({skipped} cases need separate prescalers: C returns NO_SOLUTION, by design)")
except ImportError:
    print("csb_bit_timing vs python-can   skipped (python-can not installed)")

# --- CIF / SocketCAN packets (1.2.0) vs a Python copy of NI's Build Socket CAN.vi
import struct
def ni_build(t_ns, ident, ext, mode, payload, endian):
    cid = ident & 0x1FFFFFFF
    if ext: cid |= 0x80000000
    if mode == 3: cid |= 0x40000000
    plen = 0 if mode == 3 else len(payload)
    t = max(t_ns, 0)
    return struct.pack(endian + "IIII", t // 10**9, t % 10**9, 8 + plen, 8 + plen) + \
        struct.pack(">IBBBB", cid, plen, {1: 4, 2: 5}.get(mode, 0), 0, 0) + bytes(payload[:plen])
lib.csb_frames_to_pcap.restype = C.c_int32
bad = 0; N = 2000
for _ in range(N):
    n = rnd.randint(1, 20)
    td = (C.c_double * n)(); ids = (C.c_uint32 * n)(); fl = (C.c_uint16 * n)(); dl = (C.c_uint8 * n)(); dd = (C.c_uint8 * (64 * n))()
    exp = b""; little = rnd.random() < 0.5; tai = rnd.choice([37, 0, 36]); keep = rnd.random() < 0.3
    for i in range(n):
        us = rnd.randrange(0, 2**40); td[i] = 1735689600.0 + us / 1e6
        kind = rnd.choice(["cc", "fd", "fdb", "rtr", "ack", "err"])
        ext = rnd.random() < 0.4; ids[i] = rnd.randrange(0x20000000) if ext else rnd.randrange(0x800)
        f = 1 if ext else 0
        if kind in ("fd", "fdb"): f |= 2 | (8 if kind == "fdb" else 0); L = rnd.choice(DLC)
        elif kind == "rtr": f |= 4; L = 0
        else: L = rnd.randint(0, 8)
        if kind == "ack": f |= 0x20
        if kind == "err": f = 0x40 | (rnd.randint(0, 4) << 8); ids[i] = 0; L = 0
        fl[i] = f; dl[i] = L
        for k in range(L): dd[64 * i + k] = rnd.randrange(256)
        if kind == "err" or (kind == "ack" and not keep): continue
        mode = 3 if kind == "rtr" else 2 if kind == "fdb" else 1 if kind == "fd" else 0
        t_ns = (1735689600 + tai) * 10**9 + us * 1000
        exp += ni_build(t_ns, ids[i], ext, mode, bytes(dd[64 * i:64 * i + L]), "<" if little else ">")
    out = buf(88 * n); np_ = C.c_int32(); ns_ = C.c_int32()
    opts = (4 if little else 0) | (1 if keep else 0)
    r = lib.csb_frames_to_pcap(n, td, ids, fl, dl, dd, tai, opts, out, 88 * n, C.byref(np_), C.byref(ns_))
    if r != len(exp) or bytes(out[:r]) != exp: bad += 1
report("csb_frames_to_pcap vs NI VI", bad, N)

print("CROSSCHECK PASS" if ok_all else "CROSSCHECK FAIL")
sys.exit(0 if ok_all else 1)
