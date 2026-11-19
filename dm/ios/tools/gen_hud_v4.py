#!/usr/bin/env python3














import argparse, json, re, sys, os
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0,HERE)
import romfs_art, hud_v4_design, layout_patch
from gen_paths import gen_dir
parser=argparse.ArgumentParser(description='Generate bottom HUD v4 assets')
parser.add_argument('romfs'); parser.add_argument('out',nargs='?'); parser.add_argument('tuning_json',nargs='?')
parser.add_argument('--overlay-only',action='store_true',help='Skip iOS skill silhouette atlases unused by the CASC overlay')
args=parser.parse_args()
ROMFS=args.romfs; OUT=args.out or gen_dir('NativeHUD','v4')
os.makedirs(OUT,exist_ok=True); SRC=hud_v4_design.Source(ROMFS)
def load(var='C'): return hud_v4_design.load(SRC, var)


T=dict(bx=6.5,by=5,bpx=25,bs=1.6,ex=-7,ey=-15,txt=43,tx=-238,ts=1.4)
if args.tuning_json is not None: T.update(json.loads(args.tuning_json))
g=load('C'); W,H=g['W'],g['H']; U=H/2160; RX0,RY0=-1473,-417
ux=lambda px:(px-W/2)/U-RX0; uy=lambda py:(py-H)/U-RY0
BASE='PANEL/HUD_02/BugBTN'


cv=Image.new('RGBA',(W,H))


cv.alpha_composite(g['bar'],(g['BAR_X0']-g['EXT'],H-g['BH']))
for x0,y0,_,_ in g['slots']: cv.alpha_composite(g['slot'],(x0,y0))








def dflt(im):
    a_=np.asarray(im.convert('RGBA')).astype(float); r,gc,b=a_[...,0],a_[...,1],a_[...,2]; red=(r>14)&(((r>gc*1.3)&(gc-b<0.17*r))|(r>gc*1.7)); l=r*0.5+gc*0.3+b*0.2
    hh,ww=red.shape; inner=np.zeros_like(red); inner[round(hh*0.12):round(hh*0.88),round(ww*0.12):round(ww*0.88)]=True; red&=inner
    for ch,k in ((0,0.42),(1,0.40),(2,0.38)): a_[...,ch]=np.where(red,l*k+6,a_[...,ch])
    return Image.fromarray(a_.round().clip(0,255).astype('uint8'),'RGBA')
g['layer']=Image.new('RGBA',(W,H)); g['fit'](dflt(g['lvl']),5,'十字（默认态）'); cv.alpha_composite(g['layer'])
yy,xx=np.mgrid[0:H,0:W]; ball=np.zeros((H,W),bool)
for c in (g['LBC'],g['RBC']): ball|=(xx-c[0])**2+(yy-c[1])**2<(g['BR']-2)**2


g_=np.array(cv); g_[...,3][ball]=0; cvg=Image.fromarray(g_)
L,R=g['L'],g['R']
for im_ in (cv,cvg): im_.alpha_composite(L,(g['LX'],H-L.height)); im_.alpha_composite(R,(g['RX'],H-R.height))
a=np.array(cv); a_glass=np.array(cvg)
a[...,3][ball]=0
cv=Image.fromarray(a); bx0,by0,bx1,by1=cv.getbbox(); spr=cv.crop((bx0,by0,bx1,by1))


full=a_glass
for i in (1,2,3,4): x0,y0,_,_=g['slots'][i]; full[y0+g['IY0']:y0+g['IY1']+1, x0+g['IX0']:x0+g['IX1']+1, 3]=0
x0,y0,x1,y1=g['slots'][5]; full[y0:y1,x0:x1,3]=0
FULL=Image.fromarray(full).crop((bx0,by0,bx1,by1))


spr=spr.resize((round(spr.width/(2*U)),round(spr.height/(2*U))),Image.LANCZOS)
spr.save(f'{OUT}/base.png')




j=layout_patch.apply(layout_patch.load_text(romfs_art.read(ROMFS,'global/ui/Layouts/Controller/HUDPanelHD.json').decode('utf-8-sig')),json.load(open(os.path.join(HERE,'touchrectv1_patch.json'),encoding='utf-8')))
X=lambda px:(px-W/2)/U; Y=lambda py:(py-H)/U
j['children']=[c for c in j['children'] if c['name'] not in ('TouchBeltFrame','BeltPopupArrow','TouchHealthBallRing','TouchManaBallRing')]
by={c['name']:c for c in j['children']}
def rect(n,**k):
    r=by[n]['fields'].setdefault('rect',{}); r.update({q:round(v,1) for q,v in k.items()}); by[n]['fields']['anchor']={'x':0.5,'y':1}
by['Overlay']['fields']['filename']=BASE; by['Overlay']['fields'].pop('transparency',None)
rect('Overlay',x=X(bx0),y=Y(by0)); by['Overlay']['fields']['rect'].pop('scale',None)
q=lambda v: round(v,1)
sb=q(2*g['BR']/(348*U)*T.get('ball',1.0)); ts=T.get('ts',1.4)
for n,c in (('HealthBall',g['LBC']),('ManaBall',g['RBC'])):
    rect(n,x=X(c[0])-174*sb,y=Y(c[1])-175*sb,scale=sb); by[n]['fields']['rect'].pop('width',None); by[n]['fields']['rect'].update(width=348,height=350)
    vd=[k for k in by[n]['children'] if k['name']=='ValueDisplay'][0]['fields']
    vd['rect']={'x':-6000,'y':0}
    continue
    vd['rect']={'x':T.get('tx',-174),'y':round((Y(g['YB']-T.get('txt',30))-(Y(c[1])-175*sb))/sb,1),'width':348,'scale':ts}
    vd.setdefault('style',{})['alignment']={'h':'center'}
s6=g['slots'][5]; sl=q(T.get('lvl',0.9)); lc=((s6[0]+s6[2])/2,(s6[1]+s6[3])/2)
lx,ly=X(lc[0])-62*sl+T.get('lx',0),Y(lc[1])-60*sl; rect('LevelUpButton',x=lx,y=ly,scale=sl)
rect('LevelUpFlourish',x=lx+(1179-1411)*sl,y=ly+(-1-265)*sl,scale=q(1.1*sl))
s1=g['slots'][1]; rect('Belt',x=X(s1[0])+T.get('bx',0),y=Y(s1[1])+T.get('by',0),scale=q(T.get('bs',1.0)))
by['Belt']['fields']['cellPadding']={'x':T.get('bpx',32),'y':T.get('bpy',68)}






by['Belt']['fields'].pop('cellOffsetInRow',None); by['Belt']['fields'].pop('itemOffsetInCell',None)
[k for k in by['Belt']['children'] if k['name']=='HotkeyLabelTemplate'][0]['fields']['rect']={'x':-6000,'y':98}
s5=g['slots'][5]; ss6=T.get('s6',1.0); rect('SkillButton6',x=X((s5[0]+s5[2])/2)-63*ss6+T.get('s6x',0),y=Y((s5[1]+s5[3])/2)-63*ss6+T.get('s6y',0),scale=ss6)


kids=j['children']; sb6=next(c for c in kids if c['name']=='SkillButton6'); kids.remove(sb6); kids.insert(kids.index(by['Overlay']),sb6)
es=q(T.get('exp',1.5)); rect('ExpBar',x=X(g['EX0'])+T.get('ex',0),y=Y(g['EY'])+T.get('ey',0),scale=es)


eb=by['ExpBar']; kids.remove(eb); kids.insert(kids.index(by['Overlay']),eb)
eb['fields']['rect']['y']=round(Y(g['slots'][1][1]+30),1)
eb['fields']['rect']['x']=-6000




for n in ('SkillButton1','SkillButton2','SkillButton3','SkillButton4','SkillButton5'):
    w_=by[n]; kids.remove(w_); kids.insert(kids.index(by['Overlay']),w_); w_['fields']['rect']=dict(by['SkillButton6']['fields']['rect']); w_['fields']['anchor']={'x':0.5,'y':1}
json.dump(j,open(f'{OUT}/hud.json','w'),ensure_ascii=False,indent=1)



from PIL import ImageDraw, ImageFont
UI=os.path.join(OUT,'hud_v4_ui'); os.makedirs(UI,exist_ok=True)
for f in os.listdir(UI): os.remove(os.path.join(UI,f))
font=g['num'](g['BADGE'])
def glyphs(tag,fill,font=font,chars='0123456789Lv·% '):
    m={}
    for ch in chars:
        pad=8; w=int(font.getlength(ch))+2*pad+4; asc,desc=font.getmetrics(); im=Image.new('RGBA',(w,asc+desc+2*pad+4)); d=ImageDraw.Draw(im)
        o=(pad,pad+asc)
        d.text((o[0]+2,o[1]+2),ch,font=font,fill=(0,0,0,170),anchor='ls'); d.text(o,ch,font=font,fill=fill,anchor='ls',stroke_width=1,stroke_fill=(10,8,6,255))
        bb=im.getbbox(); name=f'g_{tag}_{ord(ch):04x}.png'
        if bb: im.crop(bb).save(os.path.join(UI,name))
        m[ch]={'file':name if bb else None,'dx':(bb[0]-o[0]) if bb else 0,'dy':(bb[1]-o[1]) if bb else 0,'adv':font.getlength(ch)}
    return m
ui={'ref':[W,H],'center_x':W/2,'slots':g['slots'],'inner':[g['IX0'],g['IY0'],g['IX1'],g['IY1']],
    'exp':{'x0':g['EX0'],'x1':g['EX1'],'y':g['EY'],'h':6,'track':[22,18,14],'edge':[70,58,40],'fill':[196,160,70],'ticks':20},
    'lv':{'x':(g['EX0']+g['EX1'])/2,'baseline':g['EY']-6},
    'badge':{'glyphs':glyphs('b',(235,235,235,255)),'dx':-3,'dy':-1},
    'pick':{'center_x':1390,'pitch':120,'y':1051.5},
    'vital':{'left_x':g['LBC'][0],'right_x':g['RBC'][0],'baseline':g['YB']}}
font_v=SRC.font('blizzardglobal-v5_81.ttf',30)
import re, dm_i18n
_vt={r[0]:r[1:] for r in dm_i18n.rows()}; _vc='生命法力:/ 0123456789'
_vc+=''.join(sorted({c for k in ('dmHudLife','dmHudMana') for c in dm_i18n.FMT.sub('',''.join(_vt[k]))}-set(_vc)))
ui['vital']['glyphs']=glyphs('v',(235,235,235,255),font_v,_vc)


_vlt={r[0]:r[1:] for r in dm_i18n.rows()}; _lvc='0123456789Lv·% .'
_lvc+=''.join(sorted({c for k in ('dmAndroidHudLevel',) for c in dm_i18n.FMT.sub('',''.join(_vlt[k]))}-set(_lvc)))
ui['lv']['glyphs']=glyphs('lv',(205,185,140,255),font,_lvc)
g['slot'].save(os.path.join(UI,'slot.png'))
FULL.save(os.path.join(UI,'base_full.png')); ui['base']={'x':bx0,'y':by0}
lvlsheet=SRC.img('levelupbutton')
for fi in (0,1):
    fr=lvlsheet.crop((fi*124,0,fi*124+124,121)).resize((g['DM'],g['SH']),Image.LANCZOS); fr.save(os.path.join(UI,f'lvl_{fi}.png')); dflt(fr).save(os.path.join(UI,f'lvl_d{fi}.png'))
for n in range(1,9):
    im=g['prep'](g['metal'](str(n),220)); k=g['BOX']*0.85/im.height; im=im.resize((max(1,round(im.width*k)),round(im.height*k)),Image.LANCZOS)
    im.save(os.path.join(UI,f'diff_{n}.png'))




if not args.overlay_only:
    ART=os.path.join(HERE,'..','Sources','TouchArt'); man=json.load(open(os.path.join(ART,'native_art_manifest.json')))
    for a_ in man['assets']:
        if not a_['asset'].startswith('dm_native_skill_'): continue
        im=np.asarray(Image.open(os.path.join(ART,a_['asset'])).convert('RGBA')).astype(float)/255
        mx=im[...,:3].max(-1); mn=im[...,:3].min(-1); sat=(mx-mn)/np.maximum(mx,1e-6)
        al=np.clip((mx-0.42)/0.18,0,1)*np.clip((0.32-sat)/0.12,0,1)*im[...,3]
        fw,pitch=a_['frame_width'],a_['frame_pitch']; Hh,Ww=al.shape; yy,xx=np.mgrid[0:Hh,0:Ww]
        al*=np.hypot((xx%pitch)-fw/2,yy-Hh/2)<fw*0.46
        Image.fromarray((np.dstack([im[...,:3],al])*255).astype('uint8')).save(os.path.join(UI,'sil_'+a_['asset']))


lf=g['num'](34)
for name,txt in [('main','攻击')]+[(f'f{n}',f'F{n}') for n in range(1,9)]:
    w=int(lf.getlength(txt))+16; im=Image.new('RGBA',(w,60)); d=ImageDraw.Draw(im)
    d.text((10,8),txt,font=lf,fill=(0,0,0,190),stroke_width=2,stroke_fill=(0,0,0,190)); d.text((8,6),txt,font=lf,fill=(226,206,160,255),stroke_width=1,stroke_fill=(30,22,12,255))
    im.crop(im.getbbox()).save(os.path.join(UI,f'lbl_{name}.png'))
json.dump(ui,open(os.path.join(UI,'ui.json'),'w'),ensure_ascii=False,indent=1)
print('NativeHUD/v4：底图',spr.size,'原点',round(X(bx0),1),round(Y(by0),1),'球缩放',sb)
