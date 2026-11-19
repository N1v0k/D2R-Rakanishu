#!/usr/bin/env python3














import sys,os,re,subprocess,time
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
MODE=sys.argv[1]; ONLY=sys.argv[2:]
VIEW={'iphone':(667,375,0,0),'ipad':(1366,768,0,0),'ipadfull':(1366,768,0,0)}[MODE]
SCREEN=None
if os.environ.get("UIREG_SCREEN") and MODE!="iphone": SCREEN=tuple(float(v) for v in os.environ["UIREG_SCREEN"].split("x")); VIEW=(SCREEN[0],SCREEN[0]*9/16,0,0)
SHOT_Y={}
SHOT_BAD=[]
def shot_ys(log):
    SHOT_Y.clear(); cur=None
    SHOT_BAD[:]=[n for n in re.findall(r"已截 (\S+?\.png)",log) if not re.match(r"[a-z]\d+_\S",n)]
    for m in re.finditer(r"游戏视图摆到 y (\d+) 高 \d+（窗口 (\d+)x|已截 [a-z]\d+_(\S+?\.png)",log):
        if m.group(1): cur=(int(m.group(1)),int(m.group(2)))
        elif cur: SHOT_Y[m.group(3)]=cur
def shot_geom(W,H,path=None):
    if MODE=="iphone": k=min(W/667.0,H/375.0); return k,(W-667*k)/2,0
    if not SCREEN: k,ox,oy=W/667.0,0,(H-W*9/16)*(1 if MODE=="ipadfull" else .5)
    else: pp=W/1366.0; dm,sh=SCREEN; k,ox,oy=pp*dm/667.0, pp*(1366-dm)/2, pp*((1024-sh)/2+(sh-dm*9/16)*(1 if MODE=="ipadfull" else .5))
    v=SHOT_Y.get(os.path.basename(path)) if path else None
    return k,ox,(v[0]*W/v[1] if v else oy)
K=VIEW[0]/667.0


def P(x,y): return f"{x*K+VIEW[2]:.1f},{y*K+VIEW[3]:.1f}"
def O(dx,dy): return f"{dx*K:.0f},{dy*K:.0f}"
BASE="DM_SAVE_FRESH=warlock DM_GAME_SEED=0x12345678 DM_FIXED_TIME=1790000000 DM_C2S_LOG=pwc2s.jsonl DM_WORLD_DUMP=1 DM_LAT_LOG=1"+(" DM_IPAD_FULL=1" if MODE=="ipadfull" else " DM_IPAD_FULL=0" if MODE=="ipad" else "")+(f" DM_TEST_SCREEN={SCREEN[0]:.0f}x{SCREEN[1]:.0f}" if SCREEN else "")














OLD_SPAWN={
 "n":"点阿卡拉的屏幕坐标","p":"点阿卡拉的屏幕坐标","b":"点瓦瑞夫 / 恰西的屏幕坐标","top":"iPad 档：开箱子、走到阿卡拉旁的点位","i":"iPad 档：点卡夏雇佣菜单的点位","pg2":"点箱子","rw2":"点箱子","vt1":"点箱子","vf1":"开箱","v":"建角后点箱子",
 "rw1":"长按物品栏里的软帽后清单落点","j":"背包拖放的落点（物品给在脚下的次序）","k":"背包拖放的落点","pk":"箱子选择框边缘的屏幕坐标","g4":"点凯恩的屏幕坐标","kbb":"主钮连点要怪在跟前",
}
if os.environ.get("UIREG_EXTRA_ENV"): BASE+=" "+os.environ["UIREG_EXTRA_ENV"].strip()
SAVE_TIME_MIN=1800000000
WHO_ROW={1:"joe",2:"Warlock",3:"joebao",4:"法师"}
FORBID=os.environ.get("UIREG_FORBID")
REQUIRE=os.environ.get("UIREG_REQUIRE")


LOGRED=r"【红】|FATAL|看门狗|MTLCommandBufferStatus(Error|Fault)|CommandBufferCallbackError|Discarded \(victim|GPU (hang|error|fault|recovery)|GPU Hang|管线.*失败|[Pp]ipeline.*fail"
HERE=os.path.dirname(os.path.abspath(__file__))
RUNNER=os.environ.get("UIREG_RUNNER") or os.path.join(HERE,"run_mac.sh")
OUTBASE=(os.environ.get("UIREG_OUT") or os.path.join(DM_ROOT, "run/uireg"))
OUT=f"{OUTBASE}/tables/{MODE}_{time.strftime('%m%d_%H%M')}"; os.makedirs(OUT,exist_ok=True)


MENU={"talk2":O(0,-24),"cancel2":O(0,13),"trade3":O(8,12),"kashya2":O(8,12),"akara2":O(8,12),"trade3m":O(8,12)} if MODE!="ipadfull" else {"akara2":"0,18","trade3m":"16,38","talk2":"6,6","cancel2":"6,83","trade3":"16,80","kashya2":"0,-46"}
IPAD=MODE.startswith("ipad")
LETTERBOX=MODE=="ipad"
class G:
    def __init__(s,name,env="",save=""): s.name=name; s.run=name; s.env=env; s.save=save; s.who=None; s.padsel=False; s.t=3.5; s.L=[]; s.items=[]; s.n=0; s.cut=None
    def sect(s,name,gap=1.0,skip=False):
        if s.cut: s.L=s.L[:s.cut[0]]; s.items=s.items[:s.cut[1]]; s.t=s.cut[2]; s.cut=None
        if skip: s.cut=(len(s.L),len(s.items),s.t)
        s.name=name; s.n=0; s.t+=gap
    def at(s,line,dt=0.0): s.L.append(f"{s.t:.2f} {line}"); s.t+=dt


    def tap(s,spec,dt=1.0): s.at(f"down 0 {spec}"); s.t+=.12; s.at("up 0"); s.t+=dt-.12
    def drag(s,a,b,hold=.3): s.at(f"down 0 {a}"); s.t+=.04; s.at(f"move 0 {b}"); s.t+=hold; s.at("up 0"); s.t+=1.2
    def wait(s,dt): s.t+=dt
    def ui(s,name,dt=1.0): s.at(f"tap_ui {name} 3",dt)
    def dragp(s,a,b,pause=.3):
        ax,ay=a; bx,by=b; s.at(f"down 0 {P(ax,ay)}",pause); s.at(f"move 0 {P((ax+bx)/2,(ay+by)/2)}",.3); s.at(f"move 0 {P(bx,by)}",.4); s.at("up 0",1.0)
    def clr(s): s.tap(P(502.5 if s.run in WIDE else 464,246.5),.7)
    def dbl(s,x,y): s.tap(P(x,y),.25); s.tap(P(x,y),1.0)
    def fast(s,a,b):
        s.at(f"down 0 {P(*a)}",.04); s.at(f"move 0 {P(*b)}",.01); s.at("up 0",1.6)
    def hold(s,x,y,sec=1.3): s.at(f"down 0 {P(x,y)}",sec); s.at("up 0",1.0)
    def item(s,title,expect,pat,neg=None):
        s.at(f"note 项{len(s.items)}"); s.items.append([title,expect,pat,neg,"",s.name]); s.t+=.05
    def shot(s): n=f"{s.name}_{s.n:02d}.png"; s.n+=1; s.at(f"shot {n}",.3); s.items[-1][4]=n
    def enter(s,row=2):
        if not s.save and not s.who: s.who=WHO_ROW.get(row)
        if row!=1: s.ui(f"ListItem{row-1}@CharacterSelectPanel",1.3)
        s.tap(P(333,336),1.6)
        if row!=2: s.tap(P(333,157),1.0)
        s.at("await pos"); s.t=max(s.t,13.0); s.at("note 进局",1.5)
    def arrive(s,x,y,r,upto): s.at(f"await pos {x} {y} {r}"); s.t=max(s.t,upto); s.at("note 到位",1.0)










BIND_COLS=(275,298,321,344,368); BIND_ROWS=(202,235); BIND_TOP=(333,167)
def bind_cells(path):
    try: from PIL import Image
    except ImportError: return None
    import math
    im=Image.open(path).convert("L"); W,H=im.size; k,ox,oy=shot_geom(W,H,path)
    def sig(x,y):
        cx,cy,h=x*k+ox,y*k+oy,7*k; v=list(im.crop((int(cx-h),int(cy-h),int(cx+h),int(cy+h))).resize((12,12)).tobytes())
        m=sum(v)/len(v); v=[a-m for a in v]; n=math.sqrt(sum(a*a for a in v)); return ([a/(n or 1) for a in v], n/12)
    ts,tn=sig(*BIND_TOP)
    if tn<20: return None
    on=set()
    for r,y in enumerate(BIND_ROWS,1):
        for c,x in enumerate(BIND_COLS,1):
            v,n=sig(x,y)
            if n>20 and sum(a*b for a,b in zip(ts,v))>0.6: on.add((r,c))
    return on
def bind_check(d,prev,cur,cell):
    a=bind_cells(f"{d}/{prev}"); b=bind_cells(f"{d}/{cur}")
    if a is None or b is None: return "未判：读不出绑定框（本机没有 PIL，或框没开 / 画面档位不是这套坐标）"
    fmt=lambda S: "、".join(f"{r} 排 {c} 列" for r,c in sorted(S)) or "没有"
    if cell not in b: return f"不通：点的是 {cell[0]} 排 {cell[1]} 列，技能却在 {fmt(b)}（点之前在 {fmt(a)}）"
    extra=b-a-{cell}
    if extra: return f"不通：技能到了点的那格，但 {fmt(extra)} 也新出现了它"
    return f"通：技能在 {fmt(b)}（点之前在 {fmt(a)}）"
def spot_filled(path,x,y):
    try: from PIL import Image, ImageStat
    except ImportError: return None
    im=Image.open(path).convert("L"); W,H=im.size; k,ox,oy=shot_geom(W,H,path); cx,cy,h=x*k+ox,y*k+oy,6*k
    return ImageStat.Stat(im.crop((int(cx-h),int(cy-h),int(cx+h),int(cy+h)))).stddev[0]>15
INV_X0,INV_DX,INV_Y0,INV_DY=386.0,19.55,227.0,19.5
def inv_count(path):
    n=0
    for r in range(4):
        for k in range(10):
            f=spot_filled(path,INV_X0+INV_DX*k,INV_Y0+INV_DY*r)
            if f is None: return None
            n+=f
    return n
def item_changes(d,cls):
    import json
    try: rows=[json.loads(l) for l in open(f"{d}/state.jsonl") if l.strip()]
    except (OSError,ValueError): return None
    iid=None; seq=[]
    for r in rows:
        its={i["id"]:i for i in r.get("items",[])}
        if iid is None:
            c=[i for i in r.get("items",[]) if i["cls"]==cls and i["mode"]==0 and i["page"]==0]
            if not c: continue
            iid=c[0]["id"]
        i=its.get(iid); st=(i["mode"],i["page"],i["x"],i["y"]) if i else (-1,0,0,0)
        if not seq or seq[-1][1]!=st: seq.append((r["tick"],st))
    return seq
def stable_segs(d,cls,thr=50):


    seq=item_changes(d,cls)
    if seq is None: return None
    out=[]
    for k,(t,st) in enumerate(seq):
        dur=(seq[k+1][0]-t) if k+1<len(seq) else 10**6
        if dur>=thr: out.append(st+(dur,t))
    return out
def held_segs(d,thr=50):
    import json
    try: rows=[json.loads(l) for l in open(f"{d}/state.jsonl") if l.strip()]
    except (OSError,ValueError): return None
    cur={}; out=[]
    for r in rows:
        now={i["id"]:i["cls"] for i in r.get("items",[]) if i["mode"]==4}
        for k in list(cur):
            if k not in now: c,t0=cur.pop(k); (r["tick"]-t0>=thr) and out.append((k,c,t0,r["tick"]-t0))
        for k,c in now.items(): cur.setdefault(k,(c,r["tick"]))
    for k,(c,t0) in cur.items(): out.append((k,c,t0,10**6))
    return sorted(out,key=lambda x:x[2])
def seg_txt(sg): return ({0:"背包",3:"魔盒",4:"箱子"}.get(sg[1],f"页 {sg[1]}") if sg[0]==0 else {4:"手上 / 槽里",-1:"没了",1:"身上",2:"腰带"}.get(sg[0],f"mode {sg[0]}"))+(f" 格 ({sg[2]},{sg[3]})" if sg[0]==0 else "")
def mana(txt):
    m=re.findall(r"法力 (\d+)/\d+",txt); return [int(x) for x in m]
def cast(s,before):
    b=mana(before); a=mana(s)
    if not b or not a: return "未判：没有法力读数"
    return f"通：法力 {b[-1]} → {min(a)}" if min(a)<b[-1] else f"不通：法力没降（{b[-1]} → {min(a)}）"
def nocast(s,before):
    b=mana(before); a=mana(s)
    if "点空技能位" in s: return "通：开技能树（点空技能位）"
    if not b: return "未判：没有法力读数"
    return f"不通：法力降了（{b[-1]} → {min(a)}）" if a and min(a)<b[-1] else "通：没有施放（法力没降）"
STATPAT=r"DM_TEST_STAT_LOG：拍 \d+[^\n]*? 14=(\d+)"
def gold(chg,neg=None):
    def f(s,before):
        b=re.findall(STATPAT,before); a=re.findall(STATPAT,s)
        if not b: return "未判：动作前没有金币读数"
        g0=int(b[-1]); g1=int(a[-1]) if a else g0; d=g1-g0
        r=("通：" if ((d>0) if chg=='+' else (d<0) if chg=='-' else d==chg) else "不通：")+f"金币 {g0} → {g1}"
        m=[int(x) for x in re.findall(r"卖手上的物品[^\n]*卖价 (-?\d+)[^\n]*→ 1（?",s)]
        if chg=='+' and m and d!=sum(m): r=f"不通：金币差 {d} 不等于游戏给的卖价 {m}；"+r.split("：",1)[1]
        if neg and re.search(neg,s): r="不通：出现「"+re.search(neg,s).group(0)[:20]+"」；"+r.split("：",1)[1]
        return r
    return f
def php(chg):
    def f(s,before):
        pat=r"dm_hud：区域 [1-9]\d* [^\n]*生命 (\d+)/"; b=re.findall(pat,before); a=re.findall(pat,s)
        if not b: return "未判：动作前没有生命读数"
        h0=int(b[-1]); h1=int(a[-1]) if a else h0
        return ("通：" if ((h1>h0) if chg=='+' else h1==h0) else "不通：")+f"玩家生命 {h0} → {h1}"
    return f
def dur(want,cap=86):
    def f(s,before):
        a=re.findall(r"DM_TEST_DUR：[^\n]*耐久 (\d+)/%d\b"%cap,before+s)
        if not a: return "未判：没有耐久读数（包里没有 DM_TEST_DUR，看截图说明框）"
        return ("通：" if int(a[-1])==want else "不通：")+f"盾耐久 {a[-1]}/{cap}"
    return f
def merc(chg):
    def f(s,before):
        pat=r"DM_TEST_MERC_HP：[^\n]*生命 (\d+)/(\d+)"; b=re.findall(pat,before); a=re.findall(pat,s)
        if not b: return "未判：没有佣兵生命读数（包里没有 DM_TEST_MERC_HP）"
        h0=int(b[-1][0]); h1=int(a[-1][0]) if a else h0
        return ("通：" if ((h1>h0) if chg=='+' else h1==h0) else "不通：")+f"佣兵生命 {h0} → {h1}（上限 {b[-1][1]}）"
    return f
def belt(chg):
    def f(s,before):
        pat=r"腰带 (\d+)/(\d+)/(\d+)/(\d+)"; b=re.findall(pat,before); a=re.findall(pat,s)
        if not b: return "未判：动作前没有腰带读数"
        n0=sum(map(int,b[-1])); last=a[-1] if a else b[-1]; n1=sum(map(int,last))
        return ("通：" if ((n1>n0) if chg=='+' else (n1<n0) if chg=='-' else n1==n0) else "不通：")+f"腰带 {'/'.join(b[-1])} → {'/'.join(last)}"
    return f
def merge(*rs):
    st="不通" if any(r.startswith("不通") for r in rs) else "未判" if any(r.startswith("未判") for r in rs) else "通"
    return st+"："+"；".join(r.split("：",1)[1] for r in rs)
def both(*fs): return lambda s,before: merge(*[f(s,before) for f in fs])
def two_names(s,before):
    L=re.findall(r"随机名 → 名字框「([^」]*)」([^\n]*)",before+s); fin=[n for n,t in L if "再取一个" not in t]
    if any("【红】" in t for n,t in L): return "不通：试满次数仍重名"
    if len(fin)!=2: return f"不通：最终名字应有 2 个，读到 {fin}"
    return ("通：" if fin[0]!=fin[1] and all(fin) else "不通：")+f"两个名字「{fin[0]}」「{fin[1]}」"+("（第二次先取到重名、再取过）" if len(L)>2 else "")
def menu(k,res=None):
    def f(s,before):
        bad=re.search(r"补挪|列表结构对不上|不按确认",s)
        if bad: return "不通：出现「"+bad.group(0)+"」"
        if "tap_ui 超时" in s or re.search(r"tap_ui[^\n]*(没找到|超时)",s): return "不通：按控件名没点到这一行（菜单没开？）"
        m=re.search(r"界面对话菜单：(\d+) 行，点中第 (\d+) 行[^\n]*选中行 (\d+)",s); got=bool(res is None or re.search(res,s))
        if not m: return ("通：" if got and res else "未判：" if got else "不通：")+"没有「点中第 N 行」日志（旧包不打）"+("，结果消息对" if got and res else "" if got else "，也没有期望的结果消息")
        ok=int(m.group(2))==k
        return ("通：" if ok and got else "不通：")+f"{m.group(1)} 行菜单，点中第 {m.group(2)} 行（点之前停在第 {m.group(3)} 行）"+("" if got else "；没有期望的结果消息")
    return f
def edge_idle(s,before):
    if not re.search(r"\[script\] move slot=1[^\n]*lx=0 ly=0",s): return "不通：move 那行的轴不是 0"
    return stay(s,before)
def moved(s,before):
    p=re.findall(r"pos 区域 \d+ \((\d+),(\d+)\)",s)
    if len(p)<2: return "未判：没有两条坐标读数"
    return f"通：{p[0]} → {p[-1]}" if p[0]!=p[-1] else f"不通：坐标没变 {p[0]}"
def stay(s,before):
    p=re.findall(r"pos 区域 (\d+ \(\d+,\d+\))",s)
    if len(p)<2: return "未判：没有两条坐标读数"
    return f"通：坐标不变 {p[0]}" if p[0]==p[-1] else f"不通：{p[0]} → {p[-1]}"




WIDE=("c","d","t","u","s","sa")
def run(g):
    global INV_X0; INV_X0=424.5 if g.run in WIDE else 386.0
    script="\n".join(g.L)+f"\n{g.t+1.0:.2f} note 结束\n"; cap=int(g.t)+6; tag=f"{os.environ.get('UIREG_TAG') or 'ur'}_{MODE}_{g.run}"




    base=BASE if not g.save else BASE.replace('DM_SAVE_FRESH=warlock ','')
    if g.padsel:
        base=re.sub(r" ?DM_FIXED_TIME=\d+","",base); ft=re.search(r"DM_FIXED_TIME=(\d+)",g.env)
        if ft and int(ft.group(1))<SAVE_TIME_MIN: raise SystemExit(f"局 {g.run}：手柄选人的局不许带早于存档时间的固定时钟（DM_FIXED_TIME={ft.group(1)} < {SAVE_TIME_MIN}）")
    env=dict(os.environ,UIREG_MODE=MODE,UIREG_OUT=OUTBASE,PW_SAVE=g.save,PW_ENV=f"{base}{' DM_TOWNWP_OFF=1' if g.run in OLD_SPAWN else ''} {g.env}".strip())
    t0=time.time(); subprocess.run(["bash",RUNNER,tag,str(cap)],input=script.encode(),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    d=f"{OUTBASE}/pw/{tag}"; wall=time.time()-t0
    try: log=open(f"{d}/boot.txt",errors='replace').read()
    except OSError: log=""
    shot_ys(log)
    parts=re.split(r"\[script\] note 项(\d+)",log); seg={int(parts[i]):parts[i+1] for i in range(1,len(parts)-1,2)}
    rows=[]
    for i,it in enumerate(g.items):
        title,expect,pat,neg=it[:4]; shot=it[4]; s=seg.get(i,"")
        if neg=='late': s=s+seg.get(i+1,''); neg=None; it[3]=None
        before="".join(seg.get(j,"") for j in range(i)) if i else parts[0]
        if isinstance(pat,tuple) and pat and pat[0] in ("seg","segnot","segpage","bag","still"):
            def cls_of(c):
                if isinstance(c,int): return c
                m=re.search(r"DM_TEST_GIVE：[^\n]*生成「"+c+r"」[^\n]*物品类号 (\d+)",log); return int(m.group(1)) if m else -1
            pat=tuple(cls_of(x) if k in (1,3) and isinstance(x,str) and pat[0]=="still" or (k==1 and isinstance(x,str)) else x for k,x in enumerate(pat))
        if i not in seg: res="没执行到"
        elif callable(pat): res=pat(s,parts[0]+before)
        elif isinstance(pat,tuple) and pat[0]=="spot":
            try: f=spot_filled(f"{d}/{shot}",*pat[2])
            except OSError: f=None
            res="未判：读不了截图" if f is None else ("通：" if f==(pat[1]=="filled") else "不通：")+f"({pat[2][0]},{pat[2][1]}) 那一格{'有东西' if f else '是空的'}"
        elif isinstance(pat,tuple) and pat[0]=="inv":
            try: n0=inv_count(f"{d}/{g.items[i-1][4]}"); n1=inv_count(f"{d}/{shot}")
            except OSError: n0=n1=None
            res="未判：读不了截图" if n0 is None or n1 is None else ("通：" if (n1<n0 if pat[1]=='-' else n1>n0 if pat[1]=='+' else n1-n0==pat[1]) else "不通：")+f"背包占格 {n0} → {n1}"
            if len(pat)>2 and callable(pat[2]): res=merge(res,pat[2](s,parts[0]+before))
            elif len(pat)>2 and not re.search(pat[2],s): res="不通：这一步的触摸没有走到物品手势（日志里没有「"+pat[2]+"」"+("，走的是「界面鼠标：点击」" if "界面鼠标：点击" in s else "")+"）；"+res
        elif isinstance(pat,tuple) and pat and pat[0]=="still":
            sa=stable_segs(d,pat[1]); cb=item_changes(d,pat[3])
            if sa is None or cb is None: res="未判：没有物品表状态日志"
            elif len(sa)<=pat[2]: res=f"不通：前一件物品没有第 {pat[2]} 个停留段"
            else:
                t0=sa[pat[2]][5]; t1=t0+sa[pat[2]][4]; hit=[(t,st) for t,st in cb if t0<t<t1]
                res=("通：" if not hit and sa[pat[2]][0]==4 else "不通：")+(f"槽里那件拍 {t0} 起一直在槽里；另一件这期间没动过" if not hit else f"另一件在拍 {hit[0][0]} 动了（→ {seg_txt(hit[0][1]+(0,0))}）")
                if len(pat)>4 and not re.search(pat[4],s): res="不通：没有期望的日志「"+pat[4]+"」；"+res.split("：",1)[1]
        elif isinstance(pat,tuple) and pat[0]=="held":
            hs=held_segs(d); txt="、".join(f"类号 {c} 自拍 {t0} 起{'一直在' if du>=10**6 else f'在了 {du} 拍'}" for k,c,t0,du in hs) if hs else "没有"
            if hs is None: res="未判：没有物品表状态日志"
            elif len(hs)<=pat[1]: res=f"不通：没有第 {pat[1]} 次进槽；进过槽的：{txt}"
            else:
                k,c,t0,du=hs[pat[1]]; ended="ended" in pat; ok=(du<10**6) if ended else True
                res=("通：" if ok else "不通：")+f"类号 {c} 的物品自拍 {t0} 起在槽里"+(f"，{du} 拍后离开" if du<10**6 else "，到局末还在")
            extra=[x for x in pat[2:] if isinstance(x,str) and x!="ended"]
            if extra and not re.search(extra[0],s): res="不通：没有期望的日志「"+extra[0]+"」；"+res.split("：",1)[1]
        elif isinstance(pat,tuple) and pat[0]=="segpage":
            sg=stable_segs(d,pat[1],thr=30); allt="、".join(seg_txt(x) for x in sg) if sg else "没有"
            if sg is None: res="未判：没有物品表状态日志"
            elif len(sg)<=pat[2]: res=f"不通：这一步之后物品没有新的停留段（应是第 {pat[2]} 段）；各停留段：{allt}"
            else: x=sg[pat[2]]; res=("通：" if x[0]==0 and x[1]==pat[3] else "不通：")+f"第 {pat[2]} 段 {seg_txt(x)}"+("" if x[0]==0 and x[1]==pat[3] else f"；各停留段：{allt}")
        elif isinstance(pat,tuple) and pat[0] in ("seg","segnot"):
            sg=stable_segs(d,pat[1]); allt="、".join(seg_txt(x) for x in sg) if sg else "没有"
            if sg is None: res="未判：没有物品表状态日志（局里要带 DM_STATE=pwstate.jsonl）"
            elif pat[0]=="segnot": res=("通：" if all(x[0]!=pat[2] for x in sg) and sg else "不通：")+f"各停留段：{allt}"
            elif len(sg)<=pat[2]: res=f"不通：这一步之后物品没有新的停留段（应是第 {pat[2]} 段）；各停留段：{allt}"
            else:
                same=len(pat)>4 and pat[4]=="same"; mv=len(pat)>4 and pat[4]=="moved"; x=sg[pat[2]]; ok=x[0]==pat[3] and (not same or x[:4]==sg[pat[2]-1][:4]) and (not mv or x[:4]!=sg[pat[2]-1][:4])
                res=("通：" if ok else "不通：")+f"第 {pat[2]} 段 {seg_txt(x)}"+("（与上一段同格）" if same and ok else "")+("" if ok else f"；各停留段：{allt}")
            if len(pat)>5-(pat[0]=="segnot")*2 and isinstance(pat[-1],str) and pat[-1] not in ("same","moved") and not re.search(pat[-1],s): res="不通：没有期望的日志「"+pat[-1]+"」；"+res.split("：",1)[1]
        elif isinstance(pat,tuple) and pat[0]=="bag":
            import json
            try: last=[json.loads(l) for l in open(f"{d}/state.jsonl") if l.strip()][-1]; hit=[i for i in last["items"] if i["cls"]==pat[1] and i["mode"]==0 and i["page"]==0]
            except (OSError,ValueError,IndexError): hit=None
            res="未判：没有物品表状态日志" if hit is None else ("通：" if hit else "不通：")+(f"背包里有类号 {pat[1]} 的物品 id {hit[0]['id']}（格 {hit[0]['x']},{hit[0]['y']}）" if hit else f"局末背包里没有类号 {pat[1]} 的物品")
        elif isinstance(pat,tuple) and pat[0]=="cellhas":
            try: b=bind_cells(f"{d}/{shot}")
            except OSError: b=None
            res="未判：读不出绑定框" if b is None else (f"通：技能在 {pat[1][0]} 排 {pat[1][1]} 列" if pat[1] in b else "不通：技能不在 %d 排 %d 列，读到的是 %s"%(pat[1][0],pat[1][1],sorted(b)))
        elif isinstance(pat,tuple) and pat[0]=="cells":
            try: res=bind_check(d,pat[1],shot,pat[2])
            except OSError as e: res=f"不通：读不到截图（{e.filename}）"
        elif pat is None: res="未判：只留截图，没设判据"
        else:
            m=re.search(pat,s); bad=re.search(neg,s) if neg else None
            res=("通：" + m.group(0)[:60]) if (m and not bad) else ("不通：" + ("没等到期望的日志" if not m else "出现 "+bad.group(0)[:40]))
        blocked=re.search(r"确认 / 丢弃键不放行[^\n]*",s)
        if blocked and "数量框" not in title: res="不通：宿主拦了这一下——"+blocked.group(0)[:30]+("（正则本来判通）" if res.startswith("通") else "")
        rows.append((it[5],title,expect,res,f"{tag}/{shot}" if shot else ""))
    red=len(re.findall(LOGRED,log))
    if SHOT_BAD: rows.append((g.run,"（整局）不该出现认不出局号的截图名","日志里每张截图都认得出局号前缀（字母 + 数字 + 下划线）",f"不通：【红】认不出局号的截图 {len(SHOT_BAD)} 张（如 {SHOT_BAD[0]}）——取不到纵向偏移，这一局读截图的判据不可信",""))




    n0=len(re.findall(r"世界目标：路线 0 段（找到 0、搜过 1 格",log))
    if n0: rows.append((g.run,"（整局）不该出现一步都展不开的规划","日志里没有「世界目标：路线 0 段（找到 0、搜过 1 格」",f"不通：出现 {n0} 次——起点被当成障碍，点了走不了",""))
    if FORBID:
        m=re.search(FORBID,log); rows.append((g.run,"（整局）不该出现的日志",f"没有「{FORBID}」",("不通：出现 "+m.group(0)[:60]) if m else "通：没有",""))
    if REQUIRE:
        n=len(re.findall(REQUIRE,log)); rows.append((g.run,"（整局）该出现的日志",f"有「{REQUIRE}」",(f"通：出现 {n} 次" if n else "不通：一次都没有"),""))
    try:
        import json; ops=[json.loads(l)["pkt"][:2] for l in open(f"{d}/c2s.jsonl") if '"unit": 1' in l or '"unit":1' in l]
        casts=sum(1 for o in ops if o in ("05","06","07","08","09","0a","0c","0d","0e","0f","10","11"))
        rows.append((g.run,"（整局）技能 / 攻击类命令包","见各项",f"{casts} 条（操作码 05–11；没按项对齐）",""))
    except Exception as e: rows.append((g.run,"（整局）技能 / 攻击类命令包","见各项","未判：没取到命令包日志",""))
    try: tm=open(f"{d}/timing.txt").read().strip()
    except OSError: tm="计时：没取到"




    if g.who:
        m=re.findall(r"刷怪积分牌：「([^」]+)」",log)
        rows.append((g.run,"（整局）该出现进局角色",f"进局的是选的那一行的角色「{g.who}」",(f"通：{m[0]}" if m and m[0]==g.who else f"不通：进的是「{m[0]}」" if m else "不通：日志里读不到进局角色"),""))
    return rows,wall,cap,red,tm
def groups():
    gs=[]






    g=G("a","DM_TEST_GIVE=cap:2:1,skp:2:1,ssd:2:1,rin:4:2,isc:2:1@60 DM_TEST_GIVE_INV=1 DM_UI_TAPS_AT=OpenPanel:PlayerInventoryExpansionLayout DM_DBG_FE_SEND=60:PanelManager:OpenPanel:SaveIndicatorPanel;75:PanelManager:OpenPanel:SaveIndicatorPanel;90:PanelManager:OpenPanel:SaveIndicatorPanel;105:PanelManager:OpenPanel:SaveIndicatorPanel;120:PanelManager:OpenPanel:SaveIndicatorPanel;135:PanelManager:OpenPanel:SaveIndicatorPanel;150:PanelManager:OpenPanel:SaveIndicatorPanel;185:PanelManager:ClosePanel:SaveIndicatorPanel"+(" DM_TEST_EDGE_B=20" if MODE=="ipadfull" else "")); g.sect("equip",0); g.enter(); g.wait(2.0)






    g.tap("@menu",1.1)
    g.item("存盘指示在最上层时把骷髅帽拖到头部位","拿得起、换上，软帽落到骷髅帽原来的格子；日志没有「不放行」",r"OpenPanel SaveIndicatorPanel[\s\S]*换下来的那件[^\n]*放到被拖物品腾出的原位|换下来的那件[^\n]*放到被拖物品腾出的原位[\s\S]*OpenPanel SaveIndicatorPanel",r"原版判定 0"); g.dragp((412,235),(333,107)); g.wait(1.0); g.shot()
    g.item("单击背包里的软帽（头上戴着骷髅帽）","出说明，和身上那件并排对比（看截图）",r"物品区 grid"); g.tap(P(412,235),1.5); g.shot()
    g.item("短剑拖到头部位（戴不了）","回到原来的格子",r"原版判定 0[\s\S]*没放下[^\n]*放到被拖物品腾出的原位"); g.dragp((245,240),(333,107)); g.shot()


    g.item("双击鉴定卷轴后点戒指 A","鉴定",r"界面鉴定：单击 rin"); g.dbl(423,265); g.tap(P(245,285),0.15)
    g.item("鉴定后立刻把戒指 A 拖到左戒指位（空）","戴上（是否真戴上由后面换装与双击戒指位两项反证）",r"拖放落点类 5",r"原版判定 0|没放下"); g.dragp((245,285),(294,207)); g.shot()
    g.item("双击另一张鉴定卷轴后点戒指 B","鉴定",r"界面鉴定：单击 rin"); g.dbl(402,264); g.tap(P(403,285),0.15)
    g.item("鉴定后立刻把戒指 B 拖到左戒指位（戴着 A）","换上 B，A 落到 B 原来的格子",r"原版判定 5|换下来的那件[^\n]*原位",r"原版判定 0"); g.dragp((403,285),(294,207)); g.shot()
    g.item("换下来的戒指 A 拖到右戒指位（空）","戴上",r"拖放落点类 5",r"原版判定 0|没放下"); g.dragp((403,285),(373,207)); g.shot()
    g.item("双击右戒指位","卸下那枚戒指（反证上一步确实戴上了）",r"界面双击：身上的 rin"); g.dbl(373,167); g.shot()
    g.item("双击左戒指位","卸下那枚戒指（反证左戒指位确实戴着）",r"界面双击：身上的 rin"); g.dbl(294,167); g.shot()
    g.tap(P(615,17),1.2); g.sect("hud")
    g.item("进局","玩家状态可读、在营地",r"pos 区域 1 \(\d+,\d+\) 生命 \d+/\d+"); g.at("pos",.3); g.shot()
    g.item("点右上角区域名那一条","切全屏地图",r"点小地图那一块.*→ 全屏地图"); g.tap("@mmname"); g.shot()
    g.item("全屏地图点空处","仍是全屏地图（点地面走路不再切回小地图，#186）",r"pos 区域 \d+",r"→ 回小地图|记住小地图"); g.tap(P(300,150)); g.at("pos",.3); g.shot()
    g.item("再点区域名那一条","回小地图",r"点小地图那一块.*→ 回小地图"); g.tap("@mmname"); g.shot()
    g.item("点底栏十字","开物品栏",r"OpenPanel PlayerInventoryExpansionLayout"); g.tap("@menu"); g.shot()
    for nm,x,pat in [("任务日志",114,"OpenPanel QuestLogPanelExpansion"),("角色",223,"OpenPanel CharacterStatsPanel"),("技能树",442,"OpenPanel SkillsTreePanel"),("选项",551,"OpenPanel PauseLayout"),("物品栏",333,"OpenPanel PlayerInventoryExpansionLayout")]:
        g.item(f"点顶部页签「{nm}」",f"切到{nm}",pat); g.tap(P(x,17)); g.shot()
    g.item("点右上角 X","面板关闭",r"ClosePanel PlayerInventoryExpansionLayout"); g.tap(P(615,17)); g.shot()
    g.item("点腰带第 1 格","喝掉一瓶",r"腰带 0/1/1/1"); g.tap("@belt1",2.0); g.shot()


    g.item("摇杆向右推 60 点","满偏：lx = 32767",r"\[script\] move slot=1 spec=@joy\+60,0[^\n]*lx=32767 ly=0"); g.at("down 1 @joy",.3); g.at("move 1 @joy+60,0",.5); g.at("up 1",.6)
    g.item("摇杆向右推 20 点","lx = 8495（(20 − 6) / 54 × 32767）",r"\[script\] move slot=1 spec=@joy\+20,0[^\n]*lx=8495 ly=0"); g.at("down 1 @joy",.3); g.at("move 1 @joy+20,0",.5); g.at("up 1",.6)
    if MODE=="ipadfull":
        g.item("底边那一条里起手往上拖","整次触摸都不当摇杆：轴为 0、人不动",edge_idle); g.at("pos",.2); g.at("down 1 200,760",.3); g.at("move 1 200,690",.8); g.at("up 1",.5); g.at("pos",.3)
        g.item("从摇杆底座起手往上拖","摇杆照常：纵轴满偏（底座固定后，只有落在底座附近才起摇杆）",r"\[script\] move slot=1 spec=@joy\+0,-60[^\n]*ly=-?32767"); g.at("down 1 @joy",.3); g.at("move 1 @joy+0,-60",.5); g.at("up 1",.6)
    g.sect("skill",skip=LETTERBOX)
    for x,y,r,to in [(5788,4475,4,10),(5812,4503,4,9),(5826,4522,4,8),(5840,4560,5,8)]: g.at(f"walk_to {x} {y} {r} {to} skip",to+.5)
    for n in (4,3,2,1):
        bound=(n==3)
        g.item(f"野外点技能钮 @skill{n}","施放：法力下降" if bound else "空位：开技能树、不施放",cast if bound else nocast); g.tap(f"@skill{n}",2.2); g.shot(); g.tap(P(615,17),1.0)
        g.item(f"@skill{n} 按住向右拖","施放：法力下降" if bound else "空位：不施放",cast if bound else nocast); g.drag(f"@skill{n}",f"@skill{n}+{O(70,0)}"); g.wait(1.0); g.shot(); g.tap(P(615,17),1.0)
    g.item("按住主钮 1.5 秒","人不离开原地",stay); g.at("pos",.2); g.at("down 0 @main",1.5); g.at("up 0",.8); g.at("pos",.3); g.shot()
    g.sect("pause"); g.tap("@menu"); g.tap(P(551,17))


    g.item("暂停菜单点「编年史」","进编年史",r"OpenPanel ChroniclePanel"); g.at("tap_ui Chronicle@PauseLayoutGarden 3",2.0); g.shot()
    g.item("编年史点右上角 X","回暂停菜单",r"ClosePanel ChroniclePanel"); g.tap(P(615,17)); g.shot()
    g.item("暂停菜单点「选项」","进设置页",r"OpenPanel SettingsPanel"); g.at("tap_ui Settings@PauseLayoutGarden 3",1.8); g.shot()
    for nm,x in [("游戏",333),("游戏设定",388),("音频",277)]:
        g.item(f"设置页点子页签「{nm}」","切页",r"界面页签：切 [+-]\d+ 页"); g.tap(P(x,67),1.3); g.shot()
    if not LETTERBOX: g.item("点 NPC 语音右箭头","改值",r"OptionsPanelMessage SetNPCSpeech"); g.tap(P(485,195),1.2); g.shot()
    g.item("点「放弃」","退出设置页、不保存",r"ClosePanel SettingsPanel"); g.tap(P(420,345),1.8); g.shot()
    if not LETTERBOX: g.item("再进设置改一项后点「应用」","保存并退出设置页",r"AcceptChanges"); g.at("tap_ui Settings@PauseLayoutGarden 3",1.8); g.tap(P(485,195),1.2); g.tap(P(245,345),1.8); g.shot()
    if not LETTERBOX: g.item("有改动时点顶部「角色」页签","弹确认框",r"OpenPanel ConfirmationModal"); g.at("tap_ui Settings@PauseLayoutGarden 3",1.8); g.tap(P(485,195),1.2); g.tap(P(223,17),1.5); g.shot()
    if not LETTERBOX: g.item("确认框点「放弃」","关设置页、不保存",r"CancelChanges"); g.tap(P(378,200),1.8); g.shot()
    g.item("暂停菜单点「保存并退出」","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",4.0); g.shot()
    gs.append(g)


    g=G("e"); g.sect("front",0); g.t=3.5
    g.item("选角页点第 3 行","选中该角色",r"tap_ui ListItem2@CharacterSelectPanel：控件在[\s\S]*UpdateCharacterInfo"); g.ui("ListItem2@CharacterSelectPanel",1.3); g.shot()
    g.item("点垃圾桶","弹删除确认",r"ShowDeleteCharacterModal"); g.ui("DeleteCharacter@CharacterSelectPanel",1.5); g.shot()
    g.item("删除确认点「否」","弹窗关闭、不删",r"ClosePanel DeleteCharacterModal",r"CharacterSelect DeleteCharacter "); g.tap(P(380,203),1.5); g.shot()
    g.item("点「创建新角色」","进选职业页",r"ToCharacterCreate"); g.ui("CreateCharacter@CharacterSelectPanel",2.5); g.shot()
    g.item("点一个职业两下","选中职业",r"CharacterCreate Select"); g.tap(P(488,209),.8); g.tap(P(433,189),1.5); g.shot()
    g.item("点「创建」","进选择游戏模式页",r"OpenPanel CharacterOptionsPanel"); g.tap(P(334,343),2.8); g.shot()
    g.item("点「极乐世界」勾选框","勾上",r"极乐世界 勾上","late"); g.tap(P(70,236),1.2); g.shot()
    g.item("再点一次勾选框","取消勾选",r"极乐世界 取消","late"); g.tap(P(70,236),1.2); g.shot()
    g.item("点「标准」","弹名字输入框",r"OpenPanel CharacterCreateNamePanel"); g.tap(P(167,193),2.5); g.shot()
    g.item("名字框留空点确定","填入随机名",r"随机名 → 名字框"); g.at("alert ok",2.0); g.shot()
    g.item("起名页点「创建」","进局；建的是普通角色，不补共享仓库",r"FrontEndNavigation ToGame",r"共享仓库补齐"); g.tap(P(407,257),7.0); g.at("pos",.3); g.shot()


    g.item("暂停菜单点「保存并退出」","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",4.0); g.shot()
    g.item("再建一个：名字框同样留空","建成进局",r"随机名 → 名字框[\s\S]*FrontEndNavigation ToGame"); g.ui("CreateCharacter@CharacterSelectPanel",2.5); g.tap(P(488,209),.8); g.tap(P(433,189),1.5); g.tap(P(334,343),2.8); g.tap(P(167,193),2.5); g.at("alert ok",2.0); g.tap(P(407,257),7.0); g.at("pos",.3); g.shot()
    g.item("再保存并退出回选角页","两次填进名字框的随机名不同；选角列表里有这两个名字（看截图）",two_names); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",4.5); g.shot()
    gs.append(g)




    g=G("v"); g.sect("ely",0); g.t=3.5
    g.item("点「创建新角色」","进选职业页",r"ToCharacterCreate"); g.ui("CreateCharacter@CharacterSelectPanel",2.5); g.shot()
    g.item("点一个职业两下","选中职业",r"CharacterCreate Select"); g.tap(P(488,209),.8); g.tap(P(433,189),1.5); g.shot()
    g.item("点「创建」","进选择游戏模式页",r"OpenPanel CharacterOptionsPanel"); g.tap(P(334,343),2.8); g.shot()
    g.item("点「极乐世界」勾选框（只点一次）","勾上",r"极乐世界 勾上","late"); g.tap(P(70,236),1.2); g.shot()
    g.item("点「标准」","弹名字输入框",r"OpenPanel CharacterCreateNamePanel"); g.tap(P(167,193),2.5); g.shot()
    g.item("名字框留空点确定","填入随机名",r"随机名 → 名字框"); g.at("alert ok",2.0); g.shot()
    g.item("起名页点「创建」，进局后等 12 秒","记为极乐角色 → 共享仓库补齐 → 补齐后金币到数、角色身上赦免之证 2 枚 → 随存档落盘，四行依次出现、不带红",r"记为极乐世界角色[\s\S]*共享仓库补齐——[\s\S]*共享仓库补齐后——[^\n]*角色身上 2 枚[\s\S]*共享仓库补齐已随存档提交落盘",r"共享仓库补齐[^\n]*【红】|【红】[^\n]*共享仓库"); g.tap(P(407,257),12.0); g.at("pos",.3); g.shot()
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 4",4.5); g.shot()
    g.item("点「共享」页签","切页",r"界面页签：切 \+1 页"); g.tap(P(106.5,53),1.2); g.shot()
    g.item("共享页","金币框 ≥ 2500000；两枚赦免之证在背包里（看截图）",None); g.shot()
    gs.append(g)







    g=G("v2"); g.sect("ely2",0); g.t=3.5
    g.item("点「创建新角色」","进选职业页",r"ToCharacterCreate"); g.ui("CreateCharacter@CharacterSelectPanel",2.5); g.shot()
    g.item("点一个职业两下","选中职业",r"CharacterCreate Select"); g.tap(P(488,209),.8); g.tap(P(433,189),1.5); g.shot()
    g.item("点「创建」","进选择游戏模式页",r"OpenPanel CharacterOptionsPanel"); g.tap(P(334,343),2.8); g.shot()
    g.item("点「极乐世界」勾选框（只点一次）","勾上",r"极乐世界 勾上","late"); g.tap(P(70,236),1.2); g.shot()
    g.item("点「标准」","弹名字输入框",r"OpenPanel CharacterCreateNamePanel"); g.tap(P(167,193),2.5); g.shot()
    g.item("名字框留空点确定","填入随机名",r"随机名 → 名字框"); g.at("alert ok",2.0); g.shot()
    g.item("起名页点「创建」，进局后等 12 秒","记为极乐角色 → 共享仓库补齐 → 补齐后金币到数、角色身上赦免之证 2 枚 → 随存档落盘，四行依次出现、不带红",r"记为极乐世界角色[\s\S]*共享仓库补齐——[\s\S]*共享仓库补齐后——[^\n]*角色身上 2 枚[\s\S]*共享仓库补齐已随存档提交落盘",r"共享仓库补齐[^\n]*【红】|【红】[^\n]*共享仓库"); g.tap(P(407,257),12.0); g.at("pos",.3); g.shot()
    g.item("保存并退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",5.0); g.shot()
    g.item("同一角色再进一局，等 12 秒","进局；不再补齐（没有「共享仓库补齐——」行）、不带红",lambda st,bd: ("不通：没进局" if "FrontEndNavigation ToGame" not in st else "不通：进的不是那个极乐角色" if "极乐世界：角色" not in st else "不通：第二局又补了一次" if "共享仓库补齐——" in st else "不通：有红" if re.search(r"【红】极乐世界",st) else "通：进局、没有补齐行")); g.tap(P(333,336),8.0); g.wait(12.0); g.at("pos",.3); g.shot()
    gs.append(g)
    g=G("b","DM_TEST_GOLD=1300 DM_TEST_WORLD_GO=near2:1:c154@650"); g.sect("npc",0); g.enter()
    g.item("点瓦瑞夫（第一次）","走过去，出讲话",r"OpenPanel ScrollSpeechPanel"); g.at("tap_unit 1 155 3",5.0); g.shot()
    g.item("再点一下跳过讲话","出两行菜单（交谈 / 取消）",r"ClosePanel ScrollSpeechPanel"); g.at("tap_unit 1 155 2",1.8); g.shot()
    g.item("两行菜单点「交谈」","命中第 1 行",r"对话菜单 \+0 行"); g.at(f"tap_unit 1 155 2 +{MENU["talk2"]}",2.0); g.shot()
    g.item("讲话 / 话题页点菜单外","关闭",r"ClosePanel (NpcDialogPanel|ScrollSpeechPanel)"); g.tap(P(100,300),1.2); g.tap(P(100,300),1.2); g.shot()
    g.item("重开菜单后点「取消」","命中第 2 行并关菜单",r"对话菜单 \+1 行"); g.at("tap_unit 1 155 2",2.0); g.at(f"tap_unit 1 155 2 +{MENU["cancel2"]}",2.0); g.shot()
    g.item("重开菜单后点菜单外","关菜单、人不走",r"ClosePanel NpcDialogPanel"); g.at("tap_unit 1 155 2",2.0); g.tap(P(100,300),1.5); g.shot()
    g.arrive(5776,4433,10,80.0)
    g.item("点恰西","出三行菜单",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 154 3",5.0); g.at("tap_unit 1 154 2",2.0); g.shot()
    g.item("三行菜单点「交易或修复」","进商人页",r"OpenPanel VendorPanelLayout"); g.at(f"tap_unit 1 154 2 +{MENU["trade3"]}",2.5); g.shot()
    g.sect("shop",0)
    g.item("货架上单击一件武器（身上拿着短剑）","出说明；有没有和身上那件并排对比（看截图）",None); g.tap(P(163,53),1.2); g.tap(P(102,85),1.5); g.shot()
    g.item("点「杂项」页签","切货架",r"界面页签：切 [+-]\d+ 页"); g.tap(P(270,53)); g.shot()
    g.item("货架第一件拖进背包（1200，身上 1300）","买下",r"买选中的货.*→ 1（"); g.dragp((102,85),(491,264)); g.shot()
    g.item("再拖一件 1200 的（钱不够）","走到原版买入口且身上钱少于价（提示与不成交看截图）",r"价 1200，身上 100 \+"); g.dragp((102,124),(454,264)); g.wait(.8); g.shot()
    g.item("提示框点「继续」","关提示",r"对话菜单 \+0 行|商人页结果提示（按返回收）"); g.tap(P(100,100)); g.shot()
    g.item("背包里的第二张卷轴拖到货架","卖出",r"卖手上的物品.*→ 1"); g.dragp((562,286),(190,184)); g.shot()
    g.item("点右上角 X","关商人页",r"ClosePanel VendorPanelLayout"); g.tap(P(615,17)); g.shot()
    g.sect("inv"); g.tap("@menu")
    g.item("单击背包里的卷轴","命中物品格、出说明",r"物品区 grid"); g.tap(P(423,262)); g.shot()
    g.item("武器从装备位拖进背包","放进背包",r"拖放落点类 5"); g.dragp((257,119),(280,297)); g.shot()
    g.item("从背包拖回装备位","装上",r"拖放落点类 5"); g.dragp((280,270),(257,150)); g.shot()
    g.item("双击装备位上的武器","卸下",r"界面双击"); g.dbl(257,119); g.shot()
    g.item("卷轴快松到石板空白（一步拖过去马上松手）","按松手位置算：放回，不丢",r"拖放落点类 0[\s\S]*c2s 19[\s\S]*c2s 18",r"c2s 17"); g.fast((423,262),(195,300)); g.shot()
    g.item("卷轴快松到左侧黑网格","按松手位置算：丢到地上",r"拖放落点类 3[\s\S]*c2s 17"); g.fast((423,262),(95,240)); g.shot()
    g.item("长按背包空格","整理",r"整理："); g.hold(300,262); g.shot()
    g.item("点右上角 X","面板关闭",r"ClosePanel PlayerInventoryExpansionLayout"); g.tap(P(615,17)); g.shot()
    g.item("地上掉落物","名字压在物品上（看截图）",None); g.shot()
    g.item("在物品位置落点","命中名字标签或物品框（结果里写明是哪个），走到跟前捡回",r"类型 4 [^\n]*（(名字标签|命中框)[\s\S]*类型 4 id \d+ 到跟前[^\n]*交互 1"); g.at("tap_unit 4 16777215 2",3.0); g.shot()
    gs.append(g)
    g=G("c","DM_TEST_WORLD_GO=near2:1:c155@40 DM_UI_TAPS_AT=OpenPanel:DropGoldModal DM_DBG_FE_SEND=60:PanelManager:OpenPanel:SaveIndicatorPanel;75:PanelManager:OpenPanel:SaveIndicatorPanel;90:PanelManager:OpenPanel:SaveIndicatorPanel;105:PanelManager:OpenPanel:SaveIndicatorPanel;120:PanelManager:OpenPanel:SaveIndicatorPanel;135:PanelManager:OpenPanel:SaveIndicatorPanel;150:PanelManager:OpenPanel:SaveIndicatorPanel;185:PanelManager:ClosePanel:SaveIndicatorPanel"); g.sect("stash",0); g.enter(1); g.wait(3.0)


    g.item("NPC 名牌位置","名牌在 NPC 头顶（看截图）",None); g.shot()
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("背包里的宝石拖进箱子的空格","放进箱子",r"拖放落点类 5"); g.dragp((522.5,285),(82,235)); g.shot()
    g.item("双击箱子里那颗宝石","移回背包（开箱时双击 = 按住 Y 脉冲，另看截图）",r"InputMessage ControllerActionBegin"); g.dbl(82,201); g.shot()
    g.item("点「共享」页签","切页",r"界面页签：切 \+1 页"); g.tap(P(106.5,53)); g.shot()
    g.item("点「个人」页签","切回",r"界面页签：切 -1 页"); g.tap(P(62.5,53)); g.shot()
    g.item("点取金币按钮","弹数量框",r"OpenPanel DropGoldModal"); g.tap(P(187.5,307),1.0); g.shot()
    g.item("数量框开着、存盘指示叠在上面时拖背包里的魔盒","框关掉、魔盒没被拖走、金币不变（看截图）；日志有存盘指示",r"OpenPanel SaveIndicatorPanel[\s\S]*ClosePanel DropGoldModal|ClosePanel DropGoldModal[\s\S]*OpenPanel SaveIndicatorPanel",r"拖放落点类 5[\s\S]*换下来|原版判定"); g.dragp((590.5,238),(161,275)); g.wait(1.2); g.shot()
    g.tap(P(187.5,307),1.0)
    g.item("数量框开着等 3 秒","数字不变（看截图）",None); g.wait(3.0); g.shot()
    g.item("数量框点「取消」","关框、金币不变",r"ClosePanel DropGoldModal"); g.tap(P(375,200)); g.shot()
    g.item("数量框开着点框外的「共享」页签","框关掉、不切页、金币不变（看截图）",r"ClosePanel DropGoldModal",r"界面页签：切"); g.tap(P(187.5,307),1.0); g.tap(P(106.5,53),1.2); g.shot()
    g.item("数量框开着拖底下的魔盒","框关掉、魔盒没被拖走（看截图）",r"ClosePanel DropGoldModal",r"拖放落点类 5"); g.tap(P(187.5,307),1.0); g.dragp((590.5,238),(161,275)); g.shot()
    g.item("数量框开着点箱子里的空格子","框关掉、金币不变",r"ClosePanel DropGoldModal"); g.tap(P(187.5,307),1.0); g.tap(P(82,235),1.2); g.shot()
    g.item("数量框开着双击背包里的魔盒","框关掉、魔盒没打开",r"ClosePanel DropGoldModal",r"OpenPanel HoradricCubeLayout"); g.tap(P(187.5,307),1.0); g.dbl(590.5,238); g.shot()
    g.item("数量框里输 100 点「好」","取出 100：箱子 296、身上 100（看截图）",r"ClosePanel DropGoldModal"); g.tap(P(187.5,307),1.0); g.tap(P(332,173),1.5); g.at("clear",.3); g.at("type 100",.5); g.at("alert ok",1.2); g.tap(P(290,200),1.5); g.shot()
    g.item("存入框开着点框外的页签","框关掉、不切页、金币不变（看截图）",r"ClosePanel DropGoldModal",r"界面页签：切"); g.tap(P(494.5,308),1.0); g.shot(); g.tap(P(106.5,53),1.2); g.shot()
    g.item("开着箱子双击背包里的魔盒","打开魔盒",r"OpenPanel HoradricCubeLayout"); g.dbl(590.5,238); g.shot()
    g.tap(P(654.5,17),1.2); g.tap(P(654.5,17),1.2); g.tap("@menu",1.4)
    g.item("只开物品栏点金币（丢金币框）再点框外","框关掉、金币不变（看截图）",r"OpenPanel DropGoldModal[\s\S]*ClosePanel DropGoldModal"); g.tap(P(333,309),1.0); g.shot(); g.tap(P(120,200),1.2); g.shot()
    g.tap(P(615,17),1.2)


    g.tap("@menu",1.4); g.tap(P(442,17),1.5)
    g.item("技能树里长按已学技能","弹「重新绑定技能」框",r"OpenPanel ButtonBindingModal"); g.hold(261,90,1.3); g.shot()
    g.item("绑定框开着点框外","框关掉、留在技能树",r"技能绑定：已取消[^\n]*留在技能树"); g.tap(P(120,200),1.2); g.shot()
    g.tap(P(551,17),1.5); g.at("tap_ui Settings@PauseLayoutGarden 3",1.8); g.tap(P(485,195),1.2); g.tap(P(223,17),1.5)
    g.item("设置改了一项后点顶部页签弹出确认框，点框外","没有反应、框还在（看截图）",r"script\] down",r"ClosePanel ConfirmationModal|CancelChanges|AcceptChanges"); g.tap(P(333,330),1.2); g.shot()
    g.item("确认框点「应用」","保存生效",r"AcceptChanges"); g.tap(P(283,200),1.8); g.shot()
    gs.append(g)


    g=G("d","DM_TEST_WORLD_GO=near2:2:c267@200",save=os.environ.get("UIREG_STACK_SAVE") or "save.pw_rune_stack"); g.sect("stack",0); g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("点「符文」页签","切到符文页",r"界面页签：切 [+-]\d+ 页"); g.tap(P(148,53)); g.shot()
    g.item("单击成叠的符文格","命中成叠格、出说明、不取",r"成叠格",r"取多件"); g.tap(P(201,81),2.0); g.shot()
    g.item("双击成叠格","取 1 件",r"取多件：.*取 1："); g.dbl(201,81); g.shot()
    g.item("长按成叠格","取 3 件",r"取多件：.*取 3："); g.hold(201,81); g.shot()
    g.item("从成叠格拖到背包","取 1 件",r"取多件：.*取 1："); g.dragp((201,81),(488.5,297)); g.shot()
    g.item("从成叠格拖到面板下方松手","不取",r"拖放落点|界面命中",r"取多件"); g.dragp((201,81),(333,372)); g.shot()
    gs.append(g)




    g=G("sa","DM_TEST_WORLD_GO=near2:2:c267@200 DM_C2S_LOG=pwc2s.jsonl",save=os.environ.get("UIREG_STACK_SAVE") or "save.pw_rune_stack"); g.sect("stashauto",0); g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("珍宝匣页双击符文那一叠","取 1 件到背包",r"取多件：.*取 1："); g.tap(P(148,53)); g.dbl(201,81); g.shot()
    g.item("切到个人页，把背包里那颗符文拖进个人页格子","落下后自动转进珍宝匣，个人页不留",r"珍宝匣：仓库页里新落下的物品[\s\S]*珍宝匣：物品 id \d+ 已从仓库页转进高级仓库",r"2 秒还在仓库页"); g.tap(P(62,53)); g.dragp((600.5,227),(81,237)); g.wait(2.5); g.shot()
    g.item("珍宝匣页再取 1 件","到背包（那一叠还是 12 个：上一步拖进个人页的那颗回来了）",r"叠数 12 取 1："); g.tap(P(148,53)); g.dbl(201,81); g.shot()
    g.item("切到共享页，把它拖进共享页格子","收包处改道：放回背包原格、下一拍进珍宝匣；共享页不留",r"珍宝匣：r01[^\n]*拖进共享页 → 改成放回背包格[\s\S]*珍宝匣：物品 id \d+ 已回到背包 → 原版移到仓库",r"留在背包"); g.tap(P(106,53)); g.dragp((600.5,227),(81,237)); g.wait(2.5); g.shot()
    g.item("珍宝匣页再取 1 件","到背包（那一叠还是 12 个：上一步拖进共享页的那颗也回来了）",r"叠数 12 取 1："); g.tap(P(148,53)); g.dbl(201,81); g.shot()
    g.item("开着箱子双击背包里的那颗符文","原版「移动到堆叠栏」：背包少 1 格（回到珍宝匣那一叠）",("inv",-1)); g.dbl(600.5,246.5); g.wait(2.0); g.shot()
    gs.append(g)


    SV=os.environ.get("UIREG_STACK_SAVE") or "save.pw_rune_stack"
    g=G("t","DM_TEST_GIVE=box:2:1@60 DM_TEST_GIVE_INV=1"+" DM_TEST_WORLD_GO=near2:2:c267@200",save=SV); g.sect("cube",0); g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("点「符文」页签","切到符文页（背包里有魔盒，看截图）",r"界面页签：切 [+-]\d+ 页"); g.tap(P(148,53)); g.shot()
    g.item("双击成叠格","背包多 1 格",("inv",1,"→ 成叠格")); g.dbl(201,81); g.shot()
    g.item("长按成叠格","背包多 3 格",("inv",3,"→ 成叠格")); g.hold(201,81); g.shot()
    g.item("从成叠格拖到背包","背包多 1 格",("inv",1,"→ 成叠格")); g.dragp((201,81),(488.5,297)); g.shot()
    g.item("再双击一次","取件时游戏里那一叠是 7 个（12 − 5，没有多扣也没有少扣）",r"叠数 7 取 1"); g.dbl(201,81); g.shot()
    g.item("双击背包里的一枚符文（背包带着魔盒）","背包少 1 格",("inv",-1)); g.dbl(580.9,285.5); g.shot()
    g.item("再双击那一叠","那枚进的是这一叠（取件时叠数回到 7）、没进魔盒；背包多 1 格",("inv",1,"叠数 7 取 1")); g.dbl(201,81); g.shot()
    gs.append(g)


    g=G("u","DM_TEST_GIVE=box:2:1,cap:2:9,gcv:2:1@60 DM_TEST_GIVE_INV=1"+" DM_TEST_WORLD_GO=near2:2:c267@200",save=SV); g.sect("full",0); g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("点「符文」页签","切到符文页（背包只剩 1 个空格，看截图）",r"界面页签：切 [+-]\d+ 页"); g.tap(P(148,53)); g.shot()
    g.item("双击成叠格","背包多 1 格（满了）",("inv",1,"→ 成叠格")); g.dbl(201,81); g.shot()
    g.item("背包满了再双击","背包占格不变",("inv",0,"→ 成叠格")); g.dbl(201,81); g.shot()
    g.item("背包满了长按","那一叠还是 11 个（没取出来的不能凭空少）",r"叠数 11 取"); g.hold(201,81); g.shot()
    gs.append(g)


    g=G("s","DM_TEST_GIVE=r01:2:2@60 DM_TEST_GIVE_INV=1"+" DM_TEST_WORLD_GO=near2:2:c267@200"); g.sect("put",0); g.enter(); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()


    g.item("背包右上角的符文拖进个人页空格","自动转进珍宝匣（个人页不留，看截图）",r"珍宝匣：物品 id \d+ 已从仓库页转进高级仓库"); g.dragp((600.5,227),(81,237)); g.wait(1.5); g.clr(); g.shot()
    g.item("个人页开着双击背包里的卷轴（不可堆叠）","进箱子当前页：背包少 1 格（卷轴在个人页里，看截图）",("inv",-1)); g.dbl(600.5,266); g.clr(); g.shot()
    g.item("点「珍宝匣」页签","切到珍宝匣页（符文那一格已有 1 枚，看截图）",r"界面页签：切 [+-]\d+ 页"); g.tap(P(148,53)); g.shot()
    g.item("珍宝匣页开着双击背包里另一枚符文","进那一叠：背包少 1 格",("inv",-1)); g.dbl(600.5,246.5); g.clr(); g.shot()
    g.item("双击珍宝匣里那一叠","取出 1 枚：背包多 1 格；取件时那一叠是 2 个（自动转进去 1 + 双击进去 1）",("inv",1,"叠数 2 取 1")); g.dbl(201,81); g.clr(); g.shot()
    g.item("从那一叠拖到背包左上角空格","再取出 1 枚：背包多 1 格",("inv",1,"→ 成叠格")); g.dragp((201,81),(424.5,267)); g.clr(); g.shot()
    gs.append(g)


    g=G("g"); g.sect("bind",0); g.enter()
    for x,y,r,to in [(5788,4475,4,10),(5812,4503,4,9),(5826,4522,4,8),(5840,4560,5,8)]: g.at(f"walk_to {x} {y} {r} {to} skip",to+.5)
    g.at("pos",.3)


    g.item("点空技能位 @skill4（第一组）","开技能树、等选技能",r"技能绑定：技能树已开"); g.tap("@skill4",2.0); g.shot()
    g.item("在技能树里点已学技能","绑到这个钮、面板自己关",r"技能绑定：已绑到键 0x[0-9a-f]+，关功能页"); g.tap(P(397,91),2.5); g.shot()
    g.item("点刚绑好的 @skill4","施放：法力下降",cast); g.tap("@skill4",2.2); g.shot()
    g.item("点换组键、点第二组的空位 @skill4、再点技能","绑到第二组这个钮、面板自己关",r"技能绑定：技能树已开[\s\S]*技能绑定：已绑到键 0x[0-9a-f]+，关功能页"); g.tap("@swap",1.0); g.tap("@skill4",2.0); g.tap(P(397,91),2.5); g.shot()
    g.tap("@swap",1.0)


    g.tap("@menu",1.4); g.tap(P(442,17),1.5)
    g.item("技能树里长按已学技能","弹「重新绑定技能」框",r"OpenPanel ButtonBindingModal"); g.hold(397,91,1.3); g.shot(); prev=g.items[-1][4]
    for r,y in enumerate(BIND_ROWS,1):
        for c,x in enumerate(BIND_COLS,1):
            g.item(f"绑定框里点第 {r} 排第 {c} 列，再开框看","技能落在画面上的这一格",("cells",prev,(r,c))); g.tap(P(x,y),1.6); g.hold(397,91,1.3); g.shot(); prev=g.items[-1][4]
    g.item("绑定框开着点框外","框关掉、不绑、留在技能树",r"技能绑定：已取消[^\n]*留在技能树"); g.tap(P(120,200),1.5); g.shot()
    g.tap(P(615,17),1.2)
    gs.append(g)


    g=G("h"); g.sect("misc",0); g.enter(); g.tap("@menu",1.4)
    g.item("双击武器位卸下匕首","卸到背包",r"界面双击：身上的"); g.dbl(257,119); g.shot()
    g.tap(P(615,17),1.2)
    g.item("点空技能位 @skill4","开技能树、等选技能",r"技能绑定：技能树已开"); g.tap("@skill4",2.0); g.shot()
    g.item("在「综合技能」页点一个普通技能","绑上、面板自己关",r"技能绑定：已绑到键 0x80，关功能页"); g.tap(P(428,54),1.2); g.tap(P(275,114),2.5); g.shot()
    g.item("长按刚绑的那个技能开绑定框","框里它在第 1 排第 5 列（@skill4 那个钮）——读画面，不读日志",("cellhas",(1,5))); g.tap("@menu",1.4); g.tap(P(442,17),1.5); g.tap(P(428,54),1.2); g.hold(275,114,1.3); g.shot()
    g.tap(P(120,200),1.5)
    g.item("「综合技能」页长按另一个技能","开绑定框",r"OpenPanel ButtonBindingModal"); g.hold(301,114,1.3); g.shot()
    g.tap(P(120,200),1.5); g.tap(P(615,17),1.2)
    gs.append(g)


    g=G("shl","DM_TEST_SHARELOG_CLOSE=3"); g.sect("sharelog",0); g.enter(); g.tap("@menu",1.4); g.tap(P(551,17))
    g.item("暂停菜单点「分享诊断日志」","两份日志打成一个文件（本局那份非空）、分享面板弹出",r"诊断日志：已打包 \S+（\d+ 字节：本局 [1-9]\d*、[^\n]*\n(?:[^\n]*\n){0,40}?[^\n]*诊断日志：分享面板已弹出"); g.at("tap_ui MessageLog@PauseLayoutGarden 3",2.5); g.shot()
    g.item("等 3 秒","分享面板到点自己收起",r"到点收起分享面板"); g.wait(3.0); g.shot()
    g.item("再点一次「分享诊断日志」","又能打包（上一次收起后没卡在忙状态）",r"诊断日志：已打包"); g.at("tap_ui MessageLog@PauseLayoutGarden 3",5.5); g.shot()
    gs.append(g)


    g=G("i","DM_TEST_GOLD=200000 DM_TEST_WORLD_GO=near2:1:c150@200"); g.sect("hire",0); g.enter(1)
    g.item("让人走到卡夏旁","游戏自己寻路走到",r"\[script\] await pos：区域 1",r"走不过去"); g.arrive(5838,4454,10,45.0)
    g.item("点卡夏","出菜单（交谈 / 复活 / 雇佣 / 取消，看截图）",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 150 3",3.0); g.shot()


    g.item("点菜单第二行「复活佣兵」","复活：发出复活包、菜单重排成三行（看截图）",r"c2s 52\b"); g.at(f"tap_unit 1 150 2 +{MENU['kashya2']}",2.5); g.shot()
    g.item("点菜单第二行「雇佣」","开雇佣名单",r"OpenPanel HireMenuPanel"); g.at(f"tap_unit 1 150 2 +{MENU['kashya2']}",2.5); g.shot()
    g.item("名单同一行连点 6 下","弹确认框（看截图）",None)
    for k in range(6): g.tap(P(333,114),0.4)
    g.wait(1.0); g.shot()
    g.item("确认框点「是」","雇成：发出雇佣包",r"c2s 36\b"); g.tap(P(286,203),2.5); g.shot()
    g.item("点面板外","雇佣名单与对话菜单都关掉",r"ClosePanel (HireMenuPanel|NpcDialogPanel)"); g.tap(P(100,300),1.5); g.tap(P(100,300),1.5); g.shot()
    g.item("推摇杆","人能走：坐标变了",moved); g.at("pos",.2); g.at("down 1 @joy",.3); g.at("move 1 @joy+60,0",1.5); g.at("up 1",.5); g.at("pos",.3)
    gs.append(g)






    g=G("j","DM_TEST_GIVE=cap:2:7,r01:2:2,gcv:2:1@60 DM_TEST_GIVE_INV=1"); g.sect("socket",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.item("背包布局","软帽、符文、宝石都在上面写的位置（看截图）",None); g.shot()
    g.item("符文拖到 1 孔软帽上","镶进去：发镶嵌包、不是交换",r"c2s 28\b",r"c2s 1f\b"); g.dragp((308,238),(376,266)); g.shot()
    g.item("那枚符文原来的格子","空了",("spot","empty",(304,245))); g.shot()
    g.item("单击那顶软帽","说明里是「镶宝软帽」、带符文名（看截图）",None); g.tap(P(376,228),1.5); g.shot()
    g.item("另一枚符文拖到头部位（头上那顶没孔）","放不进：回原格",r"原版判定 0[\s\S]*放到被拖物品腾出的原位",r"c2s 28\b"); g.dragp((308,219),(333,107)); g.shot()
    g.item("那枚符文原来的格子","还在",("spot","filled",(304,225))); g.shot()
    g.item("把 2 孔软帽拖到头部位","换上",r"换下来的那件[^\n]*放到被拖物品腾出的原位",r"原版判定 0"); g.dragp((318,267),(333,107)); g.shot()
    g.item("符文拖到头部位（头上那顶有孔）","镶进身上的帽子：发镶嵌包",r"c2s 28\b",r"c2s 1f\b"); g.dragp((308,219),(333,107)); g.shot()
    g.item("那枚符文原来的格子","空了",("spot","empty",(304,225))); g.shot()
    gs.append(g)


    g=G("k","DM_TEST_GIVE=cap:2:7,r04:2:1,r03:2:1@60 DM_TEST_GIVE_INV=1"); g.sect("runeword",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.item("背包布局","软帽与两枚符文在位（看截图）",None); g.shot()
    g.item("第一枚符文拖到 2 孔软帽上","镶进去",r"c2s 28\b",r"c2s 1f\b"); g.dragp((308,238),(395,305)); g.shot()
    g.item("第二枚符文拖到同一顶软帽上","镶进去",r"c2s 28\b",r"c2s 1f\b"); g.dragp((308,219),(395,305)); g.shot()
    g.item("两枚符文原来的格子","都空了",("spot","empty",(304,245))); g.shot()
    g.item("（同上）上面那格","空了",("spot","empty",(304,225))); g.shot()
    g.item("单击那顶软帽","说明第一行是「天底」、带「+50% 增强防御」（旧译名缺省开着；关掉 item_notes 是原版的「强化防御」。看截图）",None); g.tap(P(395,267),1.5); g.shot()
    gs.append(g)




    g=G("f"); g.sect("merc",0); g.enter(4); g.tap("@menu",1.4)
    g.item("带佣兵的角色开物品栏","佣兵头像栏在、物品栏开着（看截图）",None); g.shot()
    g.item("把背包里的小护身符拖到空格","拿得起、放得下；日志没有「不放行」",r"c2s 19[\s\S]*c2s 18"); g.dragp((245,225),(364,265)); g.shot()
    g.item("再拖回原来的格子","拿得起、放得下",r"c2s 19[\s\S]*c2s 18"); g.dragp((364,225),(245,265)); g.shot()


    g.item("点页签条「佣兵」","开佣兵页，外框右上角有 X（看截图）",r"OpenPanel HirelingInventoryPanel"); g.tap(P(469,18),1.5); g.shot()
    g.item("点佣兵页右上角 X","佣兵页关掉、回到游戏画面、不留空外框（看截图）",r"ClosePanel HirelingInventoryPanel"); g.tap(P(615,17),1.5); g.shot()
    g.item("关掉后推摇杆","人能走：坐标变了（面板真关了）",moved); g.at("pos",.2); g.at("down 1 @joy",.3); g.at("move 1 @joy+60,0",1.5); g.at("up 1",.5); g.at("pos",.3)
    gs.append(g)


    g=G("map"); g.sect("map",0); g.enter(2)
    g.item("进局","在营地；缺省档是小地图（「读到 2」那行在进局前打，看截图）",r"pos 区域 1 "); g.at("pos",.3); g.shot()
    g.item("点区域名那一条","切全屏，记住全屏",r"地图：玩家点了 → 记住全屏地图"); g.tap("@mmname",1.5); g.shot()
    g.item("点底栏十字开物品栏","全屏地图先让成小地图",r"地图：→ 微缩放右（面板开着先让开"); g.tap("@menu",1.5); g.shot()
    g.item("点右上角 X 关物品栏","回到全屏地图",r"地图：→ 全屏（玩家要全屏"); g.tap(P(615,17),1.5); g.shot()
    g.item("走到出生点旁的箱子再出营地（换区域）","到了野外，仍是全屏地图（看截图）",r"pos 区域 2 "); g.at("walk_to 5808 4443 6 15 skip",15.5)
    for x,y,r,to in [(5788,4475,4,12),(5812,4503,4,9),(5826,4522,4,8),(5840,4560,5,8)]: g.at(f"walk_to {x} {y} {r} {to} skip",to+.5)
    g.wait(2.0); g.at("pos",.3); g.shot()
    g.item("全屏地图再点区域名那一条","回小地图，记住小地图",r"地图：玩家点了 → 记住小地图"); g.tap("@mmname",1.5); g.shot()
    g.item("开、关一次物品栏","关掉后仍是小地图",r"ClosePanel PlayerInventoryExpansionLayout",r"地图：→ 全屏"); g.tap("@menu",1.5); g.tap(P(615,17),1.5); g.shot()
    gs.append(g)


    g=G("mapf",env="DM_TEST_AUTOMAP=2400:0"); g.sect("mapf",0); g.enter(2); g.t=max(g.t,38.0)
    g.item("选项里选了全屏","直接全屏地图，不改回小地图档",r"地图：「透视地图尺寸」读到 0 → 触屏下缺省全屏地图",r"改回微缩地图放右"); g.at("pos",.3); g.wait(12.0); g.shot()
    g.item("点底栏十字开物品栏","全屏地图先让成小地图",r"地图：→ 微缩放右（面板开着先让开"); g.tap("@menu",1.5); g.shot()
    g.item("点右上角 X 关物品栏","回到全屏地图",r"地图：→ 全屏（玩家要全屏"); g.tap(P(615,17),1.5); g.shot()
    g.item("全屏地图点空处","仍是全屏地图（点地面走路不再切回小地图，#186）",r"pos 区域 \d+",r"→ 回小地图|记住小地图"); g.tap(P(300,150)); g.at("pos",.3); g.shot()
    g.item("全屏地图再点区域名那一条","回小地图（小地图还在，点得回来）",r"地图：玩家点了 → 记住小地图"); g.tap("@mmname",1.5); g.shot()
    g.item("点区域名那一条","又切回全屏",r"地图：玩家点了 → 记住全屏地图"); g.tap("@mmname",1.5); g.shot()
    gs.append(g)






    def worn(st,bd): 
        m=re.findall(r"佣兵装备位：拍 \d+（定时）佣兵[^\n]*",st); 
        if not m: return "不通：没有佣兵读数（没有佣兵？）"
        miss=[k for k in ("lbl 部位 8","lgl 部位 10") if k not in m[-1]]
        return "通：腰带 8 / 手套 10 都在佣兵身上" if not miss else "不通：缺 "+"、".join(miss)
    g=G("ms","DM_TEST_GIVE=lbl:2:1,lgl:2:1@60 DM_TEST_GIVE_INV=1 DM_TEST_MERC_SLOTS=1 DM_TEST_MERC_EQUIP=200:lbl:8;220:lgl:10 DM_TEST_MERC_DUMP=180,300"); g.sect("mercslot",0); g.enter(4)
    g.item("进局后等 12 秒","两件穿到佣兵的新部位上：日志两次「发完物品模式 1」，读数里两件各在其位；不带红",lambda st,bd: ("不通：有红" if re.search(r"【红】",st) else "不通：没穿上（发完物品模式不是 1）" if len(re.findall(r"替玩家发 0x5b：[^\n]*发完物品模式 1",st))!=2 else worn(st,bd))); g.wait(12.0); g.at("pos",.3); g.tap("@menu",1.4); g.tap(P(469,18),1.5); g.shot()
    g.item("保存并退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayout 3",5.0); g.shot()
    g.item("同一角色再进一局，等 14 秒","读档后两件还在佣兵的新部位上；不带红",lambda st,bd: ("不通：没进局" if "FrontEndNavigation ToGame" not in st else "不通：有红" if re.search(r"【红】",st) else worn(st,bd))); g.ui("ListItem3@CharacterSelectPanel",1.3); g.tap(P(333,336),1.6); g.tap(P(333,157),8.0); g.wait(14.0); g.at("pos",.3); g.shot()
    gs.append(g)




    g=G("m","DM_TEST_GIVE=cap:2:2,isc:2:1,box:2:1@60 DM_TEST_GIVE_INV=1 DM_TEST_WORLD_GO=near2:1:c154@200 DM_TEST_STAT_LOG=14"); g.sect("sell",0); g.enter(); g.arrive(5776,4433,10,60.0)
    g.at("tap_unit 1 154 3",5.0); g.at("tap_unit 1 154 2",2.0)
    g.item("恰西三行菜单点「交易或修复」","进商人页（背包布局看截图）",r"OpenPanel VendorPanelLayout"); g.at(f"tap_unit 1 154 2 +{MENU['trade3m']}",2.5); g.shot()
    g.item("背包里的卷轴拖到商人面板底板","卖出：金币增加、背包少 1 格",("inv",-1,gold('+'))); g.dragp((539,277),(203,340)); g.clr(); g.shot()
    g.item("软帽拖到页签条","卖出：金币增加、背包占格减少、页签没切",("inv",'-',gold('+',r"界面页签：切"))); g.dragp((550,230),(170,94)); g.clr(); g.shot()
    g.item("魔盒（不可卖）拖到有货的货架格","不成交：金币不变、背包占格不变；屏幕上有「此处无法交易物品」（看前一张截图）",("inv",0,gold(0))); g.dragp((397,230),(131,135)); g.at(f"shot {g.name}_tip.png",.3); g.clr(); g.shot()
    g.item("魔盒原来的格子","魔盒回到原格",("spot","filled",(386,227))); g.shot()
    g.item("另一张卷轴拖到货架空格","卖出：金币增加、背包少 1 格",("inv",-1,gold('+'))); g.dragp((558,277),(285,239)); g.clr(); g.shot()
    g.item("身上的匕首从武器位拖到商人面板底板","卖出：金币增加、背包占格不变、武器位空了（看截图）",("inv",0,gold('+'))); g.dragp((399,107),(203,340)); g.clr(); g.shot()
    g.item("点右上角 X","关商人页",r"ClosePanel VendorPanelLayout"); g.tap(P(615,17)); g.shot()
    gs.append(g)




    AK="DM_TEST_GIVE=tbk:2:1,key:2:1@60 DM_TEST_GIVE_INV=1 DM_TEST_STAT=12=30,14=200000 DM_TEST_WORLD_GO=near2:1:c148@200 DM_TEST_STAT_LOG=14"
    def akara(g):
        g.arrive(5874,4422,4,60.0); g.at("tap_unit 1 148 3",4.0); g.at("tap_unit 1 148 2",2.5); g.at("tap_unit 1 148 2",2.5); g.at("tap_unit 1 148 2",2.5)
        g.item("阿卡拉三行菜单点「交易」","进商人页、停在杂项页（背包布局看截图）",r"OpenPanel VendorPanelLayout"); g.at(f"tap_unit 1 148 2 +{MENU['akara2']}",2.5); g.shot()
    def lp(g,x,y): g.hold(x,y,.8); g.wait(2.0); g.clr(); g.shot()
    g=G("n",AK); g.sect("bulk",0); g.enter(); akara(g)
    g.item("货架上单击红药","出说明、不买：金币不变、背包占格不变",("inv",0,gold(0))); g.tap(P(284,196),1.5); g.clr(); g.shot()
    g.item("长按钥匙（背包里有 1 把）","买满进已有那一叠：金币减少、背包不多出新叠",("inv",0,gold('-'))); lp(g,284,177)
    g.item("单击背包里的钥匙","说明里数量是 12（满叠；看 key 那张截图）",None); g.tap(P(542.4,285.5),1.5); g.at(f"shot {g.name}_key.png",.3); g.clr(); g.shot()
    g.item("长按回城卷轴（背包里有回城书）","买满进书：金币减少、背包占格不变",("inv",0,gold('-'))); lp(g,284,140)
    g.item("长按鉴定卷轴（没有鉴定书）","只买 1 份：金币减少、背包多 1 格",("inv",1,gold('-'))); lp(g,284,159)
    g.item("长按红药（腰带满）","只买 1 瓶进背包：金币减少、背包多 1 格、腰带不变",("inv",1,both(gold('-'),belt(0)))); lp(g,284,196)
    g.item("双击红药","买 1 瓶：金币减少、背包多 1 格",("inv",1,gold('-'))); g.dbl(284,196); g.wait(1.0); g.clr(); g.shot()
    g.item("点右上角 X","关商人页",r"ClosePanel VendorPanelLayout"); g.tap(P(615,17)); g.shot()
    gs.append(g)


    g=G("p",AK); g.sect("belt",0); g.enter()
    g.item("喝掉腰带第 1、2 格","腰带 0/0/1/1",r"腰带 0/0/1/1"); g.tap("@belt1",1.0); g.tap("@belt2",2.0); g.shot()
    akara(g)
    g.item("长按红药（腰带有 2 个空槽）","两个空槽一次补满：腰带瓶数增加、金币减少、背包占格不变",("inv",0,both(gold('-'),belt('+')))); lp(g,284,196)
    g.item("再长按红药（腰带已补满）","买 1 瓶进背包：金币减少、背包多 1 格、腰带不变",("inv",1,both(gold('-'),belt(0)))); lp(g,284,196)
    g.item("点右上角 X","关商人页（腰带看截图）",r"ClosePanel VendorPanelLayout"); g.tap(P(615,17)); g.shot()
    gs.append(g)






    RE="DM_TEST_WORLD_GO=near2:1:c154@200 DM_TEST_STAT_LOG=14 DM_TEST_DUR=1@999999999"
    def charsi(g):
        g.enter(4); g.arrive(5776,4433,10,60.0); g.at("tap_unit 1 154 3",5.0)
        g.item("恰西三行菜单点「交易或修复」","进商人页",r"OpenPanel VendorPanelLayout"); g.at(f"tap_unit 1 154 2 +{MENU['trade3m']}",2.5); g.shot()
    g=G("ra",RE); g.sect("repall",0); charsi(g)
    g.item("交易页布局","左格全部修理、右格刷新货架、没有单件修理（看截图）",None); g.clr(); g.shot()
    g.item("点左格「全部修理」","全修：金币减少、盾耐久 86 / 86",both(gold('-'),dur(86))); g.tap(P(104,306),3.0); g.shot()
    g.item("再点一次全部修理","都满了：不再扣钱",gold(0)); g.tap(P(104,306),3.0); g.shot()
    g.item("点右上角 X","关商人页",r"ClosePanel VendorPanelLayout"); g.tap(P(615,17)); g.shot()
    gs.append(g)




















    g=G("q","DM_TEST_MERC_HP=30@80 DM_MERC_POT=0"); g.sect("mercpot",0); g.enter(4); g.wait(12.0); g.tap("@menu",1.4)
    g.item("点页签条「佣兵」","开佣兵页；属性表「生命」约 621 / 2142、左上角头像血条只剩三成（看截图）",r"OpenPanel HirelingInventoryPanel"); g.tap(P(469,18),1.5); g.shot()
    g.item("佣兵页开着双击腰带里的红药","喂佣兵：腰带少 1 瓶、玩家生命不变、佣兵生命读数涨；属性表「生命」也涨、左上角头像血条变长（对比上一张截图）",both(belt('-'),php(0),merc('+'))); g.dbl(486,197); g.wait(5.0); g.shot()
    g.tap(P(615,17),1.5); g.tap("@menu",1.4)
    g.item("佣兵页关着（只开物品栏）双击腰带里的红药","玩家自己喝：腰带少 1 瓶、玩家生命涨",both(belt('-'),php('+'))); g.dbl(346,198); g.wait(2.0); g.shot()
    g.item("点右上角 X","面板关闭",r"ClosePanel PlayerInventoryExpansionLayout"); g.tap(P(615,17)); g.shot()
    gs.append(g)








    HP="DM_TEST_STAT=7=2000000000,6=2000000000 DM_TEST_POS_LOG=1"; PH=MODE=="iphone"
    NOHIT=r"世界目标：点 \([^\n]*没有目标，不发令"
    def probe(g): g.item("（这一档点位还没量）点左下角空地一下","日志打出候选行",None); g.at("pos",.2); g.tap(P(170,150),2.5); g.shot()




    PK={"iphone":((198,138),(237,174),(211,115)),"ipadfull":((158,59),(205,103),(169,26))}.get(MODE)
    g=G("pk"); g.sect("pick",0); g.enter()
    if PK:
        g.item("点箱子选择框左边外面一点","不中：没有目标、不开箱",NOHIT,r"OpenPanel BankExpansionLayout"); g.tap(P(*PK[0]),2.0); g.shot()
        g.item("点箱子选择框下边外面一点","不中：没有目标、不开箱",NOHIT,r"OpenPanel BankExpansionLayout"); g.tap(P(*PK[1]),2.0); g.shot()
        g.item("点选择框左上角里面（身体框与占地菱形之外）","命中箱子：走过去开箱",r"OpenPanel BankExpansionLayout"); g.tap(P(*PK[2]),4.0); g.shot()
    else: probe(g)
    gs.append(g)


    g=G("wp","DM_TOWNWP_OFF=0"); g.sect("townwp",0)
    g.item("进局","进局那一拍就落在小站旁安全距离处（不再第 2 拍换关）；5 拍后人离小站横纵较大的那个 7–12 格（安全距离 10 格，找空格 ±2，转 45 度时横纵各约 7）",r"城镇小站落点：关卡 1 小站 \d+,\d+，进局落点 \d+,\d+[\s\S]*城镇小站落点：拍 \d+ 人在 \d+,\d+，小站 \d+,\d+（差 (?:-?(?:[7-9]|1[0-2]),-?\d+|-?\d+,-?(?:[7-9]|1[0-2]))）"); g.enter(); g.shot()
    g.item("点脚下的小站","开小站面板",r"OpenPanel WaypointsPanel"); g.at("tap_unit 2 119 5",4.0); g.shot()
    gs.append(g)








    GATE={"iphone":{"up":(369,150),"mid":(395,190),"low":(369,225),"out_up":(369,128),"out_dn":(369,276),"npc":(369,190)},
          "ipadfull":{"up":(390.6,70.3),"mid":(415.0,119.1),"low":(381.6,180.2),"out_up":(390.6,45.9),"out_dn":(381.6,241.2),"npc":(381.6,119.1)}}.get(MODE)
    for nm,key,what in (("g1","up","上段（只在选择框里）"),("g2","mid","中段右缘（只在选择框里）"),("g3","low","下段（身体框里）"),("g4","npc","中段凯恩身上")):
        g=G(nm,HP+" DM_TEST_WORLD_SEEK=120:1:c367:L103",save="save.rg_act4done"); g.sect("gate",0); g.enter(); g.wait(8.0)
        g.tap(P(170,150),1.2); g.tap(P(170,150),1.5)
        if not GATE: probe(g)
        elif nm=="g4":
            g.item("点在凯恩身上（门的选择框也盖着这一点）","选凯恩：走过去开他的对话，不换幕",r"世界目标：点 \([^\n]*类型 1 cls 246[\s\S]*OpenPanel (NpcDialogPanel|ScrollSpeechPanel)",r"pos 区域 109 "); g.at("pos",.2); g.tap(P(*GATE[key]),6.0); g.at("pos",.3); g.shot()
        else:
            if nm=="g1":
                g.item("点红门选择框上边外面","不中：没有目标、人不动",NOHIT,r"pos 区域 109 "); g.at("pos",.2); g.tap(P(*GATE["out_up"]),2.0); g.at("pos",.3); g.shot()
                g.item("点红门选择框下边外面","不中：没有目标、人不动",NOHIT,r"pos 区域 109 "); g.tap(P(*GATE["out_dn"]),2.0); g.at("pos",.3); g.shot()
            g.item(f"点红门{what}","命中：换幕到哈洛加斯（区域 109）",r"世界目标：点 \([^\n]*cls 566[\s\S]*pos 区域 109 "); g.at("pos",.2); g.tap(P(*GATE[key]),7.0); g.at("pos",.3); g.shot()
        gs.append(g)






    g=G("ex",HP+" DM_TEST_WARP_DIAG=1 DM_TEST_SEEK_NOACT=64 DM_TEST_WORLD_SEEK=150:2:c398:L103;0:5:c64:L83 DM_TEST_WORLD_XY=900:5153,1807",save="save.rg_act4"); g.sect("exit",0); g.enter(); g.wait(8.0)
    g.item("小站页点第三幕页签、第 8 行崔凡克","到崔凡克（区域 83）",r"pos 区域 83 "); g.tap(P(335,57),1.5); g.tap(P(333,280),6.0); g.at("pos",.3); g.shot()
    g.item("探路把人带到出口砖旁","还在崔凡克（看截图）",r"pos 区域 83 "); g.wait(26.0); g.at("pos",.3); g.shot()
    g.item("手指点出口砖","进憎恨囚牢一层（区域 100）",r"pos 区域 100 "); g.at("tap_unit 5 64 3",8.0); g.at("pos",.3); g.shot()
    gs.append(g)


    g=G("tb",HP+" DM_TEST_WARP_DIAG=1 DM_TEST_WORLD_XY=100:5140,5170;350:5120,5135;600:5095,5105 DM_TEST_WORLD_GO=2:c156@1000 DM_TEST_SEEK_NOACT=40 DM_TEST_WORLD_SEEK=1450:5:c40:L46",save="save.rg_joejoe"); g.sect("tomb",0)
    g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5)
    g.item("走到鲁高因小站，小站页点第 9 行法师峡谷","到法师峡谷（区域 46）",r"pos 区域 46 "); g.wait(44.0); g.shot(); g.tap(P(338,308),6.0); g.at("pos",.3); g.shot()
    g.item("探路把人带到古墓入口旁（逐间房传送，实测跳 59 次、约 14 秒）","还在法师峡谷（看截图）",r"pos 区域 46 "); g.wait(34.0); g.at("pos",.3); g.shot()
    g.item("手指点出口砖","进古墓（区域 68）；日志没有「把守它的物件」",r"pos 区域 68 ",r"把守它的物件"); g.at("tap_unit 5 40 3",8.0); g.at("pos",.3); g.shot()
    gs.append(g)


    g=G("tp","DM_TEST_WORLD_GO=2:c119@250 DM_TEST_POS_LOG=1"); g.sect("portal",0); g.enter(4); g.wait(10.0)
    g.item("营地小站页点第 2 行冰冷之原","到冰冷之原（区域 3）",r"pos 区域 3 "); g.tap(P(333,112),8.0); g.at("pos",.3); g.shot()
    g.item("点底栏回城卷轴钮","开出回城门（看截图）",None); g.tap("@portal",4.0); g.shot()
    g.item("点回城门中段（脚底往上 30 点）","回到营地（区域 1）",r"pos 区域 1 "); g.at(f"tap_unit 2 59 3 +{O(0,-30)}",7.0); g.at("pos",.3); g.shot()
    gs.append(g)
    if MODE=="ipadfull":
        TOP=256
        g=G("top","DM_TEST_GOLD=1300 DM_TEST_WORLD_GO=near2:1:c148@700 DM_TEST_WORLD_GO2=near2:2:c267@1900"); g.sect("top",0); g.enter()
        g.item("局内截图","小地图与地名在屏幕真顶部右上角、没被裁（看截图）",None); g.shot()
        g.item("点真顶部的区域名那一条","切全屏地图",r"点小地图那一块.*→ 全屏地图"); g.tap("@mmname"); g.shot()
        g.item("全屏地图再点区域名那一条","回小地图",r"点小地图那一块.*→ 回小地图"); g.tap("@mmname"); g.shot()
        g.item("开物品栏","标题栏连在面板上（看截图）",r"OpenPanel PlayerInventoryExpansionLayout"); g.tap("@menu"); g.shot()
        g.item("开设置页","标题栏连在面板上（看截图）",r"OpenPanel SettingsPanel"); g.tap(P(551,17)); g.at("tap_ui Settings@PauseLayoutGarden 3",1.8); g.shot()
        g.tap(P(420,345),1.8); g.tap(P(615,17),1.5)
        g.item("开箱子","标题栏连在面板上（看截图）",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.shot()
        g.tap(P(654.5,17),1.5)


        g.item("从出生点走到阿卡拉旁","游戏自己寻路走到",r"\[script\] await pos：区域 1",r"走不过去"); g.arrive(5874,4422,4,60.0); g.shot()
        g.item("点阿卡拉、跳过两段讲话、再点她","出三行菜单（看截图）",r"OpenPanel NpcDialogPanel[\s\S]*OpenPanel NpcDialogPanel"); g.at("tap_unit 1 148 3",4.0); g.at("tap_unit 1 148 2",2.5); g.at("tap_unit 1 148 2",2.5); g.at("tap_unit 1 148 2",2.5); g.shot()
        g.item("阿卡拉菜单点最后一行「取消」","命中那一行并关菜单",r"对话菜单 \+2 行"); g.at(f"tap_unit 1 148 2 +{MENU['trade3']}",2.0); g.shot()




        g.item("从阿卡拉处走回箱子旁","游戏自己寻路走到，不在半路停",r"\[script\] await pos：区域 1",r"走不过去|没找到"); g.arrive(5808,4443,6,130.0); g.shot()
        for x,y,r,to in [(5788,4475,4,12),(5812,4503,4,9),(5826,4522,4,8),(5840,4560,5,8)]: g.at(f"walk_to {x} {y} {r} {to} skip",to+.5)
        g.item("野外","在区域 2",r"pos 区域 2 "); g.at("pos",.3); g.shot()
        g.item("野外点真顶部区域名那一条","切全屏地图；玩家标记压在玩家身上（看截图）",r"点小地图那一块.*→ 全屏地图"); g.tap("@mmname"); g.shot()
        g.item("野外全屏地图再点区域名那一条","回小地图",r"点小地图那一块.*→ 回小地图"); g.tap("@mmname"); g.shot()


        g.item("物品栏开着，点面板上方那一截","面板关掉、不切页签",r"ClosePanel PlayerInventoryExpansionLayout",r"界面页签：切"); g.tap("@menu",1.4); g.shot(); g.tap(f"683,{-TOP//2}",1.5); g.shot()
        gs.append(g)




    def row(g,cls,k,title,expect,res=None,reopen=True):
        if reopen: g.at(f"tap_unit 1 {cls} 2",2.2)
        g.item(title,expect,menu(k,res)); g.at(f"tap_ui ListItem{k}@NpcDialogPanel 3",2.5); g.shot()
    def away(g): g.tap(P(20,240),1.2); g.tap(P(20,240),1.2)


    g=G("me1","DM_TEST_GIVE=lsd:2:2,qui:2:2@60 DM_TEST_GIVE_INV=1 DM_TEST_WORLD_GO=near2:1:c154@200"); g.sect("menu4",0); g.enter(1); g.arrive(5776,4433,10,60.0)
    g.item("点恰西","出四行菜单（看截图）",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 154 3",4.5); g.shot()
    row(g,154,0,"点第 1 行「交谈」","落在交谈",None,reopen=False); away(g)
    row(g,154,1,"点第 2 行「交易或修复」","进商人页",r"OpenPanel VendorPanelLayout"); g.tap(P(615,17),1.5)
    row(g,154,2,"点第 3 行「灌魔」","开灌魔面板（关闭钮在外框右上角凹槽里，看截图）",r"OpenPanel ImbueItemsPanel")
    g.item("空槽时点灌魔面板的关闭钮","灌魔面板、背包、外框都关（看截图）",r"ClosePanel ImbueItemsPanel",r"c2s 17\b"); g.at("tap_ui TouchClose@ImbueItemsPanel 3",2.0); g.shot()
    g.item("关掉后推摇杆","人能走：坐标变了",moved); g.at("pos",.2); g.at("down 1 @joy",.3); g.at("move 1 @joy+60,0",.5); g.at("up 1",.5); g.at("pos",.3)
    g.at("tap_unit 1 154 3",5.0)
    row(g,154,3,"点第 4 行「取消」","菜单关",r"ClosePanel NpcDialogPanel",reopen=False)
    g.item("重开菜单后点菜单外","菜单关",r"ClosePanel NpcDialogPanel"); g.at("tap_unit 1 154 2",2.2); g.tap(P(20,240),1.5); g.shot()
    gs.append(g)


    g=G("me2","DM_TEST_WORLD_GO=near2:1:c150@200"); g.sect("menu4k",0); g.enter(1); g.arrive(5838,4454,10,45.0)
    g.item("点卡夏","出四行菜单（看截图）",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 150 3",4.0); g.shot()
    row(g,150,0,"点第 1 行「交谈」","落在交谈",None,reopen=False); away(g)
    row(g,150,1,"点第 2 行「复活佣兵」（金币不够）","落在复活；出「你没有足够的金币那么做 / 继续」单项框（看截图）",None)
    g.item("单项提示框点一下","框关掉；随后再点卡夏菜单能重开（没卡死）",r"OpenPanel NpcDialogPanel"); g.tap(P(20,240),1.5); g.tap(P(20,240),1.5); g.at("tap_unit 1 150 2",2.5); g.shot()
    row(g,150,2,"点第 3 行「雇佣」","开雇佣名单",r"OpenPanel HireMenuPanel",reopen=False); away(g)
    row(g,150,3,"点第 4 行「取消」","菜单关",r"ClosePanel NpcDialogPanel")
    g.item("关掉后推摇杆","人能走：坐标变了",moved); g.at("pos",.2); g.at("down 1 @joy",.3); g.at("move 1 @joy+60,0",1.2); g.at("up 1",.5); g.at("pos",.3)
    gs.append(g)


    g=G("me3","DM_TEST_WORLD_GO=near2:1:c154@200"); g.sect("menu3",0); g.enter(4); g.arrive(5776,4433,10,60.0)
    g.item("点恰西","出三行菜单（看截图）",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 154 3",4.5); g.shot()
    row(g,154,0,"点第 1 行「交谈」","落在交谈",None,reopen=False); away(g)
    row(g,154,1,"点第 2 行「交易或修复」","进商人页",r"OpenPanel VendorPanelLayout"); g.tap(P(615,17),1.5)
    row(g,154,2,"点第 3 行「取消」","菜单关",r"ClosePanel NpcDialogPanel")
    gs.append(g)


    g=G("me4","DM_TEST_WORLD_GO=near2:1:c150@200"); g.sect("menu2",0); g.enter(); g.arrive(5838,4454,10,45.0)
    g.item("点卡夏、关掉开场白、再点她","出两行菜单（看截图）",r"OpenPanel NpcDialogPanel"); g.at("tap_unit 1 150 3",4.0); away(g); g.at("tap_unit 1 150 2",2.5); g.shot()
    row(g,150,0,"点第 1 行「交谈」","落在交谈",None,reopen=False); away(g)
    row(g,150,1,"点第 2 行「取消」","菜单关",r"ClosePanel NpcDialogPanel")
    gs.append(g)








    g=G("sl1","DM_STATE=pwstate.jsonl DM_TEST_GIVE=lgl:2:2,qui:2:2@60 DM_TEST_GIVE_INV=1 DM_TEST_WORLD_GO=near2:1:c154@200"); g.sect("slot",0); g.enter(1); g.arrive(5776,4433,10,60.0)
    def imbue(g): g.at("tap_unit 1 154 3",4.5); g.item("恰西菜单点「灌魔」","开灌魔面板",r"OpenPanel ImbueItemsPanel"); g.at("tap_ui ListItem2@NpcDialogPanel 3",3.0); g.shot()
    GL=(552.2,275.8); QU=(513.1,266); QU2=(464.2,246.5)
    imbue(g)
    g.item("手套从背包拖到面板底板（没对准槽）","回原格",("seg","lgl",1,0,"same")); g.dragp(GL,(100,330)); g.wait(2.0); g.shot()
    g.item("双击背包里的手套","进槽",("seg","lgl",2,4)); g.dbl(*GL); g.wait(2.0); g.shot()
    g.item("把槽里的手套拖回背包（松手在它原来那格上）","回背包",("seg","lgl",3,0)); g.dragp((188,197),(GL[0],GL[1]+40)); g.wait(2.0); g.shot()
    g.item("槽空着时把布甲拖到背包别的空位","照常能拖：布甲换了格",("seg","qui",1,0,"moved")); g.dragp(QU,(QU2[0],QU2[1]+40)); g.wait(2.0); g.shot()
    g.item("把手套从背包拖到槽上","进槽",("seg","lgl",4,4)); g.dragp(GL,(188,237)); g.wait(2.0); g.shot()
    g.item("槽里有东西时：把布甲拖到面板外、双击它、长按它","都不动：槽里那件一直在槽里，布甲在物品表里没有任何变化",("still","lgl",4,"qui")); g.dragp(QU2,(20,240)); g.wait(1.0); g.dbl(*QU2); g.wait(1.0); g.hold(QU2[0],QU2[1],.8); g.wait(1.0); g.shot()
    g.item("槽里有物品时点面板的关闭钮","面板关、手套回背包（不丢地上）",("seg","lgl",5,0,r"ClosePanel ImbueItemsPanel")); g.at("tap_ui TouchClose@ImbueItemsPanel 3",3.0); g.shot()
    imbue(g)
    g.item("双击背包里的宝石（不合格）","拒收：宝石始终在背包、没进过槽",("segnot",568,4)); g.dbl(483.75,285.5); g.wait(2.0); g.shot()
    g.item("双击手套再进槽","进槽",("seg","lgl",6,4)); g.dbl(*GL); g.wait(2.0); g.shot()
    g.item("点「灌魔」确认","灌魔完成：原来那件没了（被换成新物品）、面板关",("seg","lgl",7,-1,r"ClosePanel ImbueItemsPanel")); g.at("tap_ui ConfirmWithText@ImbueItemsPanel 3",3.0); g.shot()
    g.item("灌魔出的新物品","在背包里",("bag","lgl")); g.tap("@menu",1.5); g.shot()
    gs.append(g)




    g=G("sl2","DM_STATE=pwstate.jsonl DM_TEST_WORLD_GO=2:c119@250 DM_TEST_WORLD_GO2=1:c511@900 DM_TEST_STAT_LOG=14 DM_TEST_POS_LOG=1"); g.sect("socket2",0); g.enter(4); g.wait(8.0)
    g.item("营地小站页点第五幕页签、第 1 行哈洛加斯","到哈洛加斯（区域 109）",r"pos 区域 109 "); g.wait(4.0); g.tap(P(459,58),1.5); g.tap(P(337,83),8.0); g.at("pos",.3); g.shot()
    g.item("走到拉苏克出菜单，点「交易或修复」","进商人页",r"OpenPanel VendorPanelLayout"); g.wait(14.0); g.at("tap_ui ListItem1@NpcDialogPanel 8",3.0); g.shot()
    g.item("货架上双击第一件","买下：金币减少",gold('-')); g.dbl(109.5,102); g.wait(1.5); g.shot()
    g.tap(P(615,17),1.5); g.at("tap_unit 1 511 2",2.5)
    g.item("拉苏克菜单点「添加镶孔」","开打孔面板（看截图：槽、背包里刚买的那件在哪）",r"OpenPanel \w+"); g.at("tap_ui ListItem2@NpcDialogPanel 3",3.0); g.shot()
    g.item("双击背包里刚买的那件","进槽",("held",0)); g.dbl(512.6,246.2); g.wait(2.5); g.shot()
    g.item("点确认","打孔完成：那件离开槽、面板关（看截图里物品说明有孔）",("held",0,"ended")); g.tap(P(188,252),3.0); g.shot()
    gs.append(g)


    g=G("sl3","DM_STATE=pwstate.jsonl DM_TEST_GIVE=cap:2:1@600 DM_TEST_GIVE_INV=1 DM_TEST_WORLD_GO=2:c119@250 DM_TEST_WORLD_GO2=1:c512@950 DM_TEST_STAT_LOG=14 DM_TEST_POS_LOG=1"); g.sect("personal",0); g.enter(4); g.wait(8.0)
    g.item("营地小站页点第五幕页签、第 1 行哈洛加斯","到哈洛加斯（区域 109）",r"pos 区域 109 "); g.wait(4.0); g.tap(P(459,58),1.5); g.tap(P(337,83),8.0); g.at("pos",.3); g.shot()
    g.item("走到安亚、关掉两段开场白、再点她出菜单，点第 4 行个性化","开个性化面板（看截图）",r"OpenPanel \w+"); g.wait(18.0); g.tap(P(20,240),1.5); g.tap(P(20,240),1.5); g.tap(P(20,240),1.5); g.at("tap_unit 1 512 3",3.0); g.at("tap_ui ListItem3@NpcDialogPanel 5",3.0); g.shot()
    g.item("双击背包里的软帽","进槽",("seg","cap",1,4)); g.dbl(513.1,236.75); g.wait(2.5); g.shot()
    g.item("槽里有物品时点关闭钮","面板关、软帽回背包",("seg","cap",2,0)); g.tap(P(615,17),3.0); g.shot()
    gs.append(g)




    def notworn(s,before): return "不通：被穿上了（"+re.search(r"界面双击：[^\n]*装上",s).group(0)[:30]+"）" if re.search(r"界面双击：[^\n]*装上",s) else "通：没有被穿上"
    g=G("cb","DM_STATE=pwstate.jsonl DM_TEST_GIVE=cap:2:1@60 DM_TEST_GIVE_INV=1"); g.sect("cube2",0); g.enter(4); g.wait(2.0); g.tap("@menu",1.4)
    g.item("物品栏里双击背包里的魔盒","打开魔盒页",r"OpenPanel HoradricCubeLayout"); g.dbl(413.6,273.75); g.wait(1.0); g.shot()
    g.item("魔盒页开着双击背包里的符文","进魔盒：背包少 1 格",("inv",-1)); g.dbl(503.3,285.5); g.wait(1.5); g.shot()
    g.item("双击魔盒里的那枚符文","回背包：背包多 1 格",("inv",1)); g.dbl(205.6,179.6); g.wait(1.5); g.shot()
    g.item("魔盒页开着双击背包里的软帽（装备）","进魔盒（物品表里到了魔盒那一页）、没被穿上",("segpage","cap",1,3)); g.dbl(513.1,236.75); g.wait(2.0); g.shot()
    g.item("双击魔盒里的软帽","回背包",("segpage","cap",2,0)); g.dbl(178,183); g.wait(2.0); g.shot()
    g.tap(P(615,17),1.5); g.tap(P(615,17),1.2); g.tap("@menu",1.4)
    g.item("魔盒页没开时（只开物品栏）双击背包里的软帽","照旧穿上",r"界面双击：[^\n]*装上"); g.dbl(373.9,234.75); g.wait(1.5); g.shot()
    gs.append(g)


    g=G("dz",HP+" DM_STATE=pwstate.jsonl DM_TEST_WORLD_XY=100:5140,5170;350:5120,5135;600:5095,5105 DM_TEST_WORLD_GO=2:c156@1000 DM_TEST_WORLD_SEEK=1450:5:c39:L46;1200:2:c152:L67",save="save.rg_joejoe")
    g.t=3.5; g.tap(P(333,336),1.6); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5)
    g.item("走到鲁高因小站，小站页点第 9 行法师峡谷","到法师峡谷（区域 46）",r"pos 区域 46 "); g.wait(44.0); g.tap(P(338,308),6.0); g.at("pos",.3); g.shot()
    g.item("探路进真墓、交互底座","开底座放置面板（ImbueItemsPanel），在区域 67",r"OpenPanel ImbueItemsPanel"); g.t=max(g.t,200.0); g.at("pos",.3); g.shot()


    g.item("双击背包里的赫拉迪姆法杖","进底座槽（物品表里有物品稳定地在槽里）",("held",0)); g.dbl(464,255); g.wait(2.0); g.shot()
    g.item("点底座面板的确认钮","法杖插进底座：离开槽、面板关（看截图：底座上插着法杖）",("held",0,"ended",r"ClosePanel ImbueItemsPanel")); g.tap(P(188.5,252.5),4.0); g.shot()
    gs.append(g)


    g=G("sb","DM_TEST_GIVE=r01:2:1@60 DM_TEST_GIVE_INV=1 DM_TEST_GOLD=1300"+" DM_TEST_WORLD_GO=near2:2:c267@200"); g.sect("sbtn",0); g.enter(); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    for k,nm in [(1,"共享"),(2,"珍宝匣"),(5,"编年史"),(2,"珍宝匣"),(0,"个人")]:
        g.item(f"点页签 Image{k}（{nm}）","切页",r"界面页签：切 [+-]\d+ 页"); g.at(f"tap_ui Image{k}@BankExpansionLayout 3",1.5); g.shot()
    g.at("tap_ui Image1@BankExpansionLayout 3",1.5)
    g.item("共享页点右箭头","翻到第 2 页",r"SharedStashRight"); g.at("tap_ui TouchStashRight@BankExpansionLayout 3",1.5); g.shot()
    g.item("再点右箭头","翻到第 3 页",r"SharedStashRight"); g.at("tap_ui TouchStashRight@BankExpansionLayout 3",1.5); g.shot()
    g.item("点左箭头","回第 2 页",r"SharedStashLeft"); g.at("tap_ui TouchStashLeft@BankExpansionLayout 3",1.5); g.shot()
    g.item("再点左箭头","回第 1 页",r"SharedStashLeft"); g.at("tap_ui TouchStashLeft@BankExpansionLayout 3",1.5); g.shot()
    g.at("tap_ui Image0@BankExpansionLayout 3",1.5)
    g.item("点物品栏金币钮（存入）","弹数量框",r"OpenPanel DropGoldModal"); g.at("tap_ui gold_button@PlayerInventoryExpansionLayout 3",1.5); g.shot()
    g.item("数量框点取消","关框",r"ClosePanel DropGoldModal"); g.tap(P(375,200),1.5); g.shot()
    g.item("点右上角关闭 close","箱子关、加宽还原",r"ClosePanel BankExpansionLayout[\s\S]*储藏箱加宽：关|储藏箱加宽：关[\s\S]*ClosePanel BankExpansionLayout"); g.at("tap_ui close@BankExpansionLayout 3",2.0); g.shot()
    gs.append(g)






    RWE="DM_TEST_GIVE=cap:2:7,r04:2:1,r03:2:1@60 DM_TEST_GIVE_INV=1"
    LISTOK=r"符文之语：长按[^\n]*→ 清单 [1-9]\d* 行"
    def anyl(p): return lambda s,b: ("通：" if re.search(p,b+s) else "不通：")+"整局日志"






    g=G("rwlv","DM_TEST_GIVE=uhm:2:7@60 DM_TEST_GIVE_INV=1"); g.sect("rwlv",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.item("长按背包里带孔的塔盔","弹清单，需要等级 59（看截图：每行都是 59）",r"长按 uhm[^\n]*需要等级 59",r"长按 uhm[^\n]*需要等级 79|【红】符文之语"); g.hold(395,267); g.wait(1.0); g.shot()
    g.item("点清单外","清单关掉",r"清单外 → 关清单|清单关闭"); g.tap(P(120,200),1.5); g.shot()
    gs.append(g)
    g=G("rw1",RWE+" DM_TEST_RW_LIST=75"); g.sect("rw1",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.item("背包布局","软帽、符文在 k 局同样的位置（看截图）",None); g.shot()
    g.item("长按 1 孔软帽","不开清单（1 孔帽子没有能做的符文之语）",None,LISTOK); g.hold(376,228); g.wait(1.0); g.shot()
    g.item("长按 2 孔软帽","弹清单：天底那一行有「一键合成」（看截图：名字、需要等级、符文编号与数量）",LISTOK); g.hold(395,267); g.wait(1.0); g.shot()
    g.item("点清单外（左边游戏画面）","清单关掉",r"清单外 → 关清单|清单关闭"); g.tap(P(120,200),1.5); g.shot()
    g.item("同一落点再长按 2 孔软帽、点第一行「一键合成」","再开清单（不是整理背包）、成型：两颗进孔、标志带 0x04000000",r"清单 [1-9]\d* 行[\s\S]*一键合成成型",r"【红】符文之语|整理背包"); g.hold(395,267); g.wait(1.0); g.at("tap_ui TouchRwGo0 3",4.0); g.shot()
    g.item("两枚符文原来的格子","都空了",("spot","empty",(304,245))); g.shot()
    g.item("单击那顶软帽","说明第一行是「天底」（看截图）",None); g.tap(P(395,267),1.5); g.shot()
    g.item("读回","物品表里有 1 件符文之语物品、孔里 2 颗",anyl(r"DM_TEST_RW_LIST：符文之语物品[^\n]*孔里 2 颗")); g.t=max(g.t,80.0); g.shot()
    gs.append(g)


    g=G("rw2",RWE); g.sect("rw2",0); g.enter(); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.tap(P(148,53),1.5); g.shot()
    g.item("符文页开着双击背包里两枚符文","各进符文页自己那一格：背包少 2 格",None); g.clr(); g.shot(); g.dbl(483.2,227); g.wait(1.0); g.dbl(483.2,246.5); g.wait(1.0); g.clr(); g.shot()
    g.tap(P(654.5,17),1.5); g.tap("@menu",1.4)
    g.item("箱子关着、只开物品栏长按 2 孔软帽","清单写「符文页 1」、没有「一键合成」、没有红字（看截图）",LISTOK); g.hold(395,267); g.wait(1.0); g.shot()
    g.tap(P(120,200),1.2); g.tap(P(615,17),1.5)
    g.item("开箱子、长按物品栏里的 2 孔软帽","这次有「一键合成」",LISTOK); g.at("tap_unit 2 267 3",3.0); g.hold(561.4,266); g.wait(1.0); g.shot()
    g.item("点「一键合成」","先从符文页取件、再镶，成型",r"从符文页(?:取 1 颗|成叠拿起)[\s\S]*一键合成成型",r"【红】符文之语"); g.at("tap_ui TouchRwGo0 3",5.0); g.shot()
    gs.append(g)




    BX=(294,320,346,371)
    g=G("bd","DM_TEST_GIVE=hp1:2:3@60 DM_TEST_GIVE_INV=1 DM_STATE=pwstate.jsonl DM_POT_PICK=0"); g.sect("bd",0); g.enter(); g.wait(2.0)
    g.item("底栏喝掉腰带第 1、2 格","腰带 0/0/1/1",r"腰带 0/0/1/1"); g.tap("@belt1",1.0); g.tap("@belt2",2.0); g.shot()
    g.tap("@menu",1.4)
    g.item("物品栏腰带空格（第 1 格）长按 1.2 秒","不动、不整理（看截图）",r"界面长按：腰带 → 不动",r"整理背包|一键补满"); g.hold(BX[0],197,1.2); g.wait(1.0); g.shot()
    g.item("腰带空格双击","一键补满：两个空格都满（1/1/1/1）、日志有「腰带补满：调了原版自动装入」",lambda s,b: ("通：" if all(re.search(x,s) for x in (r"腰带空格 → 一键补满",r"腰带补满：调了原版自动装入",r"腰带 1/1/1/1")) else "不通：")+"补满三行",r"整理背包"); g.dbl(BX[0],197); g.wait(1.5); g.shot()
    g.item("有药格（第 4 格）双击","照旧使用（喝）",r"界面双击：[^\n]*→ 使用"); g.dbl(BX[3],197); g.wait(1.5); g.shot()
    g.item("腰带空格吸附边缘（第 2 格偏向第 3 格 6 点）长按","仍按空格算：不动、不整理",r"界面长按：腰带 → 不动",r"整理背包"); g.hold(BX[1]+6,197,1.2); g.wait(1.0); g.shot()
    gs.append(g)


    g=G("ba","DM_TEST_GIVE=hp1:2:3@60 DM_TEST_GIVE_INV=1"); g.sect("ba",0); g.enter(); g.wait(2.0)
    g.item("底栏喝掉腰带第 1、2 格","腰带 0/0/1/1",r"腰带 0/0/1/1"); g.tap("@belt1",1.0); g.tap("@belt2",2.0); g.shot()
    def ba_ok(s,b):
        m=re.search(r"开背包补腰带：开始.*?药水合计 (\d+)[\s\S]*?开背包补腰带：完成，放了 (\d+) 瓶.*?药水合计 (\d+)",s)
        if not m: return "不通：没有「开背包补腰带：开始 … 完成」两行"
        ok=m.group(2)=="2" and m.group(1)==m.group(3) and re.search(r"腰带 1/1/1/1",s)
        return ("通：" if ok else "不通：")+f"放了 {m.group(2)} 瓶、药水合计 {m.group(1)} → {m.group(3)}"+("" if re.search(r"腰带 1/1/1/1",s) else "；没见「腰带 1/1/1/1」")
    g.item("开物品栏","第 1、2 列各补 1 瓶小红，腰带 1/1/1/1，药水合计不变",ba_ok); g.tap("@menu",3.0); g.shot()
    gs.append(g)


    g=G("bdm","DM_TEST_MERC_HP=30@80 DM_MERC_POT=0"); g.sect("bdm",0); g.enter(4); g.wait(12.0)
    g.item("底栏腰带第 1 格连点两下","都给自己喝：玩家生命涨、佣兵生命不变",both(php('+'),merc(0))); g.tap("@belt1",.25); g.tap("@belt1",5.0); g.shot()
    gs.append(g)


    def ms_ok(s,b):
        m=re.findall(r"一键重开：「([^」]+)」难度 (\d) 进局读取（从点按钮起 (\d+) ms）",s); c=re.findall(r"一键重开：进局后核对，当前角色「([^」]+)」，刚退出的是「([^」]+)」→ (\S+)",s)
        bad=re.search(r"OpenPanel (CharacterSelectPanel|MainMenuPanel|CharacterDifficultyModal)|【红】",s); sel=re.search(r"原版列表第 \d+ 项是「[^」]+」，选中槽原为 \d+ → (已改选它|本来就是它)",s)
        if not m: return "不通：没有「进局读取」行"
        if not c: return "不通：没有「进局后核对」行；"+f"读取 {m[-1][2]} ms"
        if not sel: return "不通：没有「原版列表第 N 项是…→ 已改选它 / 本来就是它」行"
        ok=int(m[-1][2])<=200 and c[-1][2].startswith("相符") and not bad
        return ("通：" if ok else "不通：")+f"{sel.group(1)}；「{m[-1][0]}」难度 {m[-1][1]}，点按钮到读取 {m[-1][2]} ms；核对 {c[-1][0]} / {c[-1][1]} → {c[-1][2]}"+(f"；出现「{bad.group(0)}」" if bad else "")
    def rs3(g,panel,want_diff):
        for k in range(3):
            g.tap("@menu"); g.tap(P(551,17),1.5)
            g.item(f"第 {k+1} 次一键重开","读取 ≤ 200 ms、进局后核对相符、难度 %d、不开选角页 / 主菜单"%want_diff,lambda s,b,d=want_diff: (lambda r: r if not r.startswith("通") or f"难度 {d}" in r else "不通："+r[2:])(ms_ok(s,b)))
            g.at(f"tap_ui TouchRestart@{panel} 3",9.0); g.shot()


    g=G("rf1","DM_TEST_POS_LOG=1"); g.sect("rf1",0); g.ui("ListItem0@CharacterSelectPanel",1.3); g.tap(P(333,336),1.6); g.tap(P(333,203),1.0); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5)
    rs3(g,"PauseLayoutGarden",2); gs.append(g)


    g=G("rf2","DM_TEST_POS_LOG=1"); g.sect("rf2",0); g.enter(4); g.wait(2.0); rs3(g,"PauseLayout",0); gs.append(g)


    g=G("rf3","DM_TEST_ICLOUD_DIR=icrF/saves DM_TEST_POS_LOG=1"); g.sect("rf3",0); g.enter(); g.wait(2.0); rs3(g,"PauseLayoutGarden",0)
    g.item("iCloud","每次退出都传一次（3 次「退出这一局回到前端，上传」）",lambda s,b: (lambda n: ("通：" if n>=3 else "不通：")+f"{n} 次")(len(re.findall(r"退出这一局回到前端，上传",b+s)))); g.shot()
    gs.append(g)








    TABS={"暗金":237,"套装":302,"符文之语":366,"首领":431}
    def chron(g):
        g.tap("@menu"); g.tap(P(551,17),1.5)
        g.item("暂停 → 编年史","编年史开，四个页签（看截图）",r"OpenPanel ChroniclePanel"); g.at("tap_ui Chronicle@PauseLayoutGarden 3",2.0); g.shot()


    g=G("kba"); g.sect("kba",0); g.enter(); chron(g)
    g.item("点第四页签「首领」","切到积分牌：左列五幕、三个勾选、排序下拉、左下总击杀（看截图）",r"界面页签：切"); g.tap(P(TABS["首领"],55.5),2.0); g.shot()
    for k in range(5):
        g.item(f"点第{'一二三四五'[k]}幕","切幕（看截图：右列换成这一幕的首领）",r"TouchKB_Act%d"%k); g.at(f"tap_ui TouchKB_Act{k} 3",1.5); g.shot()
    g.item("第五幕列表往上滑","长列表能滚（看截图）",None); g.at("tap_ui TouchKB_Act4 3",1.0); g.drag(P(460,250),P(460,110),.4); g.wait(1.0); g.shot()
    for k,nm in enumerate(("普通","噩梦","地狱")):
        g.item(f"勾「{nm}」","勾上（看截图）",r"TouchKB_D%d"%k); g.at(f"tap_ui TouchKB_D{k} 3",1.5); g.shot()
        g.item(f"再点「{nm}」","取消勾选",r"TouchKB_D%d"%k); g.at(f"tap_ui TouchKB_D{k} 3",1.5)
    for k,nm in enumerate(("按幕顺序","按次数","按最快")):
        g.item(f"排序下拉选「{nm}」","选中这项、不崩（看截图）",r"DropdownListWidgetMessage",r"CRASH|【红】"); g.at("tap_ui TouchKB_Sort 3",1.2); g.at(f"shot {g.name}_dd{k}.png",.3); g.tap(P(468,(86,95.5,105.5)[k]),1.5); g.shot()
    for nm in ("暗金","套装","符文之语"):
        g.item(f"切回「{nm}」页","原版页完好、积分牌不残留（看截图）",r"界面页签：切"); g.tap(P(TABS[nm],55.5),2.0); g.shot()
    g.item("关编年史再开","头一帧不闪积分牌（看截图：停在原版页）",r"OpenPanel ChroniclePanel"); g.tap(P(615,17),1.5); g.at("tap_ui Chronicle@PauseLayoutGarden 3",0.1); g.at(f"shot {g.name}_f1.png",1.5); g.shot()
    gs.append(g)


    g=G("kbb","DM_TEST_GOTO_LEVEL=60:2 DM_TEST_SPAWN=150:156:5,5 DM_TEST_SLAY=160 DM_TEST_WORLD_GO=near:monster@200 DM_TEST_STAT=7=2560000,6=2560000 DM_TEST_KILL_DROP=cap:7,lgl:5 DM_TEST_POS_LOG=1"); g.sect("kbb",0); g.enter(); g.t=max(g.t,16.0)
    g.item("首领出现","系统提示「安达莉尔已累计击杀 0 次」类（看截图）",r"刷怪积分牌：拍 \d+ 提示出现"); g.wait(2.0); g.shot()
    for k in range(16): g.at("down 1 @main",.12); g.at("up 1",.38)
    g.item("主钮连点 8 秒","击杀：日志「第 1 次」与「暗金 1、套装 1」，提示「第 1 次成功击杀」（看截图）",lambda s,b: ("通：" if re.search(r"刷怪积分牌：拍 \d+ \S+ 难度 0 第 1 次",b+s) and re.search(r"死后那一批掉落：暗金 1、套装 1",b+s) else "不通：")+"击杀与掉落两行（整局）"); g.wait(2.0); g.shot()
    g.wait(4.0); chron(g)
    g.item("点「首领」页签、第一幕","安达莉尔「击杀 1」「暗金 1 · 套装 1」、首杀时刻（看截图）",r"界面页签：切"); g.tap(P(TABS["首领"],55.5),2.0); g.at("tap_ui TouchKB_Act0 3",1.5); g.shot()
    g.item("宿主时区","日志「ToCalendarTime：首次换算」的偏移",lambda s,b: (lambda m: ("通：" if m else "未判：")+(m.group(0) if m else "没有这行"))(re.search(r"ToCalendarTime：首次换算[^\n]*",b+s)))
    gs.append(g)


    WD="DM_WATCH_STALL=15"
    def nodie(s,b):
        m=re.search(r"【红】看门狗[^\n]*",b+s); return ("不通："+m.group(0)[:90]) if m else "通：整局没有看门狗判死"
    g=G("wd1",WD); g.sect("wd1",0); g.enter(); g.wait(2.0)
    g.item("保存并退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",5.0); g.shot()
    g.wait(30.0)
    g.item("前端停 30 秒后（看门狗 15 秒）","没有看门狗判死、进程还在",nodie); g.shot()
    g.item("再进局","进局",r"FrontEndNavigation ToGame"); g.ui("ListItem2@CharacterSelectPanel",1.3); g.tap(P(333,336),12.0); g.shot()
    gs.append(g)


    g=G("wd2",WD); g.sect("wd2",0); g.t=3.5
    g.item("启动后什么都不点","按原设计判死：「【红】看门狗：第 N 秒起无进展」",r"【红】看门狗：第 \d+ 秒起无进展"); g.wait(40.0)
    gs.append(g)


    def pglog(log,tick):
        rows=re.findall(r"个人仓库读数：拍 %d (\S+) 格 (\d+),(\d+) 孔里 (\d+) 件"%tick,log); return sorted(rows)
    def same(t0,t1):
        def f(s,b):
            a=pglog(b+s,t0); c=pglog(b+s,t1)
            if not a: return "未判：拍 %d 没有读数"%t0
            return ("通：" if a==c else "不通：")+f"拍 {t0} {len(a)} 件 / 拍 {t1} {len(c)} 件"+("，逐件（码、格、孔里件数）相同" if a==c else f"；不同：{sorted(set(a)^set(c))[:6]}")
        return f


    g=G("pg1","DM_TEST_PG_LOG=150,700 DM_TEST_PG_GO=300:+1,500:-1"); g.sect("pg1",0); g.enter(4); g.t=max(g.t,24.0)
    g.item("翻到第 2 页（开关在拍 300 翻，早于本项记号，判整局）","「第 1 → 2 页，搬出 N 件」",lambda s,b: ("通：" if re.search(r"个人仓库：第 1 → 2 页，搬出 \d+ 件",b+s) and not re.search(r"【红】个人仓库",b+s) else "不通：")+"整局日志"); g.t=max(g.t,30.0); g.shot()
    g.item("翻回第 1 页","「第 2 → 1 页 … 放回 N 件」",r"个人仓库：第 2 → 1 页",r"【红】个人仓库"); g.t=max(g.t,38.0); g.shot()
    g.item("翻走翻回后","个人仓库格里逐件（码 / 格 / 孔里件数）与翻前相同",same(150,700)); g.t=max(g.t,44.0)
    gs.append(g)


    g=G("pg2","DM_STATE=pwstate.jsonl"); g.sect("pg2",0); g.enter(); g.wait(2.0)
    g.item("点箱子","开储藏箱（个人页，底下页码「第 1/5 页」，看截图）",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("个人页开着双击背包里的卷轴","进个人页第 1 页（看截图）",None); g.dbl(600.5,266); g.wait(1.5); g.clr(); g.shot()
    g.item("个人页点右箭头","翻到第 2 页：「个人仓库：第 1 → 2 页」、不发共享页翻页",r"个人仓库：第 1 → 2 页",r"SharedStashRight|【红】个人仓库"); g.at("tap_ui TouchStashRight@BankExpansionLayout 3",2.0); g.shot()
    g.item("第 2 页双击背包里另一张卷轴","进第 2 页（看截图）",None); g.dbl(600.5,285.5); g.wait(1.5); g.clr(); g.shot()
    g.item("点左箭头","回第 1 页",r"个人仓库：第 2 → 1 页",r"SharedStashLeft|【红】个人仓库"); g.at("tap_ui TouchStashLeft@BankExpansionLayout 3",2.0); g.shot()
    g.item("第 1 页再点左箭头","不回绕：不翻",lambda s,b: ("不通：翻了" if re.search(r"个人仓库：第 1 → \d+ 页",s) else "通：没翻")); g.at("tap_ui TouchStashLeft@BankExpansionLayout 3",2.0); g.shot()
    g.item("点「共享」页签再点右箭头","共享页照旧发 SharedStashRight、页码各显各的（看截图）",r"SharedStashRight"); g.at("tap_ui Image1@BankExpansionLayout 3",1.5); g.at("tap_ui TouchStashRight@BankExpansionLayout 3",2.0); g.shot()
    g.at("tap_ui Image0@BankExpansionLayout 3",1.5); g.tap(P(654.5,17),1.5)
    g.item("保存并退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",5.0); g.shot()
    g.item("重进同一个角色","进局；载入个人仓库侧文件",r"个人仓库：\S+ 当前第 1 页，另外几页共 1 件"); g.ui("ListItem2@CharacterSelectPanel",1.3); g.tap(P(333,336),12.0); g.shot()
    g.item("开箱翻到第 2 页","第 2 页那张卷轴还在（看截图）",r"个人仓库：第 1 → 2 页"); g.at("tap_unit 2 267 3",3.0); g.at("tap_ui TouchStashRight@BankExpansionLayout 3",2.0); g.shot()
    gs.append(g)


    VE="DM_TEST_STAT=12=60 DM_TEST_GIVE=cap:7:1@60 DM_TEST_GIVE_INV=1 DM_KBD=帽"
    g=G("vt1",VE); g.sect("vt1",0); g.enter(); g.wait(2.0)
    g.item("点箱子","开储藏箱",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("点第六页签「编年史」","切到编年史仓库页（看截图：暗金格、部位下拉、档次勾选、搜索）",r"编年史仓库：进第六页"); g.tap(P(181,53),1.5); g.shot()
    g.item("双击背包里的暗金帽子","存入：「→ 存进编年史仓库」「存入 …」",r"存进编年史仓库[\s\S]*编年史仓库：存入",r"【红】编年史仓库"); g.dbl(591.9,236.3); g.wait(2.0); g.dbl(591.9,236.3); g.wait(2.0); g.shot()
    g.item("单击仓库格子","出这件的说明框（看截图）",None); g.tap(P(94.7,156.4),1.5); g.shot()
    g.item("说明框开着点「共享」页签","只收框、页签不切（vaultui2：框外任何地方只收框）",lambda s,b: ("不通：切了页签" if re.search(r"界面页签：切",s) else "通：没切页签")); g.tap(P(105.9,53),1.5); g.shot()
    g.item("从格子拖回背包","取出：「拖到背包格 … → 取出」「取出 … → 背包」",r"编年史仓库：[^\n]+ 拖到背包格[^\n]*→ 取出[\s\S]*取出 [^\n]*→ 背包",r"【红】编年史仓库"); g.dragp((94.7,156.4),(454.4,276.3)); g.wait(1.5); g.shot()
    g.item("再双击存回去","存入",r"编年史仓库：存入",r"【红】编年史仓库"); g.dbl(454.4,236.3); g.wait(2.0); g.shot()
    g.item("点搜索框（输入「帽」）","列表只剩帽子类（看截图）",None); g.tap(P(226,78),3.0); g.shot()
    g.tap(P(654.5,17),1.5)
    g.item("保存并退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",5.0); g.shot()
    g.item("重进同一个角色","进局；编年史仓库读入 1 件",r"编年史仓库：\S+ 读入 1 件"); g.ui("ListItem2@CharacterSelectPanel",1.3); g.tap(P(333,336),12.0); g.shot()
    g.item("开箱第六页","那顶帽子还在格子里；普通 / 优异 / 精英三个框全勾，同第一次打开（看截图，对照 vt1_02）",r"编年史仓库：进第六页"); g.at("tap_unit 2 267 3",3.0); g.tap(P(181,53),1.5); g.shot()
    gs.append(g)


    def ent(g): g.t=max(g.t,3.5); g.tap(P(333,336),1.6); g.tap(P(333,157),1.0); g.at("await pos"); g.t=max(g.t,13.0); g.at("note 进局",1.5); g.wait(2.0)
    g=G("vf1","DM_TEST_ITEM_DUMP=1500",save="save.sl_player"); g.sect("vf1",0); ent(g)
    g.item("开箱","个人页：3 件、金币框、页码（看截图）",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.clr(); g.shot()
    g.item("个人 → 编年史","编年史页",r"进第六页"); g.tap(P(181,53),1.5); g.shot()
    g.item("编年史 → 共享","共享页满页物品、金币（看截图）",r"界面页签"); g.tap(P(105.9,53),1.5); g.clr(); g.shot()
    g.item("共享 → 宝石","宝石页（看截图）",r"界面页签"); g.tap(P(148.8,53),1.5); g.clr(); g.shot()
    g.item("宝石 → 编年史","编年史页",r"进第六页"); g.tap(P(181,53),1.5); g.shot()
    g.item("编年史 → 个人","个人页 3 件、金币框、页码都在（看截图，对照第 1 张）",r"界面页签"); g.tap(P(62.5,53),1.5); g.clr(); g.shot()
    g.item("个人 → 共享","共享页满页物品还在（看截图，对照第 3 张）",r"界面页签"); g.tap(P(105.9,53),1.5); g.clr(); g.shot()
    g.tap(P(181,53),1.5)
    g.item("停在编年史页关箱","箱子关了",r"ClosePanel BankExpansionLayout|CloseAll|ClosePanel"); g.tap(P(654.5,16.7),1.5); g.shot()
    g.item("再开箱","开箱（看截图停在哪页）",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.clr(); g.shot()
    g.item("点个人","个人页 3 件都在（看截图）",None); g.tap(P(62.5,53),1.5); g.clr(); g.shot()
    g.item("点共享","共享页满页还在（看截图）",None); g.tap(P(105.9,53),1.5); g.clr(); g.shot()
    g.tap(P(181,53),1.5); g.tap(P(654.5,16.7),1.5)
    g.item("编年史页关箱后退出","回选角页",r"FrontEndNavigation ToFrontEnd"); g.tap("@menu"); g.tap(P(551,17)); g.at("tap_ui Exit@PauseLayoutGarden 3",5.0); g.shot()
    g.item("重进第一行","进局",None); g.t+=0.5; ent(g); g.shot()
    g.item("开箱","个人页 3 件都在（看截图）",r"OpenPanel BankExpansionLayout"); g.at("tap_unit 2 267 3",3.0); g.clr(); g.shot()
    g.item("点共享","共享页满页还在（看截图）",None); g.tap(P(105.9,53),1.5); g.clr(); g.shot()
    gs.append(g)


    g=G("vf2","DM_TEST_STAT=12=60 DM_TEST_GIVE=box:2:1@60 DM_TEST_GIVE_INV=1 DM_TEST_ITEM_DUMP=1400"+" DM_TEST_WORLD_GO=near2:2:c267@200"); g.sect("vf2",0); g.enter(); g.wait(2.0)
    g.item("开箱","个人页（背包里有魔盒，看截图）",r"OpenPanel BankExpansionLayout"); g.arrive(5808,4443,3,24.0); g.at("tap_unit 2 267 3",3.0); g.shot()
    g.item("点编年史","编年史页，没有魔盒容器 / 转化钮露出（看截图）",r"进第六页"); g.tap(P(181,53),1.5); g.shot()
    g.item("点原魔盒格那块","没打开魔盒、没有输入落到魔盒",r"界面命中",r"OpenPanel \S*Cube|魔盒|Transmute|转化"); g.tap(P(179.3,306.4),1.0); g.shot()
    g.item("从原魔盒格那块拖出去","同上，魔盒不动",r"界面命中",r"OpenPanel \S*Cube|魔盒|Transmute|转化"); g.drag(P(179.3,306.4),P(300,250)); g.shot()
    g.item("点部位页签「鞋子与腰带」","列表换成鞋子与腰带、选中帧带菱形（看截图）",None); g.tap(P(209.6,95.3),1.5); g.shot()
    g.item("点部位页签「法杖与法器」（第二排）","列表换成法杖与法器（看截图）",None); g.tap(P(279.1,117.2),1.5); g.shot()
    g.item("点部位页签「头盔」","头盔，第 1/4 页（看截图）",None); g.tap(P(70.5,95.3),1.5); g.shot()
    g.item("点右箭头","第 2/4 页（看截图）",None); g.tap(P(226.4,280.6),1.5); g.shot()
    g.item("点左箭头","回第 1/4 页（看截图）",None); g.tap(P(147.8,280.4),1.5); g.shot()
    g.item("点底部盖板","什么都不发、不冒原版说明（看截图）",lambda s,b: ("不通：" if re.search(r"OpenPanel TooltipsPanel|Cube|魔盒|转化",s) else "通：")+"盖板"); g.tap(P(179.3,306.4),1.5); g.shot()
    gs.append(g)


    RWOK=lambda s,b: ("通：" if re.search(r"排版自检",b+s) and not re.search(r"出底板 [1-9]|重叠 [1-9]|出框 [1-9]",b+s) else "不通：")+"排版自检（整局）"
    g=G("rwp","DM_TEST_GIVE=r01:2:1,r02:2:1,r03:2:1,r04:2:1,r05:2:1,r06:2:1,r07:2:1,r08:2:1,r09:2:1,r10:2:1,r11:2:1,r12:2:1,r13:2:1,r14:2:1,r15:2:1,qui:2:2@80 DM_TEST_GIVE_INV=1 DM_TEST_RW_OPEN=26:qui DM_TEST_RW_SOCKETS=3"); g.sect("rwp",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.t=max(g.t,45.0)
    g.item("清单打开（第 1 页）","第 1/2 页，每页 ≤ 8 条、条目间分隔线（看截图）",RWOK); g.shot()
    g.item("点下一页","「翻页 Next → 第 2/2 页（第 9–15 个）」",r"翻页 Next → 第 2/2 页（第 9–15 个）",r"【红】"); g.at("tap_ui TouchRwPgNext 3",2.0); g.shot()
    g.item("点第 14 条名字（谜团）","「详情「谜团」」→「详情自检 … 出框 0、重叠 0」（看截图）",r"详情「谜团」[\s\S]*详情自检「谜团」[^\n]*出框 0、重叠 0",r"【红】"); g.at("tap_ui TouchRwName13 3",2.0); g.shot()
    g.item("点返回","「收起详情」、回到清单",r"收起详情",r"【红】"); g.at("tap_ui TouchRwBack 3",2.0); g.shot()
    g.item("点上一页","「翻页 Prev → 第 1/2 页」",r"翻页 Prev → 第 1/2 页",r"【红】"); g.at("tap_ui TouchRwPgPrev 3",2.0); g.shot()
    g.item("整局排版自检","每条出底板 0、重叠 0",RWOK)
    gs.append(g)


    g=G("rwd","DM_TEST_GIVE=r04:2:1,r03:2:1,cap:2:9@80 DM_TEST_GIVE_INV=1 DM_TEST_RW_OPEN=26:cap"); g.sect("rwd",0); g.enter(); g.wait(2.0); g.tap("@menu",1.4)
    g.at("await ui TouchRwName0 40",40.0)
    g.item("点第 1 条名字进详情","详情（看截图）",r"详情「"); g.at("tap_ui TouchRwName0 3",2.0); g.shot()
    g.item("点详情里的一键合成","「一键合成成型」；宿主在结束那一刻读回：2 孔、孔里 2 颗、标志带 0x04000000",lambda s,b: ("通：" if re.search(r"一键合成成型[^\n]*标志 0x4",b+s) and re.search(r"成型读回（成型）[^\n]*2 孔、孔里 2 颗[^\n]*标志 0x4",b+s) else "不通：")+"成型与读回")
    g.at("await rw 25",0.1); g.at("tap_ui TouchRwDetGo 3",25.0); g.shot()
    gs.append(g)






    g=G("dc","DM_TEST_GOTO_LEVEL=150:2 DM_TEST_KILL=450 DM_TEST_POS_LOG=1"); g.sect("death",0); g.enter()
    g.item("第 150 拍换到鲜血荒地、第 450 拍死亡","换关、开始死亡模式、弹原版死亡画面",r"(?=[\s\S]*DM_TEST_GOTO_LEVEL：拍 \d+ 换到关卡 2)(?=[\s\S]*DM_TEST_KILL：拍 \d+ 对玩家)(?=[\s\S]*OpenPanel YouDiedModal)"); g.t=max(g.t,38.0); g.at("pos",.3); g.shot()
    g.item("点屏幕（原版「继续」）","死亡画面关、人复活在罗格营地（区域 1）、生命回满",lambda s,b: (lambda m: ("通：" if re.search(r"YouDiedModalMessage Continue",s) and re.search(r"ClosePanel YouDiedModal",s) and m and m[-1][0]=="1" and m[-1][1]==m[-1][2] and m[-1][1]!="0" else "不通：")+f"继续消息 {len(re.findall(r'YouDiedModalMessage Continue',s))} 条；最后位置读数 区域 {m[-1][0] if m else '?'} 生命 {m[-1][1]+'/'+m[-1][2] if m else '?'}")(re.findall(r"\[script\] pos 区域 (\d+) \(\d+,\d+\) 生命 (\d+)/(\d+)",s)))
    g.tap(P(333,200),2.0); g.tap(P(333,200),6.0); g.at("pos",.3); g.shot()
    g.item("点底栏腰带第 1 格","喝掉本人腰带上的一瓶：读数第 1 列变 0（读到的是复活后的本人，不是野外的尸体）",r"腰带 0/\d/\d/\d"); g.tap("@belt1",2.5); g.shot()
    gs.append(g)











    def _od_xy(t): return re.findall(r"\[script\] pos 区域 \d+ \((\d+),(\d+)\)",t)
    g=G("od","DM_TEST_NO_BC=1"); g.sect("oldpad",0); g.enter()
    g.item("老机型开机进局","用老机型补充包、开机用触屏版、覆盖层带着布局标记",lambda s,b: (lambda t,bad: ("不通：" if bad else "通：")+("；".join(bad) if bad else "补充包在用、开机触屏版、标记在"))(b+s,[x for x,ok in (("没有「boot: 用老机型补充包」（包没带老机型覆盖层，或容器里没有补充包）","boot: 用老机型补充包" in b+s),("没有「开机用触屏版」","开机用触屏版" in b+s),("出现「覆盖层没有 ctrl_touch 标记」","覆盖层没有 ctrl_touch 标记" not in b+s)) if not ok])); g.at("pos",.5); g.shot()
    g.item("注入手柄摇杆","切到原版手柄版",r"切到原版手柄版",r"覆盖层没有 ctrl_touch 标记"); g.at("pad 0 0 32767",.8); g.at("pad 0",1.5); g.shot(); g.at("pos",.5)
    g.item("碰屏幕空地","切回触屏版",r"切到触屏版"); g.tap(P(520,120),3.0); g.shot(); g.at("pos",.5)
    g.item("触屏摇杆右推","人走了一段（前后两次位置不同）",lambda s,b: (lambda a,c: ("通：" if a and c and a[-1]!=c[-1] else "不通：")+f"推之前 {a[-1:]}，之后 {c[-1:]}")(_od_xy(b),_od_xy(s))); g.at("down 0 @joy",.3); g.at("move 0 @joy+40,0",1.2); g.at("up 0",1.0); g.at("pos",.5); g.shot()
    g.item("推摇杆切到手柄版，再按 ＋ 开物品栏、B 关","切到原版手柄版、物品栏开了又关（触屏模式下第一下手柄键只切界面、不执行，所以先推摇杆切过去）",r"(?=[\s\S]*切到原版手柄版)(?=[\s\S]*OpenPanel PlayerInventoryExpansionLayout)"); g.at("pad 0 0 32767",.6); g.at("pad 0",1.6); g.at("pad 0x400",.2); g.at("pad 0",1.5); g.shot(); g.at("pad 0x2",.2); g.at("pad 0",1.1)
    g.item("再碰屏幕空地","切回触屏版；整局没有「覆盖层没有 ctrl_touch 标记」",lambda s,b: ("通：" if "切到触屏版" in s and "覆盖层没有 ctrl_touch 标记" not in b+s else "不通：")+"这一步「切到触屏版」%d 行；整局缺标记 %d 行"%(s.count("切到触屏版"),(b+s).count("覆盖层没有 ctrl_touch 标记"))); g.tap(P(520,120),3.0); g.shot()
    gs.append(g)




    g=G("pvb","DM_TEST_WORLD_GO=near2:1:c154@200 DM_TEST_GOLD=1300"); g.sect("padshop",0); charsi(g)
    g.item("店开着注入手柄右摇杆","切到手柄版、商人页还在，货架左下有刷新货架钮（看截图）",r"手柄：手柄有输入 → 手柄模式",r"CRASH|Terminating app"); g.at("pad 0 0 0 32767 0",.5); g.at("pad 0",2.5); g.at("padcur 0.414 0.790",1.5); g.shot()
    if MODE=="iphone":
        g.item("光标在刷新钮上按 A","货架换货",r"商人刷新：点了",r"CRASH|Terminating app"); g.at("pad 0x1",.2); g.at("pad 0",2.8); g.shot()
    gs.append(g)


    def _ps(m,hold=.2,after=.3): g.at(f"pad {m}",hold); g.at("pad 0",after)
    for gn,n,nm in (("psel0",0,"joe"),("psel1",1,"Warlock"),("psel2",2,"joebao"),("psel3",3,"法师")):
        g=G(gn); g.who=nm; g.padsel=True; g.sect(gn,0); g.t=5.0
        g.item(f"手柄选第 {n+1} 行「{nm}」并进局","主菜单角色牌是这一行的角色（看截图）；进局",r"\[script\] pos 区域 \d+")
        g.at("pad 0 0 0 32767 0",.5); g.at("pad 0",4.5); _ps("0x4",.2,1.6); [_ps("0x8000",.2,.5) for _ in range(n)]; g.shot(); _ps("0x1",.2,2.3); g.shot(); _ps("0x1",.2,2.8); _ps("0x1",.2,8.8); g.at("pos",.5); g.shot()
        gs.append(g)
    return gs
allrows=[]; T0=time.time(); notes=[]
for g in groups():
    if ONLY and g.run not in ONLY: continue
    if LETTERBOX and not ONLY and g.run in ("d","g","h","j","k","t","u","v","v2","m","n","p","ra","q","pk","g1","g2","g3","g4","ex","tb","tp"): continue
    rows,wall,cap,red,tm=run(g); allrows+=rows; notes.append(f"局 {g.run}：墙钟 {wall:.0f} 秒（脚本上限 {cap}）；{tm}；日志【红】{red} 行"); print("# "+notes[-1],flush=True)
real=[r for r in allrows if not r[1].startswith("（整局）") or r[1].startswith("（整局）不该出现") or r[1].startswith("（整局）该出现")]
ok=sum(r[3].startswith("通") for r in real); un=sum(r[3].startswith("未判") for r in real); bad=len(real)-ok-un
with open(f"{OUT}/表.md","w") as f:
    f.write(f"# 逐界面回归（{MODE}，{time.strftime('%m-%d %H:%M')}）\n\n共 {len(real)} 项：通 {ok}、不通 {bad}、未判 {un}；合计墙钟 {time.time()-T0:.0f} 秒\n\n"+"".join(f"- {n}\n" for n in notes)+"\n| 组 | 项 | 期望 | 真结果 | 截图 |\n|---|---|---|---|---|\n")
    for r in allrows: f.write("| "+" | ".join(r)+" |\n")
print(f"# 共 {len(real)} 项：通 {ok}、不通 {bad}、未判 {un}；合计墙钟 {time.time()-T0:.0f} 秒；表 {OUT}/表.md")
for r in real:
    if not r[3].startswith("通"): print("  "+" | ".join(r))
