





import os,sys,numpy as np
from PIL import Image, ImageFilter
d=sys.argv[1]; L=open(d+'/meta.txt').read().split('\n'); n=int(L[0].split()[-1]); W=int(L[0].split()[1]); L=L[1:n+1]
P=[(float(l.split()[8]),float(l.split()[10])) for l in L] if len(L[0].split())>8 else None
def hp(k):
    a=np.frombuffer(open(f'{d}/f{k:03d}.raw','rb').read(),dtype=np.uint8).reshape(W,W,4)[:,:,:3].mean(2)
    im=Image.fromarray(a.astype(np.uint8)); b=np.asarray(im.filter(ImageFilter.GaussianBlur(6)),dtype=float); return a-b
H=[hp(k) for k in range(n)]; c=W//2; R=14
R=int(os.environ.get("R","14")); mask=np.ones((W,W),bool); mask[c-160:c+120,c-120:c+120]=False; mask[:R+1]=mask[-R-1:]=False; mask[:,:R+1]=mask[:,-R-1:]=False
ys,xs=np.nonzero(mask)
out=[]
for k in range(n-1):
    a=H[k][ys,xs]; best=None
    for dy in range(-R,R+1):
        for dx in range(-R,R+1):
            v=np.abs(a-H[k+1][ys+dy,xs+dx]).mean()
            if best is None or v<best[0]: best=(v,dx,dy)
    out.append((best[1],best[2]))
for k,(dx,dy) in enumerate(out):
    lp='' if not P else ' 逻辑位移 %+.2f,%+.2f'%(P[k+1][0]-P[k][0],P[k+1][1]-P[k][1])
    print(f'{k:3d}→{k+1:3d} 画面位移 {dx:+d},{dy:+d}{lp}')
