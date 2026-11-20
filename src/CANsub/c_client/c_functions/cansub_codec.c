/*
 * cansub_codec.c - implementation, see cansub_codec.h.
 * Build: make (gcc, C99). No malloc, no globals except the constant CRC table.
 */
#include "cansub_codec.h"

#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ state layouts */
/* State structs live inside caller-owned U8 arrays (LabVIEW array data pointers), which may
 * not be 8-byte aligned: packed structs make gcc use alignment-safe accesses. */
#define HDLC_MAGIC 0x484C4443u /* "HDLC" */
#define WS_MAGIC   0x57534B54u /* "WSKT" */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t len;          /* bytes in buf */
    uint8_t  in_frame;
    uint8_t  escape;
    uint8_t  overflow;     /* current frame longer than CSB_HDLC_MAX_FRAME: drop it */
    uint8_t  reserved;
    uint32_t crc_errors;   /* cumulative */
    uint8_t  buf[CSB_HDLC_MAX_FRAME];
} hdlc_state_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t remaining;    /* payload bytes still to come for the current frame */
    uint32_t pos;          /* payload bytes received for the current frame */
    uint32_t total;        /* payload length of the current frame */
    uint8_t  in_payload;   /* 0 = collecting header, 1 = collecting payload */
    uint8_t  hdr_len;
    uint8_t  need;         /* header bytes needed (2, then 4/10, +4 if masked) */
    uint8_t  opcode;
    uint8_t  masked;
    uint8_t  mode;         /* 0 = unused, 1 = buffered (csb_ws_parse_frames), 2 = streaming */
    uint8_t  reserved[2];
    uint8_t  hdr[14];
    uint8_t  reserved2[10];     /* pad the header part to 48 bytes */
    uint8_t  buf[CSB_WS_MAX_BUFFER];  /* buffered mode: payload of the current frame;
                                         streaming mode: control-frame payload (<= 125) */
} ws_state_t;

_Static_assert(sizeof(hdlc_state_t) == CSB_HDLC_STATE_SIZE, "HDLC state size");
_Static_assert(sizeof(ws_state_t) == CSB_WS_STATE_SIZE, "WS state size");

/* ------------------------------------------------------------------ CRC32 table */
static const uint32_t CRC_TABLE[256] = {
#include "crc32_table.inc"
};

/* DLC code -> number of data bytes */
static const uint8_t DLC_LEN[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

#define F_CLOCK 80000000.0
#define EPOCH_2025 1735689600.0

/* ------------------------------------------------------------------ info */
uint32_t csb_version(void) { return CSB_VERSION; }
int32_t csb_hdlc_state_size(void) { return CSB_HDLC_STATE_SIZE; }
int32_t csb_ws_state_size(void) { return CSB_WS_STATE_SIZE; }

/* ------------------------------------------------------------------ 1.1 CRC32 */
static uint32_t crc_update(uint32_t crc, const uint8_t *d, int32_t n)
{
    for (int32_t i = 0; i < n; i++)
        crc = (crc >> 8) ^ CRC_TABLE[(crc ^ d[i]) & 0xFFu];
    return crc;
}

uint32_t csb_crc32(const uint8_t *data, int32_t len)
{
    if (len <= 0 || data == NULL)
        return 0u; /* CRC32 of an empty array is 0 */
    return crc_update(0xFFFFFFFFu, data, len) ^ 0xFFFFFFFFu;
}

/* ------------------------------------------------------------------ 1.2 HDLC encode */
static int32_t stuff_byte(uint8_t b, uint8_t *out, int32_t n)
{
    if (b == 0x7E) { out[n++] = 0x7D; out[n++] = 0x5E; }
    else if (b == 0x7D) { out[n++] = 0x7D; out[n++] = 0x5D; }
    else out[n++] = b;
    return n;
}

int32_t csb_hdlc_encode(const uint8_t *payload, int32_t len, uint8_t *frame, int32_t frame_cap)
{
    if (len < 0 || (len > 0 && payload == NULL) || frame == NULL)
        return CSB_ERR_INVALID;
    if (frame_cap < 2 * (len + 4) + 2)
        return CSB_ERR_BUFFER_TOO_SMALL;
    uint32_t crc = csb_crc32(payload, len);
    int32_t n = 0;
    frame[n++] = 0x7E;
    for (int32_t i = 0; i < len; i++)
        n = stuff_byte(payload[i], frame, n);
    for (int s = 24; s >= 0; s -= 8)
        n = stuff_byte((uint8_t)(crc >> s), frame, n);
    frame[n++] = 0x7E;
    return n;
}

/* ------------------------------------------------------------------ 1.3 HDLC decode (core) */
/* Sink for complete payloads. */
typedef struct {
    int (*on_payload)(void *ctx, const uint8_t *p, int32_t n);
    void *ctx;
} payload_sink_t;

static int32_t check_hdlc_state(uint8_t *state, int32_t size)
{
    if (state == NULL || size < CSB_HDLC_STATE_SIZE) return CSB_ERR_INVALID;
    hdlc_state_t *st = (hdlc_state_t *)state;
    if (st->magic != HDLC_MAGIC || st->len > CSB_HDLC_MAX_FRAME) return CSB_ERR_STATE;
    return CSB_OK;
}

int32_t csb_hdlc_state_init(uint8_t *state, int32_t state_size)
{
    if (state == NULL || state_size < CSB_HDLC_STATE_SIZE) return CSB_ERR_INVALID;
    memset(state, 0, CSB_HDLC_STATE_SIZE);
    ((hdlc_state_t *)state)->magic = HDLC_MAGIC;
    return CSB_OK;
}

/* One byte of the HDLC stream. Same logic as HDLC_Decode.vi (build doc 1.3), plus the
 * overflow guard. */
static void hdlc_feed(hdlc_state_t *st, uint8_t b, payload_sink_t *sink)
{
    if (b == 0x7E) {
        if (st->overflow) {
            st->crc_errors++;
        } else if (st->in_frame && st->len >= 5) {
            uint32_t n = st->len - 4;
            uint32_t rx = ((uint32_t)st->buf[n] << 24) | ((uint32_t)st->buf[n + 1] << 16) |
                          ((uint32_t)st->buf[n + 2] << 8) | st->buf[n + 3];
            if (csb_crc32(st->buf, (int32_t)n) == rx)
                sink->on_payload(sink->ctx, st->buf, (int32_t)n);
            else
                st->crc_errors++;
        }
        st->len = 0; st->in_frame = 1; st->escape = 0; st->overflow = 0;
    } else if (b == 0x7D) {
        st->escape = 1;
    } else {
        uint8_t v = st->escape ? (uint8_t)(b ^ 0x20) : b;
        st->escape = 0;
        if (st->overflow) return;
        if (st->len >= CSB_HDLC_MAX_FRAME) { st->overflow = 1; st->len = 0; return; }
        st->buf[st->len++] = v;
    }
}

typedef struct {
    uint8_t *out; int32_t out_cap; int32_t out_n;
    int32_t *lens; int32_t lens_cap; int32_t count;
} hdlc_out_ctx_t;

static int hdlc_out_payload(void *vctx, const uint8_t *p, int32_t n)
{
    hdlc_out_ctx_t *c = (hdlc_out_ctx_t *)vctx;
    /* capacities are checked before decoding; this is a second guard */
    if (c->count >= c->lens_cap || c->out_n + n > c->out_cap) return -1;
    memcpy(c->out + c->out_n, p, (size_t)n);
    c->out_n += n;
    c->lens[c->count++] = n;
    return 0;
}

int32_t csb_hdlc_decode(uint8_t *state, int32_t state_size,
                        const uint8_t *in, int32_t in_len,
                        uint8_t *out, int32_t out_cap,
                        int32_t *lens, int32_t lens_cap,
                        int32_t *n_payloads, uint32_t *crc_errors)
{
    int32_t rc = check_hdlc_state(state, state_size);
    if (rc != CSB_OK) return rc;
    if (in_len < 0 || (in_len > 0 && in == NULL) || out == NULL || lens == NULL ||
        n_payloads == NULL || crc_errors == NULL)
        return CSB_ERR_INVALID;
    if (out_cap < in_len + CSB_HDLC_MAX_FRAME || lens_cap < in_len / 6 + 1)
        return CSB_ERR_BUFFER_TOO_SMALL;
    hdlc_state_t *st = (hdlc_state_t *)state;
    hdlc_out_ctx_t ctx = {out, out_cap, 0, lens, lens_cap, 0};
    payload_sink_t sink = {hdlc_out_payload, &ctx};
    for (int32_t i = 0; i < in_len; i++)
        hdlc_feed(st, in[i], &sink);
    *n_payloads = ctx.count;
    *crc_errors = st->crc_errors;
    return CSB_OK;
}

/* ------------------------------------------------------------------ 1.4 CAN deserialize */
typedef struct {
    double *t_dev; uint32_t *id; uint16_t *flags; uint8_t *dlc; uint8_t *data_len; uint8_t *data;
    int32_t max_frames; int32_t n; int32_t decode_errors; int32_t overflow;
} can_out_t;

static uint8_t byte_at(const uint8_t *p, int32_t len, int32_t i) { return (i >= 0 && i < len) ? p[i] : 0; }

static void can_deserialize_into(const uint8_t *p, int32_t len, can_out_t *o)
{
    int32_t off = 0;
    if (len <= 0) return;
    for (;;) {
        int32_t rem = len - off;
        uint64_t us = 0;
        for (int k = 0; k < 6; k++) us = (us << 8) | byte_at(p, len, off + k);
        uint8_t f = byte_at(p, len, off + 6);
        int fdf = (f & 0x80) != 0, b6 = (f & 0x40) != 0, b5 = (f & 0x20) != 0, tx = (f & 0x10) != 0;
        uint8_t code = f & 0x0F;
        int32_t hl, n, flen;
        uint32_t cid = 0; uint16_t fl = 0;
        if (b5 && !fdf) {                      /* error frame: 7 bytes, no ID, no data */
            flen = 7; n = 0; hl = 7;
            fl = (uint16_t)(CSB_F_ERROR | ((uint16_t)(f & 0x1F) << CSB_F_ERRTYPE_SHIFT));
            code = 0;
        } else {
            uint8_t b7 = byte_at(p, len, off + 7);
            int ide = (b7 & 0x80) != 0;
            if (ide) {
                cid = ((uint32_t)b7 << 24) | ((uint32_t)byte_at(p, len, off + 8) << 16) |
                      ((uint32_t)byte_at(p, len, off + 9) << 8) | byte_at(p, len, off + 10);
                cid &= 0x1FFFFFFFu; hl = 11; fl |= CSB_F_IDE;
            } else {
                cid = (((uint32_t)b7 << 8) | byte_at(p, len, off + 8)) & 0x7FFu; hl = 9;
            }
            int rtr = b6 && !fdf;
            if (fdf) fl |= CSB_F_FDF;
            if (rtr) fl |= CSB_F_RTR;
            if (b6 && fdf) fl |= CSB_F_BRS;
            if (b5 && fdf) fl |= CSB_F_ESI;
            if (tx) fl |= CSB_F_TX_ACK;
            n = rtr ? 0 : DLC_LEN[code];
            flen = hl + n;
        }
        int ok = rem >= flen;
        if (ok) {
            if (o->n < o->max_frames) {
                int32_t i = o->n;
                o->t_dev[i] = EPOCH_2025 + (double)us / 1e6;
                o->id[i] = cid; o->flags[i] = fl; o->dlc[i] = code; o->data_len[i] = (uint8_t)n;
                memset(o->data + (size_t)i * CSB_MAX_CAN_DATA, 0, CSB_MAX_CAN_DATA);
                if (n > 0) memcpy(o->data + (size_t)i * CSB_MAX_CAN_DATA, p + off + hl, (size_t)n);
                o->n++;
            } else {
                o->overflow = 1;
            }
            off += flen;
        } else {
            o->decode_errors++;
        }
        if (!ok || off >= len) break;
    }
}

int32_t csb_can_deserialize(const uint8_t *payload, int32_t len,
                            double *t_dev, uint32_t *id, uint16_t *flags, uint8_t *dlc,
                            uint8_t *data_len, uint8_t *data, int32_t max_frames,
                            int32_t *n_frames, int32_t *decode_errors)
{
    if (len < 0 || (len > 0 && payload == NULL) || !t_dev || !id || !flags || !dlc || !data_len ||
        !data || !n_frames || !decode_errors)
        return CSB_ERR_INVALID;
    if (max_frames < len / 7 + 1) return CSB_ERR_BUFFER_TOO_SMALL;
    can_out_t o = {t_dev, id, flags, dlc, data_len, data, max_frames, 0, 0, 0};
    can_deserialize_into(payload, len, &o);
    *n_frames = o.n;
    *decode_errors = o.decode_errors;
    return o.overflow ? CSB_ERR_BUFFER_TOO_SMALL : CSB_OK;
}

/* ------------------------------------------------------------------ 1.5 CAN serialize */
int32_t csb_can_serialize(uint32_t id, uint8_t ide, uint8_t fdf, uint8_t brs,
                          const uint8_t *data, int32_t len,
                          uint8_t *payload, int32_t payload_cap)
{
    if (len < 0 || (len > 0 && data == NULL) || payload == NULL) return CSB_ERR_INVALID;
    if (payload_cap < CSB_MAX_CAN_PAYLOAD) return CSB_ERR_BUFFER_TOO_SMALL;
    int32_t n, padded;
    uint8_t flags;
    if (fdf) {
        n = len > 64 ? 64 : len;
        int code = 0;
        while (DLC_LEN[code] < n) code++;      /* first FD length >= n */
        padded = DLC_LEN[code];
        flags = (uint8_t)(0x80 | (brs ? 0x40 : 0x00) | code);
    } else {
        n = len > 8 ? 8 : len;
        padded = n;
        flags = (uint8_t)n;                    /* brs ignored: bit 6 would mean RTR */
    }
    int32_t k = 0;
    for (int i = 0; i < 6; i++) payload[k++] = 0;
    payload[k++] = flags;
    if (ide) {
        uint32_t v = id | 0x80000000u;
        payload[k++] = (uint8_t)(v >> 24); payload[k++] = (uint8_t)(v >> 16);
        payload[k++] = (uint8_t)(v >> 8);  payload[k++] = (uint8_t)v;
    } else {
        uint32_t v = id & 0x7FFu;
        payload[k++] = (uint8_t)(v >> 8);  payload[k++] = (uint8_t)v;
    }
    if (n > 0) memcpy(payload + k, data, (size_t)n);
    if (padded > n) memset(payload + k + n, 0, (size_t)(padded - n));
    return k + padded;
}

/* ------------------------------------------------------------------ 1.6 WS build */
int32_t csb_ws_build_frame(const uint8_t *payload, int32_t len, uint8_t opcode, uint32_t mask,
                           uint8_t *frame, int32_t frame_cap)
{
    if (len < 0 || (len > 0 && payload == NULL) || frame == NULL || opcode > 0x0F)
        return CSB_ERR_INVALID;
    if (frame_cap < len + 14) return CSB_ERR_BUFFER_TOO_SMALL;
    int32_t k = 0;
    frame[k++] = (uint8_t)(0x80 | opcode);
    if (len < 126) {
        frame[k++] = (uint8_t)(0x80 | len);
    } else if (len <= 0xFFFF) {
        frame[k++] = 0x80 | 126;
        frame[k++] = (uint8_t)(len >> 8); frame[k++] = (uint8_t)len;
    } else {
        frame[k++] = 0x80 | 127;
        for (int s = 56; s >= 0; s -= 8) frame[k++] = (uint8_t)((uint64_t)len >> s);
    }
    uint8_t m[4] = {(uint8_t)(mask >> 24), (uint8_t)(mask >> 16), (uint8_t)(mask >> 8), (uint8_t)mask};
    memcpy(frame + k, m, 4); k += 4;
    for (int32_t i = 0; i < len; i++) frame[k + i] = payload[i] ^ m[i & 3];
    return k + len;
}

/* ------------------------------------------------------------------ 1.7 WS parse (core) */
typedef struct {
    /* buffered mode */
    uint8_t *opcodes; int32_t *lens; int32_t max_frames; uint8_t *out; int32_t out_cap;
    int32_t count; int32_t out_n;
    /* streaming mode */
    hdlc_state_t *hdlc; payload_sink_t *can_sink;
    int32_t *peer_close; uint8_t *ping_payload; int32_t *ping_len;
} ws_ctx_t;

static void ws_reset_header(ws_state_t *st)
{
    st->in_payload = 0; st->hdr_len = 0; st->need = 2; st->masked = 0;
    st->remaining = 0; st->pos = 0; st->total = 0;
}

static int32_t check_ws_state(uint8_t *state, int32_t size, uint8_t mode)
{
    if (state == NULL || size < CSB_WS_STATE_SIZE) return CSB_ERR_INVALID;
    ws_state_t *st = (ws_state_t *)state;
    if (st->magic != WS_MAGIC) return CSB_ERR_STATE;
    if (st->mode == 0) st->mode = mode;
    if (st->mode != mode) return CSB_ERR_STATE;   /* one state per API */
    return CSB_OK;
}

int32_t csb_ws_state_init(uint8_t *state, int32_t state_size)
{
    if (state == NULL || state_size < CSB_WS_STATE_SIZE) return CSB_ERR_INVALID;
    memset(state, 0, CSB_WS_STATE_SIZE);
    ws_state_t *st = (ws_state_t *)state;
    st->magic = WS_MAGIC;
    ws_reset_header(st);
    return CSB_OK;
}

/* Called when a frame is complete. */
static int ws_frame_done(ws_state_t *st, ws_ctx_t *c)
{
    if (st->mode == 1) {
        if (c->count >= c->max_frames || c->out_n + (int32_t)st->total > c->out_cap) return -1;
        c->opcodes[c->count] = st->opcode;
        c->lens[c->count] = (int32_t)st->total;
        if (st->total) memcpy(c->out + c->out_n, st->buf, st->total);
        c->out_n += (int32_t)st->total;
        c->count++;
    } else {
        if (st->opcode == 8) *c->peer_close = 1;
        if (st->opcode == 9) {
            uint32_t n = st->total > 125 ? 125 : st->total;
            if (n) memcpy(c->ping_payload, st->buf, n);
            *c->ping_len = (int32_t)n;
        }
    }
    ws_reset_header(st);
    return 0;
}

/* Returns 0, or -1 on buffer overflow (buffered mode: frame larger than CSB_WS_MAX_BUFFER). */
static int ws_feed(ws_state_t *st, const uint8_t *in, int32_t len, ws_ctx_t *c)
{
    for (int32_t i = 0; i < len; i++) {
        uint8_t b = in[i];
        if (!st->in_payload) {
            st->hdr[st->hdr_len++] = b;
            if (st->hdr_len == 2) {
                uint8_t l7 = st->hdr[1] & 0x7F;
                st->masked = (st->hdr[1] & 0x80) != 0;
                st->need = (uint8_t)(2 + (l7 == 126 ? 2 : l7 == 127 ? 8 : 0) + (st->masked ? 4 : 0));
            }
            if (st->hdr_len < st->need || st->hdr_len < 2) continue;
            /* header complete */
            uint8_t l7 = st->hdr[1] & 0x7F;
            uint64_t L = l7;
            if (l7 == 126) L = ((uint64_t)st->hdr[2] << 8) | st->hdr[3];
            if (l7 == 127) { L = 0; for (int k = 2; k < 10; k++) L = (L << 8) | st->hdr[k]; }
            st->opcode = st->hdr[0] & 0x0F;
            if (L > 0x7FFFFFFFu) return -1;
            st->total = (uint32_t)L; st->remaining = (uint32_t)L; st->pos = 0;
            st->in_payload = 1;
            if (st->mode == 1 && st->total > CSB_WS_MAX_BUFFER) return -1;
            if (L == 0 && ws_frame_done(st, c) != 0) return -1;
            continue;
        }
        /* payload byte */
        if (st->masked) b ^= st->hdr[st->need - 4 + (st->pos & 3)];
        if (st->mode == 1) {
            st->buf[st->pos] = b;
        } else {
            uint8_t op = st->opcode;
            if (op == 2 || op == 0) hdlc_feed(c->hdlc, b, c->can_sink);   /* binary / continuation */
            else if (st->pos < 125) st->buf[st->pos] = b;                    /* control payload */
        }
        st->pos++; st->remaining--;
        if (st->remaining == 0 && ws_frame_done(st, c) != 0) return -1;
    }
    return 0;
}

int32_t csb_ws_parse_frames(uint8_t *state, int32_t state_size,
                            const uint8_t *in, int32_t in_len,
                            uint8_t *opcodes, int32_t *lens, int32_t max_frames,
                            uint8_t *out, int32_t out_cap, int32_t *n_frames)
{
    int32_t rc = check_ws_state(state, state_size, 1);
    if (rc != CSB_OK) return rc;
    if (in_len < 0 || (in_len > 0 && in == NULL) || !opcodes || !lens || !out || !n_frames)
        return CSB_ERR_INVALID;
    if (out_cap < in_len + CSB_WS_MAX_BUFFER || max_frames < (in_len + CSB_WS_MAX_BUFFER) / 2 + 1)
        return CSB_ERR_BUFFER_TOO_SMALL;
    ws_state_t *st = (ws_state_t *)state;
    ws_ctx_t c;
    memset(&c, 0, sizeof c);
    c.opcodes = opcodes; c.lens = lens; c.max_frames = max_frames; c.out = out; c.out_cap = out_cap;
    if (ws_feed(st, in, in_len, &c) != 0) {
        *n_frames = c.count;
        ws_reset_header(st);
        return CSB_ERR_BUFFER_TOO_SMALL;
    }
    *n_frames = c.count;
    return CSB_OK;
}

/* ------------------------------------------------------------------ 2.0 bit timing */
static int in_range(int64_t v, int64_t lo, int64_t hi) { return v >= lo && v <= hi; }

int32_t csb_bit_timing(uint32_t nominal_bitrate, double nominal_sp,
                       uint32_t data_bitrate, double data_sp,
                       uint16_t *timing, double *nominal_sp_out, double *data_sp_out)
{
    if (!timing || !nominal_sp_out || !data_sp_out) return CSB_ERR_INVALID;
    if (nominal_bitrate == 0 || data_bitrate < nominal_bitrate || nominal_sp < 50.0 || data_sp < 50.0)
        return CSB_ERR_INVALID;
    const int64_t F = 80000000;
    double best_dev = INFINITY;
    for (int64_t brp = 1; brp <= 256; brp++) {
        int64_t nbt = F / ((int64_t)nominal_bitrate * brp);
        int64_t dbt = F / ((int64_t)data_bitrate * brp);
        if (nbt < 1 || dbt < 1) break;
        if (fabs(F_CLOCK / (double)(nbt * brp) - (double)nominal_bitrate) > (double)nominal_bitrate / 256.0) continue;
        if (fabs(F_CLOCK / (double)(dbt * brp) - (double)data_bitrate) > (double)data_bitrate / 256.0) continue;
        if (!in_range(nbt, 8, 80) || !in_range(dbt, 5, 25)) continue;
        int64_t t1 = (int64_t)nearbyint(nominal_sp / 100.0 * (double)nbt) - 1;   /* half to even */
        if (t1 > nbt - 3) t1 = nbt - 3;
        int64_t t2 = nbt - t1 - 1;
        int64_t d1 = (int64_t)nearbyint(data_sp / 100.0 * (double)dbt) - 1;
        if (d1 > dbt - 3) d1 = dbt - 3;
        int64_t d2 = dbt - d1 - 1;
        if (!in_range(t1, 2, 128) || !in_range(t2, 2, 32) || !in_range(d1, 1, 16) || !in_range(d2, 2, 8)) continue;
        double nsp = 100.0 * (double)(1 + t1) / (double)nbt;
        double dsp = 100.0 * (double)(1 + d1) / (double)dbt;
        if (nsp < 50.0 || dsp < 50.0) continue;
        double dev = fabs(nsp - nominal_sp) + fabs(dsp - data_sp);
        if (dev < best_dev) {   /* strictly smaller: on a tie the lower brp stays */
            best_dev = dev;
            timing[0] = (uint16_t)brp; timing[1] = (uint16_t)t1; timing[2] = (uint16_t)t2; timing[3] = (uint16_t)t2;
            timing[4] = (uint16_t)brp; timing[5] = (uint16_t)d1; timing[6] = (uint16_t)d2; timing[7] = (uint16_t)d2;
            *nominal_sp_out = nsp; *data_sp_out = dsp;
        }
    }
    return isinf(best_dev) ? CSB_ERR_NO_SOLUTION : CSB_OK;
}

/* ------------------------------------------------------------------ helpers */
int32_t csb_encode_can_to_ws(uint32_t id, uint8_t ide, uint8_t fdf, uint8_t brs,
                             const uint8_t *data, int32_t len, uint32_t mask,
                             uint8_t *out, int32_t out_cap)
{
    uint8_t can[CSB_MAX_CAN_PAYLOAD];
    uint8_t hd[2 * (CSB_MAX_CAN_PAYLOAD + 4) + 2];
    if (out == NULL) return CSB_ERR_INVALID;
    if (out_cap < (int32_t)sizeof hd + 14) return CSB_ERR_BUFFER_TOO_SMALL;
    int32_t n = csb_can_serialize(id, ide, fdf, brs, data, len, can, (int32_t)sizeof can);
    if (n < 0) return n;
    int32_t m = csb_hdlc_encode(can, n, hd, (int32_t)sizeof hd);
    if (m < 0) return m;
    return csb_ws_build_frame(hd, m, 2, mask, out, out_cap);
}

static int can_sink_payload(void *vctx, const uint8_t *p, int32_t n)
{
    can_deserialize_into(p, n, (can_out_t *)vctx);
    return 0;
}

int32_t csb_max_frames(int32_t in_len)
{
    if (in_len < 0) in_len = 0;
    return (in_len + CSB_HDLC_MAX_FRAME) / 7 + 1;
}

int32_t csb_decode_ws_to_can(uint8_t *ws_state, int32_t ws_state_size,
                             uint8_t *hdlc_state, int32_t hdlc_state_size,
                             const uint8_t *in, int32_t in_len,
                             double *t_dev, uint32_t *id, uint16_t *flags, uint8_t *dlc,
                             uint8_t *data_len, uint8_t *data, int32_t max_frames,
                             int32_t *n_frames, int32_t *peer_close,
                             uint8_t *ping_payload, int32_t *ping_len,
                             uint32_t *crc_errors, int32_t *decode_errors)
{
    int32_t rc = check_ws_state(ws_state, ws_state_size, 2);
    if (rc != CSB_OK) return rc;
    rc = check_hdlc_state(hdlc_state, hdlc_state_size);
    if (rc != CSB_OK) return rc;
    if (in_len < 0 || (in_len > 0 && in == NULL) || !t_dev || !id || !flags || !dlc || !data_len ||
        !data || !n_frames || !peer_close || !ping_payload || !ping_len || !crc_errors || !decode_errors)
        return CSB_ERR_INVALID;
    if (max_frames < csb_max_frames(in_len)) return CSB_ERR_BUFFER_TOO_SMALL;
    can_out_t o = {t_dev, id, flags, dlc, data_len, data, max_frames, 0, 0, 0};
    payload_sink_t sink = {can_sink_payload, &o};
    ws_ctx_t c;
    memset(&c, 0, sizeof c);
    c.hdlc = (hdlc_state_t *)hdlc_state; c.can_sink = &sink;
    c.peer_close = peer_close; c.ping_payload = ping_payload; c.ping_len = ping_len;
    *peer_close = 0; *ping_len = -1;
    int fr = ws_feed((ws_state_t *)ws_state, in, in_len, &c);
    *n_frames = o.n;
    *crc_errors = ((hdlc_state_t *)hdlc_state)->crc_errors;
    *decode_errors = o.decode_errors;
    if (fr != 0) return CSB_ERR_INVALID;          /* impossible frame length */
    return o.overflow ? CSB_ERR_BUFFER_TOO_SMALL : CSB_OK;
}

/* ------------------------------------------------------------------ real-time helpers */
int32_t csb_encode_rx_frame_to_ws(uint32_t new_id, int32_t index, int32_t n_frames,
                                  const uint16_t *flags, const uint8_t *data_len,
                                  const uint8_t *data, uint32_t mask,
                                  uint8_t *out, int32_t out_cap)
{
    if (!flags || !data_len || !data || !out || index < 0 || index >= n_frames)
        return CSB_ERR_INVALID;
    uint16_t f = flags[index];
    if (f & CSB_F_ERROR) return CSB_ERR_INVALID;              /* error frames are not echoed */
    int32_t n = data_len[index];
    if (n > CSB_MAX_CAN_DATA) return CSB_ERR_INVALID;
    return csb_encode_can_to_ws(new_id, (uint8_t)((f & CSB_F_IDE) != 0), (uint8_t)((f & CSB_F_FDF) != 0),
                                (uint8_t)((f & CSB_F_BRS) != 0), data + (size_t)index * CSB_MAX_CAN_DATA,
                                n, mask, out, out_cap);
}

int32_t csb_warmup(uint8_t *ws_state, int32_t ws_state_size,
                   uint8_t *hdlc_state, int32_t hdlc_state_size)
{
    int32_t rc = csb_ws_state_init(ws_state, ws_state_size);
    if (rc != CSB_OK) return rc;
    rc = csb_hdlc_state_init(hdlc_state, hdlc_state_size);
    if (rc != CSB_OK) return rc;

    uint8_t data[CSB_MAX_CAN_DATA];
    for (int i = 0; i < CSB_MAX_CAN_DATA; i++) data[i] = (uint8_t)(i * 37 + 0x7D);  /* forces stuffing */
    uint8_t stream[2 * 174];
    int32_t a = csb_encode_can_to_ws(0x123u, 0, 0, 0, data, 8, 0x12345678u, stream, 174);
    if (a < 0) return a;
    int32_t b = csb_encode_can_to_ws(0x18FEF100u, 1, 1, 1, data, 64, 0x12345678u, stream + a, 174);
    if (b < 0) return b;

    /* decode through the internal path with a 2-frame output (no large stack arrays) */
    double t[2]; uint32_t id[2]; uint16_t fl[2]; uint8_t dl[2], dn[2], dd[2 * CSB_MAX_CAN_DATA];
    can_out_t o = {t, id, fl, dl, dn, dd, 2, 0, 0, 0};
    payload_sink_t sink = {can_sink_payload, &o};
    int32_t peer_close = 0, ping_len = -1; uint8_t ping[125];
    ws_ctx_t c;
    memset(&c, 0, sizeof c);
    c.hdlc = (hdlc_state_t *)hdlc_state; c.can_sink = &sink;
    c.peer_close = &peer_close; c.ping_payload = ping; c.ping_len = &ping_len;
    ((ws_state_t *)ws_state)->mode = 2;
    if (ws_feed((ws_state_t *)ws_state, stream, a + b, &c) != 0) return CSB_ERR_INVALID;
    int32_t frames = o.n;
    uint16_t timing[8]; double x, y;
    (void)csb_bit_timing(500000u, 80.0, 2000000u, 80.0, timing, &x, &y);  /* touches libm */

    /* leave both states fresh for the real loop */
    rc = csb_ws_state_init(ws_state, ws_state_size);
    if (rc != CSB_OK) return rc;
    rc = csb_hdlc_state_init(hdlc_state, hdlc_state_size);
    if (rc != CSB_OK) return rc;
    return frames;
}

/* ------------------------------------------------------------------ pcap CAN packets (1.2.0) */
static void put_u32(uint8_t *p, uint32_t v, int little)
{
    if (little) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
    else        { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
}

static uint32_t get_u32(const uint8_t *p, int little)
{
    return little ? ((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24))
                  : (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]);
}

int32_t csb_pcap_max_bytes(int32_t n_frames)
{
    if (n_frames < 0 || n_frames > INT32_MAX / CSB_PCAP_MAX_PACKET) return CSB_ERR_INVALID;
    return n_frames * CSB_PCAP_MAX_PACKET;
}

/* ID lists as CAN_RW_CORE "Filter Packets": both lists empty -> pass; else the 29-bit ID must be
 * in the allow list (or that list is empty) and must not be in the deny list. Linear search. */
static int pcap_id_pass(uint32_t id29, const uint32_t *allow, int32_t n_allow,
                        const uint32_t *deny, int32_t n_deny)
{
    if (n_allow > 0) {
        int found = 0;
        for (int32_t k = 0; k < n_allow; k++) if ((allow[k] & 0x1FFFFFFFu) == id29) { found = 1; break; }
        if (!found) return 0;
    }
    for (int32_t k = 0; k < n_deny; k++) if ((deny[k] & 0x1FFFFFFFu) == id29) return 0;
    return 1;
}

/* Frames first .. n_frames-1 -> packets, until max_packets packets are written. The caller has
 * checked the arguments and the capacity. */
static int32_t pcap_convert(int32_t n_frames, int32_t first, int32_t max_packets,
                            const double *t_dev, const uint32_t *id, const uint16_t *flags,
                            const uint8_t *data_len, const uint8_t *data,
                            const uint32_t *allow, int32_t n_allow, const uint32_t *deny, int32_t n_deny,
                            int32_t tai_minus_utc_s, int32_t options, uint8_t *out,
                            int32_t *n_packets, int32_t *n_filtered, int32_t *n_skipped, int32_t *next)
{
    int little = (options & CSB_PCAP_LITTLE_ENDIAN) != 0;
    int32_t w = 0, np = 0, ns = 0, nfl = 0, i = first;
    for (; i < n_frames && np < max_packets; i++) {
        uint16_t f = flags[i];
        int err = (f & CSB_F_ERROR) != 0;
        if ((err && !(options & CSB_PCAP_KEEP_ERRORS)) ||
            ((f & CSB_F_TX_ACK) && !(options & CSB_PCAP_KEEP_TX_ACK))) { ns++; continue; }
        /* TAI ns: t_dev = 2025-01-01 + us / 1e6 (exact in the decoder); recover the integer us */
        int64_t us = (int64_t)nearbyint((t_dev[i] - EPOCH_2025) * 1e6);
        int64_t ns_tai = ((int64_t)EPOCH_2025 + (int64_t)tai_minus_utc_s) * 1000000000LL + us * 1000LL;
        if (ns_tai < 0) ns_tai = 0;                             /* as NI: Max(t, 0) */
        uint32_t can_id; uint8_t plen, fd = 0; uint8_t pay[CSB_MAX_CAN_DATA];
        memset(pay, 0, sizeof pay);
        if (err) {                                               /* SocketCAN error frame */
            uint8_t et = (uint8_t)((f >> CSB_F_ERRTYPE_SHIFT) & 0x1F);
            plen = 8;
            if (et == 1) can_id = 0x20000000u | 0x20u;           /* CAN_ERR_ACK */
            else {
                can_id = 0x20000000u | 0x08u;                    /* CAN_ERR_PROT */
                if (et == 0) pay[2] = 0x01;                      /* CAN_ERR_PROT_BIT */
                else if (et == 2) pay[2] = 0x02;                 /* CAN_ERR_PROT_FORM */
                else if (et == 3) pay[2] = 0x04;                 /* CAN_ERR_PROT_STUFF */
                else if (et == 4) pay[3] = 0x08;                 /* CAN_ERR_PROT_LOC_CRC_SEQ */
            }
        } else {
            can_id = id[i] & 0x1FFFFFFFu;
            if (f & CSB_F_IDE) can_id |= 0x80000000u;
            int fdf = (f & CSB_F_FDF) != 0;
            if (fdf) fd = (uint8_t)(0x04 | ((f & CSB_F_BRS) ? 0x01 : 0x00));
            if (!fdf && (f & CSB_F_RTR)) { can_id |= 0x40000000u; plen = 0; }
            else {
                plen = data_len[i];
                if (plen > (fdf ? CSB_MAX_CAN_DATA : 8)) plen = (uint8_t)(fdf ? CSB_MAX_CAN_DATA : 8);
                for (uint8_t k = 0; k < plen; k++) pay[k] = data[(size_t)i * CSB_MAX_CAN_DATA + k];
            }
        }
        if ((n_allow > 0 || n_deny > 0) &&
            !pcap_id_pass(can_id & 0x1FFFFFFFu, allow, n_allow, deny, n_deny)) { nfl++; continue; }
        uint8_t *p = out + w;
        uint32_t len = 8u + plen;
        put_u32(p + 0, (uint32_t)(ns_tai / 1000000000LL), little);
        put_u32(p + 4, (uint32_t)(ns_tai % 1000000000LL), little);
        put_u32(p + 8, len, little);
        put_u32(p + 12, len, little);
        put_u32(p + 16, can_id, 0);                              /* always big-endian */
        p[20] = plen; p[21] = fd; p[22] = 0; p[23] = 0;
        for (uint8_t k = 0; k < plen; k++) p[24 + k] = pay[k];
        w += 24 + plen; np++;
    }
    *n_packets = np; *n_filtered = nfl; *n_skipped = ns; *next = i;
    return w;
}

int32_t csb_frames_to_pcap(int32_t n_frames, const double *t_dev, const uint32_t *id,
                          const uint16_t *flags, const uint8_t *data_len, const uint8_t *data,
                          int32_t tai_minus_utc_s, int32_t options,
                          uint8_t *out, int32_t out_cap, int32_t *n_packets, int32_t *n_skipped)
{
    if (n_frames < 0 || !t_dev || !id || !flags || !data_len || !data || !out || !n_packets || !n_skipped)
        return CSB_ERR_INVALID;
    int32_t need = csb_pcap_max_bytes(n_frames);
    if (need < 0) return need;
    if (out_cap < need) return CSB_ERR_BUFFER_TOO_SMALL;
    int32_t nfl, next;
    return pcap_convert(n_frames, 0, n_frames, t_dev, id, flags, data_len, data, NULL, 0, NULL, 0,
                        tai_minus_utc_s, options, out, n_packets, &nfl, n_skipped, &next);
}

/* 1.3.0 */
int32_t csb_frames_to_pcap_range(int32_t n_frames, int32_t first, int32_t max_packets,
                                const double *t_dev, const uint32_t *id, const uint16_t *flags,
                                const uint8_t *data_len, const uint8_t *data,
                                const uint32_t *allow, int32_t n_allow,
                                const uint32_t *deny, int32_t n_deny,
                                int32_t tai_minus_utc_s, int32_t options,
                                uint8_t *out, int32_t out_cap,
                                int32_t *n_packets, int32_t *n_filtered, int32_t *n_skipped, int32_t *next)
{
    if (n_frames < 0 || first < 0 || first > n_frames || max_packets < 0 ||
        !t_dev || !id || !flags || !data_len || !data || !out ||
        n_allow < 0 || n_deny < 0 || (n_allow > 0 && !allow) || (n_deny > 0 && !deny) ||
        !n_packets || !n_filtered || !n_skipped || !next)
        return CSB_ERR_INVALID;
    int32_t most = n_frames - first;                 /* packets this call can write at most */
    if (most > max_packets) most = max_packets;
    int32_t need = csb_pcap_max_bytes(most);
    if (need < 0) return need;
    if (out_cap < need) return CSB_ERR_BUFFER_TOO_SMALL;
    return pcap_convert(n_frames, first, max_packets, t_dev, id, flags, data_len, data,
                        allow, n_allow, deny, n_deny, tai_minus_utc_s, options, out,
                        n_packets, n_filtered, n_skipped, next);
}

int32_t csb_encode_pcap_to_ws(const uint8_t *in, int32_t in_len, int32_t options, uint32_t mask,
                             uint8_t *out, int32_t out_cap,
                             int32_t *n_packets, int32_t *n_rejected, int32_t *consumed)
{
    if (in_len < 0 || (in_len > 0 && !in) || !out || out_cap < 0 || !n_packets || !n_rejected || !consumed)
        return CSB_ERR_INVALID;
    int little = (options & CSB_PCAP_LITTLE_ENDIAN) != 0;
    int32_t pos = 0, w = 0, np = 0, nr = 0;
    *n_packets = 0; *n_rejected = 0; *consumed = 0;
    while (in_len - pos >= 24 && out_cap - w >= 174) {
        const uint8_t *p = in + pos;
        uint32_t incl = get_u32(p + 8, little);
        if (incl < 8u || incl > 72u) { *n_packets = np; *n_rejected = nr; *consumed = pos; return CSB_ERR_INVALID; }
        int32_t plen_total = 16 + (int32_t)incl;
        if (in_len - pos < plen_total) break;                    /* incomplete: keep for later */
        uint32_t can_id = get_u32(p + 16, 0);
        uint8_t plen = p[20], fd = p[21];
        int fdf = (fd & 0x04) != 0, brs = (fd & 0x01) != 0;
        int ext = (can_id & 0x80000000u) != 0;
        uint32_t cid = can_id & 0x1FFFFFFFu;
        int ok = !(can_id & 0x60000000u)                         /* no remote, no error frame */
              && (uint32_t)plen + 8u <= incl
              && plen <= (fdf ? CSB_MAX_CAN_DATA : 8)
              && (ext || cid <= 0x7FFu);
        if (ok) {
            int32_t n = csb_encode_can_to_ws(cid, (uint8_t)ext, (uint8_t)fdf, (uint8_t)(fdf && brs),
                                             p + 24, plen, mask, out + w, out_cap - w);
            if (n < 0) { *n_packets = np; *n_rejected = nr; *consumed = pos; return n; }
            w += n; np++;
        } else {
            nr++;
        }
        pos += plen_total;
    }
    *n_packets = np; *n_rejected = nr; *consumed = pos;
    return w;
}
