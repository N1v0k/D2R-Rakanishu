

















import sys, os, json, html, base64, io
import numpy as np
from PIL import Image

DEF = dict(ssim_min=0.60, nonblack_ratio_min=0.90, cell_dl_max=0.10, cell_dc_max=0.16, region_ratio_min=0.75, region_ssim_min=0.45)
W, H = 320, 180

def load(p):
    im = Image.open(p).convert('RGB').resize((W, H), Image.BILINEAR)
    return np.asarray(im, dtype=np.float64) / 255.0

def lum(a): return a[..., 0] * 0.2126 + a[..., 1] * 0.7152 + a[..., 2] * 0.0722

def box(a, r=3):

    p = np.pad(a, r, mode='edge'); c = p.cumsum(0).cumsum(1); c = np.pad(c, ((1, 0), (1, 0)))
    k = 2 * r + 1
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)

def ssim_map(x, y):
    C1, C2 = 0.01 ** 2, 0.03 ** 2
    mx, my = box(x), box(y)
    vx, vy, cxy = box(x * x) - mx * mx, box(y * y) - my * my, box(x * y) - mx * my
    return ((2 * mx * my + C1) * (2 * cxy + C2)) / ((mx * mx + my * my + C1) * (vx + vy + C2))

def rect_px(r): return int(r[0] * W), int(r[1] * H), max(int(r[0] * W) + 1, int(r[2] * W)), max(int(r[1] * H) + 1, int(r[3] * H))

def main():
    spec_p, tp, hp, out = sys.argv[1:5]
    spec = json.load(open(spec_p, encoding='utf-8')); th = {**DEF, **spec.get('thresholds', {})}
    os.makedirs(out, exist_ok=True)
    T, Hs = load(tp), load(hp); LT, LH = lum(T), lum(Hs)
    mask = np.zeros((H, W), bool)
    for r in spec.get('mask', []):
        x0, y0, x1, y1 = rect_px(r); mask[y0:y1, x0:x1] = True




    dyn = None
    if spec.get('dynmask'):
        dm = np.asarray(Image.open(os.path.expanduser(spec['dynmask'])).convert('L').resize((W, H), Image.BOX), dtype=np.float64) > 0
        mask |= dm
        vx0_, vy0_, vx1_, vy1_ = rect_px(spec.get('viewport', [0, 0, 1, 1])); dyn = float(dm[vy0_:vy1_, vx0_:vx1_].mean())
    keep = ~mask
    S = ssim_map(LT, LH); ssim = float(S[keep].mean())
    vx0, vy0, vx1, vy1 = rect_px(spec.get('viewport', [0, 0, 1, 1]))
    nb_t = float((LT[vy0:vy1, vx0:vx1] > 0.06).mean()); nb_h = float((LH[vy0:vy1, vx0:vx1] > 0.06).mean())


    G = 8; cells = []; red_cells = 0; heat = np.zeros((G, G))
    for gy in range(G):
        for gx in range(G):
            ys, xs = slice(gy * H // G, (gy + 1) * H // G), slice(gx * W // G, (gx + 1) * W // G)
            k = keep[ys, xs]
            if k.mean() < 0.5: cells.append(None); heat[gy, gx] = -1; continue
            dl = float(abs(LT[ys, xs][k].mean() - LH[ys, xs][k].mean()))
            dc = float(np.linalg.norm(T[ys, xs][k].mean(0) - Hs[ys, xs][k].mean(0)))
            bad = dl > th['cell_dl_max'] or dc > th['cell_dc_max']; red_cells += bad
            heat[gy, gx] = max(dl / th['cell_dl_max'], dc / th['cell_dc_max']); cells.append((dl, dc, bad))


    regs = {}
    for name, r in spec.get('regions', {}).items():
        rth = {}
        if isinstance(r, dict): rth = r; r = r['rect']
        x0, y0, x1, y1 = rect_px(r)
        lt, lh = float(LT[y0:y1, x0:x1].mean()), float(LH[y0:y1, x0:x1].mean())
        ratio = lh / lt if lt > 1e-4 else (1.0 if lh < 1e-4 else 99.0)
        rs = float(S[y0:y1, x0:x1].mean())
        rmin = rth.get('ratio_min', th['region_ratio_min']); smin = rth.get('ssim_min', th['region_ssim_min'])
        ok = (rmin <= 0 or rmin <= ratio <= 1 / rmin) and rs >= smin
        regs[name] = dict(truth_lum=round(lt, 4), host_lum=round(lh, 4), ratio=round(ratio, 3), ssim=round(rs, 3), ok=ok)


    fine = None
    if spec.get('fine_grid'):
        fc, fr, fmin = spec['fine_grid']
        def load2(p):
            a = np.asarray(Image.open(p).convert('RGB').resize((640, 360), Image.BILINEAR), dtype=np.float64) / 255.0; return lum(a)
        S2 = ssim_map(load2(tp), load2(hp)); bad_cells = []
        m2 = np.zeros((360, 640), bool)
        for r in spec.get('mask', []):
            m2[int(r[1] * 360):max(int(r[1] * 360) + 1, int(r[3] * 360)), int(r[0] * 640):max(int(r[0] * 640) + 1, int(r[2] * 640))] = True
        dm2 = (np.asarray(Image.open(os.path.expanduser(spec['dynmask'])).convert('L').resize((640, 360), Image.BOX)) > 0) if spec.get('dynmask') else np.zeros((360, 640), bool)
        dmin2 = th.get('dyn_ssim_min', 0.5)
        for gy in range(fr):
            for gx in range(fc):
                ys, xs = slice(gy * 360 // fr, (gy + 1) * 360 // fr), slice(gx * 640 // fc, (gx + 1) * 640 // fc)
                k = ~m2[ys, xs]
                cx, cy = (gx + 0.5) / fc, (gy + 0.5) / fr; vp = spec.get('viewport', [0, 0, 1, 1])
                if k.mean() < 0.5 or not (vp[0] <= cx <= vp[2] and vp[1] <= cy <= vp[3]): continue
                v = float(S2[ys, xs][k].mean())
                lim = dmin2 if dm2[ys, xs].mean() >= 0.5 else fmin
                if v < lim: bad_cells.append((gx, gy, round(v, 3)))
        fine = (fc, fr, fmin, bad_cells)
    crit = [
        ('① 结构相似度', ssim >= th['ssim_min'], f'{ssim:.3f}（≥ {th["ssim_min"]}）'),
        ('② 3D 视口非黑比例', nb_h >= nb_t * th['nonblack_ratio_min'], f'宿主 {nb_h:.3f} / 真值 {nb_t:.3f}（宿主 ≥ 真值 × {th["nonblack_ratio_min"]}）'),
        ('③ 8×8 网格', red_cells == 0, f'红格 {red_cells} / {sum(c is not None for c in cells)}（|ΔL| ≤ {th["cell_dl_max"]}、色差 ≤ {th["cell_dc_max"]}）'),
        ('④ 命名区域', all(v['ok'] for v in regs.values()), '；'.join(f'{k} 亮度比 {v["ratio"]} 结构 {v["ssim"]}{"" if v["ok"] else " ✗"}' for k, v in regs.items()) or '无'),
    ]
    if dyn is not None:
        crit.append(('⑥ 动画遮罩射程', True, f'遮罩占视口 {dyn:.1%}' + ('（> 30%：严格判据失明，本页为「弱判」）' if dyn > 0.30 else '')))
    if fine:
        fc, fr, fmin, bc = fine




        fmax = int(spec.get('fine_red_max', 0))
        crit.append(('⑤ 细网格', len(bc) <= fmax, f'{fc}×{fr} 格结构 ≥ {fmin}：红格 {len(bc)}' + (f'（本页允许 ≤ {fmax}）' if fmax else '') + (f'，最差 {sorted(bc, key=lambda t: t[2])[:3]}（列, 行, 值）' if bc else '')))
    elems = []
    for e in spec.get('elements', []):
        r = regs.get(e['region']); ok = bool(r and r['ok']) if e['region'] != 'viewport' else crit[1][1]
        elems.append((e['name'], e.get('expect', '有'), ok))
    green = all(c[1] for c in crit) and all(e[2] for e in elems)


    hm = np.zeros((G, G, 3))
    for gy in range(G):
        for gx in range(G):
            v = heat[gy, gx]
            hm[gy, gx] = (0.5, 0.5, 0.5) if v < 0 else ((min(1, v), 1 - min(1, v) * 0.8, 0.1) if v < 1 else (0.9, 0.05, 0.05))
    hmi = Image.fromarray((hm * 255).astype(np.uint8)).resize((W * 2, H * 2), Image.NEAREST)
    base = Image.fromarray((Hs * 255).astype(np.uint8)).resize((W * 2, H * 2)).convert('RGB')
    over = Image.blend(base, hmi, 0.55); over.save(os.path.join(out, 'heat.png'))
    res = dict(page=spec['page'], dynmask_ratio=dyn, truth=os.path.abspath(tp), host=os.path.abspath(hp), green=green, ssim=round(ssim, 4),
               nonblack=dict(truth=round(nb_t, 4), host=round(nb_h, 4)), red_cells=red_cells, regions=regs,
               elements=[dict(name=n, expect=x, ok=o) for n, x, o in elems], thresholds=th)
    json.dump(res, open(os.path.join(out, 'result.json'), 'w'), ensure_ascii=False, indent=1)
    def b64(p, w=800):
        im = Image.open(p).convert('RGB'); im.thumbnail((w, w)); bio = io.BytesIO(); im.save(bio, 'PNG')
        return base64.b64encode(bio.getvalue()).decode()
    rows = ''.join(f'<tr><td>{html.escape(n)}</td><td class={"ok" if ok else "bad"}>{"绿" if ok else "红"}</td><td>{html.escape(d)}</td></tr>' for n, ok, d in crit)
    erows = ''.join(f'<tr><td>{html.escape(n)}</td><td>{html.escape(x)}</td><td class={"ok" if ok else "bad"}>{"✓" if ok else "✗"}</td></tr>' for n, x, ok in elems)
    page = f'''<!doctype html><meta charset=utf-8><title>画面闸门 {html.escape(spec["page"])}</title>
<style>body{{font:14px -apple-system,sans-serif;margin:16px;background:#fff;color:#111}}img{{width:100%;border:1px solid #ccc}}
.g{{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:10px}}td,th{{border:1px solid #ccc;padding:4px 8px}}table{{border-collapse:collapse}}
.ok{{color:#070;font-weight:600}}.bad{{color:#c00;font-weight:600}}h1 span{{font-size:18px}}</style>
<h1>{html.escape(spec["page"])} <span class={"ok" if green else "bad"}>{"绿" if green else "红"}</span></h1>
<div class=g><div><b>真值</b><img src="data:image/png;base64,{b64(tp)}"></div><div><b>宿主</b><img src="data:image/png;base64,{b64(hp)}"></div>
<div><b>差异热图（叠在宿主上；红 = 超阈值格，灰 = 遮罩）</b><img src="data:image/png;base64,{b64(os.path.join(out, "heat.png"))}"></div></div>
<h2>判据</h2><table><tr><th>判据</th><th>结论</th><th>读数</th></tr>{rows}</table>
<h2>应有元素</h2><table><tr><th>元素</th><th>应</th><th>结论</th></tr>{erows}</table>
<p>真值 {html.escape(os.path.abspath(tp))}<br>宿主 {html.escape(os.path.abspath(hp))}</p>'''
    open(os.path.join(out, 'report.html'), 'w', encoding='utf-8').write(page)
    print(f'SHOTGATE {spec["page"]} {"OK" if green else "RED"} 结构 {ssim:.3f} 非黑 宿主 {nb_h:.3f}/真值 {nb_t:.3f} 红格 {red_cells} '
          + ' '.join(f'{k}:{v["ratio"]}/{v["ssim"]}{"" if v["ok"] else "✗"}' for k, v in regs.items()) + (f' 细网格红格 {len(fine[3])}' if fine else '') + (f' 动画遮罩 {dyn:.1%}' + ('【弱判】' if dyn > 0.30 else '') if dyn is not None else '') + f' → {out}/report.html')
    sys.exit(0 if green else 1)

if __name__ == '__main__': main()
