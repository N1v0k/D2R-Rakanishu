package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.BlendMode;
import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.LinearGradient;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.nio.file.Files;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import org.json.JSONObject;
import org.json.JSONArray;




public final class TouchOverlay extends View {
    private static native void nativeEvent(int phase, int slot, float x, float y);
    private static native void nativeResize(float width, float height, float left, float right, float bottom, boolean tablet);
    private static native void nativeLayout(float[] out);
    private static native void nativeView(float x, float y, float width, float height);
    private final float density;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private static final int GLYPHS=119, HUD=167, DRAW_FLOATS=185;
    private static final float THUMB=23;
    private final float[] layout = new float[DRAW_FLOATS];



    private final float[] probe = new float[DRAW_FLOATS], drawn = new float[DRAW_FLOATS];
    private boolean drawnValid;
    private final Choreographer.FrameCallback poll = new Choreographer.FrameCallback() {
        @Override public void doFrame(long frameTimeNanos) {
            if (!running || stopped) return;
            displaysShown(frameTimeNanos);
            if (BuildConfig.DM_QA_BUILD && !(inputEnabled && isShown())) reportControls(null,0,0);
            if (inputEnabled && isShown()) { nativeLayout(probe); boolean word=levelText.refresh(); if (word || !drawnValid || !Arrays.equals(probe, drawn)) invalidate(); }
            Choreographer.getInstance().postFrameCallback(this);
        }
    };
    private final int[] pointerIds = new int[16];

    private final boolean[] shoulder = new boolean[2];
    private final float[] lastX = new float[16], lastY = new float[16];
    private final int[] rawIds = new int[16];
    private final float[] rawX = new float[16], rawY = new float[16];
    private String[] gameLabels, menuLabels;
    private final File overlayRoot;
    private final HashMap<String,Bitmap> images=new HashMap<>(), art=new HashMap<>(), glyphs=new HashMap<>();
    private final HashMap<String,int[]> glowOffsets=new HashMap<>();
    private final Bitmap[] skillAtlases=new Bitmap[9], silAtlases=new Bitmap[9];
    private Bitmap mapArt, circleArt, socketArt;
    private final Paint bitmapPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private String lifeFormat, manaFormat, cancelLabel, setOne = "I", setTwo = "II";


    private int[] gameIds, navIds;
    private final GameLang.Watch lang = new GameLang.Watch();
    private JSONObject hudUi;


    private final ExpRail expRail;


    private final HudLevelText levelText = new HudLevelText();
    private final boolean[] railFinger = new boolean[16];
    private float safeLeft, safeRight, safeBottom;
    private boolean running, inputEnabled;


    public TouchOverlay(Context context, String[] labels, String[] menuLabels) {
        super(context);
        density = getResources().getDisplayMetrics().density;
        slop = android.view.ViewConfiguration.get(context).getScaledTouchSlop();
        Arrays.fill(pointerIds,-1);
        gameLabels = labels == null ? new String[15] : labels.clone();
        this.menuLabels = menuLabels == null ? new String[5] : menuLabels.clone();
        lifeFormat=context.getString(R.string.dmHudLife); manaFormat=context.getString(R.string.dmHudMana); cancelLabel=context.getString(R.string.dmTcCancel);
        levelText.words(context.getString(R.string.dmAndroidHudLevel),context.getString(R.string.dmAndroidHudLevelWord));
        overlayRoot=new File(context.getFilesDir(),"dm/overlay");
        expRail=new ExpRail(new File(context.getFilesDir(),"dm/appart/exp"));
        setWillNotDraw(false);
        setFocusable(false);
        setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
    }
    void labelIds(int[] game, int[] nav) { gameIds=game.clone(); navIds=nav.clone(); }
    private void relabel() {
        if (!lang.changed()) return;
        Context c=getContext();
        if (gameIds!=null) { String[] g=new String[gameIds.length]; for (int i=0;i<g.length;i++) g[i]=AppText.get(c,gameIds[i]); gameLabels=g; }
        if (navIds!=null) { String[] n=new String[navIds.length]; for (int i=0;i<n.length;i++) n[i]=AppText.get(c,navIds[i]); menuLabels=n; }
        lifeFormat=AppText.get(c,R.string.dmHudLife); manaFormat=AppText.get(c,R.string.dmHudMana); cancelLabel=AppText.get(c,R.string.dmTcCancel);
        levelText.words(AppText.get(c,R.string.dmAndroidHudLevel),AppText.get(c,R.string.dmAndroidHudLevelWord));
        if (style!=null) {
            style.refresh();


            String one=style.pick(GameStyle.EXOCET,"\u2160","I"), two=style.pick(GameStyle.EXOCET,"\u2161","II");
            setOne=one!=null?one:"I"; setTwo=two!=null?two:"II";
        }
    }



    private static final String LEVEL_GLYPHS="Lv %d · %d%%";
    public void setSafeInsets(int leftPx, int rightPx, int bottomPx) {
        safeLeft=leftPx/density; safeRight=rightPx/density; safeBottom=bottomPx/density;
        resizeNative();
    }

    public void setInputEnabled(boolean enabled) {
        if (inputEnabled==enabled) return;
        if (!enabled) cancelTouches();
        inputEnabled=enabled;
        drawnValid=false; invalidate();
        if (enabled) {
            images.clear(); art.clear(); glyphs.clear(); glowOffsets.clear();
            for(int i=0;i<skillAtlases.length;i++) {
                skillAtlases[i]=image("touchart/dm_native_skill_"+i+".png"); silAtlases[i]=image("hud_v4_ui/sil_dm_native_skill_"+i+".png");
            }
            mapArt=image("touchart/dm_native_map.png"); circleArt=image("touchart/dm_native_circle.png"); socketArt=image("touchart/dm_mobile_socket.png");
            try { hudUi=new JSONObject(new String(Files.readAllBytes(new File(overlayRoot,"hud_v4_ui/ui.json").toPath()),StandardCharsets.UTF_8)); }
            catch(Exception ignored) { hudUi=null; }
            expRail.reload();
            levelText.reload();
            resizeNative();
        }
    }
    private void resizeNative() {
        if (inputEnabled && getWidth()>0 && getHeight()>0) { nativeResize(getWidth()/density,getHeight()/density,safeLeft,safeRight,safeBottom,
                getResources().getConfiguration().smallestScreenWidthDp>=600); pushView(); }
    }

    private void pushView() {
        GameRect g=GameRect.ui();
        if (!inputEnabled || g.w()<=0 || g.h()<=0 || g.winW()!=getWidth() || g.winH()!=getHeight()) return;
        nativeView(g.x()/density,g.y()/density,g.w()/density,g.h()/density);
    }

    void viewChanged() {
        pushView(); publishRaw(); drawnValid=false; invalidate();
    }

    private float viewXpx() { GameRect g=GameRect.ui(); return g.whole() ? 0 : g.x(); }
    private float viewYpx() { GameRect g=GameRect.ui(); return g.whole() ? 0 : g.y(); }
    private float viewWpx() { GameRect g=GameRect.ui(); return g.whole() || g.w()<=0 ? getWidth() : g.w(); }
    private float viewHpx() { GameRect g=GameRect.ui(); return g.whole() || g.h()<=0 ? getHeight() : g.h(); }
    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh) {
        cancelTouches(); resizeNative();
    }
    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow(); running=true; drawnValid=false; invalidate(); Choreographer.getInstance().postFrameCallback(poll);
    }



    private boolean stopped;
    void setStopped(boolean s) {
        if (s == stopped) return;
        stopped = s;
        Choreographer.getInstance().removeFrameCallback(poll);
        if (!s && running) { drawnValid = false; Choreographer.getInstance().postFrameCallback(poll); }
    }
    @Override protected void onDetachedFromWindow() {
        running=false; Choreographer.getInstance().removeFrameCallback(poll); cancelTouches(); displayGate.reset(); super.onDetachedFromWindow();
    }
    public void cancelTouches() {
        for (int hand=0;hand<2;++hand) releaseTarget(hand);

        for (int i=0;i<16;++i) if (pending.held(i)) { int t=pending.cancel(i); pointerIds[i]=-1; if (t!=PendingFingers.NONE && effects!=null) effects.release(t); }
        if (!inputEnabled) { Arrays.fill(pointerIds,-1); return; }
        for (int i=0;i<16;++i) if (pointerIds[i]>=0) {
            nativeEvent(3,i,lastX[i],lastY[i]); pointerIds[i]=-1;
        }
        publishRaw();
    }



    private StatusEffects effects;
    private final PendingFingers pending = new PendingFingers(16);
    private final float slop;
    void setEffects(StatusEffects e) { effects = e; }






    private GameStyle style;
    private GameText vitalText, countText, buttonText;
    private final char[] number = new char[11];
    void setStyle(GameStyle s) {
        style = s;
        vitalText = s.text(GameStyleData.HUD_TEXT); countText = s.text(GameStyleData.HUD_COUNT);
        buttonText = s.text(GameStyleData.BUTTON_LABEL);
        vitalText.prepare(); countText.prepare(); buttonText.prepare();
        levelText.style(s);
        lang.reset(); invalidate();
    }

    void voidClaims() { pending.voidAll(); }
    private void publishRaw() {
        int count=0;

        float vx=viewXpx(), vy=viewYpx(), vw=viewWpx(), vh=viewHpx();
        if (vw>0 && vh>0) for (int s=0;s<16;++s) if (pointerIds[s]>=0 && !pending.held(s)) {
            rawIds[count]=s; rawX[count]=(lastX[s]*density-vx)/vw; rawY[count]=(lastY[s]*density-vy)/vh; ++count;
        }
        DmActivity.nativeTouch(count,rawIds,rawX,rawY);
    }


    boolean hudShown() {
        if (!inputEnabled || !isShown()) return false;
        nativeLayout(probe);
        return probe[2]==0 && probe[HUD]!=0;
    }



    private final DisplayGate displayGate = new DisplayGate();
    boolean displaysShown(long frameNanos) {
        if (!displayGate.seen(frameNanos)) displayGate.advance(frameNanos, DmActivity.hostStarted() ? DmActivity.nativeDisplayState() : 0);
        return displayGate.shown();
    }
    void resetDisplays() { displayGate.reset(); }


    boolean stickActive() { return probe[4+12*5+4]!=0; }

    boolean targetBox(int target,float[] out) {
        int p=4+target*5;
        if (probe[p]==0) return false;
        out[0]=probe[p]*density; out[1]=probe[p+1]*density; out[2]=probe[p+2]*density;
        return true;
    }



    boolean stickThumb(float[] out) { if (!inputEnabled || !isShown() || !targetBox(12,out)) return false; out[2]=out[2]/60*THUMB; return true; }
    int players() { if (!inputEnabled) nativeLayout(probe); return Math.max(1,(int)probe[HUD+7]); }
    Bitmap overlayImage(String name) { return image(name); }
    boolean targetCentre(int target,float[] out) {
        float[] l=probe; int p=4+target*5;
        if (l[p]==0) return false;
        out[0]=l[p]*density; out[1]=l[p+1]*density; out[2]=l[4+1*5+2]/26*density;
        return true;
    }

    boolean pressTarget(int hand,int target) {
        if (!inputEnabled || shoulder[hand]) return false;
        nativeLayout(probe);
        int p=4+target*5, s=14+hand;
        if (probe[2]!=0 || probe[p+3]==0) return false;
        lastX[s]=probe[p]+viewXpx()/density; lastY[s]=probe[p+1]+viewYpx()/density;
        nativeEvent(0,s,lastX[s],lastY[s]);
        shoulder[hand]=true;
        return true;
    }
    void releaseTarget(int hand) {
        if (!shoulder[hand]) return;
        shoulder[hand]=false;
        nativeEvent(2,14+hand,lastX[14+hand],lastY[14+hand]);
    }
    private int slot(int id) {
        for (int i=0;i<16;++i) if (pointerIds[i]==id) return i;
        return -1;
    }
    private void event(MotionEvent e,int index,int phase,int s) {
        lastX[s]=e.getX(index)/density; lastY[s]=e.getY(index)/density;
        nativeEvent(phase,s,lastX[s],lastY[s]);
    }
    @Override public boolean onTouchEvent(MotionEvent e) {
        if (!inputEnabled) return false;
        int action=e.getActionMasked(), index=e.getActionIndex();
        if (action==MotionEvent.ACTION_DOWN || action==MotionEvent.ACTION_POINTER_DOWN) {
            for (int s=0;s<14;++s) if (pointerIds[s]<0) {
                pointerIds[s]=e.getPointerId(index);
                float px=e.getX(index), py=e.getY(index);
                int t=effects!=null && !stickActive() ? effects.claim(px-effects.getLeft(),py-effects.getTop()) : 0;
                if (t!=0) { railFinger[s]=false; lastX[s]=px/density; lastY[s]=py/density; pending.land(s,t,px,py); effects.hold(t); }
                else { if (effects!=null) effects.gameTouch(); railFinger[s]=onRail(px,py); event(e,index,0,s); }
                break;
            }
        } else if (action==MotionEvent.ACTION_MOVE) {
            for (int i=0;i<e.getPointerCount();++i) {
                int s=slot(e.getPointerId(i)); if (s<0) continue;
                if (!pending.held(s)) { event(e,i,1,s); continue; }
                int t=pending.move(s,e.getX(i),e.getY(i),slop);
                if (t==PendingFingers.NONE) continue;

                nativeEvent(5,s,pending.landX(s)/density,pending.landY(s)/density); event(e,i,1,s);
                if (effects!=null) effects.release(t);
            }
        } else if (action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP) {
            int s=slot(e.getPointerId(index));
            if (s>=0 && pending.held(s)) {
                int was=pending.target(s), t=pending.up(s,e.getX(index),e.getY(index),slop); pointerIds[s]=-1; railFinger[s]=false;
                if (was!=PendingFingers.NONE && effects!=null) effects.release(was);
                if (t!=PendingFingers.NONE && effects!=null) effects.tap(t);
            } else if (s>=0) { event(e,index,railFinger[s]?4:2,s); pointerIds[s]=-1; railFinger[s]=false; }
            if (action==MotionEvent.ACTION_UP) performClick();
        } else if (action==MotionEvent.ACTION_CANCEL) cancelTouches();
        publishRaw(); return true;
    }
    @Override public boolean performClick() { super.performClick(); return true; }




    private boolean onRail(float x,float y) {
        x-=viewXpx(); y-=viewYpx();
        if (layout[2]!=0 || !expRail.covers(x,y)) return false;
        float dx=x/density, dy=y/density;
        for (int i=0;i<(GLYPHS-4)/5;++i) {
            int p=4+i*5; float r=layout[p+2];
            if (i==12) continue;
            if (layout[p+3]!=0 && r>0 && (dx-layout[p])*(dx-layout[p])+(dy-layout[p+1])*(dy-layout[p+1])<=r*r) return false;
        }
        return true;
    }
    private String label(int i,boolean navigation) {
        if (i>=15) return Integer.toString(i-14);
        String[] source=navigation && i<5?menuLabels:gameLabels;
        String text=i<source.length && source[i]!=null?source[i]:"";

        if(i==13) return layout[3]!=0?setTwo:setOne;
        if(i==11 && layout[HUD+15]!=0) return cancelLabel;
        return text;
    }


    private Bitmap image(String name) {
        if(images.containsKey(name)) return images.get(name);
        File file=new File(overlayRoot,name);
        Bitmap bitmap=file.isFile() && file.length()<=8*1024*1024 ? BitmapFactory.decodeFile(file.getAbsolutePath()):null;
        images.put(name,bitmap); return bitmap;
    }


    private static int rgba(double r,double g,double b,double a) {
        return Color.argb((int)Math.round(a*255),(int)Math.round(r*255),(int)Math.round(g*255),(int)Math.round(b*255));
    }
    private static RectF inset(float side,float d) { return new RectF(d,d,side-d,side-d); }
    private static Shader vgrad(RectF r,int... colors) { return new LinearGradient(0,r.top,0,r.bottom,colors,null,Shader.TileMode.CLAMP); }
    private static Paint stroke(int color,float width) {
        Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setStyle(Paint.Style.STROKE);p.setColor(color);p.setStrokeWidth(width);return p;
    }
    private interface Art { void draw(Canvas c,float side,Bitmap target,float scale); }


    private Bitmap artBitmap(String key,float side,Art a) {
        int px=Math.max(1,Math.round(side*density)); String k=key+":"+px; Bitmap b=art.get(k);
        if(b==null) {
            if(art.size()>=64) art.clear();
            b=Bitmap.createBitmap(px,px,Bitmap.Config.ARGB_8888); Canvas bc=new Canvas(b); float scale=px/side; bc.scale(scale,scale);
            a.draw(bc,side,b,scale); art.put(k,b);
        }
        return b;
    }
    private void drawArt(Canvas c,String key,float x,float y,float side,Art a) {
        c.drawBitmap(artBitmap(key,side,a),null,new RectF(x,y,x+side,y+side),bitmapPaint);
    }

    private static void buttonArt(Canvas c,float side,boolean held,boolean empty,boolean primary) {
        RectF rim=inset(side,side*.018f), face=inset(side,side*.055f); Paint f=new Paint(Paint.ANTI_ALIAS_FLAG);
        f.setShader(vgrad(rim,rgba(.26,.28,.28,empty?.22:.9),rgba(.055,.065,.07,empty?.22:.95),rgba(.16,.11,.10,empty?.16:.7))); c.drawOval(rim,f);
        c.drawOval(rim,stroke(rgba(.008,.012,.015,empty?.38:.9),side*.025f));
        f.setShader(vgrad(face,rgba(.021,.021,.021,empty?.08:.94),rgba(.05,.05,.05,empty?.16:.94))); c.drawOval(face,f);
        int edge=empty?rgba(held?.72:.62,held?.72:.62,held?.72:.62,held?.80:.60):rgba(held?.64:.26,held?.64:.29,held?.64:.28,held?.72:.55);
        c.drawOval(face,stroke(edge,side*(held?.018f:(empty?.012f:.009f))));
        if(primary && !empty) c.drawOval(inset(side,side*.035f),stroke(rgba(.31,.23,.22,.52),side*.012f));
        if(empty) {
            float arm=side*.055f,m=side/2; Paint p=stroke(rgba(.60,.65,.64,held?.75:.25),side*.018f); p.setStrokeCap(Paint.Cap.ROUND);
            c.drawLine(m-arm,m,m+arm,m,p); c.drawLine(m,m-arm,m,m+arm,p);
        }
    }



    private static int neutral(int argb) {
        double a=Color.alpha(argb)/255.0, l=(.3*Color.red(argb)+.59*Color.green(argb)+.11*Color.blue(argb))/255.0;
        int v=(int)Math.round(((1-a)*.5+a*l)*255); return Color.argb(255,v,v,v);
    }


    private static void neutralizeWell(Bitmap b,float side,float scale) {
        int w=b.getWidth(),h=b.getHeight(); Bitmap mask=Bitmap.createBitmap(w,h,Bitmap.Config.ALPHA_8);
        Canvas mc=new Canvas(mask); mc.scale(scale,scale); Paint mp=new Paint(Paint.ANTI_ALIAS_FLAG); mc.drawOval(inset(side,side*.08f),mp);
        int[] px=new int[w*h]; b.getPixels(px,0,w,0,0,w,h); byte[] cov=new byte[w*h]; mask.copyPixelsToBuffer(java.nio.ByteBuffer.wrap(cov)); mask.recycle();
        for(int i=0;i<px.length;i++) {
            double m=(cov[i]&255)/255.0; if(m==0) continue;
            int o=px[i],n=neutral(o); double oa=Color.alpha(o)/255.0, ra=m+(1-m)*oa;
            int r=(int)Math.round((m*Color.red(n)+(1-m)*oa*Color.red(o))/ra), g=(int)Math.round((m*Color.green(n)+(1-m)*oa*Color.green(o))/ra), bl=(int)Math.round((m*Color.blue(n)+(1-m)*oa*Color.blue(o))/ra);
            px[i]=Color.argb((int)Math.round(ra*255),r,g,bl);
        }
        b.setPixels(px,0,w,0,0,w,h);
    }


    private Bitmap buttonBitmap(float side,boolean held,boolean empty,boolean primary,boolean image) {
        return artBitmap("skill:"+(held?1:0)+(empty?1:0)+(primary?1:0)+(image?1:0),side,(bc,s,target,scale)->{
            if(image) {
                bc.drawBitmap(socketArt,null,new RectF(0,0,s,s),bitmapPaint); neutralizeWell(target,s,scale);
                if(held) bc.drawOval(inset(s,s*.052f),stroke(rgba(.64,.64,.64,.72),s*.015f));
            } else buttonArt(bc,s,held,empty,primary);
        });
    }
    private void button(Canvas c,float x,float y,float r,boolean held,boolean empty,boolean primary) {
        c.drawBitmap(buttonBitmap(2*r,held,empty,primary,socketArt!=null && !empty),null,new RectF(x-r,y-r,x+r,y+r),bitmapPaint);
    }



    void dumpArt(File dir) throws java.io.IOException {
        socketArt=image("touchart/dm_mobile_socket.png"); silAtlases[0]=image("hud_v4_ui/sil_dm_native_skill_0.png");
        if(socketArt==null || silAtlases[0]==null) throw new java.io.IOException("overlay art missing");
        if(!dir.isDirectory() && !dir.mkdirs()) throw new java.io.IOException("mkdir "+dir);
        HashMap<String,Bitmap> out=new HashMap<>();
        for(int h=0;h<2;h++) for(int e=0;e<2;e++) for(int p=0;p<2;p++) out.put("button_h"+h+"e"+e+"p"+p,buttonBitmap(80,h==1,e==1,p==1,false));
        for(int h=0;h<2;h++) out.put("socket_h"+h,buttonBitmap(80,h==1,false,false,true));
        for(int a=0;a<2;a++) { boolean act=a==1;
            out.put("joybase_a"+a,artBitmap("stick:1:"+a,120,(bc,side,t,sc)->joyBaseArt(bc,side,act)));
            out.put("joythumb_a"+a,artBitmap("stick:0:"+a,46,(bc,side,t,sc)->joyThumbArt(bc,side,act))); }
        Bitmap sil=silAtlases[0]; out.put("silver0",glyph(0,0,0,sil.getWidth(),sil.getHeight()));
        for(java.util.Map.Entry<String,Bitmap> en:out.entrySet())
            try(java.io.FileOutputStream f=new java.io.FileOutputStream(new File(dir,en.getKey()+".png"))) { en.getValue().compress(Bitmap.CompressFormat.PNG,100,f); }
    }

    private static void joyBaseArt(Canvas c,float side,boolean active) {
        float m=side/2; Paint f=new Paint(Paint.ANTI_ALIAS_FLAG);
        f.setShader(new RadialGradient(m,m,side*.48f,new int[]{rgba(.025,.025,.025,active?.10:.045),rgba(.025,.025,.025,active?.22:.12),rgba(.025,.025,.025,0)},
                new float[]{0,.78f,1},Shader.TileMode.CLAMP));
        c.drawCircle(m,m,side*.48f,f);
        Paint arc=stroke(rgba(.63,.63,.63,active?.12:.055),side*.004f); arc.setStrokeCap(Paint.Cap.ROUND);
        Paint mark=stroke(rgba(.68,.68,.68,active?.25:.13),side*.004f); mark.setStrokeCap(Paint.Cap.ROUND);
        float R=side*.416f; RectF ring=new RectF(m-R,m-R,m+R,m+R);
        for(int i=0;i<4;i++) {
            double angle=i*Math.PI/2;
            c.drawArc(ring,(float)Math.toDegrees(angle+.20),(float)Math.toDegrees(Math.PI/2-.40),false,arc);
            c.save(); c.translate(m,m); c.rotate(i*90f);
            float x=side*.414f,k=side*.028f; Path p=new Path();
            p.moveTo(x+k,0); p.lineTo(x-k,-k*.65f); p.lineTo(x-k*.42f,0); p.lineTo(x-k,k*.65f); p.close(); c.drawPath(p,mark);
            c.drawLine(x-k*.42f,0,x-k*2.1f,0,mark);
            c.restore();
        }
    }

    private static void joyThumbArt(Canvas c,float side,boolean active) {
        RectF disc=inset(side,side*.055f); Paint f=new Paint(Paint.ANTI_ALIAS_FLAG); float m=side/2;
        f.setShader(vgrad(disc,rgba(.23,.23,.23,active?.78:.46),rgba(.14,.14,.14,active?.82:.49),rgba(.065,.065,.065,active?.84:.52))); c.drawOval(disc,f);
        c.drawOval(disc,stroke(rgba(.025,.025,.025,active?.65:.32),side*.035f));
        float a=side*.437f; c.drawArc(new RectF(m-a,m-a,m+a,m+a),(float)Math.toDegrees(Math.PI*1.07),(float)Math.toDegrees(Math.PI*.84),false,stroke(rgba(.64,.64,.64,active?.40:.22),side*.014f));
        float b=side*.410f; c.drawArc(new RectF(m-b,m-b,m+b,m+b),(float)Math.toDegrees(Math.PI*1.12),(float)Math.toDegrees(Math.PI*.76),false,stroke(rgba(.42,.42,.42,active?.20:.10),side*.012f));
    }




    private void caption(Canvas c,String value,float cx,float top,float size,float alpha) {
        if (buttonText==null || value==null || value.isEmpty() || !buttonText.ready() || !(buttonText.units()>0)) return;
        float k=size*density/buttonText.units(), baseline=top*density-buttonText.paint(k).ascent();
        c.save(); c.scale(1/density,1/density);
        buttonText.draw(c,value,cx*density,baseline,k,Paint.Align.CENTER,Math.round(Math.max(0,Math.min(1,alpha))*255));
        c.restore();
    }



    private float hudK() { return GameStyleGeometry.k(viewHpx())*style.data().scale(GameStyleData.PROFILE); }
    private float countPx(float fieldPx) { return GameStyleGeometry.counterText(style.data().counter(),countText.units(),fieldPx,style.data().size("SmallFontSize"),hudK()); }

    private int digits(int v) {
        int n=0; v=Math.max(0,v);
        do { number[n++]=(char)('0'+v%10); v/=10; } while (v>0 && n<number.length);
        for (int i=0,j=n-1;i<j;i++,j--) { char t=number[i]; number[i]=number[j]; number[j]=t; }
        return n;
    }


    private Bitmap glyph(int atlas,int sx,int sy,int dm,int sh) {
        Bitmap sil=silAtlases[atlas], src=sil!=null?sil:skillAtlases[atlas]; if(src==null) return null;
        int trim=sil!=null?0:Math.round(dm*.08f); sx+=trim; sy+=trim; dm-=2*trim; sh-=2*trim;
        if(dm<=0 || sh<=0 || sx<0 || sy<0 || sx+dm>src.getWidth() || sy+sh>src.getHeight()) return null;
        String key=atlas+":"+sx+":"+sy+":"+dm+":"+sh; Bitmap g=glyphs.get(key); if(g!=null) return g;
        g=Bitmap.createBitmap(src,sx,sy,dm,sh).copy(Bitmap.Config.ARGB_8888,true);
        if(sil!=null) {
            int[] px=new int[dm*sh]; g.getPixels(px,0,dm,0,0,dm,sh);
            for(int i=0;i<px.length;i++) { int n=neutral(px[i]); px[i]=(px[i]&0xff000000)|(n&0x00ffffff); }
            g.setPixels(px,0,dm,0,0,dm,sh);
        }
        if(glyphs.size()>=96) { glyphs.clear(); glowOffsets.clear(); } glyphs.put(key,g); return g;
    }


    private Bitmap glow(Bitmap g,RectF dst,float blur,int[] offset) {
        int w=Math.max(1,Math.round(dst.width()*density)),h=Math.max(1,Math.round(dst.height()*density));
        String key="glow:"+System.identityHashCode(g)+":"+w+":"+h+":"+Math.round(blur*density*10); Bitmap out=glyphs.get(key);
        if(out==null) {

            Bitmap scaled=Bitmap.createScaledBitmap(g,w,h,true); Paint p=new Paint();
            float sigma=blur*density/2; p.setMaskFilter(new BlurMaskFilter(Math.max(.5f,(sigma-.5f)/.57735f),BlurMaskFilter.Blur.NORMAL));
            int[] o=new int[2]; out=scaled.extractAlpha(p,o); scaled.recycle(); glyphs.put(key,out); glowOffsets.put(key,o);
        }
        int[] o=glowOffsets.get(key); offset[0]=o[0]; offset[1]=o[1]; return out;
    }
    private void skill(Canvas c,int i,float x,float y,float r,boolean held,boolean navigation) {
        int p=GLYPHS+i*8, atlas=(int)layout[p+1], status=(int)layout[p];
        Bitmap g=status==2 && atlas>=0 && atlas<9 ? glyph(atlas,(int)layout[p+2],(int)layout[p+3],(int)layout[p+4],(int)layout[p+5]) : null;


        boolean empty=g==null && (i==0 || status==1);
        button(c,x,y,r,held,empty,i==0);
        if(empty) return;
        if(g==null) {
            Bitmap lbl=image("hud_v4_ui/"+(i==0?"lbl_main.png":"lbl_f"+(i+(layout[3]!=0?4:0))+".png"));
            if(lbl!=null) { float lh=r*(i==0?.30f:.36f),lw=lbl.getWidth()*lh/lbl.getHeight(); c.drawBitmap(lbl,null,new RectF(x-lw/2,y+r*.50f-lh/2,x+lw/2,y+r*.50f+lh/2),bitmapPaint); }
            return;
        }
        float size=2*r*.76f, fit=size/Math.max(g.getWidth(),g.getHeight());
        RectF ir=new RectF(x-g.getWidth()*fit/2,y-g.getHeight()*fit/2,x+g.getWidth()*fit/2,y+g.getHeight()*fit/2);
        boolean spent=((int)layout[p+6]&2)!=0 && layout[p+7]<=0;
        c.save(); Path clip=new Path(); clip.addCircle(x,y,r*.82f,Path.Direction.CW); c.clipPath(clip);
        Paint ip=new Paint(bitmapPaint);
        if(spent) {
            Paint f=new Paint(Paint.ANTI_ALIAS_FLAG); f.setColor(rgba(.30,.30,.30,1)); c.drawCircle(x,y,r*.84f,f);
            ip.setBlendMode(BlendMode.LUMINOSITY); ip.setAlpha(140); c.drawBitmap(g,null,ir,ip);
            f.setColor(rgba(0,0,0,.35)); c.drawCircle(x,y,r*.88f,f);
        } else {
            int[] o=new int[2]; Bitmap gl=glow(g,ir,held?r*.10f:r*.055f,o);
            Paint gp=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG); gp.setColor(rgba(.65,.65,.65,.32));
            c.save(); c.translate(ir.left,ir.top); c.scale(1/density,1/density); c.drawBitmap(gl,o[0],o[1],gp); c.restore();
            ip.setAlpha(held?255:240); c.drawBitmap(g,null,ir,ip);
        }
        c.restore();


        int kind=(int)layout[p+6];
        if((kind&1)!=0) {
            float br=r*.30f,bx=x+r*.74f,by=y-r*.74f; Paint f=new Paint(Paint.ANTI_ALIAS_FLAG); f.setColor(rgba(.11,.094,.078,.95)); c.drawCircle(bx,by,br,f);
            float lw=Math.max(1,br*.14f),in=Math.max(1,br*.08f); c.drawCircle(bx,by,br-in,stroke(rgba(.63,.50,.27,1),lw));
            float ts=br*1.15f; caption(c,setTwo,bx,by-ts*.62f,ts,1);
        }
        if((kind&2)!=0 && countText!=null && countText.ready()) {
            int n=digits((int)layout[p+7]); float d=density, cell=PotionButtons.CELL*r*d;
            c.save(); c.scale(1/d,1/d);
            style.cornerNumber(c,number,0,n,x*d-cell,y*d-cell,x*d+cell,y*d+cell,countPx(2*r*d),"FontColorWhite");
            c.restore();
        } else if((kind&2)!=0 && hudUi!=null) {
            float u=viewHpx()/density/1320, W=viewWpx()/density;
            try { JSONObject bd=hudUi.getJSONObject("badge"); hudText(c,Integer.toString(Math.max(0,(int)layout[p+7])),bd.getJSONObject("glyphs"),(x+r*.66f-W/2)/u+1434,(y+r*.42f)/u,true,u); }
            catch(Exception ignored) { }
        }
    }
    private float hudX(double x,float u) { return viewWpx()/density/2+((float)x-1434)*u; }
    private void hudImage(Canvas c,String name,double x,double y,float u) {
        Bitmap b=image("hud_v4_ui/"+name); if(b==null)return;
        float left=hudX(x,u),top=(float)y*u;
        paint.setColor(Color.WHITE);c.drawBitmap(b,null,new RectF(left,top,left+b.getWidth()*u,top+b.getHeight()*u),paint);
    }


    private void hudText(Canvas c,String value,JSONObject glyphs,double x,double baseline,boolean right,float u) throws Exception {
        double advance=0;
        for(int i=0;i<value.length();i++) { JSONObject g=glyphs.optJSONObject(value.substring(i,i+1)); if(g!=null)advance+=g.optDouble("adv"); }
        double pen=x-(right?advance:advance/2);
        for(int i=0;i<value.length();i++) {
            JSONObject g=glyphs.optJSONObject(value.substring(i,i+1));if(g==null)continue;
            if(!g.isNull("file")) hudImage(c,g.getString("file"),pen+g.optDouble("dx"),baseline+g.optDouble("dy"),u);
            pen+=g.optDouble("adv");
        }
    }
    private void hud(Canvas c) {
        expRail.hide();
        if(hudUi==null || layout[HUD]==0) return;
        float u=viewHpx()/density/1320;
        try {
            JSONObject base=hudUi.getJSONObject("base");hudImage(c,"base_full.png",base.getDouble("x"),base.getDouble("y"),u);
            JSONArray slots=hudUi.getJSONArray("slots"), inner=hudUi.getJSONArray("inner"), menu=slots.getJSONArray(5);
            boolean alert=layout[HUD+12]>0 || layout[HUD+13]>0, held=layout[4+10*5+4]!=0;
            Bitmap button=image("hud_v4_ui/"+(alert?"lvl_":"lvl_d")+(held?1:0)+".png");
            if(button!=null) c.drawBitmap(button,null,new RectF(hudX(menu.getDouble(0),u),(float)menu.getDouble(1)*u,hudX(menu.getDouble(2),u),(float)menu.getDouble(3)*u),paint);
            JSONObject e=hudUi.getJSONObject("exp");
            float x=hudX(e.getDouble("x0"),u),y=(float)e.getDouble("y")*u,w=(float)(e.getDouble("x1")-e.getDouble("x0"))*u,h=(float)e.getDouble("h")*u;

            if(!expRail.draw(c,x,x+w,y,u,density,(int)layout[HUD+2])) {
                paint.setColor(0xff16120e);c.drawRect(x,y,x+w,y+h,paint);
                paint.setColor(0xffc4a046);c.drawRect(x+u,y+u,x+u+(w-2*u)*Math.max(0,Math.min(1000,layout[HUD+2]))/1000,y+h-u,paint);
                paint.setColor(0xff463a28);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(u);c.drawRect(x,y,x+w,y+h,paint);paint.setStyle(Paint.Style.FILL);
                paint.setColor(0xff16120e);for(int i=1;i<20;i++)c.drawRect(x+w*i/20,y+u,x+w*i/20+u,y+h-u,paint);
            }
            JSONObject lv=hudUi.getJSONObject("lv"), vital=hudUi.getJSONObject("vital"),badge=hudUi.getJSONObject("badge");
            int level=(int)layout[HUD+1], percent=(int)layout[HUD+2]/10;
            String life=String.format(Locale.ROOT,lifeFormat,(int)layout[HUD+8],(int)layout[HUD+9]), mana=String.format(Locale.ROOT,manaFormat,(int)layout[HUD+10],(int)layout[HUD+11]);
            if(vitalText!=null && vitalText.ready() && countText.ready() && levelText.ready()) {


                float k=hudK(), d=density, cell=(float)(inner.getDouble(2)-inner.getDouble(0))*u*d;
                c.save(); c.scale(1/d,1/d);
                levelText.draw(c,level,percent,hudX(lv.getDouble("x"),u)*d,(float)lv.getDouble("baseline")*u*d,w*d,k);
                vitalText.draw(c,life,hudX(vital.getDouble("left_x"),u)*d,(float)vital.getDouble("baseline")*u*d,k,Paint.Align.CENTER,255);
                vitalText.draw(c,mana,hudX(vital.getDouble("right_x"),u)*d,(float)vital.getDouble("baseline")*u*d,k,Paint.Align.CENTER,255);
                for(int i=1;i<=4;i++) if(layout[HUD+2+i]>0) {
                    JSONArray s=slots.getJSONArray(i); int n=digits((int)layout[HUD+2+i]);
                    countText.drawPx(c,number,0,n,hudX(s.getDouble(0)+inner.getDouble(2)+badge.getDouble("dx"),u)*d,(float)(s.getDouble(1)+inner.getDouble(3)+badge.getDouble("dy"))*u*d,
                        countPx(cell),Paint.Align.RIGHT,255);
                }
                c.restore();
            } else {
                hudText(c,String.format(Locale.ROOT,LEVEL_GLYPHS,level,percent),lv.getJSONObject("glyphs"),lv.getDouble("x"),lv.getDouble("baseline"),false,u);
                hudText(c,life,vital.getJSONObject("glyphs"),vital.getDouble("left_x"),vital.getDouble("baseline"),false,u);
                hudText(c,mana,vital.getJSONObject("glyphs"),vital.getDouble("right_x"),vital.getDouble("baseline"),false,u);
                for(int i=1;i<=4;i++) if(layout[HUD+2+i]>0) {
                    JSONArray s=slots.getJSONArray(i);
                    hudText(c,Integer.toString((int)layout[HUD+2+i]),badge.getJSONObject("glyphs"),s.getDouble(0)+inner.getDouble(2)+badge.getDouble("dx"),s.getDouble(1)+inner.getDouble(3)+badge.getDouble("dy"),true,u);
                }
            }



            if(layout[4+15*5+3]!=0) {
                JSONObject pk=hudUi.getJSONObject("pick"); Bitmap tile=image("hud_v4_ui/slot.png");
                for(int i=0;i<8;i++) {
                    double cx=pk.getDouble("center_x")+(i-3.5)*pk.getDouble("pitch"), cy=pk.getDouble("y"); int alpha=i+1==(int)layout[HUD+7]?255:209;
                    paint.setAlpha(alpha);
                    if(tile!=null) c.drawBitmap(tile,null,new RectF(hudX(cx-tile.getWidth()/2.0,u),(float)(cy-tile.getHeight()/2.0)*u,hudX(cx+tile.getWidth()/2.0,u),(float)(cy+tile.getHeight()/2.0)*u),paint);
                    Bitmap d=image("hud_v4_ui/diff_"+(i+1)+".png");
                    if(d!=null) c.drawBitmap(d,null,new RectF(hudX(cx-d.getWidth()/2.0,u),(float)(cy-d.getHeight()/2.0)*u,hudX(cx+d.getWidth()/2.0,u),(float)(cy+d.getHeight()/2.0)*u),paint);
                    paint.setAlpha(255);
                }
            }
        } catch(Exception ignored) {                                                                                    }
    }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas); relabel(); if (!inputEnabled) { if (BuildConfig.DM_QA_BUILD) reportControls(null,0,0); return; } nativeLayout(layout); System.arraycopy(layout, 0, drawn, 0, layout.length); drawnValid=true;
        canvas.save(); float vx=viewXpx(), vy=viewYpx(); if (vx!=0 || vy!=0) canvas.translate(vx,vy); canvas.scale(density,density);
        boolean navigation=layout[2]!=0;
        float k=layout[4+12*5+2]/60;


        for (int i=0;i<14;++i) {
            int p=4+i*5; if (i==12 || !drawsButton(layout,i)) continue;
            float x=layout[p], y=layout[p+1], r=layout[p+2];
            boolean held=layout[p+4]!=0;
            if(!navigation && i<=5) { skill(canvas,i,x,y,r,held,navigation); continue; }
            if(!navigation && i==13) { button(canvas,x,y,r,held,false,false); float ts=12*k; caption(canvas,label(i,false),x,y-ts*.65f,ts,held?1:.86f); continue; }



            RectF b=new RectF(x-r,y-r,x+r,y+r);
            boolean cancel=!navigation && i==11 && layout[HUD+15]!=0;
            if(cancel) { Paint f=new Paint(); f.setColor(rgba(.55,.05,.04,.36)); canvas.drawRect(b,f); }
            if(circleArt!=null) { bitmapPaint.setAlpha(held?153:71); canvas.drawBitmap(circleArt,null,b,bitmapPaint); bitmapPaint.setAlpha(255); }
            if(!navigation && i==11 && !cancel && mapArt!=null && mapArt.getWidth()>=2*mapArt.getHeight()) {
                int f=mapArt.getHeight(),sx=held?f:0; float half=r*.62f;
                bitmapPaint.setAlpha(held?255:209); canvas.drawBitmap(mapArt,new Rect(sx,0,sx+f,f),new RectF(x-half,y-half,x+half,y+half),bitmapPaint); bitmapPaint.setAlpha(255);
                continue;
            }
            String text=navigation && i==5 ? "" : label(i,navigation); float ts=(i==0?13:10)*k;
            caption(canvas,text,x,y-ts*.6f,ts,held?1:.84f);
        }

        int jp=4+12*5;
        if (drawsStick(layout)) {
            float jx=layout[jp],jy=layout[jp+1],jr=layout[jp+2]; boolean active=layout[jp+4]!=0;
            drawArt(canvas,"stick:1:"+(active?1:0),jx-jr,jy-jr,2*jr,(bc,side,t,sc)->joyBaseArt(bc,side,active));
            float tx=jx+layout[HUD+16]/32767f*jr, ty=jy-layout[HUD+17]/32767f*jr, th=THUMB*k;
            drawArt(canvas,"stick:0:"+(active?1:0),tx-th,ty-th,2*th,(bc,side,t,sc)->joyThumbArt(bc,side,active));
        }
        hud(canvas);
        canvas.restore();
        if (BuildConfig.DM_QA_BUILD) reportControls(layout, vx, vy);
    }



    private static boolean drawsButton(float[] l,int i) { int p=4+i*5; return l[p+3]!=0 && l[p+2]>0 && !(i>5 && i<=10); }
    private static boolean drawsStick(float[] l) { int p=4+12*5; return l[p+3]!=0 && l[2]==0 && l[HUD+14]==0 && l[p+2]>0; }


    private int controls(float[] l,float[] out,int at) {
        for (int i=0;i<14 && at+3<=out.length;++i) {
            int p=4+i*5;
            if (i==12 ? !drawsStick(l) || l[p+4]!=0 : !drawsButton(l,i)) continue;
            out[at++]=l[p]*density; out[at++]=l[p+1]*density; out[at++]=l[p+2]*density;
        }
        return at;
    }

    int controls(float[] out,int at) {
        if (!inputEnabled || !isShown()) return at;
        nativeLayout(probe);
        return controls(probe,out,at);
    }



    private final float[] qaControls=new float[3*14];
    private final int[] qaAt=new int[2];
    private void reportControls(float[] l,float vx,float vy) {
        int n=l==null ? 0 : controls(l,qaControls,0);
        StringBuilder b=new StringBuilder();
        if (n>0) getLocationInWindow(qaAt);
        for (int i=0;i<n;i+=3) b.append(b.length()>0?"_":"").append(Math.round(qaAt[0]+vx+qaControls[i])).append('_').append(Math.round(qaAt[1]+vy+qaControls[i+1])).append('_').append(Math.round(qaControls[i+2]));
        TestHooks.overlay(this,"touch_controls",n>0?b.toString():"none");
    }
}
