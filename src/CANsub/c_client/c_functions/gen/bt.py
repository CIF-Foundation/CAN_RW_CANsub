F=80_000_000
def lv_bit_timing(nom, nsp, data, dsp):
    if data < nom or nom <= 0 or nsp < 50 or dsp < 50: return None, "invalid input"
    best=None
    for brp in range(1, 257):
        nbt = F // (nom*brp); dbt = F // (data*brp)
        if nbt < 1 or dbt < 1: break
        if abs(F/(nbt*brp) - nom) > nom/256: continue
        if abs(F/(dbt*brp) - data) > data/256: continue
        if not (8 <= nbt <= 80 and 5 <= dbt <= 25): continue
        t1 = min(round(nsp/100*nbt) - 1, nbt - 3); t2 = nbt - t1 - 1
        d1 = min(round(dsp/100*dbt) - 1, dbt - 3); d2 = dbt - d1 - 1
        if not (2 <= t1 <= 128 and 2 <= t2 <= 32 and 1 <= d1 <= 16 and 2 <= d2 <= 8): continue
        sp = 100*(1+t1)/nbt; dsp_e = 100*(1+d1)/dbt
        if sp < 50 or dsp_e < 50: continue
        dev = abs(sp - nsp) + abs(dsp_e - dsp)
        if best is None or dev < best[0]:
            best=(dev, dict(brp=brp, seg1=t1, seg2=t2, sjw=t2, dbrp=brp, dseg1=d1, dseg2=d2, dsjw=d2, sp=sp, dsp=dsp_e))
    return (best[1], None) if best else (None, "no solution")
