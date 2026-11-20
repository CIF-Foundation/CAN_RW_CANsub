/* test_cansub_codec.c - GENERATED test vectors (CSS TV-01..TV-20 and the build-doc tables).
 * Expected values come from the Python models that produced LABVIEW_NATIVE_CLIENT_BUILD.md.
 * Build and run: make test   ->   prints "ALL PASS" and exits 0. */
#include "cansub_codec.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint8_t hstate[CSB_HDLC_STATE_SIZE], wstate[CSB_WS_STATE_SIZE], wstate2[CSB_WS_STATE_SIZE];
static uint8_t out[1 << 17]; static int32_t lens[1 << 14]; static uint8_t ops[1 << 14];
static double t_dev[8192]; static uint32_t ids[8192]; static uint16_t fl[8192];
static uint8_t dlcs[8192], dlen[8192], data[8192 * 64];

static void test_crc(void) {
  { static const uint8_t d[] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39}; CHECK(csb_crc32(d, 9) == 0xCBF43926u, "crc case 0"); }
  { static const uint8_t d[] = {0}; CHECK(csb_crc32(d, 0) == 0x00000000u, "crc case 1"); }
  { static const uint8_t d[] = {0x1C,0xAE,0x8C,0x13,0xE0,0x00,0x01,0x07,0xFF,0x00}; CHECK(csb_crc32(d, 10) == 0x984FD1B8u, "crc case 2"); }
  { static const uint8_t d[] = {0x00}; CHECK(csb_crc32(d, 1) == 0xD202EF8Du, "crc case 3"); }
  { static const uint8_t d[] = {0xFF}; CHECK(csb_crc32(d, 1) == 0xFF000000u, "crc case 4"); }
  { static const uint8_t d[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x7E,0x7E,0x7E,0x7E,0x7D,0x7D,0x7D,0x7D}; CHECK(csb_crc32(d, 17) == 0x67499708u, "crc case 5"); }
  { static const uint8_t d[] = {0x00,0x00,0x00,0x00,0x2A,0xD1,0x01,0x00,0x01,0x01}; CHECK(csb_crc32(d, 10) == 0x27887E7Du, "crc case 6"); }
}
static void test_hdlc_encode(void) { uint8_t f[256]; int32_t n;
  { static const uint8_t p[] = {0x1C,0xAE,0x8C,0x13,0xE0,0x00,0x01,0x07,0xFF,0x00}; static const uint8_t e[] = {0x7E,0x1C,0xAE,0x8C,0x13,0xE0,0x00,0x01,0x07,0xFF,0x00,0x98,0x4F,0xD1,0xB8,0x7E}; n = csb_hdlc_encode(p, 10, f, sizeof f); CHECK(n == 16 && memcmp(f, e, 16) == 0, "hdlc encode case 0"); }
  { static const uint8_t p[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x7E,0x7E,0x7E,0x7E,0x7D,0x7D,0x7D,0x7D}; static const uint8_t e[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x7D,0x5E,0x7D,0x5E,0x7D,0x5E,0x7D,0x5E,0x7D,0x5D,0x7D,0x5D,0x7D,0x5D,0x7D,0x5D,0x67,0x49,0x97,0x08,0x7E}; n = csb_hdlc_encode(p, 17, f, sizeof f); CHECK(n == 31 && memcmp(f, e, 31) == 0, "hdlc encode case 1"); }
  { static const uint8_t p[] = {0x00,0x00,0x00,0x00,0x2A,0xD1,0x01,0x00,0x01,0x01}; static const uint8_t e[] = {0x7E,0x00,0x00,0x00,0x00,0x2A,0xD1,0x01,0x00,0x01,0x01,0x27,0x88,0x7D,0x5E,0x7D,0x5D,0x7E}; n = csb_hdlc_encode(p, 10, f, sizeof f); CHECK(n == 18 && memcmp(f, e, 18) == 0, "hdlc encode case 2"); }
  { static const uint8_t p[] = {0}; static const uint8_t e[] = {0x7E,0x00,0x00,0x00,0x00,0x7E}; n = csb_hdlc_encode(p, 0, f, sizeof f); CHECK(n == 6 && memcmp(f, e, 6) == 0, "hdlc encode case 3"); }
  { static const uint8_t p[] = {0x7E,0x7D}; static const uint8_t e[] = {0x7E,0x7D,0x5E,0x7D,0x5D,0xDE,0xD1,0x4B,0x06,0x7E}; n = csb_hdlc_encode(p, 2, f, sizeof f); CHECK(n == 10 && memcmp(f, e, 10) == 0, "hdlc encode case 4"); }
}
static void test_tv(void) { int32_t np, nf, de; uint32_t ce = 0;
  /* TV-01 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x1C,0xAE,0x8C,0x13,0xE0,0x00,0x01,0x07,0xFF,0x00,0x98,0x4F,0xD1,0xB8,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-01 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-01 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-01 frames %d", total); CHECK(detotal == 0, "TV-01 decode errors"); CHECK(ce == 0u, "TV-01 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x7FFu && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-01 frame 0"); }
    CHECK(fabs(t_dev[0] - 1767225600.0) < 1e-7, "TV-01 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-02 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x39,0x5D,0x18,0x27,0xC0,0x00,0x01,0x9F,0xFF,0xFF,0xFF,0x00,0x82,0x83,0x9F,0x92,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 18, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-02 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-02 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-02 frames %d", total); CHECK(detotal == 0, "TV-02 decode errors"); CHECK(ce == 0u, "TV-02 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1FFFFFFFu && fl[0] == 0x0001 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-02 frame 0"); }
    CHECK(fabs(t_dev[0] - 1798761600.0) < 1e-7, "TV-02 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-03 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x01,0xEF,0x87,0xF8,0x40,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 15, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-03 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-03 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-03 frames %d", total); CHECK(detotal == 0, "TV-03 decode errors"); CHECK(ce == 0u, "TV-03 crc errors %u", ce);
    { static const uint8_t e[] = {0}; CHECK(ids[0] == 0x1u && fl[0] == 0x0004 && dlcs[0] == 8 && dlen[0] == 0 && memcmp(data + 0*64, e, 0) == 0, "TV-03 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-03 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-04 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x01,0x00,0xAF,0x74,0x88,0x69,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-04 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-04 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-04 frames %d", total); CHECK(detotal == 0, "TV-04 decode errors"); CHECK(ce == 0u, "TV-04 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0002 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-04 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-04 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-05 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0xC1,0x00,0x01,0x00,0x34,0x60,0xD0,0x54,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-05 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-05 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-05 frames %d", total); CHECK(detotal == 0, "TV-05 decode errors"); CHECK(ce == 0u, "TV-05 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x000A && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-05 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-05 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-06 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0xA1,0x00,0x01,0x00,0x0F,0x46,0x27,0x57,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-06 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-06 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-06 frames %d", total); CHECK(detotal == 0, "TV-06 decode errors"); CHECK(ce == 0u, "TV-06 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0012 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-06 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-06 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-07 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x8F,0x00,0x01,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0xDE,0x43,0x31,0x51,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 79, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-07 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-07 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-07 frames %d", total); CHECK(detotal == 0, "TV-07 decode errors"); CHECK(ce == 0u, "TV-07 crc errors %u", ce);
    { static const uint8_t e[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F}; CHECK(ids[0] == 0x1u && fl[0] == 0x0002 && dlcs[0] == 15 && dlen[0] == 64 && memcmp(data + 0*64, e, 64) == 0, "TV-07 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-07 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-08 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x01,0x00,0x12,0x34,0x69,0xCD,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-08 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-08 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-08 frames %d", total); CHECK(detotal == 0, "TV-08 decode errors"); CHECK(ce == 0u, "TV-08 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0020 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-08 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-08 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-09 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x42,0x2D,0x3E,0x52,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-09 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-09 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-09 frames %d", total); CHECK(detotal == 0, "TV-09 decode errors"); CHECK(ce == 0u, "TV-09 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-09 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-09 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-10 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 1, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x00,0x00,0x00,0x00,0x00,0x00}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 6, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x01}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 1, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x00,0x01}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 2, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x00}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 1, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x42,0x2D,0x3E,0x52}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 4, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 1, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-10 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-10 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-10 frames %d", total); CHECK(detotal == 0, "TV-10 decode errors"); CHECK(ce == 0u, "TV-10 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-10 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-10 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-11 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x04,0x04,0xD1,0x2C,0x1F,0xD9,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 46, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-11 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-11 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 4, "TV-11 frames %d", total); CHECK(detotal == 0, "TV-11 decode errors"); CHECK(ce == 0u, "TV-11 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-11 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-11 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x02}; CHECK(ids[1] == 0x2u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-11 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-11 t_dev 1 %.6f", t_dev[1]);
    { static const uint8_t e[] = {0x03}; CHECK(ids[2] == 0x3u && fl[2] == 0x0000 && dlcs[2] == 1 && dlen[2] == 1 && memcmp(data + 2*64, e, 1) == 0, "TV-11 frame 2"); }
    CHECK(fabs(t_dev[2] - 1735689600.0) < 1e-7, "TV-11 t_dev 2 %.6f", t_dev[2]);
    { static const uint8_t e[] = {0x04}; CHECK(ids[3] == 0x4u && fl[3] == 0x0000 && dlcs[3] == 1 && dlen[3] == 1 && memcmp(data + 3*64, e, 1) == 0, "TV-11 frame 3"); }
    CHECK(fabs(t_dev[3] - 1735689600.0) < 1e-7, "TV-11 t_dev 3 %.6f", t_dev[3]);
  }
  /* TV-12 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x01,0x35,0x2A,0x0E,0xC4,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x02,0x02,0x87,0x0E,0x0C,0xBD,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x03,0x03,0xE9,0x12,0x0D,0x6A,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x04,0x04,0x38,0x37,0x0E,0x0E,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 64, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-12 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-12 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 4, "TV-12 frames %d", total); CHECK(detotal == 0, "TV-12 decode errors"); CHECK(ce == 0u, "TV-12 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-12 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-12 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x02}; CHECK(ids[1] == 0x2u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-12 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-12 t_dev 1 %.6f", t_dev[1]);
    { static const uint8_t e[] = {0x03}; CHECK(ids[2] == 0x3u && fl[2] == 0x0000 && dlcs[2] == 1 && dlen[2] == 1 && memcmp(data + 2*64, e, 1) == 0, "TV-12 frame 2"); }
    CHECK(fabs(t_dev[2] - 1735689600.0) < 1e-7, "TV-12 t_dev 2 %.6f", t_dev[2]);
    { static const uint8_t e[] = {0x04}; CHECK(ids[3] == 0x4u && fl[3] == 0x0000 && dlcs[3] == 1 && dlen[3] == 1 && memcmp(data + 3*64, e, 1) == 0, "TV-12 frame 3"); }
    CHECK(fabs(t_dev[3] - 1735689600.0) < 1e-7, "TV-12 t_dev 3 %.6f", t_dev[3]);
  }
  /* TV-13 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x01,0x02,0x03,0x04,0x05,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x42,0x2D,0x3E,0x52,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 21, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-13 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-13 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-13 frames %d", total); CHECK(detotal == 0, "TV-13 decode errors"); CHECK(ce == 0u, "TV-13 crc errors %u", ce);
    { static const uint8_t e[] = {0x00}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-13 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-13 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-14 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x01,0x35,0x2A,0x0E,0xC4,0x7E,0x01,0x02,0x03,0x04,0x05,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x02,0x02,0x87,0x0E,0x0C,0xBD,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 37, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-14 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-14 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 2, "TV-14 frames %d", total); CHECK(detotal == 0, "TV-14 decode errors"); CHECK(ce == 1u, "TV-14 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-14 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-14 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x02}; CHECK(ids[1] == 0x2u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-14 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-14 t_dev 1 %.6f", t_dev[1]);
  }
  /* TV-15 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x01,0x35,0x2A,0x0E,0xC4,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x03,0x03,0xE9,0x12,0x0D,0x6A,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 48, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-15 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-15 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 2, "TV-15 frames %d", total); CHECK(detotal == 0, "TV-15 decode errors"); CHECK(ce == 1u, "TV-15 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-15 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-15 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x03}; CHECK(ids[1] == 0x3u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-15 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-15 t_dev 1 %.6f", t_dev[1]);
  }
  /* TV-16 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x7D,0x5E,0x7D,0x5E,0x7D,0x5E,0x7D,0x5E,0x7D,0x5D,0x7D,0x5D,0x7D,0x5D,0x7D,0x5D,0x67,0x49,0x97,0x08,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 31, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-16 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-16 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-16 frames %d", total); CHECK(detotal == 0, "TV-16 decode errors"); CHECK(ce == 0u, "TV-16 crc errors %u", ce);
    { static const uint8_t e[] = {0x7E,0x7E,0x7E,0x7E,0x7D,0x7D,0x7D,0x7D}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 8 && dlen[0] == 8 && memcmp(data + 0*64, e, 8) == 0, "TV-16 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-16 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-17 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x2A,0xD1,0x01,0x00,0x01,0x01,0x27,0x88,0x7D,0x5E,0x7D,0x5D,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 18, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-17 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-17 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 1, "TV-17 frames %d", total); CHECK(detotal == 0, "TV-17 decode errors"); CHECK(ce == 0u, "TV-17 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-17 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.010961) < 1e-7, "TV-17 t_dev 0 %.6f", t_dev[0]);
  }
  /* TV-18 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x02,0x02,0xBE,0xC0,0x22,0x71,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x03,0x03,0xE9,0x12,0x0D,0x6A,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 42, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-18 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-18 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 2, "TV-18 frames %d", total); CHECK(detotal == 1, "TV-18 decode errors"); CHECK(ce == 0u, "TV-18 crc errors %u", ce);
    { static const uint8_t e[] = {0x01}; CHECK(ids[0] == 0x1u && fl[0] == 0x0000 && dlcs[0] == 1 && dlen[0] == 1 && memcmp(data + 0*64, e, 1) == 0, "TV-18 frame 0"); }
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-18 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x03}; CHECK(ids[1] == 0x3u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-18 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-18 t_dev 1 %.6f", t_dev[1]);
  }
  /* TV-19 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0xA6,0x02,0xFF,0xB6,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 13, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-19 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-19 deser"); total += nf; detotal += de; off += lens[i]; } }
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x42,0x2D,0x3E,0x52,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 16, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-19 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-19 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 2, "TV-19 frames %d", total); CHECK(detotal == 0, "TV-19 decode errors"); CHECK(ce == 0u, "TV-19 crc errors %u", ce);
    CHECK(fl[0] == 0x0040 && ids[0] == 0, "TV-19 frame 0 error flags %04X", fl[0]);
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-19 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x00}; CHECK(ids[1] == 0x1u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-19 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-19 t_dev 1 %.6f", t_dev[1]);
  }
  /* TV-20 */ csb_hdlc_state_init(hstate, sizeof hstate); { int32_t total = 0, detotal = 0;
    { static const uint8_t in[] = {0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xEE,0xD4,0x28,0x4E,0x7E}; CHECK(csb_hdlc_decode(hstate, sizeof hstate, in, 23, out, sizeof out, lens, 1<<14, &np, &ce) == 0, "TV-20 decode");
      int32_t off = 0; for (int32_t i = 0; i < np; i++) { CHECK(csb_can_deserialize(out + off, lens[i], t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0, "TV-20 deser"); total += nf; detotal += de; off += lens[i]; } }
    CHECK(total == 2, "TV-20 frames %d", total); CHECK(detotal == 0, "TV-20 decode errors"); CHECK(ce == 0u, "TV-20 crc errors %u", ce);
    CHECK(fl[0] == 0x0040 && ids[0] == 0, "TV-20 frame 0 error flags %04X", fl[0]);
    CHECK(fabs(t_dev[0] - 1735689600.0) < 1e-7, "TV-20 t_dev 0 %.6f", t_dev[0]);
    { static const uint8_t e[] = {0x00}; CHECK(ids[1] == 0x1u && fl[1] == 0x0000 && dlcs[1] == 1 && dlen[1] == 1 && memcmp(data + 1*64, e, 1) == 0, "TV-20 frame 1"); }
    CHECK(fabs(t_dev[1] - 1735689600.0) < 1e-7, "TV-20 t_dev 1 %.6f", t_dev[1]);
  }
}
static void test_serialize(void) { uint8_t p[80]; int32_t n;
  { static const uint8_t d[] = {0x00}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x07,0xFF,0x00}; n = csb_can_serialize(0x7FFu, 0, 0, 0, d, 1, p, sizeof p); CHECK(n == 10 && memcmp(p, e, 10) == 0, "serialize case 0 n=%d", n); }
  { static const uint8_t d[] = {0x00}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x9F,0xFF,0xFF,0xFF,0x00}; n = csb_can_serialize(0x1FFFFFFFu, 1, 0, 0, d, 1, p, sizeof p); CHECK(n == 12 && memcmp(p, e, 12) == 0, "serialize case 1 n=%d", n); }
  { static const uint8_t d[] = {0}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; n = csb_can_serialize(0x0u, 0, 0, 0, d, 0, p, sizeof p); CHECK(n == 9 && memcmp(p, e, 9) == 0, "serialize case 2 n=%d", n); }
  { static const uint8_t d[] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x01,0x23,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}; n = csb_can_serialize(0x123u, 0, 0, 0, d, 8, p, sizeof p); CHECK(n == 17 && memcmp(p, e, 17) == 0, "serialize case 3 n=%d", n); }
  { static const uint8_t d[] = {0x01,0x02,0x03}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x98,0xFE,0xF1,0x00,0x01,0x02,0x03}; n = csb_can_serialize(0x18FEF100u, 1, 0, 0, d, 3, p, sizeof p); CHECK(n == 14 && memcmp(p, e, 14) == 0, "serialize case 4 n=%d", n); }
  { static const uint8_t d[] = {0xAA}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0xAA}; n = csb_can_serialize(0x800u, 0, 0, 0, d, 1, p, sizeof p); CHECK(n == 10 && memcmp(p, e, 10) == 0, "serialize case 5 n=%d", n); }
  { static const uint8_t d[] = {0x01}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x23,0x01}; n = csb_can_serialize(0x123u, 0, 0, 1, d, 1, p, sizeof p); CHECK(n == 10 && memcmp(p, e, 10) == 0, "serialize case 6 n=%d", n); }
  { static const uint8_t d[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x01,0x23,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08}; n = csb_can_serialize(0x123u, 0, 0, 0, d, 9, p, sizeof p); CHECK(n == 17 && memcmp(p, e, 17) == 0, "serialize case 7 n=%d", n); }
  { static const uint8_t d[] = {0x00}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x01,0x00}; n = csb_can_serialize(0x1u, 0, 1, 0, d, 1, p, sizeof p); CHECK(n == 10 && memcmp(p, e, 10) == 0, "serialize case 8 n=%d", n); }
  { static const uint8_t d[] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0xC8,0x01,0x23,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}; n = csb_can_serialize(0x123u, 0, 1, 1, d, 8, p, sizeof p); CHECK(n == 17 && memcmp(p, e, 17) == 0, "serialize case 9 n=%d", n); }
  { static const uint8_t d[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0xC9,0x01,0x23,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x00,0x00}; n = csb_can_serialize(0x123u, 0, 1, 1, d, 10, p, sizeof p); CHECK(n == 21 && memcmp(p, e, 21) == 0, "serialize case 10 n=%d", n); }
  { static const uint8_t d[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x8B,0x98,0xFE,0xF1,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13}; n = csb_can_serialize(0x18FEF100u, 1, 1, 0, d, 20, p, sizeof p); CHECK(n == 31 && memcmp(p, e, 31) == 0, "serialize case 11 n=%d", n); }
  { static const uint8_t d[] = {0}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x07,0xFF}; n = csb_can_serialize(0x7FFu, 0, 1, 0, d, 0, p, sizeof p); CHECK(n == 9 && memcmp(p, e, 9) == 0, "serialize case 12 n=%d", n); }
  { static const uint8_t d[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F}; static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0xCF,0x00,0x01,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F}; n = csb_can_serialize(0x1u, 0, 1, 1, d, 64, p, sizeof p); CHECK(n == 73 && memcmp(p, e, 73) == 0, "serialize case 13 n=%d", n); }
}
static void test_ws_build(void) { uint8_t f[300]; int32_t n;
  { static const uint8_t d[] = {0x01,0x02,0x03,0x04,0x05}; static const uint8_t e[] = {0x82,0x85,0x12,0x34,0x56,0x78,0x13,0x36,0x55,0x7C,0x17}; n = csb_ws_build_frame(d, 5, 2, 0x12345678u, f, sizeof f); CHECK(n == 11 && memcmp(f, e, 11) == 0, "ws build case 0"); }
  { static const uint8_t d[] = {0x03,0xE8}; static const uint8_t e[] = {0x88,0x82,0x12,0x34,0x56,0x78,0x11,0xDC}; n = csb_ws_build_frame(d, 2, 8, 0x12345678u, f, sizeof f); CHECK(n == 8 && memcmp(f, e, 8) == 0, "ws build case 1"); }
  { static const uint8_t d[] = {0}; static const uint8_t e[] = {0x8A,0x80,0x12,0x34,0x56,0x78}; n = csb_ws_build_frame(d, 0, 10, 0x12345678u, f, sizeof f); CHECK(n == 6 && memcmp(f, e, 6) == 0, "ws build case 2"); }
  { static const uint8_t d[] = {0xAA,0xBB}; static const uint8_t e[] = {0x8A,0x82,0x12,0x34,0x56,0x78,0xB8,0x8F}; n = csb_ws_build_frame(d, 2, 10, 0x12345678u, f, sizeof f); CHECK(n == 8 && memcmp(f, e, 8) == 0, "ws build case 3"); }
  { static uint8_t d[125]; static const uint8_t e[] = {0x82,0xFD,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12}; memset(d, 0, sizeof d); n = csb_ws_build_frame(d, 125, 2, 0x12345678u, f, sizeof f); CHECK(n == 131 && memcmp(f, e, 131) == 0, "ws build 125 bytes"); }
  { static uint8_t d[126]; static const uint8_t e[] = {0x82,0xFE,0x00,0x7E,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34}; memset(d, 0, sizeof d); n = csb_ws_build_frame(d, 126, 2, 0x12345678u, f, sizeof f); CHECK(n == 134 && memcmp(f, e, 134) == 0, "ws build 126 bytes"); }
  { static uint8_t d[200]; static const uint8_t e[] = {0x82,0xFE,0x00,0xC8,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78}; memset(d, 0, sizeof d); n = csb_ws_build_frame(d, 200, 2, 0x12345678u, f, sizeof f); CHECK(n == 208 && memcmp(f, e, 208) == 0, "ws build 200 bytes"); }
}
static void test_ws_parse(void) { int32_t nf;
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82,0x05,0x01,0x02,0x03,0x04,0x05}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 7, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 0");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 0 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0x01,0x02,0x03,0x04,0x05}; CHECK(got_ops[0] == 2 && got_lens[0] == 5 && memcmp(got + 0, e, 5) == 0, "ws parse 0 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82,0x05,0x01}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 3, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 1");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    { static const uint8_t in[] = {0x02,0x03,0x04,0x05}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 4, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 1");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 1 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0x01,0x02,0x03,0x04,0x05}; CHECK(got_ops[0] == 2 && got_lens[0] == 5 && memcmp(got + 0, e, 5) == 0, "ws parse 1 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x89,0x00}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 2, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 2");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 2 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0}; CHECK(got_ops[0] == 9 && got_lens[0] == 0 && memcmp(got + 0, e, 0) == 0, "ws parse 2 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82,0x02,0xAA,0xBB,0x89,0x00,0x82,0x01}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 8, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 3");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 2, "ws parse 3 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0xAA,0xBB}; CHECK(got_ops[0] == 2 && got_lens[0] == 2 && memcmp(got + 0, e, 2) == 0, "ws parse 3 frame 0"); }
    { static const uint8_t e[] = {0}; CHECK(got_ops[1] == 9 && got_lens[1] == 0 && memcmp(got + 2, e, 0) == 0, "ws parse 3 frame 1"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x88,0x02,0x03,0xE8}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 4, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 4");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 4 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0x03,0xE8}; CHECK(got_ops[0] == 8 && got_lens[0] == 2 && memcmp(got + 0, e, 2) == 0, "ws parse 4 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82,0x7E,0x00,0xC8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 204, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 5");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 5 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; CHECK(got_ops[0] == 2 && got_lens[0] == 200 && memcmp(got + 0, e, 200) == 0, "ws parse 5 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82,0x7E}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 2, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 6");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    { static const uint8_t in[] = {0x00,0xC8}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 2, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 6");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    { static const uint8_t in[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 200, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 6");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 1, "ws parse 6 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
    { static const uint8_t e[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; CHECK(got_ops[0] == 2 && got_lens[0] == 200 && memcmp(got + 0, e, 200) == 0, "ws parse 6 frame 0"); }
  }
  csb_ws_state_init(wstate, sizeof wstate); { int32_t total = 0; static uint8_t got_ops[64]; static int32_t got_lens[64]; static uint8_t got[4096]; int32_t gn = 0;
    { static const uint8_t in[] = {0x82}; CHECK(csb_ws_parse_frames(wstate, sizeof wstate, in, 1, ops, lens, 1<<14, out, sizeof out, &nf) == 0, "ws parse 7");
      int32_t o2 = 0; for (int32_t k = 0; k < nf; k++) { got_ops[total] = ops[k]; got_lens[total] = lens[k]; memcpy(got + gn, out + o2, (size_t)lens[k]); gn += lens[k]; o2 += lens[k]; total++; } }
    CHECK(total == 0, "ws parse 7 frames %d", total); (void)got_ops; (void)got_lens; (void)got; (void)gn;
  }
}
static void test_bit_timing(void) { uint16_t t[8]; double a, b; int32_t rc;
  rc = csb_bit_timing(500000u, 80, 2000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==2 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==2 && t[5]==15 && t[6]==4 && t[7]==4 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 0 rc=%d", rc);
  rc = csb_bit_timing(500000u, 80, 1000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==4 && t[1]==31 && t[2]==8 && t[3]==8 && t[4]==4 && t[5]==15 && t[6]==4 && t[7]==4 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 1 rc=%d", rc);
  rc = csb_bit_timing(250000u, 80, 2000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==4 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==4 && t[5]==7 && t[6]==2 && t[7]==2 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 2 rc=%d", rc);
  rc = csb_bit_timing(500000u, 87.5, 2000000u, 75, t, &a, &b); CHECK(rc == 0 && t[0]==2 && t[1]==69 && t[2]==10 && t[3]==10 && t[4]==2 && t[5]==14 && t[6]==5 && t[7]==5 && fabs(a-87.5)<1e-9 && fabs(b-75.0)<1e-9, "bit timing case 3 rc=%d", rc);
  rc = csb_bit_timing(500000u, 80, 4000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==2 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==2 && t[5]==7 && t[6]==2 && t[7]==2 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 4 rc=%d", rc);
  rc = csb_bit_timing(500000u, 80, 5000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==2 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==2 && t[5]==5 && t[6]==2 && t[7]==2 && fabs(a-80.0)<1e-9 && fabs(b-75.0)<1e-9, "bit timing case 5 rc=%d", rc);
  rc = csb_bit_timing(1000000u, 80, 5000000u, 75, t, &a, &b); CHECK(rc == 0 && t[0]==1 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==1 && t[5]==11 && t[6]==4 && t[7]==4 && fabs(a-80.0)<1e-9 && fabs(b-75.0)<1e-9, "bit timing case 6 rc=%d", rc);
  rc = csb_bit_timing(1000000u, 80, 8000000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==1 && t[1]==63 && t[2]==16 && t[3]==16 && t[4]==1 && t[5]==7 && t[6]==2 && t[7]==2 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 7 rc=%d", rc);
  rc = csb_bit_timing(125000u, 87.5, 500000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==8 && t[1]==69 && t[2]==10 && t[3]==10 && t[4]==8 && t[5]==15 && t[6]==4 && t[7]==4 && fabs(a-87.5)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 8 rc=%d", rc);
  rc = csb_bit_timing(250000u, 80, 250000u, 80, t, &a, &b); CHECK(rc == 0 && t[0]==16 && t[1]==15 && t[2]==4 && t[3]==4 && t[4]==16 && t[5]==15 && t[6]==4 && t[7]==4 && fabs(a-80.0)<1e-9 && fabs(b-80.0)<1e-9, "bit timing case 9 rc=%d", rc);
  rc = csb_bit_timing(500000u, 80, 250000u, 80, t, &a, &b); CHECK(rc == -2, "bit timing case 10 rc=%d", rc);
  rc = csb_bit_timing(500000u, 40, 2000000u, 80, t, &a, &b); CHECK(rc == -2, "bit timing case 11 rc=%d", rc);
  rc = csb_bit_timing(125000u, 80, 5000000u, 80, t, &a, &b); CHECK(rc == -3, "bit timing case 12 rc=%d", rc);
}
static uint32_t rng = 12345u;
static uint32_t rnd(void) { rng = rng * 1103515245u + 12345u; return (rng >> 8) & 0xFFFFFFu; }

static void test_helpers(void)
{
    /* 300 random frames, each encoded with csb_encode_can_to_ws; the byte stream is fed to
     * csb_decode_ws_to_can in random-size chunks, with a ping and a close frame inserted. */
    static uint8_t stream[300 * 200 + 64]; int32_t sn = 0;
    static uint32_t eid[300]; static uint8_t eide[300], efdf[300], ebrs[300], elen[300], edat[300][64];
    for (int i = 0; i < 300; i++) {
        efdf[i] = (uint8_t)(rnd() % 2); ebrs[i] = (uint8_t)(efdf[i] && (rnd() % 2)); eide[i] = (uint8_t)(rnd() % 3 == 0);
        eid[i] = eide[i] ? rnd() % 0x20000000u : rnd() % 0x800u;
        elen[i] = (uint8_t)(rnd() % (efdf[i] ? 65u : 9u));
        for (int k = 0; k < elen[i]; k++) edat[i][k] = (uint8_t)rnd();
        int32_t n = csb_encode_can_to_ws(eid[i], eide[i], efdf[i], ebrs[i], edat[i], elen[i], 0u, stream + sn, (int32_t)sizeof stream - sn);
        CHECK(n > 0, "encode %d", i);
        /* server frames are not masked: turn the masked client frame into an unmasked one */
        uint8_t *f = stream + sn; int32_t h = (f[1] & 0x7F) == 126 ? 4 : 2;
        f[1] &= 0x7F; memmove(f + h, f + h + 4, (size_t)(n - h - 4)); n -= 4;
        sn += n;
        if (i == 150) { stream[sn++] = 0x89; stream[sn++] = 0x02; stream[sn++] = 0xAB; stream[sn++] = 0xCD; }  /* ping */
    }
    stream[sn++] = 0x88; stream[sn++] = 0x02; stream[sn++] = 0x03; stream[sn++] = 0xE8;  /* close */
    csb_ws_state_init(wstate2, sizeof wstate2); csb_hdlc_state_init(hstate, sizeof hstate);
    int32_t pos = 0, total = 0, nf, pc, pl, de, got_close = 0, got_ping = -1; uint32_t ce; uint8_t ping[125];
    while (pos < sn) {
        int32_t chunk = (int32_t)(rnd() % 700u) + 1; if (chunk > sn - pos) chunk = sn - pos;
        int32_t rc = csb_decode_ws_to_can(wstate2, sizeof wstate2, hstate, sizeof hstate, stream + pos, chunk,
                                          t_dev + total, ids + total, fl + total, dlcs + total, dlen + total,
                                          data + total * 64, 8192 - total, &nf, &pc, ping, &pl, &ce, &de);
        CHECK(rc == 0, "decode_ws_to_can rc=%d", rc);
        total += nf; pos += chunk; if (pc) got_close = 1; if (pl >= 0) got_ping = pl;
        CHECK(de == 0 && ce == 0, "stream errors de=%d ce=%u", de, ce);
    }
    CHECK(total == 300, "stream frames %d", total);
    CHECK(got_close == 1 && got_ping == 2 && ping[0] == 0xAB && ping[1] == 0xCD, "control frames");
    int bad = 0;
    for (int i = 0; i < 300 && i < total; i++) {
        int fd = (fl[i] & CSB_F_FDF) != 0, br = (fl[i] & CSB_F_BRS) != 0, ex = (fl[i] & CSB_F_IDE) != 0;
        if (ids[i] != eid[i] || fd != efdf[i] || br != ebrs[i] || ex != eide[i] || dlen[i] < elen[i] ||
            memcmp(data + i * 64, edat[i], elen[i]) != 0) bad++;
        for (int k = elen[i]; k < dlen[i]; k++) if (data[i * 64 + k] != 0) bad++;
    }
    CHECK(bad == 0, "stream round trip mismatches %d", bad);
    uint8_t small[10];
    CHECK(csb_hdlc_encode(small, 10, small, 10) == CSB_ERR_BUFFER_TOO_SMALL, "buffer check");
    CHECK(csb_hdlc_decode(small, 10, small, 0, out, sizeof out, lens, 10, &nf, &ce) == CSB_ERR_INVALID, "state size check");
    memset(hstate, 0, sizeof hstate);
    CHECK(csb_hdlc_decode(hstate, sizeof hstate, small, 0, out, sizeof out, lens, 10, &nf, &ce) == CSB_ERR_STATE, "uninitialised state");
    csb_hdlc_state_init(hstate, sizeof hstate);
    csb_ws_state_init(wstate, sizeof wstate);
    CHECK(csb_ws_parse_frames(wstate, sizeof wstate, small, 0, ops, lens, 1 << 14, out, sizeof out, &nf) == 0, "ws mode 1");
    CHECK(csb_decode_ws_to_can(wstate, sizeof wstate, hstate, sizeof hstate, small, 0, t_dev, ids, fl, dlcs, dlen, data, 8192, &nf, &pc, ping, &pl, &ce, &de) == CSB_ERR_STATE, "ws state used by both APIs is refused");
    CHECK(csb_version() == 0x00010300u && csb_hdlc_state_size() == CSB_HDLC_STATE_SIZE && csb_ws_state_size() == CSB_WS_STATE_SIZE, "info");
}

static void test_rt_helpers(void)
{
    /* csb_warmup: 2 frames, states fresh afterwards */
    CHECK(csb_warmup(wstate2, sizeof wstate2, hstate, sizeof hstate) == 2, "warmup frames");
    int32_t nf, pc, pl, de; uint32_t ce; uint8_t ping[125];
    CHECK(csb_decode_ws_to_can(wstate2, sizeof wstate2, hstate, sizeof hstate, ping, 0, t_dev, ids, fl, dlcs, dlen, data, 8192, &nf, &pc, ping, &pl, &ce, &de) == 0 && nf == 0 && ce == 0, "states fresh after warmup");
    /* csb_encode_rx_frame_to_ws == csb_encode_can_to_ws of the same frame, for 300 random frames */
    int bad = 0;
    for (int i = 0; i < 300; i++) {
        uint8_t fdf = (uint8_t)(rnd() % 2), brs = (uint8_t)(fdf && (rnd() % 2)), ide = (uint8_t)(rnd() % 3 == 0);
        uint32_t id = ide ? rnd() % 0x20000000u : rnd() % 0x800u; int n = (int)(rnd() % (fdf ? 65u : 9u));
        uint8_t d[64]; for (int k = 0; k < n; k++) d[k] = (uint8_t)rnd();
        uint8_t p[80]; int32_t pn = csb_can_serialize(id, ide, fdf, brs, d, n, p, sizeof p);
        int32_t df;
        CHECK(csb_can_deserialize(p, pn, t_dev, ids, fl, dlcs, dlen, data, 8192, &nf, &df) == 0 && nf == 1, "deser for echo");
        uint32_t nid = (id + 5u) & (ide ? 0x1FFFFFFFu : 0x7FFu);
        uint8_t a[200], b[200];
        int32_t la = csb_encode_rx_frame_to_ws(nid, 0, nf, fl, dlen, data, 0x12345678u, a, sizeof a);
        int32_t lb = csb_encode_can_to_ws(nid, ide, fdf, brs, data, dlen[0], 0x12345678u, b, sizeof b);
        if (la <= 0 || la != lb || memcmp(a, b, (size_t)la) != 0) bad++;
    }
    CHECK(bad == 0, "encode_rx_frame_to_ws mismatches %d", bad);
    uint8_t o[200];
    CHECK(csb_encode_rx_frame_to_ws(1u, 1, 1, fl, dlen, data, 0u, o, sizeof o) == CSB_ERR_INVALID, "index >= n_frames refused");
    fl[0] = CSB_F_ERROR;
    CHECK(csb_encode_rx_frame_to_ws(1u, 0, 1, fl, dlen, data, 0u, o, sizeof o) == CSB_ERR_INVALID, "error frame refused");
}

static void test_cif(void)
{
    /* reference bytes from gen/cif_ref.py (a Python copy of NI's Build Socket CAN.vi) */
    static const uint8_t exp3[] = {0x67,0x74,0x85,0xA6,0x1D,0xCD,0x65,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x01,0x23,0x08,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x67,0x74,0x85,0xA7,0x00,0x00,0x03,0xE8,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x14,0x98,0xFE,0xF1,0x00,0x0C,0x05,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x67,0x74,0x85,0xA8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x08,0x40,0x00,0x07,0xFF,0x00,0x00,0x00,0x00};
    static const uint8_t exp_le[] = {0xA6,0x85,0x74,0x67,0x00,0x65,0xCD,0x1D,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x23,0x08,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    static uint8_t o[8 * CSB_PCAP_MAX_PACKET]; int32_t np, nsk;
    /* 5 frames: classic, FD BRS ext, remote, a TX ack, an ACK error frame */
    double td[5] = {1735689601.5, 1735689602.000001, 1735689603.0, 1735689604.0, 1735689605.0};
    uint32_t id5[5] = {0x123u, 0x18FEF100u, 0x7FFu, 0x55u, 0u};
    uint16_t f5[5] = {0, CSB_F_IDE | CSB_F_FDF | CSB_F_BRS, CSB_F_RTR, CSB_F_TX_ACK, (uint16_t)(CSB_F_ERROR | (1u << CSB_F_ERRTYPE_SHIFT))};
    uint8_t dl5[5] = {8, 12, 0, 1, 0};
    static uint8_t d5[5 * 64];
    memset(d5, 0, sizeof d5);
    for (int k = 0; k < 8; k++) d5[k] = (uint8_t)(k + 1);
    for (int k = 0; k < 12; k++) d5[64 + k] = (uint8_t)k;
    d5[3 * 64] = 0xAA;
    int32_t n = csb_frames_to_pcap(5, td, id5, f5, dl5, d5, CSB_TAI_MINUS_UTC, 0, o, sizeof o, &np, &nsk);
    CHECK(n == (int32_t)sizeof exp3 && np == 3 && nsk == 2 && memcmp(o, exp3, sizeof exp3) == 0, "cif default n=%d np=%d nsk=%d", n, np, nsk);
    n = csb_frames_to_pcap(1, td, id5, f5, dl5, d5, CSB_TAI_MINUS_UTC, CSB_PCAP_LITTLE_ENDIAN, o, sizeof o, &np, &nsk);
    CHECK(n == (int32_t)sizeof exp_le && memcmp(o, exp_le, sizeof exp_le) == 0, "cif little-endian header");
    n = csb_frames_to_pcap(5, td, id5, f5, dl5, d5, CSB_TAI_MINUS_UTC, CSB_PCAP_KEEP_TX_ACK | CSB_PCAP_KEEP_ERRORS, o, sizeof o, &np, &nsk);
    CHECK(np == 5 && nsk == 0, "cif keep all np=%d", np);
    /* packet 4 = TX ack id 0x55, 1 byte AA; packet 5 = error frame CAN_ERR_FLAG|CAN_ERR_ACK, 8 bytes */
    int32_t off4 = (int32_t)sizeof exp3, off5 = off4 + 25;
    CHECK(o[off4 + 16] == 0x00 && o[off4 + 19] == 0x55 && o[off4 + 20] == 1 && o[off4 + 24] == 0xAA, "cif tx ack packet");
    CHECK(n == off5 + 32 && o[off5 + 16] == 0x20 && o[off5 + 19] == 0x20 && o[off5 + 20] == 8, "cif error packet");
    CHECK(csb_frames_to_pcap(5, td, id5, f5, dl5, d5, 37, 0, o, 4 * CSB_PCAP_MAX_PACKET, &np, &nsk) == CSB_ERR_BUFFER_TOO_SMALL, "cif capacity check");
    CHECK(csb_pcap_max_bytes(1756) == 1756 * 88, "cif max bytes");

    /* round trip: 300 random frames -> payload -> decode arrays -> CIF -> WebSocket; must equal
     * csb_encode_can_to_ws of the same frames. The CIF stream is fed in random pieces. */
    static uint8_t cif[300 * CSB_PCAP_MAX_PACKET], ws_a[300 * 174], ws_b[300 * 174];
    int32_t total = 0, wa = 0;
    for (int i = 0; i < 300; i++) {
        uint8_t fdf = (uint8_t)(rnd() % 2), brs = (uint8_t)(fdf && (rnd() % 2)), ide = (uint8_t)(rnd() % 3 == 0);
        uint32_t cid = ide ? rnd() % 0x20000000u : rnd() % 0x800u; int k = (int)(rnd() % (fdf ? 65u : 9u));
        uint8_t d[64]; for (int j = 0; j < k; j++) d[j] = (uint8_t)rnd();
        uint8_t p[80]; int32_t pn = csb_can_serialize(cid, ide, fdf, brs, d, k, p, sizeof p);
        int32_t nf, de;
        CHECK(csb_can_deserialize(p, pn, t_dev + total, ids + total, fl + total, dlcs + total, dlen + total, data + total * 64, 8192 - total, &nf, &de) == 0 && nf == 1, "cif deser");
        total += nf;
        int32_t m = csb_encode_can_to_ws(cid, ide, fdf, brs, data + (total - 1) * 64, dlen[total - 1], 0x12345678u, ws_a + wa, 174);
        CHECK(m > 0, "cif ref encode"); wa += m;
    }
    int32_t cn = csb_frames_to_pcap(total, t_dev, ids, fl, dlen, data, 37, 0, cif, sizeof cif, &np, &nsk);
    CHECK(cn > 0 && np == 300 && nsk == 0, "cif stream np=%d", np);
    int32_t pos = 0, wb = 0, enc = 0, rej = 0;
    while (pos < cn) {
        int32_t chunk = (int32_t)(rnd() % 400u) + 1; if (chunk > cn - pos) chunk = cn - pos;
        int32_t used, ep, er;
        int32_t r = csb_encode_pcap_to_ws(cif + pos, chunk, 0, 0x12345678u, ws_b + wb, (int32_t)sizeof ws_b - wb, &ep, &er, &used);
        CHECK(r >= 0, "cif encode rc=%d", r);
        if (r < 0) break;
        wb += r; enc += ep; rej += er; pos += used;
        if (used == 0 && chunk == cn - pos) break;               /* cannot happen: last packet complete */
        if (used == 0) { /* piece shorter than one packet: try a longer piece next time */ }
    }
    CHECK(pos == cn && enc == 300 && rej == 0 && wb == wa && memcmp(ws_a, ws_b, (size_t)wa) == 0, "cif round trip enc=%d rej=%d", enc, rej);
    /* rejected and corrupt packets */
    int32_t used, ep, er;
    static const uint8_t remote[] = {0x67,0x74,0x85,0xA8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x08,0x40,0x00,0x07,0xFF,0x00,0x00,0x00,0x00};
    CHECK(csb_encode_pcap_to_ws(remote, (int32_t)sizeof remote, 0, 0u, ws_b, sizeof ws_b, &ep, &er, &used) == 0 && ep == 0 && er == 1 && used == (int32_t)sizeof remote, "cif remote rejected");
    uint8_t bad[32]; memcpy(bad, exp3, 32); bad[11] = 200;     /* incl_len 200 */
    CHECK(csb_encode_pcap_to_ws(bad, 32, 0, 0u, ws_b, sizeof ws_b, &ep, &er, &used) == CSB_ERR_INVALID && used == 0, "cif corrupt stream");
    CHECK(csb_encode_pcap_to_ws(exp3, 20, 0, 0u, ws_b, sizeof ws_b, &ep, &er, &used) == 0 && used == 0 && ep == 0, "cif incomplete kept");
}

static void test_pcap_range(void)
{
    /* 1.3.0: csb_frames_to_pcap_range. Reference = csb_frames_to_pcap (checked against NI above). */
    enum { N = 400 };
    static double td[N]; static uint32_t id[N]; static uint16_t f[N]; static uint8_t dl[N]; static uint8_t d[N * 64];
    static uint8_t ref[N * CSB_PCAP_MAX_PACKET], got[N * CSB_PCAP_MAX_PACKET], one[N * CSB_PCAP_MAX_PACKET];
    for (int i = 0; i < N; i++) {
        td[i] = 1735689600.0 + (double)(rnd() % 100000000u) / 1e6;
        uint32_t r = rnd() % 10u;
        f[i] = 0;
        if (r == 0) f[i] = CSB_F_TX_ACK;
        else if (r == 1) f[i] = (uint16_t)(CSB_F_ERROR | ((rnd() % 5u) << CSB_F_ERRTYPE_SHIFT));
        else if (r == 2) f[i] = CSB_F_RTR;
        else if (r <= 5) f[i] = (uint16_t)(CSB_F_FDF | ((rnd() & 1u) ? CSB_F_BRS : 0));
        if (!(f[i] & CSB_F_ERROR) && (rnd() % 3u == 0)) f[i] |= CSB_F_IDE;
        id[i] = (f[i] & CSB_F_ERROR) ? 0u : (rnd() % 16u);          /* few IDs: the lists match often */
        dl[i] = (uint8_t)((f[i] & CSB_F_FDF) ? (rnd() % 65u) : (rnd() % 9u));
        for (int k = 0; k < 64; k++) d[i * 64 + k] = (uint8_t)rnd();
    }
    int32_t np, nsk, nfl, next;
    /* 1. whole range, no lists: identical to csb_frames_to_pcap, for each option value */
    for (int32_t opt = 0; opt < 8; opt++) {
        int32_t rn = csb_frames_to_pcap(N, td, id, f, dl, d, 37, opt, ref, sizeof ref, &np, &nsk);
        int32_t np2, nsk2;
        int32_t gn = csb_frames_to_pcap_range(N, 0, N, td, id, f, dl, d, NULL, 0, NULL, 0, 37, opt, got, sizeof got, &np2, &nfl, &nsk2, &next);
        CHECK(gn == rn && np2 == np && nsk2 == nsk && nfl == 0 && next == N && memcmp(ref, got, (size_t)rn) == 0, "range whole opt=%d gn=%d rn=%d", opt, gn, rn);
    }
    /* 2. in pieces of random size: the concatenation is identical, counters add up */
    for (int rep = 0; rep < 20; rep++) {
        int32_t opt = (int32_t)(rnd() % 4u);
        int32_t rn = csb_frames_to_pcap(N, td, id, f, dl, d, 37, opt, ref, sizeof ref, &np, &nsk);
        int32_t first = 0, w = 0, tp = 0, ts = 0, calls = 0;
        while (first < N && calls < 10 * N) {
            int32_t maxp = (int32_t)(rnd() % 40u);                    /* 0 is permitted */
            int32_t np2, nsk2;
            int32_t gn = csb_frames_to_pcap_range(N, first, maxp, td, id, f, dl, d, NULL, 0, NULL, 0, 37, opt, got + w, maxp * CSB_PCAP_MAX_PACKET, &np2, &nfl, &nsk2, &next);
            CHECK(gn >= 0 && np2 <= maxp && next >= first && next <= N && nfl == 0, "range piece gn=%d np=%d next=%d", gn, np2, next);
            if (gn < 0) break;
            if (maxp == 0) CHECK(gn == 0 && np2 == 0 && next == first, "range max 0");
            w += gn; tp += np2; ts += nsk2; first = next; calls++;
        }
        CHECK(first == N && w == rn && tp == np && ts == nsk && memcmp(ref, got, (size_t)rn) == 0, "range pieces rep=%d w=%d rn=%d", rep, w, rn);
    }
    /* 3. ID lists: compare with a direct model (one frame for each call) */
    for (int rep = 0; rep < 40; rep++) {
        uint32_t allow[6], deny[6]; int32_t na = (int32_t)(rnd() % 7u), nd = (int32_t)(rnd() % 7u);
        if (rep == 0) { na = 0; nd = 0; }
        if (rep == 1) { na = 3; nd = 0; }
        if (rep == 2) { na = 0; nd = 3; }
        for (int k = 0; k < na; k++) allow[k] = rnd() % 16u;
        for (int k = 0; k < nd; k++) deny[k] = rnd() % 16u;
        if (rep == 3 && na > 0) allow[0] |= 0x80000000u;            /* flag bits in a list entry are ignored */
        int32_t opt = (int32_t)(rnd() % 4u);
        int32_t w = 0, mp = 0, mf = 0, ms = 0;
        for (int i = 0; i < N; i++) {
            int32_t np1, ns1;
            int32_t n1 = csb_frames_to_pcap(1, td + i, id + i, f + i, dl + i, d + i * 64, 37, opt, one + w, CSB_PCAP_MAX_PACKET, &np1, &ns1);
            if (np1 == 0) { ms += ns1; continue; }
            uint32_t v = (((uint32_t)one[w + 16] << 24) | ((uint32_t)one[w + 17] << 16) | ((uint32_t)one[w + 18] << 8) | one[w + 19]) & 0x1FFFFFFFu;
            int pass = 1;
            if (na > 0) { pass = 0; for (int k = 0; k < na; k++) if ((allow[k] & 0x1FFFFFFFu) == v) pass = 1; }
            for (int k = 0; k < nd; k++) if ((deny[k] & 0x1FFFFFFFu) == v) pass = 0;
            if (pass) { w += n1; mp++; } else mf++;
        }
        int32_t gn = csb_frames_to_pcap_range(N, 0, N, td, id, f, dl, d, na ? allow : NULL, na, nd ? deny : NULL, nd, 37, opt, got, sizeof got, &np, &nfl, &nsk, &next);
        CHECK(gn == w && np == mp && nfl == mf && nsk == ms && next == N && memcmp(one, got, (size_t)w) == 0, "range lists rep=%d gn=%d w=%d np=%d/%d nfl=%d/%d", rep, gn, w, np, mp, nfl, mf);
        CHECK(np + nfl + nsk == N, "range lists count rep=%d", rep);
    }
    /* 4. the packet limit with lists: 3 packets for each call */
    {
        uint32_t deny[1] = {5u};
        int32_t rn = csb_frames_to_pcap_range(N, 0, N, td, id, f, dl, d, NULL, 0, deny, 1, 37, 0, ref, sizeof ref, &np, &nfl, &nsk, &next);
        int32_t first = 0, w = 0, tp = 0, tf = 0, ts = 0;
        while (first < N) {
            int32_t np2, nf2, ns2;
            int32_t gn = csb_frames_to_pcap_range(N, first, 3, td, id, f, dl, d, NULL, 0, deny, 1, 37, 0, got + w, 3 * CSB_PCAP_MAX_PACKET, &np2, &nf2, &ns2, &next);
            CHECK(gn >= 0 && np2 <= 3 && next > first, "range limit 3");
            if (gn < 0 || next <= first) break;
            w += gn; tp += np2; tf += nf2; ts += ns2; first = next;
        }
        CHECK(w == rn && tp == np && tf == nfl && ts == nsk && memcmp(ref, got, (size_t)rn) == 0, "range limit 3 total");
    }
    /* 5. arguments and capacity */
    CHECK(csb_frames_to_pcap_range(N, N, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, 0, &np, &nfl, &nsk, &next) == 0 && np == 0 && next == N, "range first == n_frames");
    CHECK(csb_frames_to_pcap_range(0, 0, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, 0, &np, &nfl, &nsk, &next) == 0 && next == 0, "range no frames");
    CHECK(csb_frames_to_pcap_range(N, N + 1, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, sizeof got, &np, &nfl, &nsk, &next) == CSB_ERR_INVALID, "range first > n_frames");
    CHECK(csb_frames_to_pcap_range(N, -1, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, sizeof got, &np, &nfl, &nsk, &next) == CSB_ERR_INVALID, "range first < 0");
    CHECK(csb_frames_to_pcap_range(N, 0, -1, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, sizeof got, &np, &nfl, &nsk, &next) == CSB_ERR_INVALID, "range max < 0");
    CHECK(csb_frames_to_pcap_range(N, 0, 10, td, id, f, dl, d, NULL, 2, NULL, 0, 37, 0, got, sizeof got, &np, &nfl, &nsk, &next) == CSB_ERR_INVALID, "range allow NULL with count");
    CHECK(csb_frames_to_pcap_range(N, 0, 10, td, id, f, dl, d, NULL, 0, NULL, -1, 37, 0, got, sizeof got, &np, &nfl, &nsk, &next) == CSB_ERR_INVALID, "range deny count < 0");
    CHECK(csb_frames_to_pcap_range(N, 0, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, 10 * CSB_PCAP_MAX_PACKET - 1, &np, &nfl, &nsk, &next) == CSB_ERR_BUFFER_TOO_SMALL, "range capacity for max_packets");
    CHECK(csb_frames_to_pcap_range(N, N - 4, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, 4 * CSB_PCAP_MAX_PACKET, &np, &nfl, &nsk, &next) >= 0, "range capacity for the remaining frames");
    CHECK(csb_frames_to_pcap_range(N, N - 4, 10, td, id, f, dl, d, NULL, 0, NULL, 0, 37, 0, got, 4 * CSB_PCAP_MAX_PACKET - 1, &np, &nfl, &nsk, &next) == CSB_ERR_BUFFER_TOO_SMALL, "range capacity for the remaining frames - 1");
}

int main(void)
{
    test_crc(); test_hdlc_encode(); test_tv(); test_serialize(); test_ws_build(); test_ws_parse();
    test_bit_timing(); test_helpers(); test_rt_helpers(); test_cif(); test_pcap_range();
    printf("%d checks, %d failures\n%s\n", checks, fails, fails ? "FAILED" : "ALL PASS");
    return fails ? 1 : 0;
}
