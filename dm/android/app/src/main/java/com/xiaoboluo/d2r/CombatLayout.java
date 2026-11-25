package com.xiaoboluo.d2r;

final class CombatLayout {
    private CombatLayout() {}
    static float column(float left,float right,int column){
        final float[] fractions={0,.4f,.6f,.8f,1};
        return left+Math.max(0,right-left)*fractions[Math.max(0,Math.min(4,column))];
    }

    static boolean padReady(boolean logTab,Object log,Object result){return logTab?log!=null:result!=null;}
    static int rows(float top,float bottom,float pitch){
        return Math.max(1,(int)(Math.max(0,bottom-top)/Math.max(1,pitch)));
    }



    static final float GRAPH_AT=.44f;
    static float backWidth(float left,float right,float k){return (right-left)/5-8*k;}
    static float graphLeft(float left,float right){return left+(right-left)*GRAPH_AT;}
    static float titleLeft(float left,float right,float k){return left+backWidth(left,right,k)+24*k;}
    static float titleRight(float left,float right,float k,boolean graph){return graph?graphLeft(left,right)-24*k:right;}



    static final float BANNER_PICTURE_INSET=6,BANNER_PICTURE_GAP=12;
    private static float bannerPictureSide(float left,float top,float right,float bottom,float k){
        return Math.max(0,Math.min(bottom-top,right-left)-2*BANNER_PICTURE_INSET*k);
    }
    static void bannerPicture(float left,float top,float right,float bottom,float k,float[] out){
        float side=bannerPictureSide(left,top,right,bottom,k);
        out[2]=right-BANNER_PICTURE_INSET*k;out[0]=out[2]-side;out[1]=top+(bottom-top-side)/2;out[3]=out[1]+side;
    }
    static float bannerTextRight(float left,float top,float right,float bottom,float k,boolean picture){
        if(!picture)return right-14*k;
        return right-BANNER_PICTURE_INSET*k-bannerPictureSide(left,top,right,bottom,k)-BANNER_PICTURE_GAP*k;
    }


    static final float PICTURE_SCALE=1.35f;
    static void square(float left,float top,float side,float pictureSize,float[] out){
        float s=Math.max(0,Math.min(side,pictureSize*PICTURE_SCALE));
        out[0]=left+(side-s)/2;out[1]=top+(side-s)/2;out[2]=out[0]+s;out[3]=out[1]+s;
    }





    static final float ROW_PICTURE_INSET=6,ROW_PICTURE_SIDE=82,ROW_PICTURE_GAP=10;
    static float rowPictureSide(float top,float bottom,float k){return Math.max(0,Math.min(ROW_PICTURE_SIDE*k,bottom-top-2*ROW_PICTURE_INSET*k));}
    static void rowPicture(float left,float top,float bottom,float k,float pictureSize,float[] out){
        float side=rowPictureSide(top,bottom,k);
        square(left+ROW_PICTURE_INSET*k,top+(bottom-top-side)/2,side,pictureSize,out);
    }
    static float rowTextLeft(float left,float k,float pad,boolean place){
        return place?left+(ROW_PICTURE_INSET+ROW_PICTURE_SIDE+ROW_PICTURE_GAP)*k:left+pad;
    }

    interface Fits { boolean fits(String text,float width); }


    static boolean rowPlace(Iterable<String> names,float without,float with,Fits f){
        for(String n:names)if(f.fits(n,without) && !f.fits(n,with))return false;
        return true;
    }




    static final float FIGHT_PICTURE_MAX=240,FIGHT_PICTURE_MIN=110,FIGHT_PICTURE_GAP=12;
    static boolean fightPicture(float left,float right,float linesTop,int lines,float pitch,float bottom,float k,float pictureSize,float[] out){
        if(lines>rows(linesTop,bottom,pitch))return false;
        float top=linesTop+lines*pitch+FIGHT_PICTURE_GAP*k;
        float side=Math.min(Math.min(right-left,bottom-top),FIGHT_PICTURE_MAX*k);
        if(!(side>=FIGHT_PICTURE_MIN*k))return false;
        square(left+(right-left-side)/2,top,side,pictureSize,out);
        return out[2]-out[0]>=FIGHT_PICTURE_MIN*k;
    }

    static final String SEPARATOR=" · ",CUT="...";


    static final float WRAP=.65f;

    interface Measure { float width(String s,float k); }




    static final class Title {
        final String first,second,rating;
        final float nameK,ratingK,ratingX;
        final int ratingLine;
        final boolean cut;
        Title(String first,String second,String rating,float nameK,float ratingK,float ratingX,int ratingLine,boolean cut){
            this.first=first;this.second=second;this.rating=rating;this.nameK=nameK;this.ratingK=ratingK;this.ratingX=ratingX;this.ratingLine=ratingLine;this.cut=cut;
        }
        int lines(){return Math.max(second.isEmpty()?1:2,ratingLine+1);}

        boolean whole(){return second.isEmpty() && ratingLine==0 && nameK==ratingK && !cut;}
    }

    static Title title(String name,String rating,float room,float k,float minK,float wrapK,float ratingK,boolean wrap,Measure m){
        minK=Math.min(minK,k);wrapK=Math.min(wrapK,k);
        String behind=SEPARATOR+rating;
        float nameRoom=room-m.width(behind,ratingK);
        if(k==ratingK && m.width(name+behind,k)<=room)return new Title(name,"",behind,k,ratingK,m.width(name,k),0,false);
        float small=shrink(name,nameRoom,k,minK,m);
        if(small>0)return new Title(name,"",behind,small,ratingK,m.width(name,small),0,false);
        if(wrap && !name.isEmpty()){

            int best=-1,bestKind=0;float bestK=0;
            for(int at=name.offsetByCodePoints(0,1);at<name.length();at=name.offsetByCodePoints(at,1)){
                int kind=breaks(name,at);
                if(kind==0)continue;
                float f=both(name.substring(0,at).trim(),room,name.substring(at).trim(),nameRoom,k,wrapK,m);
                if(f>0 && (f>bestK || f==bestK && kind>=bestKind)){bestK=f;best=at;bestKind=kind;}
            }
            if(best>0){String first=name.substring(0,best).trim(),second=name.substring(best).trim();
                return new Title(first,second,behind,bestK,ratingK,m.width(second,bestK),1,false);}

            float alone=shrink(name,room,k,wrapK,m);
            if(alone>0)return new Title(name,"",rating,alone,ratingK,0,1,false);
            int at=lineBreak(name,room,wrapK,m);
            String first=name.substring(0,at).trim(),last=cut(name.substring(at).trim(),nameRoom,wrapK,m);
            return new Title(first,last,behind,wrapK,ratingK,m.width(last,wrapK),1,true);
        }
        String one=cut(name,nameRoom,minK,m);
        return new Title(one,"",behind,minK,ratingK,m.width(one,minK),0,true);
    }



    static int breaks(String s,int at){
        int before=s.codePointBefore(at),after=s.codePointAt(at);
        if(before==' ' || before=='-' || before=='\u00B7' || before=='\u30FB')return 2;
        return dense(before) && dense(after) && "ぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶー".indexOf(after)<0?1:0;
    }
    private static boolean dense(int c){
        Character.UnicodeScript u=Character.UnicodeScript.of(c);
        return u==Character.UnicodeScript.HAN || u==Character.UnicodeScript.HIRAGANA || u==Character.UnicodeScript.KATAKANA;
    }


    private static float both(String a,float aw,String b,float bw,float k,float minK,Measure m){
        if(a.isEmpty() || b.isEmpty())return 0;
        float fa=shrink(a,aw,k,minK,m),fb=shrink(b,bw,k,minK,m),f=Math.min(fa,fb);
        if(!(f>0))return 0;
        for(int i=0;i<32 && (m.width(a,f)>aw || m.width(b,f)>bw) && f>minK;i++)f=Math.max(minK,f*.98f);
        return m.width(a,f)<=aw && m.width(b,f)<=bw?f:0;
    }



    static float shrink(String s,float width,float k,float minK,Measure m){
        float w=m.width(s,k);
        if(w<=width)return k;
        if(!(width>0) || !(w>0))return 0;
        float f=Math.max(minK,k*width/w);
        for(int i=0;i<32 && m.width(s,f)>width && f>minK;i++)f=Math.max(minK,f*.98f);
        return m.width(s,f)<=width?f:0;
    }



    static int lineBreak(String s,float width,float k,Measure m){
        int best=-1;
        for(int i=s.indexOf(' ');i>0;i=s.indexOf(' ',i+1))if(m.width(s.substring(0,i).trim(),k)<=width)best=i;else break;
        if(best>0)return best;
        if(m.width(s,k)<=width)return s.length();
        int at=s.offsetByCodePoints(0,1);
        while(at<s.length()){int next=s.offsetByCodePoints(at,1);if(m.width(s.substring(0,next),k)>width)break;at=next;}
        return at;
    }


    static String cut(String s,float width,float k,Measure m){
        String t=s;
        while(t.length()>1 && m.width(t+CUT,k)>width)t=t.substring(0,t.offsetByCodePoints(t.length(),-1)).trim();
        return t+CUT;
    }
}
