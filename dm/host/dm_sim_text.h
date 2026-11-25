#ifndef DM_SIM_TEXT_H
#define DM_SIM_TEXT_H
#include <math.h>
#include <stdio.h>
#include <string.h>

static inline void dm_sim_number(char out[32],double value,int german) {
    snprintf(out,32,"%.1f",value);
    if(german){char *dot=strchr(out,'.');if(dot)*dot=',';}
}
static inline int dm_sim_delta_text(char *out,size_t cap,const char *label,double before,double after,int german,const char *unit) {
    char a[32],b[32];dm_sim_number(a,before,german);dm_sim_number(b,after,german);
    if(!strcmp(a,b) || (before<=0 && after<=0)){out[0]=0;return 0;}
    return snprintf(out,cap,"\xee\x81\xbe%c%s: %s › %s%s",after>before?'3':'1',label,a,b,unit);
}

static inline int dm_sim_heading_text(char *out,size_t cap,int act,int german) {
    if(act>=1 && act<=5)return snprintf(out,cap,"\xee\x81\xbe" "0%s %d",german?"Kampfsimulation · Standardgegner Akt":"Combat Sim · standard enemy Act",act);
    return snprintf(out,cap,"\xee\x81\xbe" "0%s",german?"Kampfsimulation · Standardgegner":"Combat Sim · standard enemy");
}
static inline int dm_sim_skill_delta_text(char *out,size_t cap,double before,double after,int german) {
    return dm_sim_delta_text(out,cap,german?"Schaden/s jetzt › nächste Stufe":"Damage/s now › next level",before,after,german,"");
}

static inline int dm_sim_skill_first_text(char *out,size_t cap,double after,int german) {
    char a[32];dm_sim_number(a,after,german);
    char zero[32];dm_sim_number(zero,0,german);
    if(after<=0 || !strcmp(a,zero)){out[0]=0;return 0;}
    return snprintf(out,cap,"\xee\x81\xbe" "3%s: %s %s",german?"Erster Punkt":"First point",a,german?"Schaden/s":"damage/s");
}
#endif
