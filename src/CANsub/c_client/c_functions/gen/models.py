import zlib
def subset(a,i,n): i=max(i,0); return a[i:i+max(n,0)]
def index(a,i): return a[i] if 0<=i<len(a) else 0
def ube(b,n): return int.from_bytes(bytes(b[:n]),"big") if len(b)>=n else 0
M=0xFFFFFFFF
T=[]
for i in range(256):
    c=i
    for _ in range(8): c=(c>>1)^0xEDB88320 if c&1 else c>>1
    T.append(c)
def CRC32(d):
    c=M
    for b in d: c=(c>>8)^T[(b^c)&0xFF]
    return c^M
def HDLC_Encode(p):
    raw=list(p)+list(CRC32(p).to_bytes(4,"big")); out=[]
    for b in raw: out+= [0x7D,0x5E] if b==0x7E else [0x7D,0x5D] if b==0x7D else [b]
    return [0x7E]+out+[0x7E]
def HDLC_Decode(bs,st):
    buf,inf,esc,err=st; out=[]
    for b in bs:
        if b==0x7E:
            if inf and len(buf)>=5:
                d,c=buf[:-4],buf[-4:]
                if CRC32(d)==ube(c,4): out.append(d)
                else: err+=1
            buf,inf,esc=[],True,False
        elif b==0x7D: esc=True
        else: buf=buf+[b^0x20 if esc else b]; esc=False
    return out,(buf,inf,esc,err)
DLC=[0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64]
def CAN_Deserialize(payload):
    if not payload: return [],0
    off=0; fr=[]
    while True:
        rem=len(payload)-off; hdr=subset(payload,off,11)
        t=ube([0,0]+subset(hdr,0,6),8)/1e6+1735689600
        f=index(hdr,6); fdf,b6,b5,tx=bool(f&0x80),bool(f&0x40),bool(f&0x20),bool(f&0x10); dlc=f&0xF
        if b5 and not fdf: ln=7; x=dict(t_dev=t,error=True,errtype=f&0x1F)
        else:
            b7=index(hdr,7); ide=bool(b7&0x80)
            if ide: cid=ube(subset(hdr,7,4),4)&0x1FFFFFFF; hl=11
            else: cid=ube(subset(hdr,7,2),2)&0x7FF; hl=9
            rtr=b6 and not fdf; n=0 if rtr else DLC[dlc]; ln=hl+n
            x=dict(t_dev=t,id=cid,ide=ide,fdf=fdf,rtr=rtr,brs=b6 and fdf,esi=b5 and fdf,tx_ack=tx,error=False,dlc=dlc,data=subset(payload,off+hl,n))
        ok=rem>=ln
        if ok: fr.append(x); off+=ln
        if not ok or off>=len(payload): break
    return fr,(0 if ok else 1)
def CAN_Serialize_FD(cid, ide, fdf, brs, data):
    data=list(data)
    if fdf:
        n=min(len(data),64); k=next(i for i,v in enumerate(DLC) if v>=n); L=DLC[k]
        d=data[:n]+[0]*(L-n); flags=0x80|(0x40 if brs else 0)|k
    else:
        n=min(len(data),8); d=data[:n]; flags=n
    idb=list(((cid|0x80000000)&M).to_bytes(4,"big")) if ide else list((cid&0x7FF).to_bytes(2,"big"))
    return [0]*6+[flags]+idb+d
MASK=[0x12,0x34,0x56,0x78]
def WS_BuildFrame(p,op=2):
    n=len(p); hdr=[0x80|op,0x80|n] if n<126 else [0x80|op,0xFE]+list(n.to_bytes(2,"big"))
    return hdr+MASK+[b^MASK[i&3] for i,b in enumerate(p)]
def WS_ParseFrames(bs,buf):
    buf=buf+list(bs); fr=[]
    while True:
        size=len(buf); op=index(buf,0)&0xF; l7=index(buf,1)&0x7F
        if l7==126: h=4; n=ube(subset(buf,2,2),2)
        elif l7==127: h=10; n=ube(subset(buf,2,8),8)
        else: h=2; n=l7
        have=size>=2 and size>=h+n
        if have: fr.append((op,subset(buf,h,n))); buf=subset(buf,h+n,size)
        else: break
    return fr,buf
X=lambda a:' '.join('%02X'%b for b in a)
H=lambda s:[int(x,16) for x in s.split()]
