package com.xiaoboluo.d2r;

import java.awt.Font;
import java.awt.font.FontRenderContext;
import java.awt.font.TextAttribute;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;





final class CombatLayoutTest {
    private static int checks;
    private static void check(boolean value,String what){checks++;if(!value)throw new AssertionError(what);}


    private static void valid(CombatLayout.Title t,String name,String rating,float room,float k,float minK,CombatLayout.Measure m,String what){
        String shown=t.second.isEmpty()?t.first:t.first+" "+t.second;
        if(t.whole())check(m.width(t.first+t.rating,t.nameK)<=room+1e-3f,what+": one piece "+m.width(t.first+t.rating,t.nameK)+" > "+room);
        check(t.rating.equals(CombatLayout.SEPARATOR+rating) || t.ratingLine==1 && t.ratingX==0 && t.rating.equals(rating),what+": rating "+t.rating);
        check(t.ratingK==k && t.nameK>=minK-1e-6f,what+": sizes "+t.nameK+" "+t.ratingK);
        check(m.width(t.first,t.nameK)<=room+1e-3f,what+": first line "+m.width(t.first,t.nameK)+" > "+room);
        check(t.ratingX+m.width(t.rating,t.ratingK)<=room+1e-3f,what+": rating ends at "+(t.ratingX+m.width(t.rating,t.ratingK))+" > "+room);
        String last=t.ratingLine==0?t.first:t.second;
        check(t.ratingX>=(last.isEmpty()?0:m.width(last,t.nameK))-1e-3f,what+": rating overlaps the name");
        if(t.cut)check(shown.endsWith(CombatLayout.CUT) && name.replace(" ","").startsWith(shown.substring(0,shown.length()-CombatLayout.CUT.length()).replace(" ","")),what+": cut "+shown);
        else check(shown.replace(" ","").equals(name.replace(" ","")),what+": name kept "+shown);
    }

    private static void madeUp(){

        CombatLayout.Measure m=(s,k)->s.codePointCount(0,s.length())*10*k;
        CombatLayout.Measure round=(s,k)->{float w=0;for(int i=0;i<s.length();i=s.offsetByCodePoints(i,1))w+=(float)Math.ceil(10*k+(k==1?0:.4f));return w;};
        String rating="Rating";
        for(CombatLayout.Measure x:new CombatLayout.Measure[]{m,round}){

            CombatLayout.Title t=CombatLayout.title("Short Name",rating,300,1,.8f,.8f,1,true,x);
            check(t.whole() && t.lines()==1 && t.first.equals("Short Name") && t.rating.equals(" · Rating"),"fits");
            valid(t,"Short Name",rating,300,1,.8f,x,"fits");

            t=CombatLayout.title("Twelve Chars",rating,200,1,.8f,.8f,1,true,x);
            check(t.lines()==1 && !t.whole() && t.nameK<1 && t.nameK>=.8f,"shrinks "+t.nameK);
            valid(t,"Twelve Chars",rating,200,1,.8f,x,"shrinks");


            String name="Alpha Beta Gamma Delta";
            t=CombatLayout.title(name,rating,200,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && t.ratingLine==1 && t.nameK==1 && !t.cut && t.first.equals("Alpha Beta Gamma") && t.second.equals("Delta"),"wraps "+t.first+"|"+t.second);
            valid(t,name,rating,200,1,.8f,x,"wraps");

            name="Abcdefghijkl Mnopqrstuvwx";
            t=CombatLayout.title(name,rating,200,1,.95f,.8f,1,true,x);
            check(t.lines()==2 && t.nameK<1 && t.nameK>=.8f && !t.cut && t.first.equals("Abcdefghijkl") && t.second.equals("Mnopqrstuvwx"),"wraps smaller "+t.nameK);
            valid(t,name,rating,200,1,.8f,x,"wraps smaller");
            t=CombatLayout.title(name,rating,200,1,.95f,.95f,1,true,x);
            check(t.lines()==2 && t.nameK==.95f && t.cut,"wrapK holds "+t.nameK);
            valid(t,name,rating,200,1,.95f,x,"wrapK holds");

            name="Abcdefghijkl-Mnopqrstuvw";
            t=CombatLayout.title(name,rating,200,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && t.nameK==1 && t.first.equals("Abcdefghijkl-") && t.second.equals("Mnopqrstuvw"),"hyphen "+t.first+"|"+t.second);
            valid(t,name,rating,200,1,.8f,x,"hyphen");

            name="Alpha Beta Gamma Deltadeltadelta";
            t=CombatLayout.title(name,rating,200,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && t.cut && t.second.endsWith("...") && t.rating.equals(" · Rating"),"cut on the second line "+t.second);
            valid(t,name,rating,200,1,.8f,x,"cut second");

            t=CombatLayout.title(name,rating,200,1,.8f,.8f,1,false,x);
            check(t.lines()==1 && t.cut && t.first.startsWith("Alpha") && t.first.endsWith("...") && t.rating.equals(" · Rating"),"cut on one line "+t.first);
            valid(t,name,rating,200,1,.8f,x,"cut one line");

            t=CombatLayout.title("Abcdefghijklmnopqrst",rating,180,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && t.second.isEmpty() && t.ratingLine==1 && t.ratingX==0 && t.rating.equals(rating) && !t.cut,"one word, rating below");
            valid(t,"Abcdefghijklmnopqrst",rating,180,1,.8f,x,"one word");

            t=CombatLayout.title("Abcdefghijklmn",rating,180,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && t.nameK==1 && t.ratingLine==1 && !t.whole() && t.rating.equals(rating),"one word in full size, rating below");
            valid(t,"Abcdefghijklmn",rating,180,1,.8f,x,"one word full size");

            name="一二三四五六七八九十一二三四五六七八九十一二三四五";
            t=CombatLayout.title(name,rating,180,1,.8f,.8f,1,true,x);
            check(t.lines()==2 && !t.cut && (t.first+t.second).equals(name),"characters "+t.first+"|"+t.second);
            valid(t,name,rating,180,1,.8f,x,"characters");

            t=CombatLayout.title("Twelve Chars",rating,220,1.3f,1,1,1,false,x);
            check(t.ratingK==1 && t.nameK<1.3f && !t.cut,"banner");
            valid(t,"Twelve Chars",rating,220,1,1,x,"banner");
        }

        CombatLayout.Measure steps=(s,k)->(float)Math.ceil(s.length()*10*k)+(k<1?2:0);
        float f=CombatLayout.shrink("Twelve Chars",100,1,.5f,steps);
        check(f>0 && steps.width("Twelve Chars",f)<=100,"shrink past rounding "+f);
        check(CombatLayout.shrink("Twelve Chars",40,1,.5f,steps)==0,"shrink floor");

        check(CombatLayout.titleLeft(0,1000,1)==1000/5f-8+24 && CombatLayout.titleRight(0,1000,1,true)==440-24 && CombatLayout.titleRight(0,1000,1,false)==1000,"title room");
    }





    private static final class Face {
        final Font own,global;final float spacing;final FontRenderContext frc=new FontRenderContext(null,true,true);
        final Map<Float,Font[]> sized=new HashMap<>();
        Face(Font own,Font global,float kerning){
            this.own=own;this.global=global;
            Font hundred=own.deriveFont(100f);
            float em=(float)hundred.getStringBounds("0123456789",frc).getWidth()/10/100;
            spacing=GameStyleGeometry.letterSpacing(kerning,em);
        }
        boolean covers(String s){for(int i=0;i<s.length();i=s.offsetByCodePoints(i,1)){int c=s.codePointAt(i);if(!own.canDisplay(c) && !global.canDisplay(c))return false;}return true;}
        float width(String s,float px,boolean whole){
            Font[] f=sized.computeIfAbsent(px,p->{Map<TextAttribute,Object> a=new HashMap<>();a.put(TextAttribute.SIZE,p);a.put(TextAttribute.TRACKING,spacing);return new Font[]{own.deriveFont(a),global.deriveFont(a)};});
            float w=0;
            for(int i=0;i<s.length();){
                int c=s.codePointAt(i),n=s.offsetByCodePoints(i,1);
                Font use=own.canDisplay(c) || !global.canDisplay(c)?f[0]:f[1];
                if(whole){int j=n;while(j<s.length() && (own.canDisplay(s.codePointAt(j)) || !global.canDisplay(s.codePointAt(j)))==(use==f[0]))j=s.offsetByCodePoints(j,1);
                    w+=(float)use.getStringBounds(s.substring(i,j),frc).getWidth();i=j;}
                else{w+=(float)Math.round(use.getStringBounds(s.substring(i,n),frc).getWidth());i=n;}
            }
            return w;
        }
    }


    private static Map<String,String[]> names(String json){
        Map<String,String[]> out=new LinkedHashMap<>();
        int[] at={json.indexOf('{')+1};
        while(true){
            skip(json,at);if(json.charAt(at[0])=='}')break;
            String key=string(json,at);skip(json,at);at[0]++;skip(json,at);at[0]++;
            List<String> v=new ArrayList<>();
            while(true){skip(json,at);if(json.charAt(at[0])==']'){at[0]++;break;}v.add(string(json,at));skip(json,at);if(json.charAt(at[0])==',')at[0]++;}
            out.put(key,v.toArray(new String[0]));
            skip(json,at);if(json.charAt(at[0])==',')at[0]++;
        }
        return out;
    }
    private static void skip(String s,int[] at){while(Character.isWhitespace(s.charAt(at[0])))at[0]++;}
    private static String string(String s,int[] at){
        StringBuilder b=new StringBuilder();int i=at[0]+1;
        while(s.charAt(i)!='"'){char c=s.charAt(i++);if(c!='\\'){b.append(c);continue;}
            char e=s.charAt(i++);
            switch(e){case 'n':b.append('\n');break;case 't':b.append('\t');break;case 'r':b.append('\r');break;case 'b':b.append('\b');break;case 'f':b.append('\f');break;
                case 'u':b.append((char)Integer.parseInt(s.substring(i,i+4),16));i+=4;break;default:b.append(e);}}
        at[0]=i+1;return b.toString();
    }


    private static Set<String> enemyKeys(List<String> sim){
        Set<String> keys=new HashSet<>();Map<String,Integer> column=new HashMap<>();
        for(String line:sim){String[] f=line.split("\t",-1);
            if(f[0].equals("cols") && (f[1].equals("mon") || f[1].equals("su")))for(int i=2;i<f.length;i++)if(f[i].equals("namekey"))column.put(f[1],i-1);
            Integer c=column.get(f[0]);if(c!=null && c<f.length && !f[c].isEmpty())keys.add(f[c]);}
        check(column.size()==2 && keys.size()>300,"enemy name keys "+keys.size());
        return keys;
    }

    private static void pack(File fonts,File overlay)throws Exception{
        GameStyleData d=GameStyleData.parse(new String(Files.readAllBytes(new File(fonts.getParentFile(),"style/style.tsv").toPath()),StandardCharsets.UTF_8));
        check(d.bad()==0 && d.rows()>0,"style table");
        float body=d.size(GameStyleData.BODY.size),small=d.size("SmallFontSize"),kerning=d.spacing(GameStyleData.BODY.spacing),console=d.scale(GameStyleData.CONSOLE);
        Map<String,String[]> names=names(new String(Files.readAllBytes(new File(overlay,"sim-names.json").toPath()),StandardCharsets.UTF_8));
        Set<String> keys=enemyKeys(Files.readAllLines(new File(overlay,"sim.tsv").toPath(),StandardCharsets.UTF_8));

        int[][] screens={{2048,1536},{2560,1600},{2400,1080},{2480,1116},{2688,1216}};
        Map<String,Font> loaded=new HashMap<>();
        int wrapped=0;
        for(int lang=0;lang<GameLang.LANGS;lang++){
            String code=GameLang.groupOf(lang);
            Font own=font(loaded,fonts,d.font(code,GameStyleData.FORMAL)),global=font(loaded,fonts,d.font(code,GameStyleData.GLOBAL));
            Face face=new Face(own,global,kerning);


            boolean drawn=true;for(String key:keys){String[] n=names.get(key);if(n!=null && !face.covers(n[lang]))drawn=false;}
            int shownLang=drawn?lang:0,page=code.equals(GameLang.DE)?CombatModel.L_DE:code.equals(GameLang.PT)?CombatModel.L_PT:CombatModel.L_EN;
            for(boolean whole:new boolean[]{true,false}){
                String longest="",rating="";float widest=0,rw=0;
                for(String key:keys){String[] n=names.get(key);if(n==null)continue;float w=face.width(n[shownLang],100,whole);if(w>widest){widest=w;longest=n[shownLang];}}
                for(int v=0;v<4;v++){String r=CombatModel.light(v,page);float w=face.width(r,100,whole);if(w>rw){rw=w;rating=r;}}
                check(!longest.isEmpty() && !rating.isEmpty(),"names of language "+lang);
                for(int[] screen:screens){
                    GameStyleGeometry.Panel p=new GameStyleGeometry.Panel();p.compute(screen[0],screen[1],console,0);
                    float k=screen[1]/1080f,left=p.contentL+24*p.s,right=p.contentR-24*p.s,minK=k*Math.min(1,small/body),wrapK=k*CombatLayout.WRAP;
                    CombatLayout.Measure m=(s,kk)->face.width(s,body*kk,whole);
                    String what="language "+lang+" screen "+screen[0]+"x"+screen[1]+(whole?"":" rounded")+": "+longest+" · "+rating;

                    float room=CombatLayout.titleRight(left,right,k,true)-CombatLayout.titleLeft(left,right,k);
                    CombatLayout.Title t=CombatLayout.title(longest,rating,room,k,minK,wrapK,k,true,m);
                    valid(t,longest,rating,room,k,Math.min(minK,wrapK),m,"title "+what);
                    check(!t.cut,"title cut: "+what);
                    if(t.lines()==2)wrapped++;

                    float mini=(right-left-12*k)/2-24*k,card=right-left-24*k;
                    for(float r:new float[]{mini,card}){
                        t=CombatLayout.title(longest,rating,r,k,minK,wrapK,k,false,m);
                        valid(t,longest,rating,r,k,minK,m,"card "+what);
                        check(!t.cut,"card cut: "+what);
                    }
                }
            }
        }

        check(wrapped>0,"no title wrapped");
        System.out.println("CombatLayoutTest: player's pack, "+keys.size()+" enemy names, "+wrapped+" wrapped titles");
    }
    private static Font font(Map<String,Font> loaded,File dir,String file)throws Exception{
        Font f=loaded.get(file);
        if(f==null){f=Font.createFont(Font.TRUETYPE_FONT,new File(dir,file));loaded.put(file,f);}
        return f;
    }

    public static void main(String[] args)throws Exception{
        System.setProperty("java.awt.headless","true");
        madeUp();
        String fonts=System.getenv("D2TEST_GAME_FONTS"),overlay=System.getenv("DM_SIM_UI_DIR");
        if(fonts==null || fonts.isEmpty() || overlay==null || overlay.isEmpty())System.out.println("CombatLayoutTest: SKIPPED real-font matrix (set D2TEST_GAME_FONTS/DM_SIM_UI_DIR)");
        else pack(new File(fonts),new File(overlay));
        System.out.println("CombatLayoutTest: "+checks+" checks passed");
    }
}
