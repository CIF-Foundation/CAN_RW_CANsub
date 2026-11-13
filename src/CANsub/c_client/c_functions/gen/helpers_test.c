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
    CHECK(csb_version() == 0x00010200u && csb_hdlc_state_size() == CSB_HDLC_STATE_SIZE && csb_ws_state_size() == CSB_WS_STATE_SIZE, "info");
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

int main(void)
{
    test_crc(); test_hdlc_encode(); test_tv(); test_serialize(); test_ws_build(); test_ws_parse();
    test_bit_timing(); test_helpers(); test_rt_helpers(); test_cif();
    printf("%d checks, %d failures\n%s\n", checks, fails, fails ? "FAILED" : "ALL PASS");
    return fails ? 1 : 0;
}
