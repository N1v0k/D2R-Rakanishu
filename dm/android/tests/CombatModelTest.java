package com.xiaoboluo.d2r;

final class CombatModelTest {
    private static void check(boolean value){if(!value)throw new AssertionError();}
    private static void fightTests(String hero,String row){

        StringBuilder t=new StringBuilder(hero+row+"belt\t1\t3\t0\t0\t0\t0\t2\t0\t0\t0\t2\t0\n");
        t.append("fight\t1\t40\t4\t11\t7\nact\t0\t0\nact\t1\t7\n");
        for(int i=0;i<11;i++){float e=100-i*10,lo=Math.max(0,e-10),l=100-i*5,m=Math.max(0,90-i*15);
            t.append("trace\t"+i+"\t"+lo+","+e+","+lo+"\t"+(l-5)+","+l+","+(l-5)+"\t"+m+","+m+","+m+"\t"+(i>=5?1:0)+"\t"+(i==0?"3":i%2==0?"2":"0")+"\n");}
        t.append("drink\t24\t1\t40.0\t10.0\n");
        CombatModel m=CombatModel.parse(t.toString());
        CombatModel.Fight f=m.fight;
        check(f!=null && f.n==11 && f.width==4 && f.endKind==CombatModel.END_KILL && f.actSkill[1]==7 && f.drinks.size()==1 && f.acts[0]==3);
        check(m.belt[1]==3 && m.belt[10]==2);

        String[] groups={"Heal","Mana","Rejuv"};
        check(CombatGraph.belt(m.belt,groups).equals("Heal ×4 · Mana ×2 · Rejuv ×2"));
        check(CombatGraph.belt(new int[]{1,0,0,0,0,0,0,0,0,0,0,0},groups).equals("Heal ×1"));
        check(CombatGraph.belt(new int[]{0,0,0,0,0,2,0,0,0,0,0,1},groups).equals("Mana ×2 · Rejuv ×1") && CombatGraph.belt(new int[12],groups).isEmpty());
        CombatGraph.Words words=new CombatGraph.Words(){
            public String skill(int id){return id==0?"Normaler Angriff":id==7?"Testzauber":"?"+id;}
            public boolean icon(int id){return id==7;}
            public String shortage(String skill){return "Mana reicht nicht für "+skill;}
            public String potion(int group){return groups[group];}
        };
        CombatGraph g=CombatGraph.build(f,1000,300,2600,1100,1,20,s->s.length()*10,words,"Testgegner","Dein Leben","Kill 1,6 s",true,new int[]{1,2,3,4});
        java.util.List<String> legend=new java.util.ArrayList<>();for(CombatGraph.Entry e:g.legend)legend.add(e.text);

        check(legend.equals(java.util.Arrays.asList("Testgegner","Dein Leben","Normaler Angriff","Testzauber","Mana reicht nicht für Testzauber","Heal ×1")));
        check(g.legend.get(3).icon.equals("builds/skills/7") && g.legend.get(2).icon==null && g.legend.get(5).icon.equals("hp2"));
        check(g.legend.get(2).colour==CombatGraph.ATTACK_TICK && g.legend.get(3).colour==CombatGraph.TICKS[7%CombatGraph.TICKS.length]);

        int[] other=new int[64];java.util.Arrays.fill(other,-1);other[0]=9;other[1]=0;other[2]=7;
        check(CombatGraph.tickColours(other)[2]==g.legend.get(3).colour && CombatGraph.tickColours(other)[1]==CombatGraph.ATTACK_TICK);
        other[0]=3;check(CombatGraph.tickColours(other)[0]==CombatGraph.TICKS[3] && CombatGraph.tickColours(other)[2]==CombatGraph.TICKS[0]);
        check(g.manaShown && g.plotL>1000 && g.plotR<2600 && g.plotT<g.plotB && g.plotB<g.manaT && g.manaT<g.manaB && g.manaB<g.laneT && g.laneB<1100);

        float legendBottom=0;for(CombatGraph.Entry e:g.legend)legendBottom=Math.max(legendBottom,e.y+g.row);
        check(g.endBox[1]>=legendBottom && g.endBox[3]<g.plotT && Math.abs(g.endBox[2]-g.endX)<1e-3 && g.dash[1]>=g.endBox[3]-1e-3);
        for(float[] xy:new float[][]{g.enemy,g.life,g.mana})for(int i=0;i<xy.length;i+=2)check(xy[i]>=g.plotL-1e-3 && xy[i]<=g.plotR+1e-3);
        check(Math.abs(g.enemy[0]-g.plotL)<1e-3 && Math.abs(g.enemy[g.enemy.length-2]-g.plotR)<1e-3 && Math.abs(g.enemy[g.enemy.length-1]-g.plotB)<1e-3);
        for(int i=1;i<g.mana.length;i+=2)check(g.mana[i]>=g.manaT-1e-3 && g.mana[i]<=g.manaB+1e-3);
        check(g.shades.size()==1 && Math.abs(g.shades.get(0)[0]-(g.plotL+(g.plotR-g.plotL)*20/40f))<1e-3);
        check(g.ticks.size()==7 && g.markers.size()==1 && g.markers.get(0).icon.equals("hp2"));
        check(g.endBox!=null && g.endText.equals("Kill 1,6 s") && Math.abs(g.endX-g.plotR)<1e-3 && g.dash.length>0);
        boolean zero=false,mana=false;for(CombatGraph.Label l:g.labels){zero|=l.text.equals("0 s");mana|=l.text.equals("Mana");}
        check(zero && mana);
        check(CombatGraph.seconds(0.25,true).equals("0,25 s") && CombatGraph.seconds(5,true).equals("5 s") && CombatGraph.seconds(1.5,false).equals("1.5 s"));
        check(CombatGraph.step(1.6)==0.5 && CombatGraph.step(30)==5 && CombatGraph.step(600)==120);

        String plain=t.toString().replace("fight\t1\t40","fight\t0\t40").replaceAll("(?m)^(trace\t.*)\t[01]\t[0-9a-f]+$","$1\t0\t2").replace("drink\t24\t1\t40.0\t10.0\n","");
        CombatModel.Fight p=CombatModel.parse(plain).fight;
        CombatGraph q=CombatGraph.build(p,1000,300,2600,1100,1,20,s->s.length()*10,words,"Testgegner","Dein Leben","",true,new int[]{1,2,3,4});
        check(q.legend.size()==3 && q.legend.get(2).text.equals("Testzauber") && q.endBox==null && q.shades.isEmpty() && q.markers.isEmpty());

        String unnamed=t.toString().replace("\t1\t40\t4\t11\t7","\t1\t40\t4\t11\t-1");
        CombatGraph u=CombatGraph.build(CombatModel.parse(unnamed).fight,1000,300,2600,1100,1,20,s->s.length()*10,words,"Testgegner","Dein Leben","Kill 1,6 s",true,new int[]{1,2,3,4});
        check(u.shades.isEmpty() && u.manaShown);for(CombatGraph.Entry e:u.legend)check(e.kind!=CombatGraph.SHADE);
        String still=plain.replaceAll("(?m)^(trace\t\\d+\t[^\t]+\t[^\t]+)\t[^\t]+","$1\t100.0,100.0,100.0");
        CombatGraph v=CombatGraph.build(CombatModel.parse(still).fight,1000,300,2600,1100,1,20,s->s.length()*10,words,"Testgegner","Dein Leben","",true,new int[]{1,2,3,4});
        check(!v.manaShown && v.mana.length==0 && v.plotB>g.plotB && v.plotB<v.laneT && v.axes.length==8);
        for(CombatGraph.Label l:v.labels)check(!l.text.equals("Mana"));

        try {CombatModel.parse(t.toString().replace("trace\t10\t","trace\t11\t"));throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {CombatModel.parse(t.toString().replace("\t1\t40\t4\t11\t7","\t1\t40\t4\t12\t7"));throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {CombatModel.parse(t.toString().replace("drink\t24\t1\t40.0","drink\t24\t12\t40.0"));throw new AssertionError();}catch(IllegalArgumentException expected){}
        check(CombatModel.parse(hero+"belt\t-1\n").beltUnreadable);
    }

    private static void optionTests(){
        java.util.List<Integer> order=new java.util.ArrayList<>();
        int[] sent=CombatModel.options(0,1,false,false,0,-1,false,order);
        check(sent.length==8 && sent[6]==0 && sent[5]==-1);
        order.add(42);order.add(0);
        check(java.util.Arrays.equals(sent,CombatModel.options(0,1,false,false,0,-1,false,order)));
        int[] own=CombatModel.options(2,3,true,true,4,7,true,order);
        check(java.util.Arrays.equals(own,new int[]{2,3,1,1,4,7,1,0,42,0}));
    }

    private static void languageTests(){
        check(CombatModel.light(0,CombatModel.L_PT).equals("Vitória clara") && CombatModel.light(1,CombatModel.L_DE).equals("Besiegbar"));
        check(CombatModel.time(12.25,CombatModel.L_PT).equals("12,3 s") && CombatModel.time(12.25,CombatModel.L_EN).equals("12.3 s"));
        check(CombatModel.difficultyLabel(2,CombatModel.L_PT).equals("Dificuldade: Inferno") && CombatModel.heraldLabel(0,CombatModel.L_PT).equals("Arauto: desligado"));
    }



    private static void miniBossTests(String hero){
        String[][] rows={{"10","regular","0","1","3"},{"11","late","4","1","9"},{"12","here","4","1","3"},{"13","quest","6","1","5"},{"14","boss","5","1","37"},
            {"15","elsewhere","4","2","40"},{"16","early","4","1","2"}};
        StringBuilder t=new StringBuilder(hero);
        for(String[] r:rows)t.append("enemy\t").append(r[0]).append('\t').append(r[1]).append('\t').append(r[2]).append('\t').append(r[3]).append('\t').append(r[4])
            .append("\t30\t700\t40.0\t18\t25\t0.2\t10\t1\t0\t0\t0\t0,0,0,0,0,0\n");
        CombatModel m=CombatModel.parse(t.toString());
        check(m.act==1 && m.area==3 && m.refArea==0);
        StringBuilder keys=new StringBuilder();for(CombatModel.Enemy e:m.miniBosses())keys.append(e.key).append(' ');
        check(keys.toString().equals("here quest late early "));

        StringBuilder town=new StringBuilder(hero.replace("\t3\t0\n","\t1\t5\n"));
        for(String[] r:rows)town.append("enemy\t").append(r[0]).append('\t').append(r[1]).append('\t').append(r[2]).append('\t').append(r[3]).append('\t').append(r[4])
            .append("\t30\t700\t40.0\t18\t25\t0.2\t10\t1\t0\t0\t0\t0,0,0,0,0,0\n");
        StringBuilder tk=new StringBuilder();for(CombatModel.Enemy e:CombatModel.parse(town.toString()).miniBosses())tk.append(e.key).append(' ');
        check(tk.toString().equals("quest late early here "));
        CombatModel.Names names=(key,lang)->key;
        StringBuilder minis=new StringBuilder();for(CombatModel.Enemy e:m.filter("",0,CombatModel.SUPERUNIQUE,0,false,names))minis.append(e.key).append(' ');
        check(minis.toString().equals("early here quest late elsewhere "));
        check(m.filter("",0,CombatModel.ACT_BOSS,0,false,names).size()==1 && m.filter("",0,CombatModel.ACT_BOSS,0,false,names).get(0).key.equals("boss"));
        try {CombatModel.parse(hero.replace("\t3\t0\n","\t3\n"));throw new AssertionError();}catch(IllegalArgumentException expected){}


        int[] areas={3,18,9,4,5,25,30,27,33,28,39,8};
        StringBuilder act1=new StringBuilder(hero.replace("\t3\t0\n","\t8\t0\n"));
        for(int i=0;i<areas.length;i++)act1.append("enemy\t").append(20+i).append("\tsu").append(areas[i]).append("\t4\t1\t").append(areas[i])
            .append("\t30\t700\t40.0\t18\t25\t0.2\t10\t1\t0\t0\t0\t0,0,0,0,0,0\n");
        java.util.List<CombatModel.Enemy> all=CombatModel.parse(act1.toString()).miniBosses();
        int[] want={8,9,18,25,27,28,30,33,39,3,4,5};
        boolean order=all.size()==want.length;for(int i=0;i<want.length && order;i++)order=all.get(i).area==want[i];
        check(order);
    }
    public static void main(String[] args){
        optionTests();languageTests();
        String hero="hero\tTest\t1\t30\t2\t1\t500\t200\t0\t0\t0\t8\t1\t1\t3\t0\n";
        String row="enemy\t4\tkey\t0\t1\t5\t30\t700\t40.0\t18\t25\t0.2\t10\t1\t0\t1\t2\t0,100,0,0,0,0\n";
        CombatModel m=CombatModel.parse(hero+row+"rotation\t40\t2\t-1\t1\t0\nskill\t1\tname\t40\t2\t3\t5\t0\n");
        check(m.enemies.size()==1 && m.enemies.get(0).underestimated);
        check(m.sequence.size()==2 && m.flags==2);
        CombatModel.Names names=(key,lang)->lang==0?"Prüfgegner":lang==1?"Test enemy":"Inimigo";
        check(m.filter("pruf",0,-1,0,false,names).size()==1);
        check(m.filter("test",0,-1,0,false,names).size()==1);
        check(m.filter("inimigo",0,-1,0,false,names).size()==1);
        CombatModel.Names korean=(key,lang)->lang==5?"시험 적":"Test enemy";
        check(m.filter("시험",0,-1,0,false,korean).size()==1);
        check(!CombatModel.displayedPositive(0.02));
        check(CombatModel.displayedPositive(0.1));
        check(CombatModel.number(13.3,true).equals("13,3"));
        check(CombatModel.number(13.3,false).equals("13.3"));
        check(m.rotationOrder().equals(java.util.Arrays.asList(1)));

        CombatModel summon=CombatModel.parse(hero+row+"rotation\t40\t8194\t-1\t0\nskill\t0\tattack\t30\t2\t-1\t0\t0\nskill\t70\tsummon\t10\t8450\t-1\t6\t2\n");
        check(summon.rotationOrder().equals(java.util.Arrays.asList(0)) && !summon.manaUsed);
        check(summon.skills.get(1).minion() && summon.skills.get(1).pets==2 && !summon.skills.get(0).minion());
        check(CombatModel.minionLine("X",2,10,CombatModel.L_DE).equals("X: 2 Diener · 10,0 DPS"));
        check(CombatModel.minionLine("X",1,0.5,CombatModel.L_EN).equals("X: 1 minion · 0.5 DPS"));
        check(CombatModel.minionLine("X",3,4,CombatModel.L_PT).equals("X: 3 lacaios · 4,0 DPS"));

        try {CombatModel.parse(hero+row+"skill\t70\tsummon\t10\t8450\t-1\t6\t0\n");throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {CombatModel.parse(hero+row+"skill\t70\tsummon\t10\t258\t-1\t6\t2\n");throw new AssertionError();}catch(IllegalArgumentException expected){}
        check(m.filter("test",2,-1,0,false,names).isEmpty());
        check(CombatModel.pages(0,8)==1 && CombatModel.pages(9,8)==2);
        check(CombatModel.time(-2,true).equals("nicht verfügbar"));
        check(CombatModel.manaSupply(-1,true).equals("reicht dauerhaft"));
        check(CombatModel.manaSupply(-1,false).equals("sustainable"));
        check(CombatModel.manaSupply(3.4,true).equals("reicht für 3,4 s"));
        check(CombatModel.manaSupply(0,true).equals("zu wenig für einen Einsatz"));
        check(CombatModel.manaSupply(.04,true).equals("reicht für <0,1 s"));
        check(CombatModel.manaSupply(.04,false).equals("lasts <0.1 s"));

        CombatModel.Names portuguese=(key,lang)->lang==2?"Inimigo Ígneo":lang==1?"Test enemy":"Testgegner";
        check(m.filter("igneo",0,-1,0,false,portuguese).size()==1);
        check(m.filter("Ígneo",0,-1,0,false,portuguese).size()==1);
        check(CombatModel.fold("Pão São Ângelo").equals("pao sao angelo") && CombatModel.fold("Straße").equals("strasse"));

        check(CombatModel.nextPlayers(-1)==-1);
        for(int players=1;players<=8;players++)check(CombatModel.nextPlayers(players)>=1 && CombatModel.nextPlayers(players)<=8);
        check(CombatModel.nextPlayers(8)==1 && CombatModel.nextPlayers(3)==4);

        check(CombatModel.heraldLabel(0,true).equals("Herold: aus") && CombatModel.heraldLabel(3,true).equals("Herold: Stufe 3"));
        check(CombatModel.heraldLabel(0,false).equals("Herald: off"));
        check(CombatModel.playersLabel(-1,true).equals("Spielerzahl: …") && CombatModel.playersLabel(3,true).equals("Spielerzahl: 3"));
        check(CombatModel.difficultyLabel(0,true).equals("Schwierigkeit: Normal") && CombatModel.difficultyLabel(2,false).equals("Difficulty: Hell"));
        check(CombatModel.rotationLabel(true,true).equals("Rotation: eigene") && CombatModel.rotationLabel(false,true).equals("Rotation: Auto"));
        check(CombatModel.light(0,true).equals("Klarer Sieg"));
        check(CombatModel.light(1,true).equals("Besiegbar"));
        check(CombatModel.light(2,true).equals("Kaum Chance"));
        try {CombatModel.parse(hero+row.replace("40.0","NaN"));throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {CombatModel.parse(row);throw new AssertionError();}catch(IllegalArgumentException expected){}
        check(CombatLayout.column(20,1020,0)==20 && CombatLayout.column(20,1020,4)==1020);
        check(CombatLayout.column(20,1020,1)==420 && CombatLayout.column(20,1020,2)==620);
        check(CombatLayout.rows(200,640,88)==5 && CombatLayout.rows(200,100,88)==1);

        check(CombatLayout.padReady(true,"log",null) && !CombatLayout.padReady(true,null,"result"));
        check(CombatLayout.padReady(false,null,"result") && !CombatLayout.padReady(false,"log",null));
        miniBossTests(hero);
        fightTests(hero,row);
        QuickModel q=new QuickModel();q.setEntries(true,true,true,true);check(q.count()==6 && q.indexOf(QuickModel.COMBAT)>=0);
        System.out.println("CombatModelTest: parsing, DE/EN search, filters, paging, sentinels, fight graph and menu passed");
    }
}
