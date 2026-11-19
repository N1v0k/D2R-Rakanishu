#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#include "dm_sim_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <sys/types.h>

extern int dm_overlay_file(const char *, char *, size_t);


typedef struct sim_generation { dm_sim_data data; int pins; struct sim_generation *next; } sim_generation;
static pthread_mutex_t sim_lock=PTHREAD_MUTEX_INITIALIZER;
static sim_generation *sim_current,*sim_retired;
static char sim_path[1200],sim_set[8];
static time_t sim_mtime;
static long sim_nsec;
static ino_t sim_inode;
static long nanoseconds(const struct stat *s) {
#ifdef __APPLE__
    return s->st_mtimespec.tv_nsec;
#else
    return s->st_mtim.tv_nsec;
#endif
}
static off_t sim_size;
static unsigned sim_generation_count;
static atomic_uint sim_generation_now;

static void retire(sim_generation *g) {
    if(!g)return;
    if(g->pins){g->next=sim_retired;sim_retired=g;return;}
    dm_sim_free(&g->data);free(g);
}

static const dm_sim_data *current_locked(const char *set) {
    char path[1200];struct stat st;
    if(!dm_overlay_file("sim.tsv",path,sizeof path) || stat(path,&st) || st.st_size<1 || st.st_size>16000000)return NULL;
    if(strcmp(path,sim_path) || strcmp(set,sim_set) || st.st_mtime!=sim_mtime || st.st_size!=sim_size || nanoseconds(&st)!=sim_nsec || st.st_ino!=sim_inode) {
        retire(sim_current);sim_current=NULL;
        FILE *f=fopen(path,"rb");if(!f)return NULL;
        sim_generation *g=calloc(1,sizeof *g);char *text=malloc((size_t)st.st_size+1);
        if(!g || !text){free(g);free(text);fclose(f);return NULL;}
        size_t n=fread(text,1,(size_t)st.st_size,f);fclose(f);text[n]=0;
        if(n==(size_t)st.st_size)dm_sim_load(&g->data,text,n,set);
        g->data.generation=++sim_generation_count;atomic_store(&sim_generation_now,g->data.generation);
        free(text);sim_current=g;
        snprintf(sim_path,sizeof sim_path,"%s",path);snprintf(sim_set,sizeof sim_set,"%s",set);sim_mtime=st.st_mtime;sim_size=st.st_size;sim_nsec=nanoseconds(&st);sim_inode=st.st_ino;
    }
    return sim_current && sim_current->data.ok?&sim_current->data:NULL;
}
static int read_locked(const char *set, dm_sim_reader reader, void *ctx) {
    const dm_sim_data *d=current_locked(set);
    return d?reader(d,ctx):-1;
}

int dm_sim_read(const char *set, dm_sim_reader reader, void *ctx) {
    pthread_mutex_lock(&sim_lock);int rc=read_locked(set,reader,ctx);pthread_mutex_unlock(&sim_lock);return rc;
}
int dm_sim_try_read(const char *set, dm_sim_reader reader, void *ctx) {
    if(pthread_mutex_trylock(&sim_lock))return -3;
    int rc=read_locked(set,reader,ctx);pthread_mutex_unlock(&sim_lock);return rc;
}
const dm_sim_data *dm_sim_pin(const char *set) {
    pthread_mutex_lock(&sim_lock);
    const dm_sim_data *d=current_locked(set);
    if(d)sim_current->pins++;
    pthread_mutex_unlock(&sim_lock);
    return d;
}
const dm_sim_data *dm_sim_try_pin(const char *set, int *busy) {
    if(pthread_mutex_trylock(&sim_lock)){if(busy)*busy=1;return NULL;}
    if(busy)*busy=0;
    const dm_sim_data *d=current_locked(set);
    if(d)sim_current->pins++;
    pthread_mutex_unlock(&sim_lock);
    return d;
}
unsigned dm_sim_generation(void) { return atomic_load(&sim_generation_now); }
void dm_sim_unpin(const dm_sim_data *d) {
    if(!d)return;
    pthread_mutex_lock(&sim_lock);
    sim_generation *g=(sim_generation *)d;
    if(--g->pins==0 && g!=sim_current) {
        for(sim_generation **p=&sim_retired;*p;p=&(*p)->next)if(*p==g){*p=g->next;break;}
        dm_sim_free(&g->data);free(g);
    }
    pthread_mutex_unlock(&sim_lock);
}

typedef struct { const char *profile; char *out; size_t cap; } report_request;
static int report_read(const dm_sim_data *d, void *ctx) {
    report_request *q=ctx;dm_sim_char c;char err[128];
    if(dm_sim_profile_parse(d,q->profile,&c,err,sizeof err))return -1;
    return dm_sim_report(d,&c,1,q->out,q->cap);
}
int dm_sim_eval_text(const char *profile,char *out,size_t cap) {
    if(!profile || !out || !cap || strlen(profile)>65536)return -1;
    out[0]=0;const char *set="rotw";
    for(const char *p=profile;p && *p;) {if(!strncmp(p,"set=lod\n",8) || !strcmp(p,"set=lod"))set="lod";p=strchr(p,'\n');if(p)p++;}
    report_request q={profile,out,cap};return dm_sim_read(set,report_read,&q);
}
