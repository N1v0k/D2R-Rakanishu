#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_sim_ui.h"
#include "../dm_sim_text.h"
#include "../dm_sim_internal.h"
#include "../dm_gear_core.h"
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

int dm_overlay_file(const char *relative,char *out,size_t cap) {
    const char *base=getenv("DM_SIM_UI_DIR");
    if(!strcmp(relative,"sim.tsv")){const char *p=getenv("DM_SIM_DATA");return p && snprintf(out,cap,"%s",p)>0;}
    return base && snprintf(out,cap,"%s/%s",base,relative)>0;
}

static double now(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec*1000.0+t.tv_nsec/1e6;}

typedef struct {double kill,death,hits;int light,flags;} enemy_row;
static int enemy_line(const char *report,int id,enemy_row *e) {
    char key[32];snprintf(key,sizeof key,"enemy\t%d\t",id);const char *p=strstr(report,key);
    if(!p)return 0;
    char line[512];size_t n=strcspn(p,"\n");if(n>=sizeof line)n=sizeof line-1;memcpy(line,p,n);line[n]=0;
    char *field[24];int k=0;for(char *t=strtok(line,"\t");t && k<24;t=strtok(NULL,"\t"))field[k++]=t;
    assert(k==18);e->kill=atof(field[9]);e->death=atof(field[10]);e->hits=atof(field[12]);e->light=atoi(field[13]);e->flags=atoi(field[16]);return 1;
}


static void list_matches_detail(const dm_sim_data *d,const char *name,const char *profile,char *out,char *detail) {
    dm_sim_snapshot s={.valid=1,.session=1,.act=1,.set=1,.players=1};strcpy(s.name,"Row");char error[128];
    assert(!dm_sim_profile_parse(d,profile,&s.c,error,sizeof error));
    int checked=0;
    for(int diff=0;diff<3;diff++) {
        dm_sim_options opt={.diff=diff,.players=1,.target=-1,.potions=s.c.potions};
        assert(dm_sim_ui_report(d,&s,&opt,out,2*1024*1024)>0);
        for(int id=0;id<2000;id+=7) {
            enemy_row a,b;if(!enemy_line(out,id,&a))continue;
            opt.target=id;assert(dm_sim_ui_report(d,&s,&opt,detail,2*1024*1024)>0);opt.target=-1;
            assert(enemy_line(detail,id,&b));
            if(fabs(a.kill-b.kill)>.05 || fabs(a.death-b.death)>.05 || fabs(a.hits-b.hits)>.05 || a.light!=b.light || a.flags!=b.flags) {
                printf("%s diff %d enemy %d: row %.2f %.2f %.2f %d, detail %.2f %.2f %.2f %d\n",name,diff,id,a.kill,a.death,a.hits,a.light,b.kill,b.death,b.hits,b.light);
                assert(0);
            }
            checked++;
        }
    }
    printf("rows match details: %s, %d enemies\n",name,checked);
}


static int enemy_field(const char *report,int id,int field,char *value,size_t cap) {
    char key[32];snprintf(key,sizeof key,"enemy\t%d\t",id);const char *p=strstr(report,key);
    if(!p)return 0;
    for(int k=0;k<field;k++){p=strchr(p,'\t');assert(p);p++;}
    size_t n=strcspn(p,"\t\n");assert(n<cap);memcpy(value,p,n);value[n]=0;return 1;
}
static void minibosses_listed(const dm_sim_data *d,const dm_sim_snapshot *s,char *out) {
    int first=1;for(int i=0;i<d->nspawn;i++)first+=d->spawn[i].n;
    int den=0,stony=0,listed=0,quest=0;char v[64];
    for(int diff=0;diff<3;diff++) {
        dm_sim_options opt={.diff=diff,.players=1,.target=-1};assert(dm_sim_ui_report(d,s,&opt,out,2*1024*1024)>0);
        for(int i=0;i<d->nsu;i++) {

            dm_sim_target t;assert(!dm_sim_target_su(d,i,diff,&t));
            int id=first+i;assert(enemy_field(out,id,2,v,sizeof v) && !strcmp(v,d->su[i].namekey));
            assert(enemy_field(out,id,3,v,sizeof v) && atoi(v)==DM_TK_SUPERUNIQUE);
            assert(enemy_field(out,id,4,v,sizeof v) && atoi(v)==d->su[i].act && d->su[i].act>=1 && d->su[i].act<=5);
            assert(enemy_field(out,id,5,v,sizeof v) && atoi(v)==d->su[i].area);
            assert(enemy_field(out,id,6,v,sizeof v) && atoi(v)==t.mlvl);
            assert(enemy_field(out,id,7,v,sizeof v) && atoi(v)==t.hp && t.hp>0);
            listed++;

            if(d->su[i].idx==40){assert(d->su[i].area==8 && d->su[i].act==1 && !d->su[i].area_guess);den++;}

            if(d->su[i].idx==3){assert(d->su[i].area==4 && d->su[i].act==1 && !d->su[i].area_guess);stony++;}
        }
        for(int i=0,id=first+d->nsu;i<d->nboss;i++,id++)if(d->boss[i].kind==DM_BK_QUEST && d->boss[i].act==1) {
            assert(enemy_field(out,id,3,v,sizeof v) && atoi(v)==DM_TK_QUEST);assert(enemy_field(out,id,4,v,sizeof v) && atoi(v)==1);quest++;
        }
    }
    assert(den==3 && stony==3 && quest>=6 && listed==3*d->nsu);
    printf("mini bosses listed: all %d superunique rows over three difficulties, %d act 1 quest boss rows\n",listed,quest);
}
int main(void) {
    const char *path=getenv("DM_SIM_DATA");assert(path);size_t n;char *input=sim_read(path,&n);assert(input);
    dm_sim_data d={0};assert(!dm_sim_load(&d,input,n,"rotw"));free(input);
    const int towns[]={1,40,75,103,109};
    for(int act=1;act<=5;act++){
        int found=0;
        for(int i=0;i<d.narea;i++)if(d.area[i].id==towns[act-1]){assert(d.area[i].act==act);found=1;}
        assert(found);
    }
    dm_sim_snapshot s={.valid=1,.session=7,.act=1,.area=8,.set=1,.players=1};strcpy(s.name,"Test");
    char error[128];assert(!dm_sim_profile_parse(&d,"class=1\nlevel=60\nlife=500\nmana=40\nskill=47,10,10\nbar=47\n",&s.c,error,sizeof error));
    dm_sim_options opt={.diff=0,.players=1,.target=-1};char *out=malloc(2*1024*1024);assert(out);
    double start=now();assert(dm_sim_ui_report(&d,&s,&opt,out,2*1024*1024)>0);printf("catalog_ms=%.3f\n",now()-start);
    assert(!strncmp(out,"hero\tTest\t1\t60\t0\t1\t500\t40\t0\t0\t0\t7\t1\t1\t8\t0\n",sizeof "hero\tTest\t1\t60\t0\t1\t500\t40\t0\t0\t0\t7\t1\t1\t8\t0\n"-1));assert(strstr(out,"enemy\t"));assert(strstr(out,"rotation\t"));
    s.ref_area=5;s.area=1;assert(dm_sim_ui_report(&d,&s,&opt,out,2*1024*1024)>0);
    assert(!strncmp(out,"hero\tTest\t1\t60\t0\t1\t500\t40\t0\t0\t0\t7\t1\t1\t1\t5\n",sizeof "hero\tTest\t1\t60\t0\t1\t500\t40\t0\t0\t0\t7\t1\t1\t1\t5\n"-1));
    s.area=8;s.ref_area=0;
    {


        assert(dm_sim_is_town(1)&&dm_sim_is_town(40)&&dm_sim_is_town(75)&&dm_sim_is_town(103)&&dm_sim_is_town(109));
        assert(!dm_sim_is_town(20) && !dm_sim_is_town(50));
        dm_sim_ref r={0};
        assert(dm_sim_ref_capture(&r,1,"A",1,3)==3);
        assert(dm_sim_ref_capture(&r,1,"A",1,1)==3);
        assert(dm_sim_ref_capture(&r,1,"A",1,1)==3);
        assert(dm_sim_ref_capture(&r,1,"A",2,25)==25);
        assert(dm_sim_ref_capture(&r,1,"A",1,1)==3);
        assert(dm_sim_ref_capture(&r,1,"A",2,40)==25);
        assert(dm_sim_ref_capture(&r,2,"A",1,1)==0);
        assert(dm_sim_ref_capture(&r,2,"A",1,3)==3);
        assert(dm_sim_ref_capture(&r,2,"B",1,1)==0);
    }
    assert(dm_sim_ui_report(&d,&s,&opt,out,8)==-2 && !out[0]);
    minibosses_listed(&d,&s,out);
    {


        dm_sim_snapshot tank=s;
        assert(!dm_sim_profile_parse(&d,"class=4\nlevel=85\nlife=4000\ndef=4000\nar=8000\n",&tank.c,error,sizeof error));
        for(int diff=0;diff<3;diff++) {
            dm_sim_options o={.diff=diff,.players=1,.target=-1};double best=1e9;
            for(int k=0;k<3;k++){double t0=now();assert(dm_sim_ui_report(&d,&tank,&o,out,2*1024*1024)>0);best=fmin(best,now()-t0);}
            printf("survivable_catalog_ms[%d]=%.3f\n",diff,best);assert(best<40);
        }
        char *detail=malloc(2*1024*1024);assert(detail);
        const char *profiles[][2]={
            {"tank","class=4\nlevel=85\nlife=4000\ndef=4000\nar=8000\n"},
            {"orders","class=4\nlevel=85\nlife=4000\nmana=500\ndef=4000\nar=8000\nskill=151,20,20\nskill=149,20,20\nskill=138,20,20\nbar=151\nbar=149\nbar=138\n"},
            {"shield","class=1\nlevel=85\nlife=1500\nmana=1500\nes=40\nes_ratio=16\nskill=47,20,20\nskill=58,20,20\nbar=47\nbar=58\nactive=58\nbelt=hp5:4,mp5:4,rvl:4\npotions=1\n"},
            {"leech","class=4\nlevel=70\nlife=2000\nll=8\nar=5000\ndef=2000\n"},
            {"static","class=1\nlevel=70\nlife=1200\nmana=600\nskill=42,20,20\nskill=47,20,20\nbar=42\nbar=47\n"},
            {"potions","class=4\nlevel=60\nlife=900\nar=3000\ndef=1500\nbelt=hp4:4,rvs:4\npotions=1\n"},
            {"thirsty","class=1\nlevel=60\nlife=500\nmana=40\nskill=47,10,10\nbar=47\n"},
        };
        for(size_t i=0;i<sizeof profiles/sizeof *profiles;i++)list_matches_detail(&d,profiles[i][0],profiles[i][1],out,detail);
        free(detail);
    }
    opt.players=0;assert(dm_sim_ui_report(&d,&s,&opt,out,1024)<0);opt.players=1;
    opt.target=0;assert(dm_sim_ui_report(&d,&s,&opt,out,2*1024*1024)>0);assert(strstr(out,"taken\t5\t"));assert(strstr(out,"skill\t0\t"));
    opt.players=-1;s.players=3;dm_sim_publish(&s);
    assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0 && strstr(out,"\t7\t1\t3\t8\t0\n"));
    opt.players=1;s.players=1;
    dm_sim_publish(&s);assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0);
    {
        char *first=strdup(out);assert(first);double kept=now();int n=dm_sim_ui_query(&opt,out,2*1024*1024);kept=now()-kept;
        assert(n==(int)strlen(first) && !strcmp(first,out));assert(dm_sim_ui_query(&opt,out,16)<0);
        s.c.life++;dm_sim_publish(&s);assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0 && strcmp(first,out));
        s.c.life--;dm_sim_publish(&s);assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0 && !strcmp(first,out));
        printf("kept_report_ms=%.3f\n",kept);free(first);
    }
    opt.diff=-1;assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0);
    {

        char *kept=strdup(out),copy[1200],damaged[1200],dir[1100];assert(kept);
        const char *tmp=getenv("TMPDIR");snprintf(dir,sizeof dir,"%s/t_sim_ui.XXXXXX",tmp && *tmp?tmp:"/tmp");assert(mkdtemp(dir));
        snprintf(copy,sizeof copy,"%s/sim.tsv",dir);snprintf(damaged,sizeof damaged,"%s/damaged.tsv",dir);
        size_t size;char *text=sim_read(path,&size);assert(text);
        FILE *f=fopen(copy,"wb");assert(f && fwrite(text,1,size,f)==size && !fclose(f));
        char *payload=strstr(text,"\nset\t");assert(payload);payload[2]=payload[2]=='s'?'t':'s';
        f=fopen(damaged,"wb");assert(f && fwrite(text,1,size,f)==size && !fclose(f));free(text);
        char *original=strdup(path);assert(original);setenv("DM_SIM_DATA",copy,1);
        assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0 && !strcmp(kept,out));
        assert(!rename(damaged,copy));assert(dm_sim_ui_query(&opt,out,2*1024*1024)<0);
        f=fopen(damaged,"wb");text=sim_read(original,&size);assert(f && text && fwrite(text,1,size,f)==size && !fclose(f));free(text);
        assert(!rename(damaged,copy));assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0 && !strcmp(kept,out));
        setenv("DM_SIM_DATA",original,1);assert(!unlink(copy) && !rmdir(dir));free(original);free(kept);
    }
    dm_sim_publish(NULL);assert(dm_sim_ui_query(&opt,out,2*1024*1024)<0);dm_sim_publish(&s);
    dm_sim_target target;assert(!dm_sim_target_std(&d,0,0,&target));dm_sim_rot_out r;assert(!dm_sim_rotation(&d,&s.c,&target,&r));
    double before=r.dps;s.c.potions=1;assert(!dm_sim_rotation(&d,&s.c,&target,&r));assert(fabs(before-r.dps)<.0001);
    int skill=s.c.bar[0];dm_sim_bar_add(&s.c,skill);assert(s.c.nbar==1);dm_sim_bar_add(&s.c,-1);assert(s.c.nbar==1);
    opt.diff=2;opt.terror=1;opt.herald=1;assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0);
    opt.diff=0;assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0);
    s.set=0;dm_sim_publish(&s);opt.diff=2;assert(dm_sim_ui_query(&opt,out,2*1024*1024)>0);
    s.set=1;dm_sim_publish(&s);
    dm_sim_char c=s.c;c.potions=0;c.life=200;c.ll=0;c.liferegen=0;c.bone_pool=c.cyclone_pool=c.ward_pool=c.es_pct=0;
    target.dmin=target.dmax=25;target.ar=1000;target.cycle=25;target.crit=0;
    dm_sim_surv_out a,b;assert(!dm_sim_survive(&d,&c,&target,NULL,&a));c.potions=1;assert(!dm_sim_survive(&d,&c,&target,NULL,&b));assert(b.ttd==a.ttd);

    c.belt[DM_POT_HP1+1]=4;assert(!dm_sim_survive(&d,&c,&target,NULL,&b));assert(b.ttd<0 || b.ttd>a.ttd);
    c.potions=0;assert(!dm_sim_survive(&d,&c,&target,NULL,&b));assert(b.ttd==a.ttd);c.belt[DM_POT_HP1+1]=0;
    start=now();for(int i=0;i<100;i++){dm_sim_skill_out x,y;assert(!dm_sim_skill_eval(&d,&c,&target,skill,&x));dm_sim_char next=c;next.lvl[skill]++;assert(!dm_sim_skill_eval(&d,&next,&target,skill,&y));}
    printf("skill_pair_ms=%.3f\n",(now()-start)/100);
    uint16_t quests[48]={0};assert(dm_sim_next_boss(NULL)==-1 && dm_sim_next_boss(quests)==1);
    quests[6]=quests[14]=1;assert(dm_sim_next_boss(quests)==3);
    quests[22]=quests[26]=quests[40]=1;assert(dm_sim_next_boss(quests)==0);
    {

        uint8_t record[DM_SIM_QUEST_BYTES]={0};const uint16_t owner[][2]={{0,1},{1,0x1001},{2,0x101d},{3,0xc},{4,0x101d},{5,0x1055},{6,0x1019},{7,0x2001},{8,1},{9,0xc},{41,0x8002}};
        for(size_t i=0;i<sizeof owner/sizeof owner[0];i++){record[2*owner[i][0]]=(uint8_t)owner[i][1];record[2*owner[i][0]+1]=(uint8_t)(owner[i][1]>>8);}
        uint16_t words[3][48]={{0}};assert(!dm_sim_quest_words(record,8*sizeof record,words[0]));
        assert(words[0][6]==0x1019 && words[0][7]==0x2001 && words[0][41]==0x8002);assert(dm_sim_next_boss(words[0])==2);
        assert(dm_sim_quest_words(NULL,8*sizeof record,words[1])==-1 && dm_sim_quest_words(record,sizeof record,words[1])==-1 && dm_sim_quest_words(record,0,words[1])==-1);
        int state[3]={0,0,-1},next[3];char name[16]="Hero";
        dm_sim_quest_publish(name,(const uint16_t (*)[48])words,state);
        dm_sim_quest_next("Hero",next);assert(next[0]==2 && next[1]==1 && next[2]==-1);
        dm_sim_quest_next("Zweiter",next);assert(next[0]==-1 && next[1]==-1 && next[2]==-1);
        dm_sim_quest_next(NULL,next);assert(next[0]==-1);
        dm_sim_quest_publish(NULL,NULL,NULL);dm_sim_quest_next("Hero",next);assert(next[0]==-1);
    }
    dm_sim_char invalid=c;invalid.nbar=0;invalid.capture_bad=1;assert(!dm_sim_rotation(&d,&invalid,&target,&r));assert(r.flags&DM_SF_UNAVAIL);
    for(int id=1;id<512;id++)if(d.skill[id] && id==149){invalid.lvl[id]=1;invalid.bar[0]=id;invalid.nbar=1;assert(!dm_sim_rotation(&d,&invalid,&target,&r));assert(r.flags&DM_SF_UNAVAIL);}
    invalid=c;invalid.capture_bad=2;assert(!dm_sim_survive(&d,&invalid,&target,NULL,&a) && a.ttd==-2);double types[6];assert(dm_sim_damage_taken(&d,&invalid,&target,types)<0);
    dm_sim_char custom=c;custom.ordered=1;custom.nbar=2;custom.bar[0]=0;custom.bar[1]=skill;
    assert(!dm_sim_rotation(&d,&custom,&target,&r));double normal_priority=r.dps;
    custom.bar[0]=skill;custom.bar[1]=0;assert(!dm_sim_rotation(&d,&custom,&target,&r));assert(r.dps>normal_priority);
    dm_sim_char combo;
    assert(!dm_sim_profile_parse(&d,"class=1\nlevel=60\nlife=500\nmana=500\nskill=47,10,10\nskill=56,10,10\nbar=47\nbar=56\n",&combo,error,sizeof error));
    dm_sim_rot_out automatic,reversed,manual;
    assert(!dm_sim_rotation(&d,&combo,&target,&automatic));
    assert(automatic.nseq==2 && automatic.seq[0]==combo.bar[1]);
    int first=combo.bar[0];combo.bar[0]=combo.bar[1];combo.bar[1]=first;
    assert(!dm_sim_rotation(&d,&combo,&target,&reversed));
    assert(reversed.seq[0]==automatic.seq[0] && fabs(reversed.dps-automatic.dps)<.001);
    combo.ordered=1;assert(!dm_sim_rotation(&d,&combo,&target,&manual));assert(fabs(manual.dps-automatic.dps)<.001);

    dm_sim_char worn,swapped;dm_gear_tip tip;dm_gear_goal goal={15,0.5};
    assert(!dm_sim_profile_parse(&d,"class=1\nlevel=60\nlife=500\nmana=5000\nskill=47,10,10\nbar=47\n",&worn,error,sizeof error));
    worn.lvl[0]=1;dm_sim_bar_add(&worn,0);
    dm_sim_rot_out alone;assert(!dm_sim_rotation(&d,&worn,&target,&alone));assert(alone.nseq==1 && alone.seq[0]==worn.bar[0]);
    swapped=worn;swapped.lvl[worn.bar[0]]+=4;
    {


        dm_sim_char now,after;int weapon_passive=383;assert(d.skill[weapon_passive]);
        assert(!dm_sim_profile_parse(&d,"class=7\nlevel=15\nlife=200\nmana=50\nstr=46\ndex=25\nar=300\nweapon=3,5,0,75,-20,1,0,0,1HS\nskill=383,2,0\n",&now,error,sizeof error));
        now.lvl[0]=1;dm_sim_bar_add(&now,0);now.ar_base_known=1;now.ar_base=200;now.ar_bonus[0]=45;now.mastery_cs=9;now.ed=29;
        now.active_lvl[weapon_passive]=2;
        after=now;after.lvl[weapon_passive]=3;
        static int th[DM_SIM_MAXSKILL],dmg[DM_SIM_MAXSKILL];for(int i=0;i<DM_SIM_MAXSKILL;i++)th[i]=dmg[i]=DM_SIM_NO_NODE;th[0]=45;dmg[0]=29;
        dm_sim_passive_shift shift;assert(!dm_sim_passive_levels(&d,&now,&after,th,dmg,9,&shift));
        assert(shift.ar_mastery==5 && shift.damage_mastery==4 && after.ar_bonus[0]==50 && after.mastery_cs==12 && after.damage_mastery_delta[0]==0);
        assert(after.active_lvl[weapon_passive]==3);
        after.ed+=shift.damage_mastery;
        now.act=after.act=1;assert(!dm_gear_tip_rate(&d,&now,&after,&goal, 0, &tip) && tip.score.change[DM_GEAR_M_SINGLE]>0);

        after=now;after.lvl[weapon_passive]=3;th[0]=30;assert(dm_sim_passive_levels(&d,&now,&after,th,dmg,9,&shift)<0);

        after=now;after.lvl[weapon_passive]=3;th[0]=dmg[0]=0;assert(!dm_sim_passive_levels(&d,&now,&after,th,dmg,0,&shift) && !shift.ar_mastery && after.ar_bonus[0]==45 && after.mastery_cs==9);
    }
    {

        int regen_passive=37,regen=-1;assert(d.skill[regen_passive]);
        for(int i=0;i<512;i++)if(!strcmp(d.stat_name[i],"manarecoverybonus"))regen=i;
        assert(regen>=0);
        dm_sim_char now,after;
        assert(!dm_sim_profile_parse(&d,"class=1\nlevel=30\nlife=500\nmana=1200\nskill=37,1,1\nskill=47,10,10\nbar=47\n",&now,error,sizeof error));
        now.active_lvl[regen_passive]=1;now.stat_value[regen]=now.manaregen_pct=30;
        static int th[DM_SIM_MAXSKILL],dmg[DM_SIM_MAXSKILL];for(int i=0;i<DM_SIM_MAXSKILL;i++)th[i]=dmg[i]=DM_SIM_NO_NODE;
        after=now;after.lvl[regen_passive]=2;dm_sim_passive_shift shift;assert(!dm_sim_passive_levels(&d,&now,&after,th,dmg,0,&shift));
        assert(after.stat_value[regen]==42);after.manaregen_pct=after.stat_value[regen];
        dm_sim_char prepared;dm_sim_tmod mods;dm_sim_rot_out unused;sim_prepare(&d,&after,&target,&prepared,&mods,&unused);
        assert(prepared.manaregen_pct==42);
        sim_prepare(&d,&now,&target,&prepared,&mods,&unused);assert(prepared.manaregen_pct==30);
    }
    {
        dm_sim_char here=worn,there=swapped;here.act=there.act=2;dm_sim_target act2,all;
        assert(dm_sim_target_act(&d,&here,&act2)==2 && !dm_sim_target_std(&d,here.diff,0,&all) && (act2.hp!=all.hp || act2.ac!=all.ac));
        assert(!dm_gear_tip_rate(&d,&here,&there,&goal, 0, &tip) && tip.act==2 && tip.score.change[DM_GEAR_M_GROUP]>0);
    }
    {

        uint64_t town=dm_sim_tip_key(&d,0,1,5,7);
        assert(dm_sim_area_act(&d,1)==1 && dm_sim_area_act(&d,2)==1 && dm_sim_area_act(&d,40)==2 && !dm_sim_area_act(&d,30000));
        assert(dm_sim_tip_key(&d,0,2,5,7)==town && dm_sim_tip_key(&d,0,40,5,7)!=town && dm_sim_tip_key(&d,1,1,5,7)!=town && dm_sim_tip_key(&d,0,1,5,8)!=town);
        dm_sim_char nowhere=worn;nowhere.act=0;dm_sim_target all;assert(dm_sim_target_act(&d,&nowhere,&all)==0);
        assert(dm_sim_tip_key(&d,0,30000,5,7)!=town && dm_sim_tip_key(&d,0,30000,5,7)==dm_sim_tip_key(&d,0,30001,5,7));

        const dm_sim_std *a1=&d.std[0][1];
        assert(a1->lmin<a1->lmax && dm_sim_tip_key(&d,0,1,a1->lmin+1,7)!=town && dm_sim_tip_key(&d,0,1,a1->lmin,7)==dm_sim_tip_key(&d,0,1,1,7));
        assert(dm_sim_tip_key(&d,0,1,a1->lmax,7)==dm_sim_tip_key(&d,0,1,a1->lmax+30,7));
    }
    {

        dm_sim_char here=worn,there=swapped;here.act=there.act=1;here.diff=there.diff=2;dm_sim_target t;
        assert(dm_sim_target_act(&d,&here,&t)==1 && t.immune_share[DM_EL_FIRE]==d.std[2][1].immune[DM_EL_FIRE] && t.immune_share[DM_EL_FIRE]>0);
        for(int e=0;e<DM_EL_N;e++)assert(t.res[e]<100);
        assert(!dm_gear_tip_rate(&d,&here,&there,&goal, 0, &tip) && tip.act==1);
        assert(tip.immune[DM_EL_FIRE]==d.std[2][1].immune[DM_EL_FIRE] && !tip.immune[DM_EL_COLD] && !tip.immune[DM_EL_LTNG] && !tip.immune[DM_EL_POIS]);
        dm_sim_char sturdier=here;sturdier.def+=500;
        assert(!dm_gear_tip_rate(&d,&here,&sturdier,&goal, 0, &tip));
        for(int e=0;e<DM_EL_N;e++)assert(!tip.immune[e]);

        here.act=there.act=0;assert(dm_gear_tip_rate(&d,&here,&there,&goal, 0, &tip)<0);

        here.act=there.act=1;here.diff=there.diff=0;assert(!dm_gear_tip_rate(&d,&here,&there,&goal, 0, &tip));
        assert(!d.std[0][1].immune[DM_EL_FIRE] && !tip.immune[DM_EL_FIRE]);

        dm_sim_char spear,stronger;
        assert(!dm_sim_profile_parse(&d,"class=2\nlevel=60\nlife=500\nmana=5000\nskill=84,10,10\nbar=84\n",&spear,error,sizeof error));
        spear.act=2;spear.diff=2;stronger=spear;stronger.lvl[84]+=4;
        assert(d.std[2][2].immune[DM_EL_MAG]>0 && !dm_gear_tip_rate(&d,&spear,&stronger,&goal, 0, &tip));
        assert(tip.immune[DM_EL_MAG]==d.std[2][2].immune[DM_EL_MAG] && !tip.immune[DM_EL_FIRE] && !tip.immune[DM_EL_PHYS]);
    }
    {
        dm_sim_char same=worn;same.act=1;static const dm_gear_words words={{"G","S","U","E"},{"a","b","c","d","e","f","g","h","i","j","k"},"mehr %s","weniger %s","unter %d","auf %d","noch %s statt %s","noch %s"};
        char lines[DM_GEAR_LINES][DM_GEAR_LINEB];
        assert(!dm_gear_tip_rate(&d,&same,&same,&goal, 0, &tip) && tip.score.goal==DM_GEAR_GOAL_NONE && !dm_gear_tip_text(&tip,&words,1,lines,DM_GEAR_LINES));
    }


    dm_sim_char charged=worn;int ball=worn.bar[0];
    charged.blvl[ball]=1;charged.lvl[ball]=14;
    char preview[240],expected[240];
    assert(dm_sim_skill_preview(&d,&charged,&target,ball,8,9,1,preview,sizeof preview)>0);
    dm_sim_char own=worn;own.blvl[ball]=1;own.lvl[ball]=8;dm_sim_skill_out at8,at9;
    assert(!dm_sim_skill_eval(&d,&own,&target,ball,&at8));own.blvl[ball]=2;own.lvl[ball]=9;assert(!dm_sim_skill_eval(&d,&own,&target,ball,&at9));
    assert(dm_sim_skill_delta_text(expected,sizeof expected,at8.dps,at9.dps,1)>0);
    printf("charge preview: %s\n",preview+4);
    assert(!strcmp(preview,expected) && preview[3]=='3');

    assert(dm_sim_skill_preview(&d,&worn,&target,ball,0,8,1,preview,sizeof preview)>0);
    printf("first point: %s\n",preview+4);
    assert(!strncmp(preview+4,"Erster Punkt: ",14) && !strstr(preview,"jetzt") && !strstr(preview,"0,0 ") && preview[3]=='3');
    free(out);dm_sim_free(&d);puts("sim UI: catalog, options, snapshot lifecycle, capacity, potions and skill preview passed");return 0;
}
