





import sys,re
rows=[tuple(float(v) for v in m.groups()) for m in (re.search(r'转向帧迹 (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d) ([\d.]+) ([\d.]+)',l) for l in open(sys.argv[1],encoding='utf-8',errors='replace')) if m]
a=int(sys.argv[2]) if len(sys.argv)>2 else 0; n=int(sys.argv[3]) if len(sys.argv)>3 else len(rows)
def scr(dx,dy): return (dx-dy)/65536,(dx+dy)/131072
for i in range(max(1,a),min(len(rows),a+n)):
    p,q=rows[i-1],rows[i]; sx,sy=scr(q[0]+q[2]-p[0]-p[2],q[1]+q[3]-p[1]-p[3]); rx,ry=scr(q[0]-p[0],q[1]-p[1])
    print('%4d 画面 %+.3f %+.3f 模 %.3f | 不平滑 %+.3f %+.3f 模 %.3f | 误差 %.3f 在走 %d 外推 %.2f'%(i,sx,sy,(sx*sx+sy*sy)**.5,rx,ry,(rx*rx+ry*ry)**.5,((q[2]/65536)**2+(q[3]/65536)**2)**.5,int(q[4]),q[5]))
