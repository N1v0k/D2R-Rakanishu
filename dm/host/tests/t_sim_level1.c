


#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_sim_ui.h"
#include "../dm_sim_internal.h"
#include <assert.h>
#include <string.h>

int dm_overlay_file(const char *relative,char *out,size_t cap){(void)relative;(void)out;(void)cap;return 0;}

static int failures;
#define CHECK(cond,...) do { if(!(cond)){failures++;fprintf(stderr,"FAIL %s:%d: ",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);fputc('\n',stderr);} } while (0)



static const struct { const char *name, *profile; } CASES[] = {
    { "amazon",          "class=0\nlife=50\nmana=15\nstr=20\ndex=25\nvit=20\nene=15\nar=105\ndef=11\nblock=30\nweapon=1,5,75,75,-10,2,0,0,1HT\n" },
    { "sorceress",       "class=1\nlife=40\nmana=35\nstr=10\ndex=25\nvit=10\nene=35\nar=95\ndef=6\nweapon=1,5,100,0,-10,1,1,0,STF\n" },
    { "necromancer",     "class=2\nlife=45\nmana=25\nstr=15\ndex=25\nvit=15\nene=25\nar=100\ndef=6\nweapon=2,4,100,0,0,1,0,0,1HS\n" },
    { "paladin",         "class=3\nlife=55\nmana=15\nstr=25\ndex=20\nvit=25\nene=15\nar=85\ndef=9\nblock=30\nweapon=2,7,100,0,0,1,0,0,1HS\n" },
    { "barbarian",       "class=4\nlife=55\nmana=10\nstr=30\ndex=20\nvit=25\nene=10\nar=85\ndef=9\nblock=25\nweapon=3,6,100,0,0,1,0,0,1HS\n" },
    { "barbarian bare",  "class=4\nlife=55\nmana=10\nstr=30\ndex=20\nvit=25\nene=10\nar=85\ndef=5\nweapon=1,2,0,0,0,0,0,0,HTH\n" },
    { "druid",           "class=5\nlife=55\nmana=20\nstr=15\ndex=20\nvit=25\nene=20\nar=85\ndef=9\nblock=25\nweapon=1,6,100,0,-10,1,0,0,1HS\n" },
    { "assassin",        "class=6\nlife=50\nmana=25\nstr=20\ndex=20\nvit=20\nene=25\nar=85\ndef=9\nblock=25\nweapon=4,7,75,75,-10,5,0,0,HT1\n" },
    { "assassin bare",   "class=6\nlife=50\nmana=25\nstr=20\ndex=20\nvit=20\nene=25\nar=85\ndef=5\nweapon=1,2,0,0,0,0,0,0,HTH\n" },
    { "warlock",         "class=7\nlife=55\nmana=20\nstr=15\ndex=20\nvit=25\nene=20\nar=85\ndef=5\nweapon=1,4,75,75,-20,2,0,0,1HT\n" },
};

int main(void) {
    const char *path=getenv("DM_SIM_DATA");if(!path){fputs("DM_SIM_DATA is required\n",stderr);return 2;}
    size_t n;char *text=sim_read(path,&n);assert(text);
    dm_sim_data *d=calloc(1,sizeof *d);assert(d && !dm_sim_load(d,text,n,"rotw"));free(text);
    char *out=malloc(2*1024*1024);assert(out);
    for(size_t k=0;k<sizeof CASES/sizeof *CASES;k++) {
        char profile[512];snprintf(profile,sizeof profile,"level=1\ndiff=0\nact=1\n%s",CASES[k].profile);
        dm_sim_snapshot s={.valid=1,.session=1,.act=1,.set=1,.players=1};strcpy(s.name,"Row");char error[128];
        if(dm_sim_profile_parse(d,profile,&s.c,error,sizeof error)){CHECK(0,"%s: profile: %s",CASES[k].name,error);continue;}
        dm_sim_options opt={.diff=0,.players=1,.target=-1};
        if(dm_sim_ui_report(d,&s,&opt,out,2*1024*1024)<=0){CHECK(0,"%s: no report",CASES[k].name);continue;}
        double dps=0;int flags=-1;const char *r=strstr(out,"\nrotation\t");
        CHECK(r && sscanf(r,"\nrotation\t%lf\t%d",&dps,&flags)==2 && dps>0 && !(flags&DM_SF_UNAVAIL),"%s: rotation %.2f flags %d",CASES[k].name,dps,flags);
        int rows=0,killed=0;
        for(const char *p=strstr(out,"enemy\t");p;p=strstr(p+1,"\nenemy\t")) {
            const char *line=*p=='\n'?p+1:p;char buf[512];size_t len=strcspn(line,"\n");if(len>=sizeof buf)len=sizeof buf-1;memcpy(buf,line,len);buf[len]=0;
            char *field[24];int f=sim_fields(buf,field,24);
            assert(f==21);rows++;
            double kill=atof(field[9]);int row_flags=atoi(field[16]);
            CHECK(kill!=-2 && !(row_flags&DM_SF_UNAVAIL),"%s: %s kill %s flags %d",CASES[k].name,field[2],field[9],row_flags);
            if(kill>0)killed++;
            if(!strcmp(field[1],"0"))CHECK(kill>0,"%s: standard enemy kill %s",CASES[k].name,field[9]);
        }
        CHECK(rows>1 && killed>0,"%s: %d enemy rows, %d with a kill time",CASES[k].name,rows,killed);
        printf("%-16s rotation %6.2f DPS, %d enemies, %d with a kill time\n",CASES[k].name,dps,rows,killed);
    }
    free(out);dm_sim_free(d);free(d);
    if(failures){fprintf(stderr,"sim level 1: %d failures\n",failures);return 1;}
    puts("sim level 1: every class's fresh character has a rotation and kill times");return 0;
}
