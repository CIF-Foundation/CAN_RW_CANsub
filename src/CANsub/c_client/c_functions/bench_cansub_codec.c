/*
 * bench_cansub_codec.c - execution time per call of the real-time path, for NI Linux RT.
 *
 *   ./bench_cansub_codec_static              normal priority
 *   ./bench_cansub_codec_static --fifo 80    SCHED_FIFO priority 80 + mlockall (as admin)
 *
 * Measures, 100 000 calls each, with CLOCK_MONOTONIC:
 *   decode : csb_decode_ws_to_can on one realistic device message (one WS frame holding one
 *            HDLC frame with 70 classic 8-byte CAN frames, ~1.4 kB)
 *   echo   : csb_encode_rx_frame_to_ws (one 8-byte frame from the decode buffers)
 *   to_pcap: csb_frames_to_pcap, the 70 decoded frames -> pcap CAN packets
 *   pcap_ws: csb_encode_pcap_to_ws, one 8-byte pcap CAN packet -> WebSocket frame
 *   crc    : csb_crc32 over 1400 bytes
 * Prints median / p99 / p99.9 / max in microseconds. The max under --fifo is the number for a
 * worst-case budget; without --fifo it includes ordinary scheduling noise.
 */
#define _GNU_SOURCE
#include "cansub_codec.h"
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#define N 100000
static double t_ns[N];
static uint8_t ws[CSB_WS_STATE_SIZE], hd[CSB_HDLC_STATE_SIZE];
static double t_dev[1200]; static uint32_t ids[1200]; static uint16_t fl[1200];
static uint8_t dl[1200], dn[1200], dd[1200 * 64];

static double now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec; }
static int cmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static void report(const char *name)
{
    qsort(t_ns, N, sizeof t_ns[0], cmp);
    printf("%-7s median %7.2f us   p99 %7.2f us   p99.9 %7.2f us   max %8.2f us\n", name,
           t_ns[N / 2] / 1e3, t_ns[(int)(N * 0.99)] / 1e3, t_ns[(int)(N * 0.999)] / 1e3, t_ns[N - 1] / 1e3);
}

int main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "--fifo") == 0) {
        struct sched_param sp = {.sched_priority = atoi(argv[2])};
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) { perror("sched_setscheduler (run as admin)"); return 1; }
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { perror("mlockall"); return 1; }
        printf("SCHED_FIFO priority %d, memory locked\n", sp.sched_priority);
    }
    printf("libcansub_codec version 0x%08X\n", csb_version());

    /* one realistic device message: 70 classic frames -> one HDLC frame -> one unmasked WS frame */
    static uint8_t can[70 * 17 + CSB_MAX_CAN_PAYLOAD], hdlc[4096], msg[4096];   /* serialize needs 75 B free per call */
    int32_t cn = 0;
    for (int i = 0; i < 70; i++) {
        uint8_t d[8] = {(uint8_t)(i >> 8), (uint8_t)i, (uint8_t)(i % 10), 0x11, 0x22, 0x33, 0x44, 0x55};
        int32_t k = csb_can_serialize(0x100u + (uint32_t)(i % 10), 0, 0, 0, d, 8, can + cn, (int32_t)sizeof can - cn);
        if (k < 0) { printf("serialize error %d at frame %d\n", k, i); return 1; }
        cn += k;
    }
    int32_t hn = csb_hdlc_encode(can, cn, hdlc, sizeof hdlc);
    int32_t mn = 0;
    msg[mn++] = 0x82; msg[mn++] = 126; msg[mn++] = (uint8_t)(hn >> 8); msg[mn++] = (uint8_t)hn;   /* server frame: unmasked */
    memcpy(msg + mn, hdlc, (size_t)hn); mn += hn;

    int32_t w = csb_warmup(ws, sizeof ws, hd, sizeof hd);
    if (w != 2) { printf("warmup failed: %d\n", w); return 1; }

    int32_t nf, pc, pl, de; uint32_t ce; uint8_t ping[125], out[200];
    for (int i = 0; i < N; i++) {
        double a = now_ns();
        int32_t rc = csb_decode_ws_to_can(ws, sizeof ws, hd, sizeof hd, msg, mn, t_dev, ids, fl, dl, dn, dd, 1200,
                                          &nf, &pc, ping, &pl, &ce, &de);
        t_ns[i] = now_ns() - a;
        if (rc != 0 || nf != 70 || ce != 0) { printf("decode error rc=%d nf=%d ce=%u\n", rc, nf, ce); return 1; }
    }
    printf("message: %d bytes, 70 CAN frames\n", mn);
    report("decode");
    for (int i = 0; i < N; i++) {
        double a = now_ns();
        int32_t n = csb_encode_rx_frame_to_ws(ids[i % 70] - 0x100u, i % 70, nf, fl, dn, dd, 0x12345678u, out, sizeof out);
        t_ns[i] = now_ns() - a;
        if (n <= 0) { printf("echo error %d\n", n); return 1; }
    }
    report("echo");
    for (int i = 0; i < N; i++) {
        double a = now_ns();
        volatile uint32_t c = csb_crc32(can, 1400 < cn ? 1400 : cn);
        (void)c;
        t_ns[i] = now_ns() - a;
    }
    report("crc");
    static uint8_t cif[70 * CSB_PCAP_MAX_PACKET], wso[70 * 174];
    int32_t cn2 = 0, np, nsk;
    for (int i = 0; i < N; i++) {
        double a = now_ns();
        cn2 = csb_frames_to_pcap(nf, t_dev, ids, fl, dn, dd, CSB_TAI_MINUS_UTC, 0, cif, sizeof cif, &np, &nsk);
        t_ns[i] = now_ns() - a;
        if (cn2 <= 0 || np != 70) { printf("to_pcap error %d\n", cn2); return 1; }
    }
    report("to_pcap");
    for (int i = 0; i < N; i++) {
        int32_t used, ep, er;
        double a = now_ns();
        int32_t n = csb_encode_pcap_to_ws(cif, 32, 0, 0x12345678u, wso, sizeof wso, &ep, &er, &used);
        t_ns[i] = now_ns() - a;
        if (n <= 0 || ep != 1) { printf("pcap_to_ws error %d\n", n); return 1; }
    }
    report("pcap_ws");
    double ns = now_ns(); for (int i = 0; i < 1000; i++) (void)now_ns();
    printf("clock_gettime overhead: %.0f ns per call (included in every number above)\n", (now_ns() - ns) / 1000.0);
    return 0;
}
