package com.xiaoboluo.d2r;

import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.widget.EditText;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.BitSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.json.JSONObject;
import org.json.JSONArray;

final class CombatPage extends View implements PadPage, QuickPage {
    private final AppArt art;
    private final GameStyle style;
    private final Runnable freeInput;
    private final GameText body, title, note;
    private final Paint picture=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final ExecutorService worker=Executors.newSingleThreadExecutor(r->new Thread(r,"combat-page"));
    private final List<RectF> boxes=new ArrayList<>();
    private final BitSet buttons=new BitSet();
    private final RectF focusBox=new RectF();
    private final List<Runnable> actions=new ArrayList<>();
    private final List<Integer> order=new ArrayList<>();
    private final float[] box=new float[4];
    private CombatModel model, detail, last;
    private CombatModel.Enemy nextBoss;
    private List<CombatModel.Enemy> tableRows=new ArrayList<>(), bosses=new ArrayList<>(), typical=new ArrayList<>(), minis=new ArrayList<>();
    private List<String> detailLines=new ArrayList<>(),wrappedLines=new ArrayList<>();


    private CombatLayout.Title header;
    private final HashMap<String,CombatLayout.Title> titles=new HashMap<>();
    private JSONObject names=new JSONObject(), skillNames=new JSONObject();
    private boolean de, paused, busy, failed, descending, ordered, potions, terror, padUsed, arrows;
    private int nameLang, fontLogged=-1;
    private int tab, page, pages=1, act, type=-1, sort, diff=-1, players=-1, tablePage, tableFocus, herald, selected=-1, focus, down=-1, epoch;
    private String query="";
    private Double rotationBefore;
    private AlertDialog dialog;
    private final Runnable refresh=()->request(false);


    private int[] listOpts,detailOpts;
    private byte[] listBytes,detailBytes;
    private boolean stale;

    private int calls,callsShown=-1;

    private CombatGraph graph;
    private final RectF graphBox=new RectF(),cell=new RectF();
    private final Path enemyLine=new Path(),lifeLine=new Path(),manaLine=new Path();
    private final Paint stroke=new Paint(Paint.ANTI_ALIAS_FLAG),area=new Paint(Paint.ANTI_ALIAS_FLAG);
    private Bitmap[] legendIcons=new Bitmap[0],markerIcons=new Bitmap[0];
    private int[] graphColours=new int[4];
    private int gold;


    static final int TAB_LOG=3,TAB_BEST=4,TABS=5;


    private BestGearState best;
    private float bestFill;
    private boolean bestBusy;
    private String bestRev="",bestStateLine="",bestFoot="",bestOff="",bestTotal="",bestFirst="";
    private boolean bestBar;
    private float bestTop;
    private List<BestGearPages.Page> bestPlan=java.util.Collections.singletonList(new BestGearPages.Page(0,0,0,0,false));

    private static final float BEST_HEAD=66,BEST_DOT_LINE=28,BEST_DOT_TEXT=24;
    static final int HIT_BEST_BAR=7,HIT_BEST_ROW=8;
    static final int HIT_BEST_CHARMS=9;
    static final int HIT_BEST_TIP=10,HIT_BEST_TIP_DOTS=11;
    private boolean bestCharms=true;
    private String bestCharmsLabel="",bestCharm="";
    private Bitmap checkOn,checkOff;
    private final List<String> bestWhere=new ArrayList<>();
    private final List<Bitmap> bestPictures=new ArrayList<>();
    private final List<String> bestColours=new ArrayList<>();
    private final java.util.Map<String,Bitmap> itemPictures=new java.util.HashMap<>();


    private int tipId,tipPlace=-1,tipDots;
    private String tipText="";
    private List<GearPreview.Line> tipLines;
    private Bitmap tipPicture;
    private ItemTooltip tooltip;
    private float tipDownX,tipDownY;
    private final DotRows dotRows;


    private DotLayout tipRows,totalRows;
    private int tipRowsCode,tipRowsRevision;
    private final float[] totalRowsAt=new float[5];
    private final RectF tipRowsBox=new RectF();
    private final Paint barFill=new Paint();
    private final Runnable bestPoll=this::pollBest,stockPoll=this::pollStock;

    private static final int STOCK_WAIT_MS=1000;
    private int stockAsk;
    private long stockAt;
    private boolean stockReading;
    private FightLog log;
    private FightLog.Detail logDetail;
    private List<FightLog.Row> logRows=new ArrayList<>();
    private List<String[]> logCells=new ArrayList<>();
    private List<Fit[]> logFits=new ArrayList<>();
    private static final float[] LOG_COLS={0,.50f,.68f,.82f,1};
    private final Set<Integer> opened=new HashSet<>();
    private int logSort=FightLog.SORT_NONE,logFight=-1,logGroup=-1,logAsked=-1,logEpoch,logShown=-1;
    private boolean logDescending,simShown=true,logBusy,logFailed,triangles;
    private JSONObject states=new JSONObject();
    private JSONArray difficulties=new JSONArray();
    private final Path groupLine=new Path(),starPath=new Path();
    private String logTitle="",logLine="",logNote="";



    private StreakLog streaks;
    private boolean streakView,streakDescending,streakBonus;

    private DpsLog dps;private boolean dpsView;private int dpsSel=-1,dpsFrom;
    private final List<String[]> dpsCells=new ArrayList<>();private final List<Fit[]> dpsFits=new ArrayList<>();private final List<Float> dpsRowY=new ArrayList<>();
    private Fit dpsHead,dpsLine,dpsCurveTitle,dpsSelLine;private String dpsLineColour="FontColorWhite",dpsSelColour="FontColorWhite";private float dpsSelX,dpsSelY,dpsLineY,dpsTitleY;
    private DpsLog.Curve dpsCurve;private final RectF dpsBox=new RectF();private final List<Fit> dpsMarks=new ArrayList<>();private final List<Fit> dpsTicks=new ArrayList<>();
    private boolean dpsOlder;
    private final List<Path> dpsMarkPaths=new ArrayList<>();private final List<RectF> dpsMarkHits=new ArrayList<>();
    private int streakSort=StreakLog.SORT_NONE;
    private final List<String[]> streakCells=new ArrayList<>();
    private final List<Fit[]> streakFits=new ArrayList<>();
    private final List<Float> streakStars=new ArrayList<>();
    private final Fit[] streakColumns=new Fit[4];
    private final Path streakStar=new Path();
    private Fit streakHead;
    private boolean streakOff;
    private String streakOffText="";
    private int streakGold;
    private static final float[] STREAK_COLS={0,.34f,.48f,.60f,1},STREAK_COLS_PLAIN={0,.34f,.34f,.46f,1};
    private String bestTab="";



    static final int HIT_BEST_EQUIP=10,HIT_BEST_EQUIP_ALL=11,HIT_BEST_NO_ROOM=12;
    private int[] bestButtons=new int[0];
    private final List<String> bestButtonText=new ArrayList<>();
    private boolean bestAll,equipBusy;
    private String bestAllLabel="",bestNote="",bestNoteColour="FontColorLightGray",equipName="",equipSlot="";

    private static final long EQUIP_LIMIT_MS=135000;
    private final Runnable equipWatch=this::equipStalled;
    private int equipSeq,equipResult=BestGearEquip.R_NONE;
    private BestGearEquip.Job equipJob;
    private static final int[] WHY_TEXT={R.string.dmAndroidBestWhyChanged,R.string.dmAndroidBestWhyCode,R.string.dmAndroidBestWhyLate,R.string.dmAndroidBestWhyPanel,
        R.string.dmAndroidBestWhyPage,R.string.dmAndroidBestWhySave,R.string.dmAndroidBestWhyHand,R.string.dmAndroidBestWhyRun,R.string.dmAndroidBestWhyFault,
        R.string.dmAndroidBestWhyTime,R.string.dmAndroidBestWhyOther};

    CombatPage(Context context,AppArt art,GameStyle style,Runnable freeInput) {
        super(context);this.art=art;this.style=style;this.freeInput=freeInput;
        body=style.text(GameStyleData.BODY);title=style.text(GameStyleData.TITLE);note=style.text(GameStyleData.NOTE);
        dotRows=new DotRows(style.rowGlyphs());
        setVisibility(GONE);
    }
    private int lang(){return de?CombatModel.L_DE:GameLang.PT.equals(GameLang.code())?CombatModel.L_PT:CombatModel.L_EN;}
    private String tr(String german,String english,String portuguese){return CombatModel.pick(lang(),german,english,portuguese);}
    static String menuName(){return GameLang.german()?"Kampfsimulation":"Combat Sim";}
    @Override public boolean available(){return DmActivity.nativeInGame()!=0;}
    @Override public void takePause(){if(!paused){paused=true;DmActivity.nativePause(DmActivity.PAUSE_COMBAT,1);}}
    @Override public void open(){
        DmActivity.syncLang();de=GameLang.german();style.refresh();takePause();
        arrows=style.covers(GameStyle.FORMAL,"↑↓");if(!arrows)AppArt.log("ASSUME combat.sort_arrows: game fonts lack the sort arrows");
        setVisibility(VISIBLE);freeInput.run();
        padUsed=PadInput.isPadMode();epoch++;tab=page=0;selected=-1;diff=-1;players=-1;herald=0;order.clear();ordered=false;rotationBefore=null;model=detail=last=null;failed=false;listOpts=detailOpts=null;listBytes=detailBytes=null;stale=false;calls=0;callsShown=-1;
        bestTab=AppText.get(getContext(),R.string.dmAndroidBestTab);best=null;bestRev="";bestFill=0;equipBusy=false;equipSeq=0;equipResult=BestGearEquip.R_NONE;equipJob=null;
        bestCharms=BestGearStore.searchCharms(getContext());DmActivity.nativeBestGearCharms(bestCharms?1:0);
        logEpoch++;log=null;logDetail=null;logRows=new ArrayList<>();opened.clear();logFight=logGroup=logShown=-1;logBusy=logFailed=false;
        streaks=null;streakView=false;streakSort=StreakLog.SORT_NONE;streakDescending=false;dps=null;dpsView=false;dpsSel=-1;
        triangles=style.covers(GameStyle.FORMAL,"▶▼");
        worker.execute(()->{JSONObject n=read("overlay/sim-names.json"),s=read("appart/builds/gamenames.json"),fx=read("appart/effects.json");post(()->{names=n;skillNames=s.optJSONObject("skills");if(skillNames==null)skillNames=new JSONObject();difficulties=s.optJSONArray("difficulties");if(difficulties==null)difficulties=new JSONArray();states=fx.optJSONObject("states");if(states==null)states=new JSONObject();prepare();invalidate();});});
        request(true);
    }

    void openBest(){open();switchTab(TAB_BEST);}
    private JSONObject read(String path){try {File f=new File(getContext().getFilesDir(),"dm/"+path);if(f.length()>4*1024*1024)return new JSONObject();return new JSONObject(new String(Files.readAllBytes(f.toPath()),StandardCharsets.UTF_8));}catch(Exception e){AppArt.log("ASSUME combat.names: name catalog unavailable");return new JSONObject();}}
    boolean isOpen(){return getVisibility()==VISIBLE;}
    void stop(){if(isOpen())close();}
    void dispose(){stop();worker.shutdownNow();}
    private void close(){epoch++;removeCallbacks(refresh);removeCallbacks(bestPoll);removeCallbacks(stockPoll);removeCallbacks(equipWatch);stockReading=false;closeTip();DmActivity.nativeBestGear(0);if(dialog!=null)dialog.dismiss();setVisibility(GONE);if(paused){paused=false;DmActivity.nativePause(DmActivity.PAUSE_COMBAT,0);}}
    private int[] options(int target){return CombatModel.options(diff,players,potions,terror,herald,target,ordered,order);}

    private static int[] resolved(int[] opts,CombatModel m){int[] o=opts.clone();if(o[0]<0)o[0]=m.difficulty;if(o[1]<0)o[1]=m.players;return o;}
    private void restale(){CombatModel m=selected<0?model:detail;stale=m!=null && !java.util.Arrays.equals(selected<0?listOpts:detailOpts,options(selected));}
    private void request(boolean changed){
        removeCallbacks(refresh);if(!isOpen())return;
        int target=selected;
        if(changed)failed=false;
        restale();qa();invalidate();
        if(busy){postDelayed(refresh,300);return;}
        int token=epoch;int[] opts=options(target);
        CombatModel shown=target<0?model:detail;byte[] before=shown==null?null:target<0?listBytes:detailBytes;
        busy=true;calls++;
        worker.execute(()->{
            CombatModel result=null;byte[] bytes=null;
            try {bytes=DmActivity.nativeCombat(opts);if(bytes!=null && !java.util.Arrays.equals(bytes,before))result=CombatModel.parse(new String(bytes,StandardCharsets.UTF_8));}catch(RuntimeException e){AppArt.log("combat.response: "+e);bytes=null;result=null;}
            final CombatModel ready=result;final byte[] response=bytes;
            post(()->{busy=false;if(token!=epoch){if(isOpen())request(false);return;}failed=response==null;
                if(target==selected){
                    if(ready!=null){
                        int[] used=resolved(opts,ready);
                        if(target<0){model=last=ready;listOpts=used;listBytes=response;}else{detail=ready;detailOpts=used;detailBytes=response;}
                        if(diff<0)diff=ready.difficulty;if(players<0)players=ready.players;prepare();
                    } else if(response!=null && shown!=null && shown==(target<0?model:detail)){

                        if(target<0)listOpts=resolved(opts,shown);else detailOpts=resolved(opts,shown);
                        if(changed)prepare();
                    } else if(response==null && stale){if(target<0)model=null;else detail=null;}
                    else if(response!=null){if(target<0)listBytes=null;else detailBytes=null;}
                    restale();
                }
                qa();invalidate();if(isOpen())postDelayed(refresh,1500);
            });
        });
    }

    private String name(String key,int language){JSONArray a=names.optJSONArray(key);return a==null?key:a.optString(language,key);}
    private String shownName(String key){return name(key,nameLang);}

    private void chooseNameLanguage(){
        int language=GameLang.index();if(language<0)language=de?1:GameLang.PT.equals(GameLang.code())?12:0;
        StringBuilder sample=new StringBuilder();for(int g=0;g<3;g++){JSONArray a=names.optJSONArray("@potion:"+g);if(a!=null)sample.append(a.optString(language,""));}
        String text=sample.toString();
        nameLang=text.isEmpty() || style.covers(GameStyle.GLOBAL,text) || style.covers(GameStyle.FORMAL,text)?language:0;
        if(nameLang!=language && language!=fontLogged){fontLogged=language;AppArt.log("ASSUME combat.name_fonts: game names of language "+language+" not drawable, English names shown");}
    }
    private String skillName(int id){if(id==0)return tr("Normaler Angriff","Normal attack","Ataque normal");if(names.has("@skill:"+id))return shownName("@skill:"+id);JSONObject s=skillNames.optJSONObject(Integer.toString(id));JSONArray a=s==null?null:s.optJSONArray("name");return a==null?Integer.toString(id):a.optString(de?0:1,Integer.toString(id));}
    private void hit(RectF r,Runnable action){hit(r,0,action);}
    private void hit(RectF r,int kind,Runnable action){boxes.add(new RectF(r));actions.add(action);if(BuildConfig.DM_QA_BUILD)TestHooks.hit(r,kind,actions.size()-1);}

    private static final class Fit{final String text;final float k;Fit(String text,float k){this.text=text;this.k=k;}}
    private static Fit fit(GameText font,String value,float width,float k){
        float fit=CombatLayout.shrink(value,width,k,k*.55f,font::measure);
        if(fit>0)return new Fit(value,fit);
        fit=k*.55f;String s=value;while(s.length()>1 && font.measure(s+"...",fit)>width)s=s.substring(0,s.length()-1);
        return new Fit(s+"...",fit);
    }

    private CombatLayout.Title title(String name,String rating,float width,float nameK,float k,boolean wrap){
        String key=name+'\n'+rating+'\n'+width+'\n'+nameK+'\n'+k+'\n'+wrap;
        CombatLayout.Title t=titles.get(key);
        if(t==null){t=CombatLayout.title(name,rating,width,nameK,body.floorK(nameK),nameK*CombatLayout.WRAP,k,wrap,body::measure);titles.put(key,t);}
        return t;
    }

    private void title(Canvas c,CombatLayout.Title t,float x,float y,float pitch,String colour){
        body.colour(colour);
        if(t.whole()){body.draw(c,t.first+t.rating,x,y,t.nameK,Paint.Align.LEFT,255);return;}
        body.draw(c,t.first,x,y,t.nameK,Paint.Align.LEFT,255);
        if(!t.second.isEmpty())body.draw(c,t.second,x,y+pitch,t.nameK,Paint.Align.LEFT,255);
        body.draw(c,t.rating,x+t.ratingX,y+t.ratingLine*pitch,t.ratingK,Paint.Align.LEFT,255);
    }
    private void text(Canvas c,GameText font,String value,float x,float y,float width,float k,String colour){
        Fit f=fit(font,value,width,k);font.colour(colour);font.draw(c,f.text,x,y,f.k,Paint.Align.LEFT,255);
    }

    private void button(Canvas c,RectF r,String label,Runnable action,float k){buttons.set(boxes.size());style.panelButton(c,r,padUsed && boxes.size()==focus?GameStyle.BUTTON_HOVER:GameStyle.BUTTON_NORMAL,label);hit(r,action);}
    private void switchTab(int next){if(equipBusy)return;closeTip();equipResult=BestGearEquip.R_NONE;int was=tab;tab=Math.floorMod(next,TABS);page=focus=0;if(was==TAB_BEST && tab!=TAB_BEST){removeCallbacks(bestPoll);DmActivity.nativeBestGear(0);if(stockReading)endStock(false);}if(tab==TAB_BEST && was!=TAB_BEST){readStock();pollBest();}if(selected>=0){epoch++;selected=-1;detail=null;request(false);}if(tab==TAB_LOG)requestLog();prepare();qa();invalidate();}
    private void change(){epoch++;page=0;rotationBefore=null;request(true);}
    private void loaded(Runnable move){if(tab==TAB_BEST || CombatLayout.padReady(tab==TAB_LOG,log,selected<0?model:detail))move.run();}
    private String maxHit(CombatModel.Enemy e){return e.death==-2?tr("nicht verfügbar","unavailable","indisponível"):number(e.maxHit*100)+" %";}
    private String mana(CombatModel m){return (m.flags&CombatModel.UNAVAILABLE)!=0?tr("Mana: nicht verfügbar","Mana: unavailable","Mana: indisponível"):"Mana: "+CombatModel.manaSupply(m.manaSeconds,lang());}
    private String dps(CombatModel m){return (m.flags&CombatModel.UNAVAILABLE)!=0?tr("nicht verfügbar","unavailable","indisponível"):!CombatModel.displayedPositive(m.rotation)?tr("Kein wirksamer Schaden","No effective damage","Sem dano efetivo"):number(m.rotation)+" DPS";}
    private String combatSummary(CombatModel m){return dps(m)+(CombatModel.displayedPositive(m.rotation) && m.manaUsed?" · "+mana(m):"");}
    private String rotation(CombatModel m){StringBuilder s=new StringBuilder("Rotation: ");for(int id:m.sequence){if(s.length()>10)s.append(" › ");s.append(skillName(id));}return s.toString();}
    private String immunity(CombatModel.Enemy e){String[] elements={tr("Physisch","Physical","Físico"),tr("Feuer","Fire","Fogo"),tr("Blitz","Lightning","Raio"),tr("Kälte","Cold","Frio"),tr("Gift","Poison","Veneno"),tr("Magie","Magic","Magia")};StringBuilder s=new StringBuilder();for(int i=0;i<6;i++)if(e.resistance[i]>=100){if(s.length()>0)s.append(", ");s.append(elements[i]);}return s.toString();}
    private String number(double value){return CombatModel.number(value,lang());}
    private String location(CombatModel.Enemy e){return e.area>0?(e.act>0?tr("Akt ","Act ","Ato ")+e.act+" · ":"")+shownName("@area:"+e.area):tr("Ereignis","Event","Evento");}
    private String ratingNote(CombatModel.Enemy e){return e.underestimated && e.light>=0 && e.light<2?tr("Schaden unterschätzt; Bewertung zu günstig","Damage underestimated; rating optimistic","Dano subestimado; avaliação otimista"):"";}
    private void enemy(Canvas c,CombatModel.Enemy e,RectF r,float k){
        art.nine(c,"build_inset",r,8,k,picture);float pad=12*k;
        boolean table=tab==1 && selected<0;
        if(table){
            String[] cells={shownName(e.key),CombatModel.time(e.kill,lang()),CombatModel.time(e.death,lang()),CombatModel.light(e.light,lang())};
            for(int col=0;col<4;col++){float left=CombatLayout.column(r.left,r.right,col)+pad,right=CombatLayout.column(r.left,r.right,col+1)-pad;text(c,body,cells[col],left,r.top+body.size(k)*1.2f,right-left,k,col==3?CombatModel.colour(e.light):"FontColorWhite");}
        }else title(c,title(shownName(e.key),CombatModel.light(e.light,lang()),r.width()-2*pad,k,k,false),r.left+pad,r.top+body.size(k)*1.2f,0,CombatModel.colour(e.light));
        String data=table?location(e):tr("Kill ","Kill ","Abate ")+CombatModel.time(e.kill,lang())+tr(" · Tod "," · Death "," · Morte ")+CombatModel.time(e.death,lang());
        data+=tr(" · max. Treffer "," · max hit "," · golpe máx. ")+maxHit(e);
        if(!immunity(e).isEmpty())data+=tr(" · Immun: "," · Immune: "," · Imune: ")+immunity(e);
        text(c,body,data,r.left+pad,r.top+body.size(k)*2.5f,r.width()-2*pad,k,"FontColorLightGray");
        if(!table && r.height()>=body.size(k)*4.1f)text(c,body,location(e),r.left+pad,r.top+body.size(k)*3.8f,r.width()-2*pad,k,"FontColorLightGray");
        hit(r,()->open(e));
    }
    private void open(CombatModel.Enemy e){tablePage=page;tableFocus=focus;epoch++;selected=e.id;detail=null;page=focus=0;request(true);}



    private void banner(Canvas c,CombatModel.Enemy e,String label,String missing,RectF r,float k){
        art.row(c,"build_long0",r,BANNER_END,picture);picture.setAlpha(BuildBook.LIT_ALPHA);art.row(c,"build_long1",r,BANNER_END,picture);picture.setAlpha(255);
        Bitmap frame=art.image("build_long0");
        float pad=(frame==null?0:BANNER_END*r.height()/frame.getHeight())+14*k,x=r.left+pad,width=r.right-14*k-x,y=r.top+note.size(k)*1.35f;
        text(c,note,label,x,y,width,k,"FontColorGoldYellow");
        if(e==null){text(c,body,missing,x,y+body.size(k)*1.5f,width,k,"FontColorLightGray");return;}
        y+=body.size(k)*BANNER_NAME*1.2f;
        CombatLayout.Title t=title(shownName(e.key),CombatModel.light(e.light,lang()),width,k*BANNER_NAME,k,false);
        body.colour("FontColorGoldYellow");body.draw(c,t.first,x,y,t.nameK,Paint.Align.LEFT,255);
        body.colour(CombatModel.colour(e.light));body.draw(c,t.rating,x+t.ratingX,y,t.ratingK,Paint.Align.LEFT,255);
        String data=tr("Kill ","Kill ","Abate ")+CombatModel.time(e.kill,lang())+tr(" · Tod "," · Death "," · Morte ")+CombatModel.time(e.death,lang())+tr(" · max. Treffer "," · max hit "," · golpe máx. ")+maxHit(e);
        if(!immunity(e).isEmpty())data+=tr(" · Immun: "," · Immune: "," · Imune: ")+immunity(e);
        y+=body.size(k)*1.3f;text(c,body,data,x,y,width,k,"FontColorLightGray");
        y+=body.size(k)*1.3f;text(c,body,location(e),x,y,width,k,"FontColorLightGray");
        hit(r,()->open(e));
    }
    private static final float BANNER_NAME=1.3f;
    private static final int BANNER_END=18;
    private float bannerHeight(float k){return note.size(k)*1.35f+body.size(k)*(BANNER_NAME*1.2f+2.6f+.75f);}

    private void mini(Canvas c,CombatModel.Enemy e,RectF r,float k){
        art.nine(c,"build_inset",r,8,k,picture);float pad=12*k;
        title(c,title(shownName(e.key),CombatModel.light(e.light,lang()),r.width()-2*pad,k,k,false),r.left+pad,r.top+body.size(k)*1.2f,0,CombatModel.colour(e.light));
        text(c,body,(e.area>0?shownName("@area:"+e.area)+" · ":"")+tr("Kill ","Kill ","Abate ")+CombatModel.time(e.kill,lang())+tr(" · Tod "," · Death "," · Morte ")+CombatModel.time(e.death,lang()),
            r.left+pad,r.top+body.size(k)*2.5f,r.width()-2*pad,k,"FontColorLightGray");
        hit(r,()->open(e));
    }
    @Override protected void onDraw(Canvas c){
        if(!isOpen())return;
        if(BuildConfig.DM_QA_BUILD)TestHooks.begin(this,"combat");
        try {
        boxes.clear();actions.clear();buttons.clear();
        style.backdrop(c);GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());style.panelFrame(c,p);
        String[] tabs={menuName(),tr("Alle Gegner","All enemies","Todos os inimigos"),"Rotation",tr("Protokoll","Fight log","Registro"),bestTab};
        for(int i=0;i<TABS;i++){final int t=i;style.panelTab(c,p,TABS,i,tabs[i],tab==i,style.panelTabK(p,TABS,i,tabs[i]));p.tabHit(TABS,i,box);hit(new RectF(box[0],box[1],box[2],box[3]),()->switchTab(t));}
        style.closeX(c,p);hit(new RectF(p.hitCloseL,p.hitCloseT,p.hitCloseR,p.hitCloseB),this::close);
        float k=getHeight()/1080f,x=p.contentL+24*p.s,right=p.contentR-24*p.s,y=p.contentT+40*p.s,bottom=p.contentB-20*p.s;
        CombatModel m=selected<0?model:detail;
        if(tab==TAB_LOG){drawLog(c,p,x,right,y,bottom,k);drawFoot(c,p,x,right,k);return;}
        if(tab==TAB_BEST){drawBest(c,x,right,y,bottom,k);drawFoot(c,p,x,right,k);drawTip(c);return;}


        CombatModel shown=m!=null?m:last;
        if(shown!=null)text(c,body,shown.name+" · "+tr("Stufe ","Level ","Nível ")+shown.level+" · "+tr("Akt ","Act ","Ato ")+shown.act+" · "+shown.life+tr(" Leben · "," life · "," vida · ")+shown.mana+" Mana",x,y,right-x,k,"FontColorWhite");

        if(stale && m!=null){note.colour("FontColorLightGray");note.draw(c,tr("Berechnung …","Calculating …","Calculando …"),right,y,k,Paint.Align.RIGHT,255);}
        y+=42*k;
        boolean showHerald=shown!=null && shown.rotw && diff==2;
        float bh=48*k,bw=(right-x)/(showHerald?6:5);int col=0;
        button(c,new RectF(x,y,x+bw-8*k,y+bh),CombatModel.difficultyLabel(diff,lang()),()->{if(diff<0)return;diff=(diff+1)%3;if(diff!=2)herald=0;selected=-1;detail=null;change();},k);col++;
        button(c,new RectF(x+col*bw,y,x+(col+1)*bw-8*k,y+bh),CombatModel.playersLabel(players,lang()),()->{if(players<1)return;players=CombatModel.nextPlayers(players);change();},k);col++;
        button(c,new RectF(x+col*bw,y,x+(col+1)*bw-8*k,y+bh),tr("Tränke: ","Potions: ","Poções: ")+(potions?tr("an","on","ligado"):tr("aus","off","desligado")),()->{potions=!potions;change();},k);col++;
        button(c,new RectF(x+col*bw,y,x+(col+1)*bw-8*k,y+bh),tr("Terror-Zone: ","Terror Zone: ","Zona de Terror: ")+(terror?tr("an","on","ligado"):tr("aus","off","desligado")),()->{terror=!terror;if(!terror)herald=0;change();},k);col++;
        if(showHerald){button(c,new RectF(x+col*bw,y,x+(col+1)*bw-8*k,y+bh),CombatModel.heraldLabel(herald,lang()),()->{herald=(herald+1)%6;if(herald>0)terror=true;change();},k);col++;}
        button(c,new RectF(x+col*bw,y,right,y+bh),CombatModel.rotationLabel(ordered,lang()),()->{ordered=!ordered;change();},k);y+=bh+28*k;
        if(m==null && selected>=0){backButton(c,x,right,y,k);y+=56*k;}
        if(m==null){text(c,body,failed?tr("Nicht verfügbar: Charakter oder Simulationsdaten fehlen.","Unavailable: character or simulation data missing.","Indisponível: faltam personagem ou dados da simulação."):tr("Berechnung …","Calculating …","Calculando …"),x,y+40*k,right-x,k,"FontColorLightGray");}
        else if(selected>=0)drawDetail(c,m,x,right,y,bottom,k);
        else if(tab==0)drawLanding(c,m,x,right,y,bottom,k);
        else if(tab==1)drawTable(c,m,x,right,y,bottom,k);
        else drawRotation(c,m,x,right,y,bottom,k);
        drawFoot(c,p,x,right,k);
        } finally {if(BuildConfig.DM_QA_BUILD)TestHooks.end(this);}
    }

    private void drawFoot(Canvas c,GameStyleGeometry.Panel p,float x,float right,float k){
        CombatModel m=tab==TAB_LOG || tab==TAB_BEST?null:selected<0?model:detail;
        if(tab==TAB_BEST){pages=bestPlan.size();page=Math.min(page,pages-1);}
        if(pages>1){
            style.footButton(c,p,getWidth(),0,GameStyle.BUTTON_NORMAL,"‹",-1);p.footHit(getWidth(),0,box);hit(new RectF(box[0],box[1],box[2],box[3]),()->flip(-1));
            style.footText(c,p,getWidth(),1,(page+1)+" / "+pages,"FontColorLightGray");
            style.footButton(c,p,getWidth(),2,GameStyle.BUTTON_NORMAL,"›",1);p.footHit(getWidth(),2,box);hit(new RectF(box[0],box[1],box[2],box[3]),()->flip(1));
        }
        if(padUsed) {
            text(c,note,tr("LB/RB Reiter · Y Suche · A Öffnen · B Zurück · LT/RT Seiten","LB/RB Tabs · Y Search · A Open · B Back · LT/RT Pages","LB/RB Abas · Y Procurar · A Abrir · B Voltar · LT/RT Páginas"),x,p.footT-8*k,right-x,k,"FontColorGoldYellow");
            if((m!=null || (tab==TAB_LOG && log!=null) || tab==TAB_BEST) && !boxes.isEmpty())focus=Math.max(0,Math.min(focus,boxes.size()-1));

            if(focus<boxes.size()){focusBox.set(boxes.get(focus));if(buttons.get(focus))focusBox.inset(-6*k,-6*k);art.rim(c,"build_row",focusBox,8,k,picture);}
        }
    }
    private void prepare(){
        if(de!=GameLang.german()){de=GameLang.german();style.refresh();}
        chooseNameLanguage();
        bosses=new ArrayList<>();typical=new ArrayList<>();minis=new ArrayList<>();nextBoss=null;
        if(model!=null){
            tableRows=model.filter(query,act,type,sort,descending,this::name);
            for(CombatModel.Enemy e:model.enemies)if(e.kind==CombatModel.ACT_BOSS){if(e.act==model.act)bosses.add(e);if(e.act==model.nextBoss && nextBoss==null)nextBoss=e;}
            minis=model.miniBosses();
            for(CombatModel.Enemy e:model.enemies)if(e.act==model.act && e.kind==CombatModel.REGULAR){boolean seen=false;for(CombatModel.Enemy a:typical)seen|=a.key.equals(e.key);if(!seen && typical.size()<5)typical.add(e);}
            if(!ordered || order.isEmpty()){order.clear();order.addAll(model.rotationOrder());}
        }
        detailLines=detail==null?new ArrayList<>():detailText(detail);
        wrappedLines=new ArrayList<>();
        graph=null;header=null;titles.clear();
        if(tab==TAB_LOG){prepareLog();return;}
        if(tab==TAB_BEST){prepareBest();return;}
        if(getWidth()>0 && getHeight()>0){
            GameStyleGeometry.Panel panel=style.panel(getWidth(),getHeight());float scale=getHeight()/1080f;prepareGraph(panel,scale);
            float left=panel.contentL+24*panel.s,right=panel.contentR-24*panel.s,width=(graph!=null?graphBox.left-24*scale:right)-left;
            if(detail!=null && !detail.enemies.isEmpty()){CombatModel.Enemy e=detail.enemies.get(0);
                header=title(shownName(e.key),CombatModel.light(e.light,lang()),CombatLayout.titleRight(left,right,scale,graph!=null)-CombatLayout.titleLeft(left,right,scale),scale,scale,true);}
            for(String line:detailLines.subList(Math.min(1,detailLines.size()),detailLines.size()))wrappedLines.addAll(EffectLayout.wrap(line,width,s->body.measure(s,scale)));
        }
    }
    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){super.onSizeChanged(w,h,oldw,oldh);prepare();}

    private static final int MINI_ROWS=2;
    private void drawLanding(Canvas c,CombatModel m,float x,float right,float y,float bottom,float k){
        text(c,body,rotation(m),x,y+20*k,right-x,k,"FontColorWhite");y+=55*k;
        text(c,note,combatSummary(m),x,y,right-x,k,"FontColorLightGray");y+=14*k;
        if(!bosses.isEmpty()) {
            CombatModel.Enemy boss=bosses.get(0);
            boolean same=nextBoss!=null && nextBoss.id==boss.id;
            float mid=same?right:(x+right)/2,height=bannerHeight(k);
            String missing=m.nextBoss==0?tr("Alle besiegt","All defeated","Todos derrotados"):tr("Nicht verfügbar","Unavailable","Indisponível");
            banner(c,nextBoss,tr("Nächster Hauptboss","Next main boss","Próximo chefe principal"),missing,new RectF(x,y,same?right:mid-10*k,y+height),k);
            if(!same)banner(c,boss,tr("Aktboss","Act boss","Chefe do ato"),missing,new RectF(mid+10*k,y,right,y+height),k);
            y+=height+12*k;
        }
        float card=body.size(k)*3.1f;
        if(!minis.isEmpty()) {
            y+=note.size(k)*1.1f;text(c,note,tr("Minibosse","Mini bosses","Minichefes"),x,y,right-x,k,"FontColorGoldYellow");y+=10*k;
            float half=(right-x-12*k)/2;
            for(int i=0;i<Math.min(minis.size(),2*MINI_ROWS);i++){float left=x+(i%2)*(half+12*k),top=y+(i/2)*(card+8*k);mini(c,minis.get(i),new RectF(left,top,left+half,top+card),k);}
            y+=((Math.min(minis.size(),2*MINI_ROWS)+1)/2)*(card+8*k)+6*k;
        }
        int shown=Math.min(typical.size(),(int)((bottom-y)/(card+8*k)));
        float pitch=shown>0?Math.min(94*k,(bottom-y)/shown):0;
        for(int i=0;i<shown;i++){enemy(c,typical.get(i),new RectF(x,y,right,y+Math.max(card,pitch-8*k)),k);y+=pitch;}pages=1;page=0;
    }
    private void drawTable(Canvas c,CombatModel m,float x,float right,float y,float bottom,float k){
        float third=(right-x)/3;
        RectF search=new RectF(x,y,x+third,y+48*k);
        art.nine(c,"book_input_c",search,8,k,picture);text(c,body,query.isEmpty()?tr("Gegner suchen ...","Search enemies ...","Procurar inimigos ..."):query,x+12*k,y+34*k,third-24*k,k,"FontColorWhite");hit(search,1,this::search);
        button(c,new RectF(x+third+8*k,y,x+2*third-8*k,y+48*k),act==0?tr("Alle Akte","All acts","Todos os atos"):tr("Akt ","Act ","Ato ")+act,()->{act=(act+1)%6;page=0;prepare();invalidate();},k);
        button(c,new RectF(x+2*third,y,right,y+48*k),type<0?tr("Alle Typen","All types","Todos os tipos"):type==0?tr("Regulär","Regular","Comum"):type==CombatModel.SUPERUNIQUE?tr("Minibosse","Mini bosses","Minichefes"):tr("Bosse / Ubers","Bosses / Ubers","Chefes / Ubers"),()->{type=type<0?CombatModel.REGULAR:type==CombatModel.REGULAR?CombatModel.SUPERUNIQUE:type==CombatModel.SUPERUNIQUE?CombatModel.ACT_BOSS:-1;page=0;prepare();invalidate();},k);y+=64*k;
        String[] columns={tr("Akt / Gebiet / Name","Act / Area / Name","Ato / Área / Nome"),tr("Kill-Zeit","Kill time","Tempo de abate"),tr("Zeit bis Tod","Time to death","Tempo até a morte"),tr("Bewertung","Rating","Avaliação")};
        for(int i=0;i<4;i++){final int s=i;float left=CombatLayout.column(x,right,i);RectF r=new RectF(left,y,CombatLayout.column(x,right,i+1),y+38*k);text(c,note,columns[i]+(sort==i && arrows?(descending?" ↓":" ↑"):""),left,y+26*k,r.width(),k,"FontColorGoldYellow");hit(r,()->{descending=sort==s?!descending:false;sort=s;page=0;prepare();invalidate();});}y+=45*k;
        List<CombatModel.Enemy> all=tableRows;
        int count=CombatLayout.rows(y,bottom,102*k);pages=CombatModel.pages(all.size(),count);page=Math.min(page,pages-1);
        for(int i=page*count;i<Math.min(all.size(),(page+1)*count);i++){CombatModel.Enemy e=all.get(i);enemy(c,e,new RectF(x,y,right,y+94*k),k);y+=102*k;}
        if(all.isEmpty())text(c,body,tr("Keine Treffer","No results","Nenhum resultado"),x,y+35*k,right-x,k,"FontColorLightGray");
    }
    private List<String> detailText(CombatModel m){
        List<String> lines=new ArrayList<>();
        if(!m.enemies.isEmpty()) {
            CombatModel.Enemy e=m.enemies.get(0);lines.add(shownName(e.key)+" · "+CombatModel.light(e.light,lang()));
            lines.add(tr("Stufe ","Level ","Nível ")+e.level+" · "+e.life+tr(" Leben · "," life · "," vida · ")+location(e)+(immunity(e).isEmpty()?"":tr(" · Immun: "," · Immune: "," · Imune: ")+immunity(e)));
            lines.add(tr("Kill ","Kill ","Abate ")+CombatModel.time(e.kill,lang())+tr(" · Tod "," · Death "," · Morte ")+CombatModel.time(e.death,lang()));
            if(e.death==-2)lines.add(tr("Überleben: nicht verfügbar","Survival: unavailable","Sobrevivência: indisponível"));
            else if(e.hits>=0)lines.add(tr("Überleben: ","Survival: ","Sobrevivência: ")+Long.toString((long)Math.ceil(e.hits))+tr(" Treffer · größter Treffer "," hits · largest hit "," golpes · maior golpe ")+maxHit(e)+" "+tr("Leben","life","vida"));
            if(!ratingNote(e).isEmpty())lines.add(ratingNote(e));
            if(e.kind==9)lines.add(tr("Diablo Clone: Referenzgegner","Diablo Clone: reference opponent","Clone de Diablo: oponente de referência"));
        }
        lines.add(rotation(m));lines.add(combatSummary(m));
        for(CombatModel.Skill s:m.skills)if(CombatModel.damageSkill(s) && s.minion())lines.add(CombatModel.minionLine(skillName(s.id),s.pets,s.dps,lang()));
        else if(CombatModel.damageSkill(s) && !(m.sequence.size()==1 && m.sequence.get(0)==s.id))lines.add(skillName(s.id)+": "+number(s.dps)+" DPS"+(CombatModel.displayedPositive(s.mana)?" · "+number(s.mana)+" Mana · "+CombatModel.manaSupply(s.manaSeconds,lang()):""));
        boolean survival=m.enemies.isEmpty() || m.enemies.get(0).death!=-2;
        if(survival && CombatModel.displayedPositive(m.incoming))lines.add(tr("Eingehend: ","Incoming: ","Recebido: ")+number(m.incoming)+tr(" Schaden/s"," damage/s"," dano/s")+(CombatModel.displayedPositive(m.sustain)?tr(" · Heilung "," · Healing "," · Cura ")+number(m.sustain)+"/s":""));
        if(!survival)lines.add(tr("Eingehender Schaden: nicht verfügbar","Incoming damage: unavailable","Dano recebido: indisponível"));

        String[] elements={tr("Physisch","Physical","Físico"),tr("Feuer","Fire","Fogo"),tr("Blitz","Lightning","Raio"),tr("Kälte","Cold","Frio"),tr("Gift","Poison","Veneno"),tr("Magie","Magic","Magia")};
        List<String> split=new ArrayList<>();for(int i=0;survival && i<6;i++)if(CombatModel.displayedPositive(m.taken[i]))split.add(elements[i]+": "+number(m.taken[i])+"/s");
        if(split.size()>=2 && CombatModel.displayedPositive(m.incoming)){lines.add(tr("Eingehender Schaden nach Art","Incoming damage by type","Dano recebido por tipo"));lines.addAll(split);}
        if(m.itemCharges)lines.add(tr("Ladungen: Verbrauch nicht modelliert.","Item charges: consumption not modeled.","Cargas: consumo não modelado."));
        if(potions && (m.belt!=null || m.beltUnreadable))lines.add(belt(m));
        if(terror || herald>0)lines.add(tr("Terror / Herold: Stufen- und Basisboni; Zusatzmodifikatoren fehlen.","Terror / Herald: level and base bonuses; extra modifiers omitted.","Terror / Arauto: bônus de nível e base; modificadores extras omitidos."));
        if(m.rift!=0 || m.rush!=0)lines.add("Rift: "+m.rift+" · Boss Rush: "+m.rush+tr(" · Sonderregeln nicht berechnet."," · special rules not computed."," · regras especiais não calculadas."));
        return lines;
    }

    private String potionName(int group){return shownName("@potion:"+group);}
    private String belt(CombatModel m){
        if(m.beltUnreadable)return tr("Gürtel: nicht lesbar","Belt: unreadable","Cinto: ilegível");
        String text=CombatGraph.belt(m.belt,new String[]{potionName(0),potionName(1),potionName(2)});
        return text.isEmpty()?tr("Gürtel: leer","Belt: empty","Cinto: vazio"):tr("Gürtel: ","Belt: ","Cinto: ")+text;
    }

    private float detailTop(GameStyleGeometry.Panel p,float k){return p.contentT+40*p.s+42*k+48*k+28*k;}

    private void prepareGraph(GameStyleGeometry.Panel p,float k){
        graph=null;
        if(detail==null || detail.fight==null || detail.enemies.isEmpty())return;
        CombatModel.Enemy e=detail.enemies.get(0);
        float x=p.contentL+24*p.s,right=p.contentR-24*p.s;
        graphBox.set(CombatLayout.graphLeft(x,right),detailTop(p,k),right,p.contentB-20*p.s);
        graphColours=new int[]{style.colour("FontColorPartyGreen"),style.colour("FontColorRed"),style.colour("FontColorBlue"),style.colour("FontColorGoldYellow")};
        gold=graphColours[3];
        CombatGraph.Words words=words();
        int kind=detail.fight.endKind;
        String end=kind==CombatModel.END_KILL?tr("Kill ","Kill ","Abate ")+CombatModel.time(e.kill,lang()):kind==CombatModel.END_DEATH?tr("Tod ","Death ","Morte ")+CombatModel.time(e.death,lang()):"";
        CombatGraph g=CombatGraph.build(detail.fight,graphBox.left,graphBox.top,graphBox.right,graphBox.bottom,k,note.size(k),s->note.measure(s,k),words,
            shownName(e.key),tr("Dein Leben","Your life","Sua vida"),end,lang()!=CombatModel.L_EN,graphColours);
        pictures(g);
        graph=g;
    }
    private CombatGraph.Words words(){
        return new CombatGraph.Words(){
            public String skill(int id){return skillName(id);}
            public boolean icon(int id){return art.image("builds/skills/"+id)!=null;}
            public String shortage(String skill){return tr("Mana reicht nicht für ","Not enough mana for ","Mana insuficiente para ")+skill;}
            public String potion(int group){return potionName(group);}
        };
    }
    private void pictures(CombatGraph g){
        path(enemyLine,g.enemy);path(lifeLine,g.life);path(manaLine,g.mana);
        legendIcons=new Bitmap[g.legend.size()];for(int i=0;i<legendIcons.length;i++)legendIcons[i]=g.legend.get(i).icon==null?null:art.image(g.legend.get(i).icon);
        markerIcons=new Bitmap[g.markers.size()];for(int i=0;i<markerIcons.length;i++)markerIcons[i]=art.image(g.markers.get(i).icon);
        groupLine.rewind();if(g.group!=null)path(groupLine,g.group);
        starPath.rewind();for(float[] p:g.stars){for(int i=0;i+1<p.length;i+=2){if(i==0)starPath.moveTo(p[i],p[i+1]);else starPath.lineTo(p[i],p[i+1]);}starPath.close();}
    }



    private float logTop(GameStyleGeometry.Panel p,float k){return p.contentT+40*p.s+42*k;}
    private float logSplit(float x,float right){return x+(right-x)*.42f;}
    private String logName(String key){
        if(key.equals("@others"))return tr("weitere","others","outros");
        return key.equals("-") || key.isEmpty()?tr("Unbekannt","Unknown","Desconhecido"):shownName(key);
    }

    private String difficultyWord(int d){JSONArray a=difficulties.optJSONArray(d);return a==null?"":a.optString(de?0:1);}
    private String rankColour(FightLog.Group g){return g==null?"FontColorWhite":g.elite()?"FontColorGold":g.rank==FightLog.RANK_CHAMPION?"FontColorBlue":"FontColorWhite";}

    private String stateName(int id){
        JSONObject o=states.optJSONObject(Integer.toString(id));if(o==null)return null;
        String kind=o.optString("kind");if(!kind.equals("buff") && !kind.equals("aura") && !kind.equals("shrine"))return null;
        String n=o.optString(de?"de":"en");return n.isEmpty()?null:n;
    }
    private void requestLog(){
        if(!isOpen())return;
        int token=++logEpoch;logBusy=true;qa();
        worker.execute(()->{
            FightLog l=null;StreakLog k=null;DpsLog v=null;
            try {byte[] b=DmActivity.nativeFightLog(-1,-1,0,0);if(b!=null)l=FightLog.parse(new String(b,StandardCharsets.UTF_8));}catch(RuntimeException e){AppArt.log("fightlog.response: "+e);}
            try {byte[] b=DmActivity.nativeStreaks();if(b!=null)k=StreakLog.parse(new String(b,StandardCharsets.UTF_8));}catch(RuntimeException e){AppArt.log("streaks.response: "+e);}
            try {byte[] b=DmActivity.nativeFightLog(-2,-1,0,0);if(b!=null)v=DpsLog.parse(new String(b,StandardCharsets.UTF_8));}catch(RuntimeException e){AppArt.log("dps.response: "+e);}
            final FightLog ready=l;final StreakLog kept=k;final DpsLog view=v;
            post(()->{if(token!=logEpoch)return;logBusy=false;logFailed=ready==null;log=ready;logDetail=null;logFight=-1;logGroup=-1;
                streaks=kept;if(streaks==null || !streaks.any())streakView=false;
                dps=view;if(!dpsShown())dpsView=false;dpsSel=-1;
                if(log!=null && !log.entries.isEmpty())openFight(0,-1);else{prepare();qa();invalidate();}});
        });
    }

    private void openFight(int index,int group){
        if(log==null || index<0 || index>=log.entries.size())return;
        FightLog.Entry e=log.entries.get(index);int asked=simGroup(e,group);
        logGroup=group;
        if(index==logFight && logDetail!=null && asked==logAsked){prepare();qa();invalidate();return;}
        logFight=index;logDetail=null;logAsked=asked;
        int token=++logEpoch;logBusy=true;prepare();qa();invalidate();
        worker.execute(()->{
            FightLog.Detail d=null;
            try {byte[] b=DmActivity.nativeFightLog(e.index,asked,e.key,1);if(b!=null)d=FightLog.detail(new String(b,StandardCharsets.UTF_8));}catch(RuntimeException x){AppArt.log("fightlog.fight: "+x);}
            final FightLog.Detail ready=d;
            post(()->{if(token!=logEpoch)return;logBusy=false;
                if(ready==null){requestLog();return;}
                logDetail=ready;prepare();qa();invalidate();});
        });
    }

    private static int simGroup(FightLog.Entry e,int group){return group>=0 && group<e.groups.size() && e.groups.get(group).sim?group:-1;}


    private String ended(FightLog.Entry e){String p=e.partial();return p!=null?p:tr("abgebrochen","aborted","interrompido");}
    private boolean simulated(FightLog.Entry e){return logDetail!=null && logDetail.sim!=null && e.duration>0 && logDetail.simGroup==simGroup(e,logGroup);}
    private void prepareLog(){
        if(streakView){prepareStreaks();return;}
        if(dpsView){prepareDps();return;}
        logRows=log==null?new ArrayList<>():log.rows(logSort,logDescending,opened,this::logName);
        logCells=new ArrayList<>();for(FightLog.Row r:logRows)logCells.add(cells(r));
        logFits=new ArrayList<>();
        if(getWidth()>0 && getHeight()>0){
            GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());float k=getHeight()/1080f,x=p.contentL+24*p.s,split=logSplit(x,p.contentR-24*p.s),pad=10*k;
            for(int r=0;r<logRows.size();r++){
                String[] cell=logCells.get(r);Fit[] f=new Fit[4];float indent=logRows.get(r).group==null?0:36*k;
                for(int i=0;i<4;i++){float a=x+(split-x)*LOG_COLS[i],b=x+(split-x)*LOG_COLS[i+1],in=i==0?indent:0;if(!cell[i].isEmpty())f[i]=fit(body,cell[i],b-a-2*pad-in,k);}
                logFits.add(f);
            }
        }
        logTitle=logLine=logNote="";
        if(log==null || logFight<0 || logFight>=log.entries.size())return;
        FightLog.Entry e=log.entries.get(logFight);
        logTitle=FightLog.label(e,this::logName)+(e.area>0?" · "+shownName("@area:"+e.area):"")+(difficultyWord(e.difficulty).isEmpty()?"":" · "+difficultyWord(e.difficulty))+(e.aborted()?" · "+ended(e):"");
        boolean sim=simShown && simulated(e);
        FightLog.Group one=sim && logDetail.simGroup>=0?e.groups.get(logDetail.simGroup):null;
        List<String> parts=new ArrayList<>();

        if(one!=null){double dps=one.own(e.lead+e.duration,e.known);if(dps>=0)parts.add(FightLog.label(one,this::logName)+": "+(e.known?tr("Du ","You ","Você "):"")+number(dps)+" DPS");}
        else if(e.known){if(e.own()>0)parts.add(tr("Du ","You ","Você ")+number(e.own())+" DPS");if(e.companionDps()>0)parts.add(tr("Begleiter ","Companions ","Companheiros ")+number(e.companionDps()));}
        else if(e.dps()>=0)parts.add(number(e.dps())+" DPS");
        if(sim && logDetail.simDps>=0)parts.add(tr("Simulation ","Simulation ","Simulação ")+number(logDetail.simDps)+(e.known?"":" DPS"));
        if(one==null && !e.known && e.minionShare()>0)parts.add(tr("Söldner und Beschwörungen ","Mercenary and summons ","Mercenário e invocações ")+e.minionShare()+" %");
        logLine=String.join(" · ",parts);
        int rest=e.enemies-1;
        logNote=one==null?"":tr("mit "+rest+(rest==1?" weiteren Gegner":" weiteren Gegnern")+" im Kampf","with "+rest+(rest==1?" other enemy":" other enemies")+" in the fight","com "+rest+(rest==1?" outro inimigo":" outros inimigos")+" no combate");
        if(logDetail==null || getWidth()<=0 || getHeight()<=0)return;
        GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());float k=getHeight()/1080f,x=p.contentL+24*p.s,right=p.contentR-24*p.s,top=logTop(p,k);
        graphBox.set(logSplit(x,right)+28*k,top+(logNote.isEmpty()?92:128)*k,right,p.contentB-20*p.s);
        graphColours=new int[]{style.colour("FontColorPartyGreen"),style.colour("FontColorRed"),style.colour("FontColorBlue"),style.colour("FontColorGoldYellow")};
        gold=graphColours[3];
        FightLog.Detail d=logDetail;
        CombatGraph.Extra xx=new CombatGraph.Extra();
        xx.enemy=d.enemy;xx.own=d.own;xx.zero=e.lead;
        FightLog.Group lead=e.lead();
        xx.ownLane=tr("Du","You","Você");xx.enemyLane=lead==null?tr("Gegner","Enemy","Inimigo"):logName(lead.key);xx.enemyLaneShort=tr("Gegner","Enemy","Inimigo");
        xx.hit=tr("Treffer","hit","acerto");xx.miss=tr("verfehlt","missed","errou");xx.dodge=tr("ausgewichen","dodged","esquivou");xx.block=tr("geblockt","blocked","bloqueado");xx.crit=tr("Krit","Crit","Crítico");
        xx.hitColour=graphColours[1];xx.missColour=style.colour("FontColorGray");xx.dodgeColour=style.colour("FontColorWhite");xx.blockColour=style.colour("FontColorLightBlue");xx.starColour=gold;xx.simColour=style.colour("FontColorVeryLightGray");
        for(FightLog.Band b:FightLog.bands(d,id->stateName(id)!=null,3)){
            String when=b.began && b.ended?CombatGraph.seconds((b.from-e.lead)/25.0,lang()!=CombatModel.L_EN)+" – "+CombatGraph.seconds((b.to-e.lead)/25.0,lang()!=CombatModel.L_EN)
                :b.ended?tr("bis ","until ","até ")+CombatGraph.seconds((b.to-e.lead)/25.0,lang()!=CombatModel.L_EN):tr("ab ","from ","desde ")+CombatGraph.seconds((b.from-e.lead)/25.0,lang()!=CombatModel.L_EN);
            xx.bandText.add(stateName(b.id)+" · "+when);xx.bandSpan.add(new int[]{b.from,b.to});
        }
        if(logGroup>=0 && d.groups!=null && logGroup<d.groups.length && logGroup<e.groups.size()){xx.group=d.groups[logGroup];xx.groupLabel=FightLog.label(e.groups.get(logGroup),this::logName);}
        if(sim){xx.sim=d.sim;xx.simOffset=d.simOffset;xx.simLabel=tr("Simulation","Simulation","Simulação");xx.simLife=one==null;}
        String end=e.end==FightLog.END_KILL?tr("Kill ","Kill ","Abate ")+FightLog.time(e.seconds(),lang()):e.end==FightLog.END_DEATH?tr("Tod ","Death ","Morte ")+FightLog.time(e.seconds(),lang()):"";
        CombatGraph g=CombatGraph.build(d.fight,graphBox.left,graphBox.top,graphBox.right,graphBox.bottom,k,note.size(k),s->note.measure(s,k),words(),
            FightLog.label(e,this::logName),tr("Dein Leben","Your life","Sua vida"),end,lang()!=CombatModel.L_EN,graphColours,xx);
        pictures(g);
        graph=g;
    }
    private void drawLog(Canvas c,GameStyleGeometry.Panel p,float x,float right,float y,float bottom,float k){
        CombatModel shown=model!=null?model:last;
        String count=log==null?"":log.count==1?tr("1 Kampf","1 fight","1 combate"):tr("die letzten ","the last ","os últimos ")+log.count+tr(" Kämpfe"," fights"," combates");
        String head=shown==null?count:shown.name+" · "+tr("Stufe ","Level ","Nível ")+shown.level+(count.isEmpty()?"":" · "+count);
        float top=logTop(p,k),split=logSplit(x,right);pages=1;
        if(streakView){drawStreaks(c,x,right,y,top,bottom,k);return;}
        if(dpsView){drawDps(c,x,right,y,top,bottom,k);return;}
        if(!head.isEmpty())text(c,body,head,x,y,right-x,k,"FontColorWhite");
        if(log!=null && !log.recording && log.count>0){note.colour("FontColorLightGray");note.draw(c,tr("Aufzeichnung aus","Recording off","Gravação desligada"),right,y,k,Paint.Align.RIGHT,255);}
        float lt=viewSwitch(c,x,split,top,k);
        if(log==null){text(c,body,logFailed?tr("Nicht verfügbar","Unavailable","Indisponível"):tr("Laden …","Loading …","Carregando …"),x,lt+40*k,right-x,k,"FontColorLightGray");return;}
        if(log.entries.isEmpty()){text(c,body,log.recording?tr("Noch keine Kämpfe aufgezeichnet.","No fights recorded yet.","Nenhum combate gravado ainda."):tr("Aufzeichnung aus: einschalten unter Extras.","Recording off: switch it on under Custom.","Gravação desligada: ative em Extras."),x,lt+40*k,right-x,k,"FontColorLightGray");return;}
        final float[] cols=LOG_COLS;
        String[] heads={tr("Gegner","Enemy","Inimigo"),"Kill","DPS",tr("Min. Leben","Min. life","Vida mín.")};
        for(int i=0;i<4;i++){
            final int col=i;float l=x+(split-x)*cols[i],r=x+(split-x)*cols[i+1];
            text(c,note,heads[i]+(logSort==i && arrows?(logDescending?" ↓":" ↑"):""),l+10*k,lt+26*k,r-l-10*k,k,logSort==i?"FontColorGoldYellow":"FontColorLightGray");
            hit(new RectF(l,lt-4*k,r,lt+38*k),()->{if(logSort!=col){logSort=col;logDescending=false;}else if(!logDescending)logDescending=true;else{logSort=FightLog.SORT_NONE;logDescending=false;}page=0;prepare();qa();invalidate();});
        }
        float ry=lt+45*k,pitch=46*k;
        int count2=CombatLayout.rows(ry,bottom,pitch);pages=CombatModel.pages(logRows.size(),count2);page=Math.min(page,pages-1);
        for(int i=page*count2;i<Math.min(logRows.size(),(page+1)*count2);i++){logRow(c,logRows.get(i),logCells.get(i),i<logFits.size()?logFits.get(i):null,x,split,ry,pitch,cols,k);ry+=pitch;}
        stroke.setStyle(Paint.Style.STROKE);stroke.setStrokeWidth(k);stroke.setColor(alpha(style.colour("FontColorGoldYellow"),70));c.drawLine(split+14*k,top,split+14*k,bottom,stroke);
        if(logFight<0 || logFight>=log.entries.size())return;
        FightLog.Entry e=log.entries.get(logFight);float dl=split+28*k,bw=(right-dl)*.3f;
        boolean toggle=simulated(e);
        text(c,body,logTitle,dl,top+26*k,right-dl-(toggle?bw+16*k:0),k,e.end==FightLog.END_DEATH?"FontColorRed":"FontColorWhite");
        if(toggle)button(c,new RectF(right-bw,top-8*k,right,top+40*k),tr("Simulation: ","Simulation: ","Simulação: ")+(simShown?tr("an","on","ligada"):tr("aus","off","desligada")),()->{simShown=!simShown;prepare();qa();invalidate();},k);
        if(!logLine.isEmpty())text(c,body,logLine,dl,top+72*k,right-dl,k,"FontColorWhite");
        if(!logNote.isEmpty())text(c,body,logNote,dl,top+110*k,right-dl,k,"FontColorGray");
        if(graph!=null)drawGraph(c,k);
        else text(c,body,tr("Laden …","Loading …","Carregando …"),dl,top+130*k,right-dl,k,"FontColorLightGray");
    }

    private static final float[] DPS_COLS={0f,.36f,.52f,.70f,1f};
    private static final int DPS_SEGS=12;
    private String range(double lo,double hi){return Math.round(lo)+" – "+Math.round(hi);}
    private String sourceName(DpsLog.Source s){
        switch(s.kind){
        case 's':return skillName(s.id);
        case 'c':return s.key.equals("-")?tr("Begleiter","Companion","Companheiro"):logName(s.key);
        case 'm':return tr("Söldner","Mercenary","Mercenário");
        case 'r':return tr("weitere Begleiter","other companions","outros companheiros");
        case 'u':return tr("unbekannt (Gift, Brand)","unknown (poison, burning)","desconhecido (veneno, queimadura)");
        case 'n':return tr("ohne Fertigkeit","without a skill","sem habilidade");
        default:return tr("weitere Fertigkeiten","other skills","outras habilidades");
        }
    }

    private String changeWords(DpsLog.Segment g,boolean axis){
        List<String> w=new ArrayList<>();
        if((g.changed&DpsLog.CHANGE_LEVEL)!=0)w.add(axis?Integer.toString(g.level):tr("Stufe ","Level ","Nível ")+g.level);
        if((g.changed&DpsLog.CHANGE_POINTS)!=0)w.add(tr("Fertigkeit","Skill","Habilidade"));
        if((g.changed&DpsLog.CHANGE_ATTRS)!=0)w.add(tr("Attribute","Attributes","Atributos"));
        if((g.changed&DpsLog.CHANGE_GEAR)!=0)w.add(tr("Ausrüstung","Gear","Equipamento"));
        if((g.changed&DpsLog.CHANGE_DIFFICULTY)!=0 && !difficultyWord(g.difficulty).isEmpty())w.add(difficultyWord(g.difficulty));
        return String.join(" · ",w);
    }
    private String fightTime(double seconds){long t=Math.round(seconds);return t>=60?t/60+":"+String.format(java.util.Locale.ROOT,"%02d",t%60)+" min":t+" s";}
    private void prepareDps(){
        dpsCells.clear();dpsFits.clear();dpsRowY.clear();dpsMarks.clear();dpsTicks.clear();dpsMarkPaths.clear();dpsMarkHits.clear();dpsHead=dpsLine=dpsCurveTitle=dpsSelLine=null;dpsCurve=null;dpsOlder=false;
        if(dps==null || getWidth()<=0 || getHeight()<=0)return;
        GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());float k=getHeight()/1080f,x=p.contentL+24*p.s,right=p.contentR-24*p.s,pad=10*k,bottom=p.contentB-20*p.s;
        CombatModel shown=model!=null?model:last;
        if(shown!=null)dpsHead=fit(body,shown.name+" · "+tr("Stufe ","Level ","Nível ")+shown.level,right-x,k);
        float top=logTop(p,k)+((streaks!=null && streaks.any()) || dpsShown()?56*k:0),y=top+26*k,pitch=40*k;
        dpsLineY=y;
        if(!dps.enough()){dpsLineColour="FontColorGray";dpsLine=fit(body,tr("Weniger als 3 Kämpfe mit diesem Stand","Fewer than 3 fights with this state","Menos de 3 combates com este estado"),right-x,k);}
        else{dpsLineColour="FontColorWhite";dpsLine=fit(body,tr("Die letzten "+dps.fights+" Kämpfe mit diesem Stand","The last "+dps.fights+" fights with this state","Os últimos "+dps.fights+" combates com este estado")+" · "+fightTime(dps.seconds())+" "+tr("Kampfzeit","fight time","tempo de combate"),right-x,k);}
        y+=48*k;
        if(dps.enough()){
            List<String[]> rows=new ArrayList<>();
            rows.add(new String[]{"","DPS",dps.range()?tr("Spanne","Range","Faixa"):"","","FontColorLightGray","FontColorLightGray","FontColorLightGray","FontColorLightGray"});
            rows.add(new String[]{tr("Du","You","Você"),number(dps.own),dps.range()?range(dps.lo,dps.hi):"","","FontColorWhite","FontColorWhite","FontColorWhite","FontColorWhite"});
            if(dps.companions())rows.add(new String[]{tr("mit Begleitern","with companions","com companheiros"),number(dps.team),dps.range() && dps.teamHi>0?range(dps.teamLo,dps.teamHi):"","","FontColorWhite","FontColorWhite","FontColorWhite","FontColorWhite"});
            if(dps.sim)rows.add(new String[]{tr("Simulation","Simulation","Simulação"),number(dps.simDps),tr("erreicht ","reached ","atingido ")+Math.round(dps.reached)+" %",
                "("+dps.simFights+" "+(dps.simFights==1?tr("Einzelkampf","single fight","combate individual"):tr("Einzelkämpfe","single fights","combates individuais"))+")","FontColorGray","FontColorGray","FontColorGray","FontColorGray"});
            if(dps.table()){
                rows.add(null);
                rows.add(new String[]{tr("Quelle","Source","Fonte"),"DPS",tr("Spanne","Range","Faixa"),tr("Anteil","Share","Parte"),"FontColorLightGray","FontColorLightGray","FontColorLightGray","FontColorLightGray"});
                for(DpsLog.Source sr:dps.sources){String c=sr.kind=='u'?"FontColorGray":"FontColorWhite";rows.add(new String[]{sourceName(sr),number(sr.dps),sr.range()?range(sr.lo,sr.hi):"",Math.round(sr.share)+" %",c,c,c,c});}
            }
            float limit=bottom-(dps.segments.size()>=2?330*k:0);
            for(String[] r:rows){
                if(r==null){y+=pitch*.5f;continue;}
                if(y>limit)break;
                Fit[] f=new Fit[4];
                for(int i=0;i<4;i++){float a=x+(right-x)*DPS_COLS[i],b=x+(right-x)*DPS_COLS[i+1];if(!r[i].isEmpty())f[i]=fit(i==0 || r[4].equals("FontColorLightGray")?note:body,r[i],b-a-2*pad,k);}
                dpsCells.add(r);dpsFits.add(f);dpsRowY.add(y);y+=pitch;
            }
        }
        if(dps.segments.size()<2)return;
        int last=dps.segments.size()-1;if(dpsSel<0 || dpsSel>last)dpsSel=last;
        int to=Math.min(last,Math.max(dpsSel,dpsFrom+DPS_SEGS-1)),from=Math.max(0,to-DPS_SEGS+1);if(dpsSel<from){from=dpsSel;to=Math.min(last,from+DPS_SEGS-1);}
        dpsFrom=from;dpsOlder=from>0;
        dpsTitleY=Math.max(y+10*k,bottom-300*k);
        dpsCurveTitle=fit(note,tr("Verlauf · deine DPS","History · your DPS","Histórico · seu DPS"),right-x,k);
        float gl=x+80*k,gt=dpsTitleY+20*k,gb=bottom-110*k;
        dpsBox.set(gl,gt,right,gb);
        dpsCurve=dps.curve(from,to,gl,gt,right,gb);
        for(float[] t:dpsCurve.ticks)dpsTicks.add(fit(note,number(t[1]).replaceAll("[,.]0$",""),70*k,k));
        for(int i=from;i<=to;i++){
            int j=i-from;String w=i==from?"":changeWords(dps.segments.get(i),true);dpsMarks.add(w.isEmpty()?null:fit(note,w,Math.max(60*k,(right-gl)/(to-from+1)*1.6f),k*.85f));
            float mx=j==0?dpsCurve.stepX0[0]:dpsCurve.markX[j];
            Path m=new Path();m.moveTo(mx,gb+2*k);m.lineTo(mx-9*k,gb+18*k);m.lineTo(mx+9*k,gb+18*k);m.close();dpsMarkPaths.add(m);
            dpsMarkHits.add(new RectF(j==0?dpsCurve.stepX0[0]:mx-30*k,gt,i<to?dpsCurve.markX[j+1]:dpsCurve.stepX1[j],gb+60*k));
        }
        DpsLog.Segment g=dps.segments.get(dpsSel);double ch=dps.change(dpsSel);
        String what=changeWords(g,false);
        String line=(what.isEmpty()?"":what+" · ")+number(g.value)+" DPS"+(g.range()?" ("+range(g.lo,g.hi)+")":"");
        dpsSelColour=Double.isNaN(ch) || Math.abs(ch)<.05?"FontColorWhite":ch>0?"FontColorPartyGreen":"FontColorRed";
        String pct=Double.isNaN(ch) || Math.abs(ch)<.05?"":(ch>0?"+":"-")+number(Math.abs(ch))+" %";
        dpsSelLine=fit(body,line+(pct.isEmpty()?"":" · "+pct),right-x,k);
        dpsSelX=x;dpsSelY=bottom-20*k;
        dpsSelPct=pct;dpsSelHead=line+" · ";dpsSelPctX=pct.isEmpty()?0:x+body.measure(dpsSelHead,dpsSelLine.k);
    }
    private String dpsSelPct="",dpsSelHead="";private float dpsSelPctX;
    private void drawDps(Canvas c,float x,float right,float y,float top,float bottom,float k){
        if(dpsHead!=null){body.colour("FontColorWhite");body.draw(c,dpsHead.text,x,y,dpsHead.k,Paint.Align.LEFT,255);}
        viewSwitch(c,x,right,top,k);
        if(dpsLine!=null){body.colour(dpsLineColour);body.draw(c,dpsLine.text,x,dpsLineY,dpsLine.k,Paint.Align.LEFT,255);}
        float pad=10*k;
        for(int r=0;r<dpsCells.size();r++){
            String[] cell=dpsCells.get(r);Fit[] f=dpsFits.get(r);float ry=dpsRowY.get(r);
            for(int i=0;i<4;i++){if(f[i]==null)continue;GameText font=i==0 || cell[4].equals("FontColorLightGray")?note:body;font.colour(cell[4+i]);font.draw(c,f[i].text,x+(right-x)*DPS_COLS[i]+pad,ry,f[i].k,Paint.Align.LEFT,255);}
        }
        if(dpsCurve==null)return;
        note.colour("FontColorLightGray");note.draw(c,dpsCurveTitle.text,x,dpsTitleY,dpsCurveTitle.k,Paint.Align.LEFT,255);
        DpsLog.Curve cv=dpsCurve;
        stroke.setStyle(Paint.Style.STROKE);stroke.setStrokeWidth(k);stroke.setColor(alpha(style.colour("FontColorGoldYellow"),90));
        c.drawLine(dpsBox.left,dpsBox.bottom,dpsBox.right,dpsBox.bottom,stroke);c.drawLine(dpsBox.left,dpsBox.top,dpsBox.left,dpsBox.bottom,stroke);
        for(int i=0;i<cv.ticks.size();i++){float ty=cv.ticks.get(i)[0];Fit t=dpsTicks.get(i);note.colour("FontColorGray");note.draw(c,t.text,dpsBox.left-8*k,ty+8*k,t.k,Paint.Align.RIGHT,255);}
        int green=style.colour("FontColorPartyGreen");
        area.setStyle(Paint.Style.FILL);area.setColor(alpha(green,60));
        for(int i=0;i<cv.stepY.length;i++)if(!Float.isNaN(cv.bandY0[i]))c.drawRect(cv.stepX0[i],cv.bandY0[i],cv.stepX1[i],cv.bandY1[i],area);
        stroke.setColor(green);stroke.setStrokeWidth(3*k);
        for(int i=0;i<cv.stepY.length;i++){c.drawLine(cv.stepX0[i],cv.stepY[i],cv.stepX1[i],cv.stepY[i],stroke);if(i>0)c.drawLine(cv.stepX0[i],cv.stepY[i-1],cv.stepX0[i],cv.stepY[i],stroke);}
        int gold=style.colour("FontColorGoldYellow");
        for(int i=0;i<cv.stepY.length;i++){
            final int seg=cv.from+i;float mx=i==0?cv.stepX0[0]:cv.markX[i];boolean sel=seg==dpsSel;
            area.setColor(sel?gold:alpha(gold,150));c.drawPath(dpsMarkPaths.get(i),area);
            Fit w=dpsMarks.get(i);if(w!=null){note.colour(sel?"FontColorGoldYellow":"FontColorLightGray");note.draw(c,w.text,mx,dpsBox.bottom+50*k,w.k,i==0?Paint.Align.LEFT:Paint.Align.CENTER,255);}
            hit(dpsMarkHits.get(i),3,()->{dpsSel=seg;prepare();qa();invalidate();});
        }
        if(dpsOlder)hit(new RectF(x,dpsBox.top,dpsBox.left,dpsBox.bottom),3,()->{dpsSel=Math.max(0,dpsFrom-1);dpsFrom=Math.max(0,dpsFrom-DPS_SEGS);prepare();qa();invalidate();});
        if(dpsSelLine!=null){body.colour("FontColorWhite");body.draw(c,dpsSelPct.isEmpty()?dpsSelLine.text:dpsSelHead,dpsSelX,dpsSelY,dpsSelLine.k,Paint.Align.LEFT,255);
            if(!dpsSelPct.isEmpty()){body.colour(dpsSelColour);body.draw(c,dpsSelPct,dpsSelPctX,dpsSelY,dpsSelLine.k,Paint.Align.LEFT,255);}}
    }


    private float viewSwitch(Canvas c,float x,float to,float top,float k){
        boolean kept=streaks!=null && streaks.any(),own=dpsShown();
        if(!kept && !own)return top;
        int n=1+(kept?1:0)+(own?1:0);float bw=Math.min((to-x)/n-8*k,260*k),bx=x;
        viewButton(c,new RectF(bx,top-8*k,bx+bw,top+40*k),tr("Kämpfe","Fights","Combates"),!streakView && !dpsView,0,k);bx+=bw+16*k;
        if(kept){viewButton(c,new RectF(bx,top-8*k,bx+bw,top+40*k),tr("Serien","Streaks","Sequências"),streakView,1,k);bx+=bw+16*k;}
        if(own)viewButton(c,new RectF(bx,top-8*k,bx+bw,top+40*k),"DPS",dpsView,2,k);
        return top+56*k;
    }
    private void viewButton(Canvas c,RectF r,String label,boolean active,int view,float k){
        buttons.set(boxes.size());
        style.panelButton(c,r,padUsed && boxes.size()==focus?GameStyle.BUTTON_HOVER:active?GameStyle.BUTTON_DOWN:GameStyle.BUTTON_NORMAL,label);
        hit(r,()->{if(active)return;streakView=view==1;dpsView=view==2;page=0;prepare();qa();invalidate();});
    }

    private boolean dpsShown(){return dps!=null && (dps.fights>0 || !dps.segments.isEmpty());}
    private float[] streakCols(){return streakBonus?STREAK_COLS:STREAK_COLS_PLAIN;}
    private String grouped(int v){return String.format(lang()==CombatModel.L_EN?java.util.Locale.US:java.util.Locale.GERMANY,"%,d",v);}

    private String streakKills(StreakLog.Streak s){return s.kills+" · "+KillStreak.title(getContext(),streaks.tier(s.kills));}
    private String streakArea(StreakLog.Streak s,boolean mixed){
        String a=s.area>0?shownName("@area:"+s.area):"";String d=mixed?difficultyWord(s.difficulty):"";
        return a.isEmpty()?d:d.isEmpty()?a:a+" · "+d;
    }



    private void prepareStreaks(){
        streakCells.clear();streakFits.clear();streakStars.clear();java.util.Arrays.fill(streakColumns,null);streakHead=null;streakOff=false;
        if(streaks==null)return;
        CombatModel shown=model!=null?model:last;
        String count=streaks.count==0?"":streaks.count==1?tr("1 Serie","1 streak","1 sequência"):tr("die letzten ","the last ","as últimas ")+streaks.count+tr(" Serien"," streaks"," sequências");
        String head=shown==null?count:shown.name+" · "+tr("Stufe ","Level ","Nível ")+shown.level+(count.isEmpty()?"":" · "+count);
        streakOff=log!=null && !log.recording;
        streakBonus=streaks.bonusColumn();boolean mixed=streaks.mixed();
        for(int d=0;d<StreakLog.DIFFICULTIES;d++){StreakLog.Streak b=streaks.dropped(d);if(b!=null){String a=streakArea(b,mixed);streakCells.add(new String[]{tr("Rekord ","Record ","Recorde ")+streakKills(b)+(a.isEmpty()?"":" · "+a),"FontColorGold"});streakStars.add(Float.NaN);}}
        for(StreakLog.Streak s:streaks.rows(streakSort,streakDescending,id->id>0?shownName("@area:"+id):"")){
            streakCells.add(new String[]{streakKills(s),s.death()?tr("Tod","Death","Morte"):s.bonus>0?"+"+grouped(s.bonus):"",CombatModel.time(s.seconds(),lang()),streakArea(s,mixed),
                "FontColorWhite",s.death()?"FontColorRed":"FontColorPartyGreen","FontColorWhite","FontColorWhite"});
            streakStars.add(streaks.record(s)?0f:Float.NaN);
        }
        if(getWidth()<=0 || getHeight()<=0)return;
        GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());float k=getHeight()/1080f,x=p.contentL+24*p.s,right=p.contentR-24*p.s,pad=10*k;float[] cols=streakCols();
        streakOffText=tr("Aufzeichnung aus","Recording off","Gravação desligada");
        if(!head.isEmpty())streakHead=fit(body,head,right-x-(streakOff?note.measure(streakOffText,k)+24*k:0),k);
        String[] heads={"Kills",tr("Bonus","Bonus","Bônus"),tr("Dauer","Time","Duração"),tr("Gebiet","Area","Área")};
        for(int i=0;i<4;i++){float l=x+(right-x)*cols[i],r=x+(right-x)*cols[i+1];streakColumns[i]=r<=l?null:fit(note,heads[i]+(streakSort==i && arrows?(streakDescending?" ↓":" ↑"):""),r-l-10*k,k);}
        float size=body.size(k);
        for(int r=0;r<streakCells.size();r++){
            String[] cell=streakCells.get(r);Fit[] f=new Fit[4];
            if(cell.length==2)f[0]=fit(body,cell[0],right-x-2*pad,k);
            else for(int i=0;i<4;i++){float a=x+(right-x)*cols[i],b=x+(right-x)*cols[i+1];if(!cell[i].isEmpty() && b>a)f[i]=fit(body,cell[i],b-a-2*pad,k);}
            streakFits.add(f);
            if(!Float.isNaN(streakStars.get(r)) && f[0]!=null)streakStars.set(r,x+pad+body.measure(f[0].text,f[0].k)+.55f*size);
        }
        streakStar.rewind();float[] star=CombatGraph.star(0,-.36f*size,.42f*size);
        for(int i=0;i+1<star.length;i+=2){if(i==0)streakStar.moveTo(star[i],star[i+1]);else streakStar.lineTo(star[i],star[i+1]);}
        streakStar.close();streakGold=style.colour("FontColorGold");
    }
    private void drawStreaks(Canvas c,float x,float right,float y,float top,float bottom,float k){
        if(streakHead!=null){body.colour("FontColorWhite");body.draw(c,streakHead.text,x,y,streakHead.k,Paint.Align.LEFT,255);}
        if(streakOff){note.colour("FontColorLightGray");note.draw(c,streakOffText,right,y,k,Paint.Align.RIGHT,255);}
        float lt=viewSwitch(c,x,right,top,k);float[] cols=streakCols();
        for(int i=0;i<4;i++){
            final int col=i;float l=x+(right-x)*cols[i],r=x+(right-x)*cols[i+1];
            if(streakColumns[i]==null)continue;
            note.colour(streakSort==i?"FontColorGoldYellow":"FontColorLightGray");note.draw(c,streakColumns[i].text,l+10*k,lt+26*k,streakColumns[i].k,Paint.Align.LEFT,255);
            hit(new RectF(l,lt-4*k,r,lt+38*k),()->{if(streakSort!=col){streakSort=col;streakDescending=false;}else if(!streakDescending)streakDescending=true;else{streakSort=StreakLog.SORT_NONE;streakDescending=false;}page=0;prepare();qa();invalidate();});
        }
        float ry=lt+45*k,pitch=46*k,pad=10*k;
        int count=CombatLayout.rows(ry,bottom,pitch);pages=CombatModel.pages(streakCells.size(),count);page=Math.min(page,pages-1);
        area.setStyle(Paint.Style.FILL);area.setColor(streakGold);
        for(int r=page*count;r<Math.min(Math.min(streakCells.size(),streakFits.size()),(page+1)*count);r++){
            String[] cell=streakCells.get(r);Fit[] f=streakFits.get(r);float base=ry+pitch*.66f;
            int n=cell.length==2?1:4;
            for(int i=0;i<n;i++){
                if(f[i]==null)continue;
                body.colour(cell[n+i]);body.draw(c,f[i].text,x+(right-x)*cols[i]+pad,base,f[i].k,Paint.Align.LEFT,255);
            }
            float star=streakStars.get(r);
            if(!Float.isNaN(star)){c.save();c.translate(star,base);c.drawPath(streakStar,area);c.restore();}
            ry+=pitch;
        }
    }


    private String[] cells(FightLog.Row row){
        FightLog.Entry e=row.entry;FightLog.Group g=row.group;
        String[] c={"","","","","FontColorWhite","FontColorWhite","FontColorWhite","FontColorWhite"};
        if(g==null){
            String mark=e.groups.size()<2?"":opened.contains(e.index)?(triangles?"▼ ":"– "):(triangles?"▶ ":"+ ");
            c[0]=mark+FightLog.label(e,this::logName);c[4]=rankColour(e.lead());
            c[1]=e.end==FightLog.END_KILL?FightLog.time(e.seconds(),lang()):e.end==FightLog.END_DEATH?tr("Tod ","Death ","Morte ")+FightLog.time(e.seconds(),lang()):ended(e);
            c[5]=e.end==FightLog.END_DEATH?"FontColorRed":e.end==FightLog.END_KILL?"FontColorWhite":"FontColorGray";
            c[2]=e.own()>=0?number(e.own()):"";
            c[3]=e.minLifePercent()+" %";c[7]=e.minLife==0?"FontColorRed":"FontColorWhite";
        }else{
            c[0]=FightLog.label(g,this::logName);c[4]=rankColour(g);
            c[1]=g.seconds()>=0?FightLog.time(g.seconds(),lang()):"";
            double dps=g.own(e.lead+e.duration,e.known);c[2]=dps>=0?number(dps):"";
        }
        return c;
    }
    private void logRow(Canvas c,FightLog.Row row,String[] cell,Fit[] fits,float x,float split,float ry,float pitch,float[] cols,float k){
        FightLog.Entry e=row.entry;FightLog.Group g=row.group;
        RectF r=new RectF(x,ry,split,ry+pitch-4*k);
        float base=ry+pitch*.66f,pad=10*k,indent=g==null?0:36*k;
        if(e.index==logFight && (g==null?logGroup<0:g.index==logGroup))art.rim(c,"build_row",r,8,k,picture);
        if(g==null){
            if(e.groups.size()>=2)hit(new RectF(x,ry,x+60*k,ry+pitch-4*k),()->{if(!opened.remove(e.index))opened.add(e.index);prepare();qa();invalidate();});
            hit(r,()->openFight(e.index,-1));
        }else hit(r,()->openFight(e.index,g.index));
        for(int i=0;i<4;i++){
            if(cell[i].isEmpty())continue;
            float a=x+(split-x)*cols[i],b=x+(split-x)*cols[i+1],in=i==0?indent:0;
            if(fits!=null && fits[i]!=null){body.colour(cell[4+i]);body.draw(c,fits[i].text,a+pad+in,base,fits[i].k,Paint.Align.LEFT,255);}
            else text(c,body,cell[i],a+pad+in,base,b-a-2*pad-in,k,cell[4+i]);
        }
    }
    private static void path(Path out,float[] xy){out.rewind();for(int i=0;i+1<xy.length;i+=2){if(i==0)out.moveTo(xy[i],xy[i+1]);else out.lineTo(xy[i],xy[i+1]);}}
    private static int alpha(int argb,int a){return (argb&0xFFFFFF)|a<<24;}
    private void icon(Canvas c,Bitmap b,float cx,float cy,float size){if(b==null)return;cell.set(cx-size/2,cy-size/2,cx+size/2,cy+size/2);c.drawBitmap(b,null,cell,picture);}
    private void drawGraph(Canvas c,float k){
        CombatGraph g=graph;
        art.nine(c,"build_inset",graphBox,8,k,picture);
        area.setStyle(Paint.Style.FILL);area.setColor(alpha(graphColours[2],40));
        for(float[] r:g.shades)c.drawRect(r[0],r[1],r[2],r[3],area);
        stroke.setStyle(Paint.Style.STROKE);stroke.setStrokeJoin(Paint.Join.ROUND);stroke.setStrokeWidth(k);
        stroke.setColor(alpha(gold,45));c.drawLines(g.grid,stroke);
        stroke.setColor(alpha(gold,28));c.drawLines(g.faint,stroke);
        for(CombatGraph.Band b:g.bands){area.setColor(alpha(gold,55));c.drawRect(b.box[0],b.box[1],b.box[2],b.box[3],area);}
        stroke.setStrokeWidth(3*k);
        stroke.setColor(graphColours[2]);c.drawPath(manaLine,stroke);
        stroke.setColor(graphColours[0]);c.drawPath(enemyLine,stroke);
        stroke.setColor(graphColours[1]);c.drawPath(lifeLine,stroke);
        if(g.group!=null){stroke.setStrokeWidth(1.5f*k);stroke.setColor(alpha(graphColours[0],170));c.drawPath(groupLine,stroke);}
        if(g.simEnemy!=null){stroke.setStrokeWidth(2*k);stroke.setColor(alpha(graphColours[0],210));c.drawLines(g.simEnemy,stroke);stroke.setColor(alpha(graphColours[1],210));c.drawLines(g.simLife,stroke);}
        stroke.setStrokeWidth(k);stroke.setColor(gold);c.drawLines(g.axes,stroke);
        for(int i=0;i<g.ticks.size();i++){float[] r=g.ticks.get(i);area.setColor(g.tickColours.get(i));c.drawRect(r[0],r[1],r[2],r[3],area);}
        for(int i=0;i<markerIcons.length;i++){CombatGraph.Marker m=g.markers.get(i);icon(c,markerIcons[i],m.x,m.y,g.icon);}
        if(!g.stars.isEmpty()){area.setColor(gold);c.drawPath(starPath,area);}
        note.colour("FontColorGoldYellow");
        for(CombatGraph.Label l:g.labels)note.draw(c,l.text,l.x,l.baseline,k,l.right?Paint.Align.RIGHT:Paint.Align.LEFT,255);
        note.colour("FontColorWhite");
        for(CombatGraph.Band b:g.bands)note.draw(c,b.text,b.textX,b.baseline,k,Paint.Align.LEFT,255);
        float base=g.row*.68f;
        for(int i=0;i<g.legend.size();i++){
            CombatGraph.Entry e=g.legend.get(i);float mid=e.y+g.row/2;
            if(e.kind==CombatGraph.LINE_ENEMY || e.kind==CombatGraph.LINE_LIFE){stroke.setStrokeWidth(3*k);stroke.setColor(e.colour);c.drawLine(e.x,mid,e.x+24*k,mid,stroke);}
            else if(e.kind==CombatGraph.LINE_GROUP){stroke.setStrokeWidth(1.5f*k);stroke.setColor(alpha(e.colour,170));c.drawLine(e.x,mid,e.x+24*k,mid,stroke);}
            else if(e.kind==CombatGraph.LINE_SIM){stroke.setStrokeWidth(2*k);stroke.setColor(e.colour);for(int d=0;d<3;d++)c.drawLine(e.x+d*9*k,mid,e.x+d*9*k+5*k,mid,stroke);}
            else if(e.kind==CombatGraph.STAR){}
            else if(e.kind==CombatGraph.TICK){float tx=e.x;if(legendIcons[i]!=null){icon(c,legendIcons[i],tx+g.icon/2,mid,g.icon);tx+=26*k;}area.setColor(e.colour);c.drawRect(tx,mid-8*k,tx+3*k,mid+8*k,area);}
            else if(e.kind==CombatGraph.SHADE){area.setColor(alpha(e.colour,70));c.drawRect(e.x,mid-7*k,e.x+22*k,mid+7*k,area);}
            else icon(c,legendIcons[i],e.x+g.icon/2,mid,g.icon);
            note.colour(e.kind==CombatGraph.LINE_ENEMY || e.kind==CombatGraph.LINE_GROUP?"FontColorPartyGreen":e.kind==CombatGraph.LINE_LIFE?"FontColorRed":"FontColorWhite");
            note.draw(c,e.text,e.textX,e.y+base,k,Paint.Align.LEFT,255);
        }
        if(g.endBox!=null){
            int colour=graphColours[g.endKind==CombatModel.END_KILL?0:1];
            stroke.setStrokeWidth(2*k);stroke.setColor(colour);c.drawLines(g.dash,stroke);
            area.setColor(0xEB0C0C0C);c.drawRoundRect(g.endBox[0],g.endBox[1],g.endBox[2],g.endBox[3],4*k,4*k,area);
            note.colour(g.endKind==CombatModel.END_KILL?"FontColorPartyGreen":"FontColorRed");
            note.draw(c,g.endText,g.endBox[2]-5*k,g.endBox[1]+(g.endBox[3]-g.endBox[1])*.72f,k,Paint.Align.RIGHT,255);
            area.setColor(colour);c.drawCircle(g.endX,g.endY,4*k,area);
        }
    }

    private void drawDetail(Canvas c,CombatModel m,float x,float right,float y,float bottom,float k){
        float bw=backButton(c,x,right,y,k),bh=48*k,textRight=graph!=null?graphBox.left-24*k:right,below=0;

        if(header!=null && !m.enemies.isEmpty()){float pitch=body.size(header.nameK)*1.1f,half=(header.lines()-1)*pitch/2;below=half;
            title(c,header,CombatLayout.titleLeft(x,right,k),y+34*k-half,pitch,CombatModel.colour(m.enemies.get(0).light));}
        else if(!detailLines.isEmpty())text(c,body,detailLines.get(0),x+bw+24*k,y+34*k,textRight-x-bw-24*k,k,m.enemies.isEmpty()?"FontColorWhite":CombatModel.colour(m.enemies.get(0).light));
        drawLines(c,x,textRight,y+bh+8*k+below,bottom,k);
        if(graph!=null)drawGraph(c,k);
    }
    private void drawRotation(Canvas c,CombatModel m,float x,float right,float y,float bottom,float k){
        text(c,body,rotation(m),x,y+24*k,right-x,k,"FontColorWhite");y+=60*k;
        String change=rotationBefore!=null && !number(rotationBefore).equals(number(m.rotation))?number(rotationBefore)+" › "+number(m.rotation)+" DPS":dps(m);
        text(c,body,change,x,y,right-x,k,(rotationBefore==null || number(rotationBefore).equals(number(m.rotation)))?"FontColorWhite":m.rotation<rotationBefore?"FontColorRed":"FontColorPartyGreen");y+=40*k;
        text(c,note,tr("Antippen / A: Skill vorziehen und eigene Rotation verwenden.","Tap / A: move skill earlier and use custom rotation.","Tocar / A: antecipar a habilidade e usar rotação própria."),x,y,right-x,k,"FontColorLightGray");y+=35*k;

        int count=CombatLayout.rows(y,bottom,55*k);pages=CombatModel.pages(order.size(),count);page=Math.min(page,pages-1);
        final int first=boxes.size(),rows=count;
        for(int i=page*count;i<Math.min(order.size(),(page+1)*count);i++){final int at=i,own=boxes.size();button(c,new RectF(x,y,right,y+48*k),(i+1)+". "+skillName(order.get(i)),()->{if(at>0)moveUp(at,m.rotation,focus==own?first:-1,rows);},k);y+=55*k;}
    }

    private void moveUp(int at,double before,int firstRow,int rows){
        int id=order.remove(at);order.add(at-1,id);ordered=true;
        page=(at-1)/rows;if(firstRow>=0)focus=firstRow+(at-1)%rows;
        epoch++;rotationBefore=before;request(true);
    }
    private float backButton(Canvas c,float x,float right,float y,float k){
        float bw=CombatLayout.backWidth(x,right,k);
        button(c,new RectF(x,y,x+bw,y+48*k),tr("Zurück","Back","Voltar"),this::back,k);
        return bw;
    }
    private void drawLines(Canvas c,float x,float right,float y,float bottom,float k){
        List<String> wrapped=wrappedLines;
        int count=CombatLayout.rows(y,bottom,43*k);pages=CombatModel.pages(wrapped.size(),count);page=Math.min(page,pages-1);
        for(int i=page*count;i<Math.min(wrapped.size(),(page+1)*count);i++){text(c,body,wrapped.get(i),x,y+30*k,right-x,k,"FontColorWhite");y+=43*k;}
    }
    private void flip(int delta){page=Math.max(0,Math.min(pages-1,page+delta));focus=0;invalidate();}
    private void search(){if(dialog!=null)return;EditText edit=new EditText(getContext());edit.setSingleLine(true);edit.setText(query);dialog=new AlertDialog.Builder(getContext()).setTitle(tr("Gegner suchen","Search enemies","Procurar inimigos")).setView(edit).setPositiveButton("OK",(d,w)->{query=edit.getText().toString();tab=1;selected=-1;page=0;prepare();request(true);}).setNegativeButton(tr("Abbrechen","Cancel","Cancelar"),null).create();dialog.setOnDismissListener(d->{dialog=null;if(BuildConfig.DM_QA_BUILD)TestHooks.overlay("combat_search","idle");});dialog.show();edit.requestFocus();if(BuildConfig.DM_QA_BUILD)TestHooks.overlay("combat_search","editing");}
    private int at(float x,float y){for(int i=0;i<boxes.size();i++)if(boxes.get(i).contains(x,y))return i;return -1;}
    @Override public boolean onTouchEvent(MotionEvent e){if(equipBusy)return true;if(tipId!=0)return tipTouch(e);int hit=at(e.getX(),e.getY());if(e.getActionMasked()==MotionEvent.ACTION_DOWN){padUsed=false;down=hit;invalidate();return true;}if(e.getActionMasked()==MotionEvent.ACTION_MOVE && hit!=down)down=-1;if(e.getActionMasked()==MotionEvent.ACTION_UP){if(hit>=0 && hit==down)actions.get(hit).run();down=-1;}if(e.getActionMasked()==MotionEvent.ACTION_CANCEL)down=-1;return true;}
    @Override public boolean padOpen(){return isOpen();}
    @Override public void back(){if(equipBusy)return;if(tipId!=0){closeTip();return;}if(tab==TAB_LOG || tab==TAB_BEST){close();return;}if(selected>=0){epoch++;selected=-1;detail=null;page=tablePage;focus=tableFocus;qa();invalidate();request(false);}else if(!query.isEmpty()){query="";page=0;prepare();invalidate();}else close();}
    @Override public void onPad(int cmd){padUsed=true;if(equipBusy){invalidate();return;}if(tipId!=0){tipPad(cmd);invalidate();return;}switch(cmd){case PadNav.B:back();break;case PadNav.MENU:close();break;case PadNav.L1:switchTab(tab-1);break;case PadNav.R1:switchTab(tab+1);break;case PadNav.L2:flip(-1);break;case PadNav.R2:flip(1);break;case PadNav.Y:search();break;case PadNav.UP:case PadNav.LEFT:loaded(()->focus=Math.max(0,focus-1));break;case PadNav.DOWN:case PadNav.RIGHT:loaded(()->focus=Math.min(actions.size()-1,focus+1));break;case PadNav.A:if(focus>=0 && focus<actions.size())actions.get(focus).run();break;default:break;}invalidate();}
    private String text(int id,Object... args){return args.length==0?AppText.get(getContext(),id):AppText.get(getContext(),id,args);}



    private void readStock(){
        stockAsk=DmActivity.nativeBestGearStockAsk();stockAt=SystemClock.uptimeMillis();stockReading=true;
        if(paused){paused=false;DmActivity.nativePause(DmActivity.PAUSE_COMBAT,0);}
        removeCallbacks(stockPoll);post(stockPoll);
    }
    private void pollStock(){
        if(!stockReading)return;
        if(DmActivity.nativeBestGearStockAnswered()-stockAsk>=0){endStock(true);return;}
        if(!isOpen() || tab!=TAB_BEST || SystemClock.uptimeMillis()-stockAt>=STOCK_WAIT_MS){endStock(false);return;}
        postDelayed(stockPoll,16);
    }
    private void endStock(boolean answered){
        stockReading=false;removeCallbacks(stockPoll);

        if(isOpen() && !equipBusy)takePause();
        qa();invalidate();
        AppArt.log("best gear: stash page stock "+(answered?"answered":"not answered")+" after "+(SystemClock.uptimeMillis()-stockAt)+" ms");
    }

    private void pollBest(){
        removeCallbacks(bestPoll);if(!isOpen() || tab!=TAB_BEST)return;
        if(bestBusy){postDelayed(bestPoll,250);return;}
        bestBusy=true;
        worker.execute(()->{
            byte[] bytes=null;try{bytes=DmActivity.nativeBestGear(1);}catch(RuntimeException e){AppArt.log("bestgear.response: "+e);}
            BestGearState state=bytes==null?null:BestGearState.parse(new String(bytes,StandardCharsets.UTF_8));
            post(()->{bestBusy=false;
                if(!isOpen() || tab!=TAB_BEST){DmActivity.nativeBestGear(0);return;}
                if(state!=null){if(!state.rev.equals(bestRev)){bestRev=state.rev;bestFill=0;}bestFill=state.fill(bestFill);best=state;
                    if(equipBusy && equipSeq>0 && state.job!=null && state.job.seq==equipSeq){equipJob=state.job;if(!BestGearEquip.running(state.job.st))equipEnded(state.job);}
                    prepareBest();
                    if(tipId!=0 && state.tip!=null && state.tip.id==tipId && state.tip.place==tipPlace && (!(state.tip.text+"\n\n"+state.tip.block).equals(tipText) || state.tip.dots!=tipDots)){
                        tipText=state.tip.text+"\n\n"+state.tip.block;tipDots=state.tip.dots;
                        List<GearPreview.Line> l=new ArrayList<>(GearPreview.tooltipLines(state.tip.text));l.addAll(GearPreview.blockLines(state.tip.block));tipLines=l;}}
                qa();invalidate();postDelayed(bestPoll,250);});
        });
    }
    private void prepareBest(){
        BestGearState s=best;bestWhere.clear();bestPictures.clear();bestColours.clear();

        if(dotRows.words(getContext())){tipRows=null;totalRows=null;}
        barFill.setColor(style.colour("FontColorGoldYellow"));
        bestOff=text(R.string.dmAndroidBestOff);bestTotal=text(R.string.dmAndroidBestTotal);bestFirst="· "+text(R.string.dmAndroidBestFirst);
        bestCharmsLabel=text(R.string.dmAndroidBestSearchCharms);bestCharm=text(R.string.dmAndroidBuildSlotCharm);
        if(checkOn==null)checkOn=art.image("build_check1");if(checkOff==null)checkOff=art.image("build_check0");
        bestBar=s!=null && s.usable() && !s.failed();
        prepareEquip(s);
        if(s==null)bestStateLine=text(R.string.dmAndroidBestReading);
        else bestStateLine=text("set2".equals(s.state)?R.string.dmAndroidBestSet2:"nosim".equals(s.state)?R.string.dmAndroidBestNoSim:"noref".equals(s.state)?R.string.dmAndroidBestNoRef
            :!s.usable()?R.string.dmAndroidBestReading:s.failed()?R.string.dmAndroidBestFailed
            :!s.done()?(BestGearEquip.interim(false,bestButtons,bestAll)?R.string.dmAndroidBestInterim:R.string.dmAndroidBestSearching):s.rows.isEmpty()?R.string.dmAndroidBestSame:R.string.dmAndroidBestFound);
        for(int i=0;s!=null && i<s.rows.size();i++){
            BestGearState.Row r=s.rows.get(i);
            String where=r.where==BestGearState.INV?text(R.string.dmAndroidBestInv):r.where==BestGearState.STASH?text(R.string.dmAndroidBestStash,r.page)
                :r.where==BestGearState.SHARED?(r.page>0?text(R.string.dmAndroidBestSharedTab,r.page):text(R.string.dmAndroidBestShared)):r.where==BestGearState.WORN?text(R.string.dmAndroidBestWorn):"";

            if(r.charm!=0)bestWhere.add(bestCharm+" · "+(r.charm==2?bestOff:where));
            else bestWhere.add(r.slot+(where.isEmpty()?"":" · "+where));

            bestColours.add(r.colour.isEmpty()?BestGearState.quality(r.quality):GearPreview.colourOf(r.colour.charAt(0)));
            String name=r.code.isEmpty()?null:art.invPictures().name(r.code,r.quality,r.row,r.gfx);
            String key=name!=null?"inv/"+name:r.code.isEmpty()?null:"items/"+r.code;
            Bitmap pic=null;if(key!=null){if(!itemPictures.containsKey(key))itemPictures.put(key,art.image(key));pic=itemPictures.get(key);}
            bestPictures.add(pic);
        }

        if(s!=null){
            List<String> f=new ArrayList<>();
            if(!s.charms)f.add(text(R.string.dmAndroidBestCharms));
            if(s.pages>0)f.add(text(R.string.dmAndroidBestPages,s.pages));
            if(s.shared>0)f.add(text(R.string.dmAndroidBestSharedTabs));
            if(s.result && s.unrated>0)f.add(text(R.string.dmAndroidBestUnrated,s.unrated));
            if(s.skipped>0)f.add(text(R.string.dmAndroidBestSkipped,s.skipped));
            bestFoot=s.usable() && !f.isEmpty()?text(R.string.dmAndroidBestLeftOut)+" "+android.text.TextUtils.join(" · ",f):"";
        }else bestFoot="";

        if(getWidth()>0 && getHeight()>0){
            GameStyleGeometry.Panel p=style.panel(getWidth(),getHeight());float k=getHeight()/1080f,bottom=p.contentB-20*p.s;
            bestTop=p.contentT+40*p.s+20*k+(bestBar?48*k:0);
            boolean shown=s!=null && s.result;
            bestPlan=BestGearPages.split(shown?s.rows.size():0,shown?s.totals.size():0,bottom-44*k-bestTop,88*k,BEST_HEAD*k,36*k);
        }
        page=Math.min(page,bestPlan.size()-1);
    }
    private void drawBest(Canvas c,float x,float right,float y,float bottom,float k){
        BestGearState s=best;
        boolean usable=s!=null && s.usable();

        float box=30*k,label=note.measure(bestCharmsLabel,k),boxL=right-label-12*k-box;
        RectF charms=new RectF(boxL-8*k,y-34*k,right,y+10*k);
        Bitmap check=bestCharms?checkOn:checkOff;if(check!=null){cell.set(boxL,y-box+4*k,boxL+box*35/33f,y+4*k);c.drawBitmap(check,null,cell,picture);}
        text(c,note,bestCharmsLabel,right-label,y,label+1,k,"FontColorWhite");
        hit(charms,HIT_BEST_CHARMS,this::switchCharms);

        float stateRight=boxL-24*k;

        boolean locked=equipBusy || stockReading;
        if(bestAll){float aw=Math.max(200*k,note.measure(bestAllLabel,k)+80*k);RectF all=new RectF(stateRight-aw,y-36*k,stateRight,y+12*k);
            buttons.set(boxes.size());style.panelButton(c,all,locked?GameStyle.BUTTON_DISABLED:padUsed && boxes.size()==focus?GameStyle.BUTTON_HOVER:GameStyle.BUTTON_NORMAL,bestAllLabel);
            hit(all,HIT_BEST_EQUIP_ALL,locked?()->{}:()->equip(-1));stateRight=all.left-16*k;}
        boolean counting=equipBusy && equipJob!=null && equipJob.n>0;
        String stateText=counting?text(R.string.dmAndroidBestEquipping,equipJob.k,equipJob.n):bestStateLine;


        float stateRoom=stateRight-x,sepW=body.measure("  ·  ",k);
        float stateW=bestNote.isEmpty()?stateRoom:Math.min(body.measure(stateText,k),Math.max(stateRoom*.25f,stateRoom-sepW-body.measure(bestNote,k)));
        text(c,body,stateText,x,y,stateW,k,counting?"FontColorWhite":usable && s.done()?"FontColorPartyGreen":bestBar?"FontColorWhite":"FontColorLightGray");
        if(!bestNote.isEmpty()){float sep=sepW;text(c,body,"·",x+stateW+sep/2-body.measure("·",k)/2,y,sep,k,"FontColorLightGray");
            text(c,body,bestNote,x+stateW+sep,y,stateRoom-stateW-sep,k,bestNoteColour);}
        y+=20*k;
        if(bestBar){
            RectF bar=new RectF(x,y,right,y+26*k);art.nine(c,"build_inset",bar,8,k,picture);
            float inset=5*k;if(bestFill>0)c.drawRect(bar.left+inset,bar.top+inset,bar.left+inset+(bar.width()-2*inset)*bestFill,bar.bottom-inset,barFill);
            if(BuildConfig.DM_QA_BUILD)TestHooks.hit(bar,HIT_BEST_BAR,0);
        }
        y=bestTop;
        if(s!=null && s.result && page<bestPlan.size()){
            BestGearPages.Page pg=bestPlan.get(page);
            float rowH=88*k,pic=72*k;
            for(int i=pg.rowFrom;i<pg.rowTo && i<s.rows.size();i++){
                BestGearState.Row r=s.rows.get(i);RectF card=new RectF(x,y,right,y+rowH-8*k);art.nine(c,"build_inset",card,8,k,picture);

                int kind=i<bestButtons.length?bestButtons[i]:BestGearEquip.BUTTON_NONE;float textRight=right-16*k;RectF btn=null;
                if(kind!=BestGearEquip.BUTTON_NONE){float bw=230*k,bh=54*k;btn=new RectF(right-14*k-bw,card.centerY()-bh/2,right-14*k,card.centerY()+bh/2);textRight=btn.left-16*k;}
                if(r.id!=0){final int row=i;hit(btn==null?card:new RectF(card.left,card.top,btn.left-6*k,card.bottom),HIT_BEST_ROW,()->openTip(row));}
                if(btn!=null){final int row=i;boolean full=kind==BestGearEquip.BUTTON_NO_ROOM || kind==BestGearEquip.BUTTON_NO_ROOM_OFF,live=!full && !locked;buttons.set(boxes.size());
                    style.panelButton(c,btn,!live?GameStyle.BUTTON_DISABLED:padUsed && boxes.size()==focus?GameStyle.BUTTON_HOVER:GameStyle.BUTTON_NORMAL,bestButtonText.get(i));
                    hit(btn,full?HIT_BEST_NO_ROOM:HIT_BEST_EQUIP,live?()->equip(row):()->{});}
                Bitmap b=i<bestPictures.size()?bestPictures.get(i):null;float tx=x+16*k;
                if(b!=null){float sc=Math.min(pic/b.getWidth(),pic/b.getHeight()),w=b.getWidth()*sc,h=b.getHeight()*sc;cell.set(tx,card.centerY()-h/2,tx+w,card.centerY()+h/2);c.drawBitmap(b,null,cell,picture);}
                tx+=pic+16*k;

                String where=i<bestWhere.size()?bestWhere.get(i):r.slot;
                if(r.first){
                    float mark=note.measure(bestFirst,k),room=textRight-tx-mark-10*k;Fit f=fit(note,where,room,k);
                    note.colour("FontColorGoldYellow");note.draw(c,f.text,tx,card.top+30*k,f.k,Paint.Align.LEFT,255);
                    text(c,note,bestFirst,tx+note.measure(f.text,f.k)+10*k,card.top+30*k,mark+1,k,"FontColorLightGray");
                }else text(c,note,where,tx,card.top+30*k,textRight-tx,k,"FontColorGoldYellow");
                text(c,body,r.name.isEmpty()?bestOff:r.name,tx,card.top+66*k,textRight-tx,k,r.name.isEmpty()?"FontColorLightGray":i<bestColours.size()?bestColours.get(i):"FontColorWhite");
                y+=rowH;
            }
            if(pg.heading){

                y+=10*k;text(c,note,bestTotal,x,y+24*k,right-x,k,"FontColorGoldYellow");
                float left=x+note.measure(bestTotal,k)+32*k;
                if(dotRows.ready()){
                    totalRows=dotRows(totalRows,totalRowsAt,s.dots,left,y,BEST_DOT_LINE*k,BEST_DOT_TEXT*k);
                    dotRows.draw(c,s.dots,totalRows);
                }
                y+=BEST_HEAD*k-10*k;
            }
            for(int i=pg.lineFrom;i<pg.lineTo && i<s.totals.size();i++){BestGearState.Line l=s.totals.get(i);text(c,body,l.text,x,y+26*k,right-x,k,l.colour);y+=36*k;}
        }
        if(!bestFoot.isEmpty())text(c,note,bestFoot,x,bottom-6*k,right-x,k,"FontColorLightGray");
    }

    private void prepareEquip(BestGearState s){
        int n=s==null?0:s.rows.size();int stash=s==null?-1:s.stash;
        bestButtons=new int[n];bestButtonText.clear();
        for(int i=0;i<n;i++){BestGearState.Row r=s.rows.get(i);int kind=BestGearEquip.button(stash,s.eq[i],r.name.isEmpty() && r.charm==0);bestButtons[i]=kind;
            bestButtonText.add(kind==BestGearEquip.BUTTON_EQUIP?text(R.string.dmAndroidBestEquip):kind==BestGearEquip.BUTTON_TAKE_OFF?bestOff:kind==BestGearEquip.BUTTON_NO_ROOM?text(R.string.dmAndroidBestNoRoom):kind==BestGearEquip.BUTTON_NO_ROOM_OFF?text(R.string.dmAndroidBestNoRoomOff):"");}
        bestAll=s!=null && s.result && BestGearEquip.all(stash,s.eqall);bestAllLabel=text(R.string.dmAndroidBestEquipAll);
        bestNote="";bestNoteColour="FontColorLightGray";
        if(!equipBusy && equipResult!=BestGearEquip.R_NONE){
            BestGearEquip.Job j=equipJob;int k=j==null?0:j.k,all=j==null?0:j.n;String why=j==null?"":text(WHY_TEXT[BestGearEquip.why(j.why)]);
            switch(equipResult){
            case BestGearEquip.R_DONE_ONE:case BestGearEquip.R_DONE_N:{

                int off=j==null?0:j.off,units=Math.max(1,all),words=BestGearEquip.done(units,off);
                bestNote=units==1 && words==BestGearEquip.DONE_OFF?text(R.string.dmAndroidBestTakeOffDone,equipSlot)
                    :units==1 && !equipName.isEmpty()?text(R.string.dmAndroidBestEquipDone,equipName)
                    :words==BestGearEquip.DONE_OFF?text(R.string.dmAndroidBestTakeOffDoneN,units)
                    :words==BestGearEquip.DONE_MIXED?text(R.string.dmAndroidBestEquipDoneMixed,units-off,off):text(R.string.dmAndroidBestEquipDoneN,units);
                bestNoteColour="FontColorPartyGreen";break;}
            case BestGearEquip.R_BACK:bestNote=text(R.string.dmAndroidBestEquipBack,why);bestNoteColour="FontColorWhite";break;
            case BestGearEquip.R_PARTIAL:bestNote=text(R.string.dmAndroidBestEquipPartial,k,all,why);bestNoteColour="FontColorWhite";break;
            case BestGearEquip.R_IN_HAND:bestNote=text(R.string.dmAndroidBestEquipInHand);bestNoteColour="FontColorWhite";break;
            case BestGearEquip.R_LOST:bestNote=text(R.string.dmAndroidBestEquipLost);bestNoteColour="FontColorRed";break;
            case BestGearEquip.R_CHANGED:bestNote=text(R.string.dmAndroidBestChanged);break;
            case BestGearEquip.R_BUSY:bestNote=text(R.string.dmAndroidBestEquipBusy);break;
            case BestGearEquip.R_AWAY:bestNote=text(R.string.dmAndroidBestAtStash);break;
            case BestGearEquip.R_UNSURE:bestNote=text(R.string.dmAndroidBestEquipUnsure);bestNoteColour="FontColorWhite";break;
            default:break;
            }
        }
        else if(!equipBusy && s!=null && s.result && BestGearEquip.hint(stash,s.eq))bestNote=text(R.string.dmAndroidBestAtStash);
    }


    private void equip(int row){

        BestGearState s=best;if(s==null || equipBusy || stockReading || !s.result)return;
        final String key=s.eqkey;
        equipBusy=true;equipSeq=0;equipJob=null;equipResult=BestGearEquip.R_NONE;
        equipName=row>=0 && row<s.rows.size()?s.rows.get(row).name:"";equipSlot=row>=0 && row<s.rows.size()?s.rows.get(row).slot:"";
        removeCallbacks(equipWatch);postDelayed(equipWatch,EQUIP_LIMIT_MS);
        if(paused){paused=false;DmActivity.nativePause(DmActivity.PAUSE_COMBAT,0);}
        AppArt.log("best gear: equip row "+row);
        prepareBest();qa();invalidate();
        worker.execute(()->{
            int rc;try{rc=DmActivity.nativeBestGearEquip(key,row);}catch(RuntimeException e){AppArt.log("bestgear.equip: "+e);rc=-3;}
            final int answer=rc;
            post(()->{
                if(answer>0)equipSeq=answer;
                else{equipBusy=false;removeCallbacks(equipWatch);equipResult=BestGearEquip.refused(answer);if(isOpen() && !stockReading)takePause();}
                AppArt.log("best gear: equip row "+row+" -> "+answer);
                prepareBest();qa();invalidate();
            });
        });
    }

    private void equipEnded(BestGearEquip.Job j){
        equipBusy=false;removeCallbacks(equipWatch);equipResult=BestGearEquip.result(j.st,j.k,j.n);
        AppArt.log("best gear: equip "+j.st+" "+j.k+"/"+j.n+(j.off>0?" ("+j.off+" taken off)":"")+(j.why.isEmpty()?"":" ("+j.why+")"));
        if(isOpen() && tab==TAB_BEST)readStock();else if(isOpen())takePause();
    }


    private void equipStalled(){
        if(!equipBusy)return;
        equipBusy=false;equipResult=BestGearEquip.R_UNSURE;
        AppArt.log("best gear: equip request "+equipSeq+" reported no end within "+EQUIP_LIMIT_MS+" ms");
        if(isOpen() && tab==TAB_BEST)readStock();else if(isOpen())takePause();
        prepareBest();qa();invalidate();
    }

    private void switchCharms(){
        bestCharms=!bestCharms;BestGearStore.searchCharms(getContext(),bestCharms);DmActivity.nativeBestGearCharms(bestCharms?1:0);
        AppArt.log("best gear: search charms "+(bestCharms?1:0));
        qa();invalidate();
    }

    private void openTip(int row){
        BestGearState s=best;if(s==null || row<0 || row>=s.rows.size() || s.rows.get(row).id==0)return;
        if(tooltip==null)tooltip=new ItemTooltip(style);
        tipId=s.rows.get(row).id;tipPlace=s.rows.get(row).place;tipText="";tipDots=0;tipLines=null;tipPicture=row<bestPictures.size()?bestPictures.get(row):null;tooltip.reset();
        DmActivity.nativeBestGearTip(tipId,tipPlace);qa();invalidate();
    }
    private void closeTip(){if(tipId==0)return;tipId=0;tipPlace=-1;tipLines=null;tipText="";DmActivity.nativeBestGearTip(0,-1);qa();invalidate();}

    private boolean tipTouch(MotionEvent e){
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN){tipDownX=e.getX();tipDownY=e.getY();padUsed=false;return true;}
        if(e.getActionMasked()==MotionEvent.ACTION_UP){
            int step=GearPreview.touch(e.getX()-tipDownX,e.getY()-tipDownY,60*getHeight()/1080f);
            if(step==GearPreview.TOUCH_CLOSE || tooltip==null || !tooltip.flip(step))closeTip();else{qa();invalidate();}
        }
        return true;
    }
    private void tipPad(int cmd){
        if(cmd==PadNav.L2 || cmd==PadNav.LEFT || cmd==PadNav.UP){if(tooltip!=null)tooltip.flip(-1);qa();return;}
        if(cmd==PadNav.R2 || cmd==PadNav.RIGHT || cmd==PadNav.DOWN){if(tooltip!=null)tooltip.flip(1);qa();return;}
        if(cmd==PadNav.MENU){close();return;}
        if(cmd==PadNav.A || cmd==PadNav.B)closeTip();
    }


    private void drawTip(Canvas c){
        if(tipId==0 || tipLines==null || tooltip==null)return;
        tooltip.draw(c,getWidth(),getHeight(),tipPicture,tipLines);
        RectF b=tooltip.box();
        if(BuildConfig.DM_QA_BUILD)TestHooks.hit(b,HIT_BEST_TIP,0);
        if(tipDots==0 || !dotRows.ready())return;
        if(tipRows==null || tipRowsCode!=tipDots || tipRowsRevision!=tooltip.revision()){
            tipRowsCode=tipDots;tipRowsRevision=tooltip.revision();
            tipRows=dotRows.layout(tipDots,tooltip.lineH(),dotRows.units()*tooltip.k())
                .inTooltip(b.left,b.top,b.right,b.bottom,tooltip.inset(),tooltip.textTop(),tooltip.lineLeft,tooltip.lineRight,getWidth(),getHeight(),false);
        }
        dotRows.draw(c,tipDots,tipRows);
        if(BuildConfig.DM_QA_BUILD && tipRows.where!=DotLayout.NONE)TestHooks.hit(dotRows.block(tipRows,tipRowsBox),HIT_BEST_TIP_DOTS,0);
    }
    private DotLayout dotRows(DotLayout old,float[] at,int code,float left,float top,float lineH,float px){
        if(old!=null && at[0]==code && at[1]==left && at[2]==top && at[3]==lineH && at[4]==px)return old;
        at[0]=code;at[1]=left;at[2]=top;at[3]=lineH;at[4]=px;
        return dotRows.layout(code,lineH,px).at(left,top);
    }
    private void qa(){
        if(!BuildConfig.DM_QA_BUILD)return;
        TestHooks.overlay("combat_log",!isOpen() || tab!=TAB_LOG?"closed":streakView && streaks!=null?"streaks_"+streaks.count:dpsView && dps!=null?"dps_"+dps.fights+"_"+dps.segments.size()+"_"+dpsSel:logFailed?"failed":log==null || logBusy?"loading":logFight<0?"list":logDetail==null?"loading":"fight_"+logFight+"_"+(graph!=null && graph.simEnemy!=null?"sim":"plain"));

        TestHooks.overlay("combat_best",!isOpen() || tab!=TAB_BEST?"closed":best==null?"reading":!best.usable()?best.state:best.failed()?"failed":best.done()?"done":"searching");

        StringBuilder ids=new StringBuilder();
        if(isOpen() && tab==TAB_BEST && best!=null && best.result && page<bestPlan.size())for(int i=bestPlan.get(page).rowFrom;i<bestPlan.get(page).rowTo && i<best.rows.size();i++)ids.append(ids.length()==0?"":"_").append(best.rows.get(i).id);
        TestHooks.overlay("combat_best_rows",ids.length()==0?"none":ids.toString());
        TestHooks.overlay("combat_best_charms",bestCharms?"on":"off");
        TestHooks.overlay("combat_best_equip",!isOpen() || tab!=TAB_BEST || best==null?"none":BestGearEquip.overlay(best.stash,equipBusy,equipJob,equipResult,stockReading));
        TestHooks.overlay("combat_best_tip",tipId==0?"closed":tipLines==null?"loading_"+tipId:"open_"+tipId+"_"+(tooltip==null?1:tooltip.pages()));
        String state=!isOpen()?"closed":tab==TAB_BEST?(best==null?"loading":"open"):tab==TAB_LOG?(log==null?(logFailed?"failed":"loading"):"open"):failed?"failed":(selected<0?model:detail)==null || stale?"loading":"open";
        if(state.equals("open") && callsShown<0)callsShown=calls;
        TestHooks.overlay("combat_calls",callsShown<0?"none":Integer.toString(callsShown));
        TestHooks.overlay(this,"combat",state);
    }
    @Override protected void onVisibilityChanged(View changed,int visibility){super.onVisibilityChanged(changed,visibility);qa();DmActivity.padPagesChanged();}
}
