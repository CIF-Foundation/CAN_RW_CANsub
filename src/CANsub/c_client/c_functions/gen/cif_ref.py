"""Python reference of NI's CIF Build Socket CAN.vi / Flatten SocketCAN Header.vi /
Build Packet Header.vi (LabVIEW 2025 vi.lib, read with lvkit 2026-11-02), used to check
csb_frames_to_cif. Input = one decoded CANsub frame (t_dev UTC seconds, id, flags, data)."""
import struct
EPOCH_2025 = 1735689600
F_IDE, F_FDF, F_RTR, F_BRS, F_TXACK, F_ERR = 0x1, 0x2, 0x4, 0x8, 0x20, 0x40

def tai_ns(t_dev, tai_minus_utc=37):
    us = round((t_dev - EPOCH_2025) * 1e6)
    return max(0, (EPOCH_2025 + tai_minus_utc) * 1_000_000_000 + us * 1000)

def ni_build(tai_time_ns, identifier, extended, mode, payload, endian=">"):
    """Exactly the NI VI: mode 0 CAN 2.0, 1 CAN FD, 2 CAN FD BRS, 3 Remote."""
    cid = identifier & 0x1FFFFFFF
    if extended: cid |= 0x80000000
    remote = mode == 3
    if remote: cid |= 0x40000000
    fd = {1: 4, 2: 5}.get(mode, 0)
    plen = 0 if remote else len(payload)
    t = max(tai_time_ns, 0)
    hdr = struct.pack(endian + "IIII", t // 10**9, t % 10**9, 8 + plen, 8 + plen)
    sc = struct.pack(">IBBBB", cid, plen, fd, 0, 0)
    return hdr + sc + (b"" if plen == 0 else bytes(payload))

def frame_to_cif(t_dev, cid, flags, data, endian=">", tai_minus_utc=37):
    if flags & F_FDF: mode = 2 if flags & F_BRS else 1
    elif flags & F_RTR: mode = 3
    else: mode = 0
    return ni_build(tai_ns(t_dev, tai_minus_utc), cid, bool(flags & F_IDE), mode, data, endian)
