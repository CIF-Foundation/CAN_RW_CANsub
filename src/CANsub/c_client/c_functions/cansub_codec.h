/*
 * cansub_codec.h - CANsub.4 WebSocket/HDLC/CAN codec for LabVIEW (NI Linux RT x64).
 *
 * C11 with GNU extensions (gcc -std=gnu11), no LabVIEW headers. Every array is passed as "pointer + length";
 * every output array is allocated by the caller (LabVIEW: Initialize Array,
 * pass as Array Data Pointer). No global state, no malloc: all functions are
 * thread-safe and reentrant ("Run in any thread" in the Call Library Function Node).
 *
 * Same algorithms and byte layouts as LABVIEW_NATIVE_CLIENT_BUILD.md, steps 1.1-1.7
 * and 2.0. Device docs: CANsub.4 firmware 02.05.00 / API 05.00 (WebSocket API, TV-01..TV-20).
 *
 * Return values: >= 0 = success (usually a length or a count), < 0 = CSB_ERR_*.
 */
#ifndef CANSUB_CODEC_H
#define CANSUB_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSB_VERSION              0x00010200   /* 1.2.0: pcap CAN packets (LINKTYPE_CAN_SOCKETCAN, NI CIF format) (csb_frames_to_pcap, csb_encode_pcap_to_ws) */

#define CSB_OK                   0
#define CSB_ERR_BUFFER_TOO_SMALL (-1)  /* an output buffer is smaller than required */
#define CSB_ERR_INVALID          (-2)  /* invalid input (NULL, negative length, bad argument) */
#define CSB_ERR_NO_SOLUTION      (-3)  /* bit timing: no solution with one prescaler */
#define CSB_ERR_STATE            (-4)  /* state buffer not initialised or corrupted */

/* Sizes */
#define CSB_MAX_CAN_DATA         64    /* CAN FD */
#define CSB_MAX_CAN_PAYLOAD      75    /* 11-byte header + 64 data bytes */
#define CSB_HDLC_MAX_FRAME       4096  /* max unstuffed bytes between two 7E (incl. CRC) */
#define CSB_WS_MAX_BUFFER        16384 /* max buffered bytes of an incomplete WS frame */

/* Opaque state buffers. LabVIEW: U8 array of this size, initialise once with the
 * *_state_init function, keep it in a shift register, pass it as Array Data Pointer. */
#define CSB_HDLC_STATE_SIZE      (CSB_HDLC_MAX_FRAME + 16)
#define CSB_WS_STATE_SIZE        (CSB_WS_MAX_BUFFER + 48)

/* CAN frame flag bits (output of the decoders, one uint16 per frame) */
#define CSB_F_IDE      0x0001
#define CSB_F_FDF      0x0002
#define CSB_F_RTR      0x0004
#define CSB_F_BRS      0x0008
#define CSB_F_ESI      0x0010
#define CSB_F_TX_ACK   0x0020
#define CSB_F_ERROR    0x0040
/* bits 8..12: error type for error frames (0 bit, 1 ack, 2 form, 3 stuff, 4 CRC) */
#define CSB_F_ERRTYPE_SHIFT 8

/* ---------------------------------------------------------------- info */
uint32_t csb_version(void);
int32_t  csb_hdlc_state_size(void);
int32_t  csb_ws_state_size(void);

/* ---------------------------------------------------------------- 1.1 CRC32 */
/* IEEE 802.3 / zlib CRC32 (poly EDB88320 reflected, init FFFFFFFF, final XOR FFFFFFFF). */
uint32_t csb_crc32(const uint8_t *data, int32_t len);

/* ---------------------------------------------------------------- 1.2 HDLC encode */
/* frame = 7E + stuff(payload + CRC32 big-endian) + 7E. Needs frame_cap >= 2*(len+4)+2.
 * Returns the frame length. */
int32_t csb_hdlc_encode(const uint8_t *payload, int32_t len, uint8_t *frame, int32_t frame_cap);

/* ---------------------------------------------------------------- 1.3 HDLC decode */
int32_t csb_hdlc_state_init(uint8_t *state, int32_t state_size);
/* Feeds in[0..in_len). Complete CRC-valid payloads are appended to out (concatenated);
 * payload i has length lens[i]. *n_payloads = number of payloads. *crc_errors = cumulative
 * CRC error count kept in the state (frames longer than CSB_HDLC_MAX_FRAME are dropped and
 * counted as CRC errors).
 * Requirements (checked first, nothing consumed on error):
 *   out_cap >= in_len + CSB_HDLC_MAX_FRAME (a payload can include bytes buffered in the state),
 *   lens_cap >= in_len/6 + 1. Returns CSB_OK. */
int32_t csb_hdlc_decode(uint8_t *state, int32_t state_size,
                        const uint8_t *in, int32_t in_len,
                        uint8_t *out, int32_t out_cap,
                        int32_t *lens, int32_t lens_cap,
                        int32_t *n_payloads, uint32_t *crc_errors);

/* ---------------------------------------------------------------- 1.4 CAN deserialize */
/* Decodes all CAN frames in one HDLC payload into parallel arrays (capacity max_frames each):
 *   t_dev[i]   seconds since 1970-01-01 UTC (device time)
 *   id[i]      11- or 29-bit ID (0 for error frames)
 *   flags[i]   CSB_F_* bits, error type in bits 8..12
 *   dlc[i]     DLC code 0..15
 *   data_len[i] number of data bytes (0..64)
 *   data[i*64 .. i*64+data_len[i])   data bytes (data must hold max_frames*64 bytes)
 * *n_frames = frames decoded; *decode_errors = 1 if the payload ended in a truncated frame.
 * Requirement: max_frames >= len/7 + 1. Returns CSB_OK. */
int32_t csb_can_deserialize(const uint8_t *payload, int32_t len,
                            double *t_dev, uint32_t *id, uint16_t *flags, uint8_t *dlc,
                            uint8_t *data_len, uint8_t *data, int32_t max_frames,
                            int32_t *n_frames, int32_t *decode_errors);

/* ---------------------------------------------------------------- 1.5 CAN serialize */
/* Classic (fdf=0): at most 8 data bytes are sent, brs ignored (bit 6 = RTR must stay 0).
 * FD (fdf=1): at most 64 bytes, zero-padded to the next FD length (12,16,20,24,32,48,64).
 * Timestamp bytes 0, TX bit 0. Standard IDs are masked to 11 bits.
 * Needs payload_cap >= 75. Returns the payload length. */
int32_t csb_can_serialize(uint32_t id, uint8_t ide, uint8_t fdf, uint8_t brs,
                          const uint8_t *data, int32_t len,
                          uint8_t *payload, int32_t payload_cap);

/* ---------------------------------------------------------------- 1.6 WS build frame */
/* Client frame: FIN + opcode, masked. mask = 4 mask bytes as big-endian uint32
 * (the build doc uses 0x12345678). Needs frame_cap >= len + 14. Returns the frame length. */
int32_t csb_ws_build_frame(const uint8_t *payload, int32_t len, uint8_t opcode, uint32_t mask,
                           uint8_t *frame, int32_t frame_cap);

/* ---------------------------------------------------------------- 1.7 WS parse frames */
int32_t csb_ws_state_init(uint8_t *state, int32_t state_size);
/* Appends in[] to the buffered bytes and extracts every complete server frame (unmasked).
 * Frame i: opcodes[i], payload = out[offset_i .. offset_i+lens[i]) (payloads concatenated).
 * Requirements: out_cap >= in_len + CSB_WS_MAX_BUFFER, max_frames >= (in_len + CSB_WS_MAX_BUFFER)/2 + 1.
 * Returns CSB_OK, or CSB_ERR_BUFFER_TOO_SMALL if an incomplete frame would exceed
 * CSB_WS_MAX_BUFFER (then the state is reset). */
int32_t csb_ws_parse_frames(uint8_t *state, int32_t state_size,
                            const uint8_t *in, int32_t in_len,
                            uint8_t *opcodes, int32_t *lens, int32_t max_frames,
                            uint8_t *out, int32_t out_cap, int32_t *n_frames);

/* ---------------------------------------------------------------- 2.0 bit timing */
/* python-can 4.6.1 BitTimingFd.from_sample_point (f_clock 80 MHz), one prescaler for both
 * phases. timing[8] = brp, seg1, seg2, sjw, data_brp, data_seg1, data_seg2, data_sjw.
 * Returns CSB_OK, CSB_ERR_INVALID (data < nominal, nominal 0, sample point < 50) or
 * CSB_ERR_NO_SOLUTION. */
int32_t csb_bit_timing(uint32_t nominal_bitrate, double nominal_sp,
                       uint32_t data_bitrate, double data_sp,
                       uint16_t *timing, double *nominal_sp_out, double *data_sp_out);

/* ---------------------------------------------------------------- one-call helpers */
/* CAN frame -> WebSocket frame ready for TCP Write (serialize + HDLC encode + WS build,
 * opcode 2). Needs out_cap >= 2*(75+4)+2 + 14 = 174. Returns the frame length. */
int32_t csb_encode_can_to_ws(uint32_t id, uint8_t ide, uint8_t fdf, uint8_t brs,
                             const uint8_t *data, int32_t len, uint32_t mask,
                             uint8_t *out, int32_t out_cap);

/* Bytes from TCP Read -> CAN frames (WS parse + HDLC decode + CAN deserialize), using both
 * state buffers. Streams byte by byte: binary payload bytes go straight to the HDLC decoder,
 * so WebSocket frames of any length work. A ws_state used here must not also be used with
 * csb_ws_parse_frames (and the reverse); initialise a separate state for each. Frame arrays as in csb_can_deserialize (capacity max_frames).
 * Control frames: *peer_close = 1 if a close frame (opcode 8) arrived; the payload of the
 * last ping (opcode 9) is copied to ping_payload (cap 125), *ping_len = its length or -1 if
 * no ping arrived (send a pong with csb_ws_build_frame, opcode 10).
 * *crc_errors = cumulative HDLC CRC errors; *decode_errors = truncated CAN frames in this call.
 * Requirement: max_frames >= csb_max_frames(in_len). Returns CSB_OK. */
int32_t csb_decode_ws_to_can(uint8_t *ws_state, int32_t ws_state_size,
                             uint8_t *hdlc_state, int32_t hdlc_state_size,
                             const uint8_t *in, int32_t in_len,
                             double *t_dev, uint32_t *id, uint16_t *flags, uint8_t *dlc,
                             uint8_t *data_len, uint8_t *data, int32_t max_frames,
                             int32_t *n_frames, int32_t *peer_close,
                             uint8_t *ping_payload, int32_t *ping_len,
                             uint32_t *crc_errors, int32_t *decode_errors);

/* Safe frame-array capacity for csb_decode_ws_to_can with in_len new bytes
 * (= (in_len + CSB_HDLC_MAX_FRAME) / 7 + 1). */
int32_t csb_max_frames(int32_t in_len);

/* ---------------------------------------------------------------- real-time helpers (1.1.0) */
/* Echo without copying: encodes frame `index` of the decode arrays (as filled by
 * csb_decode_ws_to_can / csb_can_deserialize) with a new ID, keeping IDE/FDF/BRS and the data
 * (data_len[index] bytes at data + index*64), into a WebSocket frame ready for TCP Write.
 * n_frames = the count returned by the decoder (index must be < n_frames).
 * Needs out_cap >= 174. Returns the frame length. */
int32_t csb_encode_rx_frame_to_ws(uint32_t new_id, int32_t index, int32_t n_frames,
                                  const uint16_t *flags, const uint8_t *data_len,
                                  const uint8_t *data, uint32_t mask,
                                  uint8_t *out, int32_t out_cap);

/* Call once before the real-time loop, with the two state buffers that the loop will use.
 * Runs one classic and one FD frame through the whole encode -> decode path, so that the code
 * pages, the CRC table and the state buffers are touched (no page faults later in the loop).
 * Then re-initialises both states. Returns the number of frames decoded (must be 2), or < 0. */
int32_t csb_warmup(uint8_t *ws_state, int32_t ws_state_size,
                   uint8_t *hdlc_state, int32_t hdlc_state_size);


/* ---------------------------------------------------------------- pcap CAN packets (1.2.0) */
/* pcap packet record + LINKTYPE_CAN_SOCKETCAN frame, as the NI CIF "CAN packet" (vi.lib CIF Foundation, CIF PCAP Parse and Build, Build Socket CAN.vi and
 * Parse Next Socket CAN.vi), one packet per CAN frame, packets concatenated:
 *   pcap packet header, 16 bytes: ts_sec, ts_nsec, incl_len, orig_len (U32 each; byte order =
 *     the CIF class's "OS Endian": big-endian by default, CSB_PCAP_LITTLE_ENDIAN for little);
 *     the timestamp is TAI; incl_len = orig_len = 8 + payload length
 *   LINKTYPE_CAN_SOCKETCAN header, 8 bytes: CAN ID + flags (U32, ALWAYS big-endian: id & 1FFFFFFF,
 *     | 80000000 extended, | 40000000 remote, | 20000000 error), payload length, FD flags
 *     (0 classic, 04 FD, 05 FD + BRS; ESI not set, as NI), reserved 0, len8_dlc 0
 *   payload: payload-length bytes, no padding (remote frames: length 0)
 * So one packet is 24 + payload length bytes, at most CSB_PCAP_MAX_PACKET. */
#define CSB_PCAP_MAX_PACKET    88       /* 16 + 8 + 64 */
#define CSB_PCAP_KEEP_TX_ACK   0x0001   /* option: also convert TX acknowledgements (default: skip) */
#define CSB_PCAP_KEEP_ERRORS   0x0002   /* option: also convert error frames, as SocketCAN error
                                          frames (CAN_ERR_FLAG, 8-byte payload as Linux error.h).
                                          Not for NI CIF: its parser ignores the error flag */
#define CSB_PCAP_LITTLE_ENDIAN 0x0004   /* option: pcap packet header little-endian (default big) */
#define CSB_TAI_MINUS_UTC     37       /* leap seconds since 2017-01-01 (TAI - UTC, seconds) */

/* Received frames (the decode arrays of csb_decode_ws_to_can / csb_can_deserialize, frames
 * 0 .. n_frames-1) -> concatenated pcap CAN packets in out. tai_minus_utc_s: TAI - UTC in seconds
 * (CSB_TAI_MINUS_UTC). options: CSB_PCAP_* bits (0 = skip TX acks and error frames, big-endian).
 * *n_packets = packets written; *n_skipped = frames not converted (TX acks, error frames).
 * Requirement: out_cap >= n_frames * CSB_PCAP_MAX_PACKET. Returns the number of bytes written. */
int32_t csb_frames_to_pcap(int32_t n_frames, const double *t_dev, const uint32_t *id,
                          const uint16_t *flags, const uint8_t *data_len, const uint8_t *data,
                          int32_t tai_minus_utc_s, int32_t options,
                          uint8_t *out, int32_t out_cap, int32_t *n_packets, int32_t *n_skipped);

/* Concatenated pcap CAN packets (for example from CIF, to transmit) -> concatenated WebSocket frames, ready for
 * ONE TCP Write. Encodes packets until the input ends, the next packet is incomplete, or out has
 * less than 174 bytes free. *consumed = input bytes used (keep in[*consumed ..] for the next
 * call); *n_packets = frames encoded; *n_rejected = complete packets that the CANsub cannot send
 * and that were skipped (remote or error frames, classic > 8 or FD > 64 bytes, standard ID >
 * 7FF, length fields that do not match). options: CSB_PCAP_LITTLE_ENDIAN only.
 * Returns the number of bytes written, or CSB_ERR_INVALID if incl_len is outside 8..72 (the
 * stream is corrupt: *consumed = the offset of that packet). */
int32_t csb_encode_pcap_to_ws(const uint8_t *in, int32_t in_len, int32_t options, uint32_t mask,
                             uint8_t *out, int32_t out_cap,
                             int32_t *n_packets, int32_t *n_rejected, int32_t *consumed);

/* Capacity helper: bytes needed by csb_frames_to_pcap for n_frames frames (n_frames * 88). */
int32_t csb_pcap_max_bytes(int32_t n_frames);

#ifdef __cplusplus
}
#endif
#endif /* CANSUB_CODEC_H */
