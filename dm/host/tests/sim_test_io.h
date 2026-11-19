#ifndef SIM_TEST_IO_H
#define SIM_TEST_IO_H
#include "../dm_sim_core.h"
#include <stdio.h>
#include <stdlib.h>
static char *sim_read(const char *path,size_t *n) {
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long size=ftell(f);
    if(size<0 || size>16000000){fclose(f);return NULL;}rewind(f);
    char *s=malloc((size_t)size+1);if(!s){fclose(f);return NULL;}
    *n=fread(s,1,(size_t)size,f);fclose(f);
    if(*n!=(size_t)size){free(s);return NULL;}s[*n]=0;return s;
}
#endif
