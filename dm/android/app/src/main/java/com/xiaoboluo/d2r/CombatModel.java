package com.xiaoboluo.d2r;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

final class CombatModel {
    static final int UNAVAILABLE=1, ESTIMATE=2, MODIFIER=32, UTILITY=64, MINION=8192;

    static final int REGULAR=0, SUPERUNIQUE=4, ACT_BOSS=5, QUEST_BOSS=6, EXCLUDED=10;

    static final int HANDS_FIRST=1, HANDS_SECOND=2;
    static final class Enemy {
        int id, kind, act, area, level, life, light, reason, flags;
        String key;

        String look;

        String kindKey;

        boolean estimated;
        double dps, kill, death, maxHit, hits;
        boolean underestimated;
        int[] resistance=new int[6];
    }
    static final class Skill {
        int id, flags, pets; String name; double dps, manaSeconds, mana;

        boolean minion(){return (flags&MINION)!=0;}
    }
    static final int POTIONS=12, END_NONE=0, END_KILL=1, END_DEATH=2, SERIES=3, ENEMY=0, LIFE=1, MANA=2;

    static final class Fight {
        int endKind, end, width, n, best, filled;
        int[] actSkill=new int[64];
        float[][] lo, hi, last;
        boolean[] shortage;
        long[] acts;
        final List<Drink> drinks=new ArrayList<>();
        float seconds(int frame){return frame/25f;}
    }
    static final class Drink { int frame, kind; float life, mana; }
    String name="";
    int cls, level, difficulty, act, area, refArea, life, mana, rift, rush, nextBoss, session;

    int magicFind;
    double rotation, manaSeconds, incoming, sustain;
    int flags, players; boolean itemCharges, rotw, manaUsed, beltUnreadable;

    boolean elysium;
    int[] belt;
    Fight fight;

    Stats stats;
    static final int ATTACK_A1=0, ATTACK_A2=1, ATTACK_SKILL=2, ATTACK_OTHER=3;
    static final class Stats {
        int level, lifeMin, lifeMax, defense, block, drain;

        final List<int[]> attacks=new ArrayList<>();

        final List<int[]> elements=new ArrayList<>();
        final List<Integer> skills=new ArrayList<>(), mods=new ArrayList<>();
        boolean undead, demon, melee, ranged;
    }
    private static List<int[]> tuples(String v,int n,int[] lo,int[] hi){
        List<int[]> out=new ArrayList<>();if(v.equals("-"))return out;
        for(String t:v.split(";")){String[] f=t.split(",");if(f.length!=n)throw new IllegalArgumentException();int[] x=new int[n];
            for(int k=0;k<n;k++){x[k]=i(f[k]);if(x[k]<lo[k] || x[k]>hi[k])throw new IllegalArgumentException();}out.add(x);}
        return out;
    }
    private static Stats stats(String[] v){
        if(v.length!=12)throw new IllegalArgumentException();
        Stats s=new Stats();s.level=i(v[1]);s.lifeMin=i(v[2]);s.lifeMax=i(v[3]);s.defense=i(v[4]);s.block=i(v[5]);s.drain=i(v[6]);
        if(s.level<0 || s.lifeMin<0 || s.lifeMax<s.lifeMin || s.defense<0 || s.block<0 || s.block>100 || s.drain<0)throw new IllegalArgumentException();
        int big=Integer.MAX_VALUE;
        s.attacks.addAll(tuples(v[7],4,new int[]{0,0,0,0},new int[]{2,big,big,big}));
        s.elements.addAll(tuples(v[8],6,new int[]{0,0,0,0,0,0},new int[]{3,5,big,big,100,big}));
        for(int[] a:s.attacks)if(a[1]>a[2])throw new IllegalArgumentException();
        for(int[] e:s.elements)if(e[2]>e[3])throw new IllegalArgumentException();
        if(!v[9].equals("-"))for(String t:v[9].split(","))s.skills.add(i(t));
        if(!v[10].equals("-"))for(String t:v[10].split(","))s.mods.add(i(t));
        if(!v[11].matches("-|u?d?m?r?"))throw new IllegalArgumentException();
        s.undead=v[11].contains("u");s.demon=v[11].contains("d");s.melee=v[11].contains("m");s.ranged=v[11].contains("r");
        return s;
    }
    final List<Integer> sequence=new ArrayList<>();
    final List<Enemy> enemies=new ArrayList<>();
    final List<Skill> skills=new ArrayList<>();

    final List<int[]> hands=new ArrayList<>();
    final double[] damage=new double[6], taken=new double[6];


    static int[] options(int diff,int players,boolean potions,boolean terror,int herald,int target,boolean ordered,List<Integer> order){
        int n=ordered?order.size():0;int[] opts=new int[8+n];
        opts[0]=diff;opts[1]=players;opts[2]=potions?1:0;opts[3]=terror?1:0;opts[4]=herald;opts[5]=target;opts[6]=ordered?1:0;
        for(int i=0;i<n;i++)opts[8+i]=order.get(i);
        return opts;
    }
    static CombatModel parse(String text) {
        CombatModel m=new CombatModel();boolean hero=false;
        for(String line:text.split("\n")) {
            String[] v=line.split("\t",-1);
            try {
                switch(v[0]) {
                case "hero":
                    if(v.length!=17)throw new IllegalArgumentException();
                    m.name=v[1];m.cls=i(v[2]);m.level=i(v[3]);m.difficulty=i(v[4]);m.act=i(v[5]);m.life=i(v[6]);m.mana=i(v[7]);m.rift=i(v[8]);m.rush=i(v[9]);m.nextBoss=i(v[10]);m.session=i(v[11]);m.rotw=i(v[12])!=0;m.players=i(v[13]);m.area=i(v[14]);m.refArea=i(v[15]);m.elysium=i(v[16])!=0;hero=true;break;
                case "enemy":
                    if(v.length!=21 || !v[19].matches("([MS][0-9]{1,5})?"))throw new IllegalArgumentException();
                    Enemy e=new Enemy();e.id=i(v[1]);e.key=v[2];e.kind=i(v[3]);e.act=i(v[4]);e.area=i(v[5]);e.level=i(v[6]);e.life=i(v[7]);e.dps=d(v[8]);e.kill=d(v[9]);e.death=d(v[10]);e.maxHit=d(v[11]);e.hits=d(v[12]);e.light=i(v[13]);e.reason=i(v[14]);e.underestimated=i(v[15])!=0;e.flags=i(v[16]);
                    String[] rs=v[17].split(",");if(rs.length!=6)throw new IllegalArgumentException();for(int k=0;k<6;k++)e.resistance[k]=i(rs[k]);e.look=v[18];e.kindKey=v[19];e.estimated=i(v[20])!=0;m.enemies.add(e);break;
                case "skill":
                    if(v.length!=8)throw new IllegalArgumentException();
                    Skill s=new Skill();s.id=i(v[1]);s.name=v[2];s.dps=d(v[3]);s.flags=i(v[4]);s.manaSeconds=d(v[5]);s.mana=d(v[6]);s.pets=i(v[7]);
                    if(s.pets<0 || s.minion()!=(s.pets>0))throw new IllegalArgumentException();
                    m.skills.add(s);break;
                case "loot":if(v.length!=2)throw new IllegalArgumentException();m.magicFind=i(v[1]);break;
                case "stats":m.stats=stats(v);break;
                case "note":if(v.length==2 && v[1].equals("charges"))m.itemCharges=true;break;
                case "hands":{
                    if(v.length!=3)throw new IllegalArgumentException();
                    int id=i(v[1]),kind=i(v[2]);
                    if(id<0 || (kind!=HANDS_FIRST && kind!=HANDS_SECOND))throw new IllegalArgumentException();
                    m.hands.add(new int[]{id,kind});break;}
                case "rotation":m.rotation=d(v[1]);m.flags=i(v[2]);m.manaSeconds=d(v[3]);for(int k=4;k<v.length;k++)m.sequence.add(i(v[k]));break;
                case "survival":m.incoming=d(v[1]);m.sustain=d(v[2]);break;
                case "taken":int element=i(v[1]);if(element>=0 && element<6)m.taken[element]=d(v[2]);break;
                case "damage":int el=i(v[1]);if(el>=0 && el<6)m.damage[el]=d(v[2]);break;
                case "belt":
                    if(v.length==2 && v[1].equals("-1")){m.beltUnreadable=true;break;}
                    if(v.length!=POTIONS+1)throw new IllegalArgumentException();
                    m.belt=new int[POTIONS];for(int k=0;k<POTIONS;k++){m.belt[k]=i(v[k+1]);if(m.belt[k]<0 || m.belt[k]>16)throw new IllegalArgumentException();}
                    break;
                case "fight":case "act":case "trace":case "drink":m.fight=fightLine(m.fight,v);break;
                default:break;
                }
            } catch(RuntimeException e) {throw new IllegalArgumentException("Invalid combat response",e);}
        }
        if(!hero || m.level<1 || m.difficulty<0 || m.difficulty>2)throw new IllegalArgumentException("No character snapshot");
        if(m.fight!=null && m.fight.filled!=m.fight.n)throw new IllegalArgumentException("Incomplete fight trace");
        for(Skill skill:m.skills)if(m.sequence.contains(skill.id) && skill.mana>0)m.manaUsed=true;
        return m;
    }

    static Fight fightLine(Fight fight,String[] v){
        switch(v[0]) {
        case "fight":{
            if(v.length!=6 || fight!=null)throw new IllegalArgumentException();
            Fight f=new Fight();f.endKind=i(v[1]);f.end=i(v[2]);f.width=i(v[3]);f.n=i(v[4]);f.best=i(v[5]);
            if(f.endKind<END_NONE || f.endKind>END_DEATH || f.n<1 || f.n>256 || f.width<1 || f.end<0 || f.end>=f.n*f.width)throw new IllegalArgumentException();
            java.util.Arrays.fill(f.actSkill,-1);
            f.lo=new float[SERIES][f.n];f.hi=new float[SERIES][f.n];f.last=new float[SERIES][f.n];f.shortage=new boolean[f.n];f.acts=new long[f.n];
            return f;}
        case "act":{
            int k=i(v[1]);if(v.length!=3 || fight==null || k<0 || k>=64)throw new IllegalArgumentException();
            fight.actSkill[k]=i(v[2]);return fight;}
        case "trace":{
            Fight f=fight;int k=i(v[1]);if(v.length!=7 || f==null || k!=f.filled || k>=f.n)throw new IllegalArgumentException();
            for(int q=0;q<SERIES;q++){String[] t=v[2+q].split(",");if(t.length!=3)throw new IllegalArgumentException();f.lo[q][k]=percent(t[0]);f.hi[q][k]=percent(t[1]);f.last[q][k]=percent(t[2]);}
            f.shortage[k]=i(v[5])!=0;f.acts[k]=Long.parseUnsignedLong(v[6],16);f.filled++;return f;}
        case "drink":{
            if(v.length!=5 || fight==null)throw new IllegalArgumentException();
            Drink x=new Drink();x.frame=i(v[1]);x.kind=i(v[2]);x.life=percent(v[3]);x.mana=percent(v[4]);
            if(x.kind<0 || x.kind>=POTIONS || x.frame<0)throw new IllegalArgumentException();
            fight.drinks.add(x);return fight;}
        default:throw new IllegalArgumentException();
        }
    }
    static int i(String s){return Integer.parseInt(s);}
    static double d(String s){double n=Double.parseDouble(s);if(!Double.isFinite(n))throw new IllegalArgumentException();return n;}
    static float percent(String s){double n=d(s);if(n<0 || n>100)throw new IllegalArgumentException();return (float)n;}

    interface Names {String get(String key,int language);}

    static boolean ofType(int kind,int type){boolean mini=kind==SUPERUNIQUE || kind==QUEST_BOSS;return type==SUPERUNIQUE?mini:type==ACT_BOSS?kind>=ACT_BOSS && !mini:kind==type;}

    static boolean miniBoss(Enemy e){return e.kind==SUPERUNIQUE || e.kind==QUEST_BOSS;}


    List<Enemy> miniBosses(){
        List<Enemy> out=new ArrayList<>();
        for(Enemy e:enemies)if(e.act==act && miniBoss(e))out.add(e);
        int key=refArea>0?refArea:area;
        out.sort(Comparator.comparingInt((Enemy e)->e.area==key?0:e.area>key?1:2).thenComparingInt(e->e.area));
        return out;
    }
    static boolean displayedPositive(double value){return value>0 && !number(value,false).equals("0.0");}
    static boolean damageSkill(Skill s){return displayedPositive(s.dps) && (s.flags&(UNAVAILABLE|MODIFIER|UTILITY))==0;}
    List<Integer> rotationOrder(){
        List<Integer> result=new ArrayList<>();
        for(int id:sequence)for(Skill s:skills)if(s.id==id && damageSkill(s) && !s.minion() && !result.contains(id))result.add(id);
        List<Skill> rest=new ArrayList<>(skills);rest.sort(Comparator.comparingDouble((Skill s)->s.dps).reversed());
        for(Skill s:rest)if(damageSkill(s) && !s.minion() && !result.contains(s.id))result.add(s.id);
        return result;
    }
    static String fold(String s){return s==null?"":Normalizer.normalize(s.toLowerCase(Locale.ROOT),Normalizer.Form.NFD).replaceAll("\\p{M}+","").replace("ß","ss");}
    static int pages(int n,int size){return Math.max(1,(n+Math.max(1,size)-1)/Math.max(1,size));}

    static final int L_DE=0, L_EN=1, L_PT=2;
    static String pick(int lang,String german,String english,String portuguese){return lang==L_DE?german:lang==L_PT?portuguese:english;}
    static String number(double v,boolean de){return number(v,de?L_DE:L_EN);}
    static String number(double v,int lang){return String.format(lang==L_EN?Locale.ROOT:Locale.GERMAN,"%.1f",v);}
    static String time(double v,boolean de){return time(v,de?L_DE:L_EN);}
    static String time(double v,int lang){return v==-2?pick(lang,"nicht verfügbar","unavailable","indisponível"):v<0?"> 600 s":number(v,lang)+" s";}

    static String minionLine(String name,int pets,double dps,int lang){
        String count=pets+" "+(pets==1?pick(lang,"Diener","minion","lacaio"):pick(lang,"Diener","minions","lacaios"));
        return name+": "+count+" · "+number(dps,lang)+" DPS";
    }

    static String manaSupply(double v,boolean de){return manaSupply(v,de?L_DE:L_EN);}
    static String manaSupply(double v,int lang){return v<0?pick(lang,"reicht dauerhaft","sustainable","sustentável"):v==0?pick(lang,"zu wenig für einen Einsatz","not enough for one use","insuficiente para um uso"):pick(lang,"reicht für ","lasts ","dura ")+(!displayedPositive(v)?(lang==L_EN?"<0.1 s":"<0,1 s"):time(v,lang));}

    static int nextPlayers(int players){return players<1?players:players%8+1;}
    static String difficultyLabel(int diff,boolean de){return difficultyLabel(diff,de?L_DE:L_EN);}
    static String difficultyLabel(int diff,int lang){return pick(lang,"Schwierigkeit: ","Difficulty: ","Dificuldade: ")+(diff<0?"…":diff==1?pick(lang,"Albtraum","Nightmare","Pesadelo"):diff==2?pick(lang,"Hölle","Hell","Inferno"):"Normal");}
    static String playersLabel(int players,boolean de){return playersLabel(players,de?L_DE:L_EN);}
    static String playersLabel(int players,int lang){return pick(lang,"Spielerzahl: ","Players: ","Jogadores: ")+(players<1?"…":Integer.toString(players));}
    static String heraldLabel(int tier,boolean de){return heraldLabel(tier,de?L_DE:L_EN);}
    static String heraldLabel(int tier,int lang){return tier>0?pick(lang,"Herold: Stufe ","Herald: tier ","Arauto: nível ")+tier:pick(lang,"Herold: aus","Herald: off","Arauto: desligado");}
    static String rotationLabel(boolean ordered,boolean de){return rotationLabel(ordered,de?L_DE:L_EN);}
    static String rotationLabel(boolean ordered,int lang){return ordered?pick(lang,"Rotation: eigene","Rotation: custom","Rotação: própria"):pick(lang,"Rotation: Auto","Rotation: Auto","Rotação: auto");}
    static String light(int value,boolean de){return light(value,de?L_DE:L_EN);}
    static String light(int value,int lang){return value==0?pick(lang,"Klarer Sieg","Clear victory","Vitória clara"):value==1?pick(lang,"Besiegbar","Beatable","Vencível"):value==2?pick(lang,"Kaum Chance","Little chance","Pouca chance"):pick(lang,"Offen","Unknown","Incerto");}
    static String colour(int value){return value==0?"FontColorPartyGreen":value==1?"FontColorOrange":value==2?"FontColorRed":"FontColorLightGray";}
}
