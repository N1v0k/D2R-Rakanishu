

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>






int dm_private_aim_vector(int32_t rx,int32_t ry,float out[2]) {
#pragma clang fp contract(off)
    if(!rx && !ry)return 0;
    if(rx>32767)rx=32767;if(rx<-32767)rx=-32767;
    if(ry>32767)ry=32767;if(ry<-32767)ry=-32767;
    float x=(float)rx/32767.0f,y=(float)ry/32767.0f;
    float dx=(x-2.0f*y)*0.5f,dy=(-x-2.0f*y)*0.5f,n=sqrtf(dx*dx+dy*dy);
    if(!(n>0.0f) || !isfinite(n))return 0;
    out[0]=dx/n;out[1]=dy/n;
    return 1;
}
#ifndef DM_PRIVATE_AIM_VECTOR_ONLY
#include "dm.h"
#include "dm_input.h"
#include <mach/mach.h>
static int read_live(uint64_t a,void *out,size_t n) {
    vm_size_t copied=0;
    return a && a<=UINT64_MAX-n && vm_read_overwrite(mach_task_self(),a,n,(vm_address_t)out,&copied)==KERN_SUCCESS && copied==n;
}










#define AIM_CONE_DEG 30.0f
#define AIM_CONE_MAX_DIST 22.0f
static uint64_t g_cone_rec, g_cone_slot_mgr; static uint8_t g_cone_rec_was; static _Atomic int g_cone_set;
static float cone_cos(void) { static float c = -2; if (c < -1.5f) { const char *e = getenv("DM_AIM_CONE"); float d = e ? (float)atof(e) : AIM_CONE_DEG; c = d > 0 ? cosf(d * 3.14159265f / 180.0f) : 2.0f; } return c; }
static int cone_unit_xy(uint64_t u, float *x, float *y) { uint64_t pp = 0; uint16_t a = 0, c = 0; if (!read_live(u + 0x38, &pp, 8) || !pp || !read_live(pp + 2, &a, 2) || !read_live(pp + 6, &c, 2)) return 0; *x = (float)a + 0.5f; *y = (float)c + 0.5f; return 1; }
static void cone_restore(void) {
    if (g_cone_rec) { *(uint8_t *)(g_cone_rec + 0x18) = g_cone_rec_was; g_cone_rec = 0; }
    if (g_cone_slot_mgr) { *(uint64_t *)(g_cone_slot_mgr + 0xd10) = 0; g_cone_slot_mgr = 0; }
    atomic_store(&g_cone_set, 0);
}


void dm_aim_cone_pump(void) {
    if (!atomic_load(&g_cone_set)) return; int32_t rx = 0, ry = 0; dm_input_sampled_right(&rx, &ry);
    if (!(getenv("DM_TEST_AIM") || getenv("DM_TEST_AIM_AT")) && !rx && !ry) { cone_restore(); return; }
}




static uint32_t g_cone_dropped[8]; static int g_cone_ndrop;
static uint64_t cone_pick(const module_t *m, uint64_t pl, const float pos[2], const float vec[2], float cmin, int *out_n, float *out_cos) {
    uint64_t best = 0; float bc = -2; int n = 0;
    for (int k = 0; k < 128; k++) for (uint64_t u = *(uint64_t *)(m->base + 0x2d603a0 + 1 * 0x400 + k * 8); u; u = *(uint64_t *)(u + 0x158)) {
        float x, y; uint32_t mode = *(uint32_t *)(u + 0xc); if (mode == 0 || mode == 12 || !cone_unit_xy(u, &x, &y)) continue;
        if (((int (*)(int, uint64_t, uint64_t))(m->base + 0x2f7b7c))(0x165, pl, u) != 1 || !(((int (*)(uint64_t, int, int, int, int))(m->base + 0x464a70))(u, 0, 0, -1, -1) & 1)) continue;
        { int dr = 0; for (int i = 0; i < g_cone_ndrop; i++) if (g_cone_dropped[i] == *(uint32_t *)(u + 8)) dr = 1; if (dr) continue; }
        float dx = x - pos[0], dy = y - pos[1], d = sqrtf(dx * dx + dy * dy); if (d < 0.01f || d > AIM_CONE_MAX_DIST) continue; n++;
        float c = (dx * vec[0] + dy * vec[1]) / d; if (c >= cmin && c > bc) { bc = c; best = u; } }
    *out_n = n; *out_cos = bc; return best;
}


static int cone_test_dir(const module_t *m, uint64_t pl, const float pos[2], int32_t *rx, int32_t *ry) {
    const char *e = getenv("DM_TEST_AIM_AT"); if (!e) return 0; float ang[64], dist[64]; uint32_t ids[64]; int n = 0;
    static int latched; static float la; if (latched) { *rx = (int32_t)(32767.0f * cosf(la)); *ry = (int32_t)(32767.0f * sinf(la)); return 1; }
    for (int k = 0; k < 128 && n < 64; k++) for (uint64_t u = *(uint64_t *)(m->base + 0x2d603a0 + 1 * 0x400 + k * 8); u && n < 64; u = *(uint64_t *)(u + 0x158)) {
        float x, y; uint32_t mode = *(uint32_t *)(u + 0xc); if (mode == 0 || mode == 12 || !cone_unit_xy(u, &x, &y)) continue;
        if (((int (*)(int, uint64_t, uint64_t))(m->base + 0x2f7b7c))(0x165, pl, u) != 1) continue;
        float dx = x - pos[0], dy = y - pos[1], d = sqrtf(dx * dx + dy * dy); if (d < 0.01f || d > AIM_CONE_MAX_DIST) continue;
        ang[n] = atan2f(-(dx + dy) * 0.5f, dx - dy); dist[n] = d; ids[n] = *(uint32_t *)(u + 8); n++; }
    float a = 0;
    if (!strncmp(e, "gap", 3)) { if (n) { for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (ang[j] < ang[i]) { float t = ang[i]; ang[i] = ang[j]; ang[j] = t; }
            float bg = -1; for (int i = 0; i < n; i++) { float g = (i + 1 < n ? ang[i + 1] : ang[0] + 6.2831853f) - ang[i]; if (g > bg) { bg = g; a = ang[i] + g * 0.5f; } }
            { static int lg; if (lg++ < 6) LOG("瞄准测试方向 gap：22 格内怪 %d 只，朝最大空档中间（屏幕角 %.0f°，空档宽 %.0f°）", n, a * 57.29578f, bg * 57.29578f); } } }
    else { int want = e[1] == '1', bi = -1, b2 = -1; for (int i = 0; i < n; i++) { if (bi < 0 || dist[i] < dist[bi]) { b2 = bi; bi = i; } else if (b2 < 0 || dist[i] < dist[b2]) b2 = i; }
        int pick = want ? b2 : bi; if (pick < 0) return 0; a = ang[pick];
        { static int lg; if (lg++ < 6) LOG("瞄准测试方向 %s：22 格内怪 %d 只，朝 id %u（距 %.1f 格，屏幕角 %.0f°）；最近 id %u 距 %.1f", e, n, ids[pick], dist[pick], a * 57.29578f, ids[bi], dist[bi]); } }
    if (strchr(e, 'L')) { latched = 1; la = a; }
    *rx = (int32_t)(32767.0f * cosf(a)); *ry = (int32_t)(32767.0f * sinf(a)); return 1;
}
void svc_private_skill_aim(uint64_t *regs) {
    const module_t *m=NULL;for(int i=0;i<g_nmod;i++)if(!strcmp(g_mod[i].name,"main"))m=&g_mod[i];
    if(!m)die("skill-aim缺main");
    typedef void (*Original)(uint64_t,uint64_t,float *,float *);
    float *position=(float *)regs[2],*direction=(float *)regs[3];
    ((Original)(m->base+0x2b4ef8))(regs[0],regs[1],position,direction);
    uint64_t caller=0;uint32_t type=99,id=0,current=0;
    int scope=regs[29]<=UINT64_MAX-8 && read_live(regs[29]+8,&caller,8) &&






        (caller==(uint64_t)m->base+0x2b5b80 || caller==(uint64_t)m->base+0x2b3298) &&
        read_live(regs[1],&type,4) && !type && read_live(regs[1]+8,&id,4) &&
        read_live((uint64_t)m->base+0x2d60380,&current,4) && id==current;
    if(!scope)return;
    int32_t rx=0,ry=0;dm_input_sampled_right(&rx,&ry);
    { const char *t=getenv("DM_TEST_AIM");if(t)sscanf(t,"%d,%d",&rx,&ry); }
    { float p0[2]={position[0],position[1]}; cone_test_dir(m,regs[1],p0,&rx,&ry); }
    float vec[2],original[2]={direction[0],direction[1]};int active=dm_private_aim_vector(rx,ry,vec);
    if(!active && atomic_load(&g_cone_set))cone_restore();
    if(active) {




        typedef float (*Range)(uint64_t,uint64_t,int); typedef uint64_t (*Mgr)(void);












        typedef uint64_t (*Rec)(uint64_t,int32_t);
        int32_t sk=-1;read_live(regs[23]+(uint64_t)(uint32_t)regs[25]*0xb8+0xd4c,&sk,4);
        uint64_t mg=((Mgr)(m->base+0x2acecc))(),rec=sk>=0?((Rec)(m->base+0x2b4bc8))(mg,sk):0;
        float dist=0.0f;uint8_t scaled=1,ver=0;uint16_t fn=0;int far=0;
        if(rec && read_live(rec+8,&dist,4) && read_live(rec+0x10,&scaled,1) && dist>0.0f && dist<=2.0f && !scaled && read_live(regs[1]+0x1bd,&ver,1) && ver>=1 && ver<=3) {
            uint64_t tbl=0,base=0,cnt=0;
            if(read_live((uint64_t)m->base+0x2d64a18+(uint64_t)ver*16,&tbl,8) && read_live(tbl+0x11e8,&base,8) && read_live(tbl+0x11f0,&cnt,8) && (uint64_t)sk<cnt &&
               read_live(base+(uint64_t)sk*0x2ec+0x4e,&fn,2)) far=!(fn==27||fn==52||fn==67||fn==76||fn==77||fn==78);
        }
        float rg=((Range)(m->base+0x2b5c70))(mg,regs[1],0),k=(far && rg>0.0f && isfinite(rg))?24.0f/rg:1.0f;
        { static int lg; if(getenv("DM_SKILL_AIM_LOG") && lg++<12)LOG("瞄准适配：技能号 %d 手柄落点距离 %.2f 施放函数 %u 射程 %.2f 放大 %.1f",sk,dist,fn,rg,k); }
        direction[0]=vec[0]*k;direction[1]=vec[1]*k;
        if(cone_cos()<=1.0f) {
            cone_restore(); int nmon=0; float bc=0; float p0[2]={position[0],position[1]};
            uint64_t tgt=cone_pick(m,regs[1],p0,vec,cone_cos(),&nmon,&bc);
            if(tgt){ *(uint64_t *)(mg+0xd10)=tgt; g_cone_slot_mgr=mg; }
            else if(rec){ g_cone_rec=rec; g_cone_rec_was=*(uint8_t *)(rec+0x18); *(uint8_t *)(rec+0x18)=1; }
            { static int dn=-1; static uint32_t lid, cnt; if(dn<0){ const char *e=getenv("DM_TEST_AIM_DROP"); dn=e?atoi(e):0; } if(dn>0&&tgt){ uint32_t id=*(uint32_t *)(tgt+8); cnt=id==lid?cnt+1:1; lid=id; if(cnt>=(uint32_t)dn&&g_cone_ndrop<8){ g_cone_dropped[g_cone_ndrop++]=id; LOG("瞄准测试：id %u 被锁满 %d 次，之后当它死了",id,dn); } } }
            atomic_store(&g_cone_set,1);
            { static int lg; static uint32_t last=0xfffffffe; uint32_t now=tgt?*(uint32_t *)(tgt+8):0xffffffff; if(now!=last && lg++<60){ last=now; uint32_t tc=0,ti=0; if(tgt){ tc=*(uint32_t *)(tgt+4); ti=*(uint32_t *)(tgt+8); }
                LOG("瞄准选目标：22 格内怪 %d 只；手指方向锥内 %s（类号 %u id %u，夹角 %.1f°）→ %s",nmon,tgt?"有":"无",tc,ti,tgt?acosf(bc>1?1:bc)*57.29578f:0.0f,tgt?"锁它":"对位置放"); } }
        }
    }
    const char *path=getenv("DM_SKILL_AIM_LOG");if(path){static FILE *f;static unsigned n;if(!f)f=fopen(path,"w");
        if(f && n++<128){fprintf(f,"{\"event\":\"skill_ground_adapter\",\"time_ns\":%llu,\"caller\":\"%#llx\",\"unit_id\":%u,\"rx\":%d,\"ry\":%d,\"active\":%d,\"position\":[%.9g,%.9g],\"original_direction\":[%.9g,%.9g],\"direction\":[%.9g,%.9g]}\n",now_ns(),caller-(uint64_t)m->base,id,rx,ry,active,position[0],position[1],original[0],original[1],direction[0],direction[1]);fflush(f);}}
}
#endif
