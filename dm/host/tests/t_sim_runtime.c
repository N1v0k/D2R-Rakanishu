#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#include "sim_test_io.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
static const char *overlay;
int dm_overlay_file(const char *relative,char *out,size_t cap) {
    assert(!strcmp(relative,"sim.tsv"));
    if(!overlay)return 0;
    return snprintf(out,cap,"%s",overlay)>0;
}
static double milliseconds(void) {struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec*1000.0+t.tv_nsec/1e6;}
static int compare(const void *a,const void *b){double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
int main(void) {
    const char *path=getenv("DM_SIM_DATA");if(!path){fputs("DM_SIM_DATA is required\n",stderr);return 2;}
    char *out=malloc(262144);assert(out);
    assert(dm_sim_eval_text("class=1\n",out,262144)<0);
    overlay=path;
    const char *profile="class=1\nlevel=80\ndiff=2\nlife=1200\nmana=600\nskill=47,20,20\nbar=47\n";
    char row[96];{size_t len;char *text=sim_read(path,&len);dm_sim_data *d=calloc(1,sizeof *d);
        assert(text && d && !dm_sim_load(d,text,len,"rotw") && d->skill[47]);snprintf(row,sizeof row,"skill\t%s\t",d->skill[47]->name);free(text);dm_sim_free(d);free(d);}
    int n=dm_sim_eval_text(profile,out,262144);assert(n>0);assert(strstr(out,row));assert(strstr(out,"fight"));
    double samples[100];
    for(int i=0;i<100;i++){double start=milliseconds();assert(dm_sim_eval_text(profile,out,262144)>0);samples[i]=milliseconds()-start;}
    qsort(samples,100,sizeof *samples,compare);
    printf("native_entry_ms median=%.4f p99=%.4f max=%.4f calls=100 warm_full_report\n",samples[50],samples[98],samples[99]);
    assert(dm_sim_eval_text(profile,out,10)==-2);assert(!*out);
    assert(dm_sim_eval_text("players=2\n",out,262144)<0);
    assert(dm_sim_eval_text("set=lod\nclass=7\n",out,262144)<0);
    assert(dm_sim_eval_text("set=lod\nclass=1\n",out,262144)>0);
    char temp[]="/tmp/sim-runtime-XXXXXX";int fd=mkstemp(temp);assert(fd>=0);
    size_t size;char *data=sim_read(path,&size);assert(data);data[size/2]^=1;
    FILE *f=fdopen(fd,"wb");assert(f);assert(fwrite(data,1,size,f)==size);assert(!fclose(f));overlay=temp;
    assert(dm_sim_eval_text(profile,out,262144)<0);assert(!*out);
    assert(dm_sim_eval_text(profile,out,262144)<0);assert(!*out);
    struct stat st;assert(!stat(temp,&st));data[size/2]^=1;
    f=fopen(temp,"wb");assert(f);assert(fwrite(data,1,size,f)==size);assert(!fclose(f));
    struct timespec times[2];times[0].tv_sec=st.st_atime;times[0].tv_nsec=0;
#ifdef __APPLE__
    times[1]=st.st_mtimespec;
#else
    times[1]=st.st_mtim;
#endif
    assert(!utimensat(AT_FDCWD,temp,times,0));
    assert(dm_sim_eval_text(profile,out,262144)<0);
    times[1].tv_sec++;assert(!utimensat(AT_FDCWD,temp,times,0));
    assert(dm_sim_eval_text(profile,out,262144)>0);
    unlink(temp);free(data);free(out);puts("sim runtime: overlay, mode changes, invalid input, capacity and corruption passed");return 0;
}
