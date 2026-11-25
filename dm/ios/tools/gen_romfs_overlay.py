#!/usr/bin/env python3






import re
import json, os, struct, subprocess, sys, tempfile, shutil, runpy


runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools', 'winlf', 'sitecustomize.py'))
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); V4=os.environ.get('DM_HUD_V4_DIR') or __import__('gen_paths').gen_dir('NativeHUD','v4')
romfs, out = sys.argv[1], sys.argv[2]
GENERATED_OVERLAYS = []
sys.path.insert(0,HERE); import gen_assets; gen_assets.ensure(romfs, overlay_only=True)
REPL={'data/hd/global/ui/panel/hud_02/bugbtn.sprite':'base.sprite',
      'data/hd/global/ui/panel/hud_02/front_panel.sprite':'close.sprite',
      'data/global/ui/layouts/controller/hudpanelhd.json':os.path.join(V4,'hud.json')}
with tempfile.TemporaryDirectory() as t:








    ui=json.load(open(os.path.join(V4,'hud_v4_ui','ui.json'))); ref_w,ref_h=ui['ref']; k=2160/ref_h
    x0,x1=ui['slots'][0][0]-1,ui['slots'][-1][2]+1
    nw=round((x1-x0)*k); nx=round(((x0+x1)/2-ref_w/2)*k)-1








    note={'type':'NotificationPanel','name':'NotificationPanel',
          'fields':{'priority':9002,'notificationSeconds':5.0,'transitionSeconds':0.8,'rect':{'x':nx,'y':-790,'width':nw,'height':244},'anchor':{'x':0.5,'y':1.0}},
          'children':[{'type':'NineTileImageWidget','name':'NotificationBorder',
                       'fields':{'rect':{'x':0,'y':0,'width':nw,'height':244},'anchor':{'x':-0.5,'y':0.0},'filename':'PANEL/TooltipBorder/TooltipBorder','backgroundColor':[0.0,0.0,0.0,0.70],'tileScale':1},
                       'children':[{'type':'TextBoxWidget','name':'Prompt','fields':{'fitToParent':True,'style':'$StyleNotification'}}]}]}
    json.dump(note,open(os.path.join(t,'notification.json'),'w'),indent=1)
    REPL['data/global/ui/layouts/controller/notificationpanelhd.json']=os.path.join(t,'notification.json')


    sys.path.insert(0,HERE); import touch_layout
    import tab_art
    REPL.update(tab_art.build(romfs, t, touch_layout._VIDEO_TAB_W.values()))
    REPL.update(tab_art.build(romfs, t, (400,), chronicle=True))
    import looks_tab; REPL.update(looks_tab.art(romfs, t))
    tl,ntl=touch_layout.build(romfs,t); REPL.update(tl)
    REPL.update(touch_layout.build_pcfront(romfs,t))
    tp,ntp=touch_layout.build_present(romfs,t); REPL.update(tp)
    ti,nti=touch_layout.build_ingame(romfs,t); REPL.update(ti)
    import merc_slots; REPL.update(merc_slots.build(romfs,t,REPL))
    REPL.update(touch_layout.build_cursor(romfs,t))
    REPL.update(touch_layout.build_glyphs(romfs,t))
    import romfs_art




    for line in open(os.path.join(romfs,'data','build_manifest.psv'),encoding='utf-8'):
        f=line.rstrip('\n').split('|')
        if len(f)<11 or line.startswith('#') or '/ui/controller/controllerbuttons/switch/' not in f[9].lower(): continue
        nm=os.path.basename(f[9]); src={'ButtonRight.sprite':'ButtonBottom.sprite','ButtonBottom.sprite':'ButtonRight.sprite'}.get(nm,nm)
        o=os.path.join(t,'xbox_'+nm); open(o,'wb').write(romfs_art.read(romfs,'data/hd/global/ui/Controller/ControllerButtons/xbox/'+src))
        REPL['data/hd/global/ui/ztouch_controller_buttons_ui/switchx'+nm.lower()]=o



    man=[l.rstrip('\n').split('|') for l in open(os.path.join(romfs,'data','build_manifest.psv'),encoding='utf-8') if not l.startswith('#')]
    have={f[9].lower().split(':',1)[-1] for f in man if len(f)>=11}; cb='data/hd/global/ui/controller/controllerbuttons/'
    face={'buttonright':'buttonbottom','buttonbottom':'buttonright','buttontop':'buttonleft','buttonleft':'buttontop'}
    for nm in sorted(p[len(cb)+7:] for p in have if p.startswith(cb+'switch/')):
        src=face.get(nm[:-7],nm[:-7])+'.sprite'; q=next(p for p in (cb+'ps5/'+src, cb+'ps5/'+src.replace('rectangle',''), cb+'switch/'+nm) if p in have)
        o=os.path.join(t,'ps_'+nm); open(o,'wb').write(romfs_art.read(romfs,q)); REPL['data/hd/global/ui/ztouch_controller_buttons_ui/switchp'+nm]=o
    if os.environ.get('DM_OVERLAY_MAC'):
        import kbd_glyphs; REPL.update({'data/hd/global/ui/ztouch_controller_buttons_ui/switchk'+os.path.basename(k):v for k,v in kbd_glyphs.build(romfs,t).items()})
    REPL.update(touch_layout.build_profile(romfs,t))
    import touch_strings; ts,nts=touch_strings.build(romfs,t); REPL.update(ts)
    TS=touch_strings.build_sidecar(romfs)
    import dm_i18n; si,nsi=dm_i18n.build(romfs,t,REPL); REPL.update(si)
    tab_metrics = tab_art.text_metrics(romfs, next(iter(si.values())))
    REPL.update(tab_art.build_skill_strings(romfs, t, REPL, tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_WIDTH))
    import drop_light; dl,dls=drop_light.build(romfs,t,REPL); REPL.update(dl)
    import look_table; lk,LOOKS_TSV,lks=look_table.build(romfs,t); REPL.update(lk)
    import pouch_layout; pl,pls=pouch_layout.build(romfs,t,REPL); REPL.update(pl)
    import wide_stash; ws,wss=wide_stash.build(romfs,t,REPL); REPL.update(ws)







    tpad=os.path.join(t,'pad'); os.makedirs(tpad); PR={}
    lp,lt=touch_layout.read_layout(romfs,pouch_layout.INV); o=os.path.join(tpad,'pad_inv.json'); open(o,'w',encoding='utf-8').write(touch_layout.pad_inv_rw(lt)); PR[lp.lower()]=o
    for rel,gd in (('controller/pauselayoutgardenhd.json',1),('controller/pauselayouthd.json',0)):
        lp,lt=touch_layout.read_layout(romfs,rel); o=os.path.join(tpad,'pad_pause%d.json'%gd); open(o,'w',encoding='utf-8').write(touch_layout.pad_pause(lt,gd,share=not os.environ.get('DM_OVERLAY_MAC'))); PR[lp.lower()]=o




    lp,lt=touch_layout.read_layout(romfs,'controller/notificationpanelhd.json'); assert lt.count('"y": -500')==1, lp
    o=os.path.join(tpad,'pad_notification.json'); open(o,'w',encoding='utf-8').write(lt.replace('"y": -500','"y": -590')); PR[lp.lower()]=o
    lp,lt=touch_layout.read_layout(romfs,'controller/messagelogpanelhd.json'); o=os.path.join(tpad,'pad_rift.json'); open(o,'w',encoding='utf-8').write(touch_layout._rift_panel(lt)[0]); PR[lp.lower()]=o
    PR.update(merc_slots.build_pad(romfs,tpad))




    lp,lt=touch_layout.read_layout(romfs,'controller/console/charactercreatepanelhd.json')
    for nm,x0,x1 in touch_layout.CLASS_STRIPS: lt=touch_layout.set_rect(lt,nm,x0,450,x1-x0,1300)
    o=os.path.join(tpad,'pad_charcreate.json'); open(o,'w',encoding='utf-8').write(lt); PR[lp.lower()]=o
    PR.update(touch_layout.build_ingame(romfs,tpad,touch_layout.PAD_VENDOR,present=False)[0])
    PR.update(touch_layout.build_ingame(romfs,tpad,touch_layout.PAD_TREE,present=False)[0])
    lp,lt=touch_layout.read_layout(romfs,'controller/console/mainmenupanelhd.json'); o=os.path.join(tpad,'pad_mainmenu.json'); open(o,'w',encoding='utf-8').write(touch_layout.pad_mainmenu(lt)); PR[lp.lower()]=o
    lp,lt=touch_layout.read_layout(romfs,'controller/console/mainmenubuttonribbonhd.json'); o=os.path.join(tpad,'pad_ribbon.json'); open(o,'w',encoding='utf-8').write(touch_layout.pad_ribbon(lt, movies=os.environ.get('MOVIE_FFMPEG')=='1')); PR[lp.lower()]=o
    lp,lt=touch_layout.read_layout(romfs,'controller/hudmonsterhealthhd.json'); o=os.path.join(tpad,'pad_bosspct.json'); open(o,'w',encoding='utf-8').write(touch_layout.pad_boss_pct(lt)); PR[lp.lower()]=o
    lp,lt=touch_layout.read_layout(romfs,'controller/console/characteroptionspanelhd.json'); o=os.path.join(tpad,'pad_charopt.json'); open(o,'w',encoding='utf-8').write(touch_layout.pad_bliss(lt)); PR[lp.lower()]=o
    for rel,fn in (('controller/console/settingspanelhd.json',touch_layout.pad_settings),('controller/console/videooptionspanelhd.json',touch_layout.pad_custom_page)):
        lp,lt=touch_layout.read_layout(romfs,rel); o=os.path.join(tpad,'pad_'+rel.replace('/','_')); open(o,'w',encoding='utf-8').write(fn(lt)); PR[lp.lower()]=o
    PR.update(pouch_layout.build(romfs,tpad,PR)[0]); PR.update(wide_stash.build(romfs,tpad,PR)[0])
    cb=Image.open(os.path.join(HERE,'..','NativeHUD','touch','close.png')).convert('RGBA')
    open(os.path.join(t,'close.sprite'),'wb').write(b'SpA1'+struct.pack('<HHIIIIII',31,cb.width,cb.width,cb.height,0,1,0,0)+struct.pack('<II',cb.width*cb.height*4,4)+cb.tobytes())


    shutil.copy(os.path.join(__import__('gen_paths').gen_dir('NativeHUD','touch'),'fe_rightpanel.sprite'),os.path.join(t,'fe_rightpanel.sprite'))
    REPL['data/hd/global/ui/frontend/hd/final/frontend_rightpanel.sprite']='fe_rightpanel.sprite'

    shutil.copy(os.path.join(__import__('gen_paths').gen_dir('NativeHUD','touch'),'build_card.sprite'),os.path.join(t,'build_card.sprite'))
    REPL['data/hd/global/ui/panel/beta_exit/betasurveyqr.sprite']='build_card.sprite'
    im=Image.open(os.path.join(V4,'base.png')).convert('RGBA'); w,h=im.size


    open(os.path.join(t,'base.sprite'),'wb').write(b'SpA1'+struct.pack('<HHIIIIII',31,w,w,h,0,1,0,0)+struct.pack('<II',w*h*4,4)+im.tobytes())






    ASTC=os.environ.get('DM_ASTC_PACK','')














    LAY='data/global/ui/layouts/'
    TDIR=[('controller/console/switch/','0touch_console_switch_lay/'),('controller/console/','ztouch_console_lay/'),('controller/','xtouch_lay/')]
    import romfs_art
    MAN={f[9].lower().split(':',1)[-1] for f in (l.rstrip('\n').split('|') for l in open(os.path.join(romfs,'data','build_manifest.psv'),encoding='utf-8')) if len(f)>=11}






    UI='data/hd/global/ui/'
    def touch_path(k):
        if k.startswith(UI+'controller/controllerbuttons/'):
            r=k[len(UI+'controller/controllerbuttons/'):]; return UI+'ztouch_controller_buttons_ui/'+r.replace('/','_') if r.count('/')==1 else None
        if k.startswith(UI+'controller/cursor/') and '/' not in k[len(UI+'controller/cursor/'):]: return UI+'xtouch_cursor_lay/'+k[len(UI+'controller/cursor/'):]
        if not k.startswith(LAY) or '/_profile' in k: return None
        r=k[len(LAY):]
        for a,b in TDIR:
            if r.startswith(a): rest=r[len(a):]; return None if '/' in rest else LAY+b+rest
        return None
    ORIG=sorted(q for q in MAN if touch_path(q) and q.startswith(LAY))
    def to_touch(repl):
        r={}
        for k,v in repl.items(): tp=touch_path(k); r[tp or k]=v
        for q in ORIG:
            k=touch_path(q)
            if k not in r:
                o=os.path.join(t,'orig_'+q[len(LAY):].replace('/','__'))
                if not os.path.exists(o): open(o,'wb').write(romfs_art.read(romfs,q))
                r[k]=o
        return r




    SHARED_TODO=('chroniclebaselistpanelhd.json','chroniclepanelhd.json','controller/_profileconsolehd.json','controller/_profilehandheldhd.json','dropdownlistcontentshd.json')
    PADROOT=touch_layout.build_pad_root(romfs,tpad); PADROOT_KEYS={k[len(LAY):] for k in PADROOT}
    UNSHARE=('confirmationmodalhd.json','dismissablemodalhd.json','helpbuttonpanelhd.json','npcdialogpanelhd.json','youdiedmodalhd.json')
    PADLAY={k:v for k,v in PR.items() if touch_path(k)}


    PAD_PAGES=('controller/bankexpansionlayouthd.json','controller/console/charactercreatepanelhd.json','controller/console/characteroptionspanelhd.json','controller/console/mainmenubuttonribbonhd.json','controller/console/mainmenupanelhd.json','controller/console/settingspanelhd.json','controller/console/switch/playerinventoryexpansionlayouthd.json','controller/console/videooptionspanelhd.json',
               'controller/controlleroverlayhd.json','controller/hirelinginventorypanelhd.json','controller/hudmonsterhealthhd.json','controller/messagelogpanelhd.json','controller/notificationpanelhd.json','controller/pauselayoutgardenhd.json','controller/pauselayouthd.json','controller/vendorpanellayouthd.json',
               'controller/skillstreepanelhd.json','controller/characterstatspanelhd.json')
    if sorted(k[len(LAY):] for k in PADLAY)!=sorted(PAD_PAGES): raise SystemExit('gen_romfs_overlay：手柄专用布局与清单 PAD_PAGES 不一致：多 %s，少 %s' % (sorted(set(k[len(LAY):] for k in PADLAY)-set(PAD_PAGES)), sorted(set(PAD_PAGES)-set(k[len(LAY):] for k in PADLAY))))
    def emit(repl, dst, tag, prebuilt=None):
        repl=to_touch(repl); repl.update(PADLAY)




        for name in UNSHARE+tuple(sorted(PADROOT_KEYS)):
            k=LAY+name; mod=repl.pop(k); pat=re.compile(r'("basedOn"\s*:\s*")(%s)(")' % re.escape(name), re.I); inh=[]
            for tk,tv in list(repl.items()):
                if not any(tk.startswith(LAY+d) for _,d in TDIR): continue
                try: txt=open(os.path.join(t,tv),encoding='utf-8').read()
                except UnicodeDecodeError: continue
                if pat.search(txt): inh.append((tk,txt))
            if inh:
                repl[LAY+'1touchbase_lay/'+name]=mod
                for tk,txt in inh:
                    o=os.path.join(t,tag+'unshare_'+tk[len(LAY):].replace('/','__')); open(o,'w',encoding='utf-8').write(pat.sub(lambda m: m.group(1)+'1touchbase_lay/'+m.group(2)+m.group(3), txt)); repl[tk]=o
            else: assert LAY+'xtouch_lay/'+name not in repl, name; repl[LAY+'xtouch_lay/'+name]=mod
            if name in PADROOT_KEYS: repl[LAY+name]=PADROOT[LAY+name]
            print('按模式分开：%s → %s（继承它的触屏页 %d 个；0 = 触屏模式直接按名字加载）' % (name, '1touchbase_lay/' if inh else 'xtouch_lay/', len(inh)))




        shared=sorted(k[len(LAY):] for k in repl if k.startswith(LAY) and k in MAN and k not in PADLAY and k[len(LAY):] not in PADROOT_KEYS and not any(k.startswith(LAY+d) for _,d in TDIR))
        if shared!=sorted(SHARED_TODO): raise SystemExit('gen_romfs_overlay：两种模式共用的改过的布局与存量清单不一致（多出的要按模式分开：改过的放触屏目录 / 另起新名，原路径留原版）：多 %s，少 %s' % (sorted(set(shared)-set(SHARED_TODO)), sorted(set(SHARED_TODO)-set(shared))))
        with open(os.path.join(t,tag+'repl.tsv'),'w') as f:
            for k,v in repl.items(): f.write(f'{"" if k in MAN else "+"}{k}\t{os.path.join(t,v)}\n')
        subprocess.run(gen_assets.python_command(os.path.join(HERE,'..','..','tools','casc_overlay.py'),romfs,os.path.join(t,tag+'ov'),os.path.join(t,tag+'repl.tsv'),'--thin')+(['--prebuilt',prebuilt] if prebuilt else []),check=True,stdout=subprocess.DEVNULL)
        src=os.path.join(t,tag+'ov','data'); new=[n for n in os.listdir(src) if n.endswith('.data') and not os.path.islink(os.path.join(src,n))]
        assert len(new)==1, new
        shutil.rmtree(dst,ignore_errors=True); os.makedirs(os.path.join(dst,'data'))
        for n in ('.build.config',new[0]): shutil.copyfile(os.path.join(src,n),os.path.join(dst,'data',n))
        with open(os.path.join(dst, 'tab_text.tsv'), 'w', encoding='utf-8') as f:
            f.write(tab_metrics)
        with open(os.path.join(dst,'touch_strings.tsv'),'w',encoding='utf-8') as f:
            for k,o,v in TS: f.write('%s\t%s\t%s\n' % (k,o,v.replace('\\','\\\\').replace('\n','\\n').replace('\t','\\t')))
        sys.path.insert(0,HERE); import gen_itemcmp_data; gen_itemcmp_data.write(romfs,dst); gen_itemcmp_data.write_gear(romfs,dst)
        import gen_sim_data; gen_sim_data.write(romfs, dst)
        shutil.copyfile(LOOKS_TSV, os.path.join(dst, 'looks.tsv'))
        shutil.copyfile(os.path.join(os.path.dirname(LOOKS_TSV), 'dyes.tsv'), os.path.join(dst, 'dyes.tsv'))
        open(os.path.join(dst,'xbox_glyphs'),'w').write('ztouch_controller_buttons_ui/switchx*.sprite\n')
        open(os.path.join(dst,'ps_glyphs'),'w').write('ztouch_controller_buttons_ui/switchp*.sprite\n')
        if os.environ.get('DM_OVERLAY_MAC'): open(os.path.join(dst,'kbd_glyphs'),'w').write('ztouch_controller_buttons_ui/switchk*.sprite\n')
        open(os.path.join(dst,'ctrl_touch'),'w').write('触屏版布局在 layouts/xtouch_lay、ztouch_console_lay、0touch_console_switch_lay（dm_padmode.c 见到本文件才改写前缀）\n')
        GENERATED_OVERLAYS.append(dst)
        return new[0], os.path.getsize(os.path.join(dst,'data',new[0]))
    n0,z0=emit(REPL,out,'')
    print(f'romfs_overlay：{n0} {z0} 字节，底图 {w}x{h}；前端布局 {len(tl)} 个、挪走键位提示 {ntl} 处')
    for d in (out+'_astc',out+'_astc_variants'): shutil.rmtree(d,ignore_errors=True); os.makedirs(d)
    if ASTC:
        idx=os.path.join(ASTC,'pack_index.tsv'); assert os.path.isfile(idx) and os.path.isfile(os.path.join(ASTC,'pack.json')), 'DM_ASTC_PACK 目录里缺 pack_index.tsv / pack.json：'+ASTC
        na,za=emit(REPL,out+'_astc','astc_',idx); shutil.copyfile(os.path.join(ASTC,'pack.json'),os.path.join(out+'_astc','pack.json'))
        assert na==n0, f'ASTC 版覆盖层的新数据档 {na} 与 BC 版 {n0} 不同名（预取清单按名字找档）'
        print(f'romfs_overlay_astc：{na} {za} 字节（贴图指向补充包 {ASTC}）')






    FEATURES=[('item_notes','annot_strings')]
    import importlib, itertools
    shutil.rmtree(out+'_variants',ignore_errors=True)
    for r in range(1,len(FEATURES)+1):
        for combo in itertools.combinations(FEATURES,r):
            RS=dict(REPL); touched=set(); stats={}
            for key,mod in combo:
                ta,st=importlib.import_module(mod).build(romfs,t,RS); RS.update(ta); touched|=set(ta); stats[key]=st


            diff=[k for k in RS if k not in touched and RS[k]!=REPL.get(k)]
            if diff: raise SystemExit('gen_romfs_overlay：组合 %s 除特性文件外与全关那份不一致：%s' % ([k for k,_ in combo],diff))
            name='+'.join(k for k,_ in combo); dst=os.path.join(out+'_variants',name)
            n1,z1=emit(RS,dst,name+'_')
            for key,mod in combo:
                m=importlib.import_module(mod)
                if hasattr(m,'write_textbox_table'): m.write_textbox_table(dst)
            with open(os.path.join(dst,'features.txt'),'w') as f: f.write('\n'.join(k for k,_ in combo)+'\n'+''.join(f'{p}\n' for p in sorted(touched)))
            print(f'romfs_overlay_variants/{name}：{n1} {z1} 字节；改了 {len(touched)} 个文件；{stats}')
            if ASTC:
                da=os.path.join(out+'_astc_variants',name); emit(RS,da,name+'_astc_',os.path.join(ASTC,'pack_index.tsv'))
                for f in os.listdir(dst):
                    if f!='data': shutil.copyfile(os.path.join(dst,f),os.path.join(da,f))



import overlay_contract
from pathlib import Path
source_sha = overlay_contract.source_fingerprint(Path(__file__).resolve().parents[3])
for directory in GENERATED_OVERLAYS:
    overlay_contract.write_manifest(directory, source_sha)
    overlay_contract.verify_manifest(directory, source_sha)
print('overlay source fingerprint: ' + source_sha)
