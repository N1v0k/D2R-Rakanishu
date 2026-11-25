



















#include "../../host/hle.h"
#include "nvn_protos.h"
#include "nvn_pool_flags_registry.h"
#include "nvn_count.h"
#include "nvn_tex.h"
#include "../xtrace.h"
#include "nvn_mtl.h"
#include "nvn_pace.h"
#include "nvn_adaptive_pace.h"
#include "../../host/front.h"
#include "nvn_dirty.h"
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <TargetConditionals.h>
#include <unistd.h>
#if TARGET_OS_OSX
#define DM_MEM_CAP_DEFAULT_GB 24
#else
#define DM_MEM_CAP_DEFAULT_GB 0
#endif
#include <pthread/qos.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <mach/thread_policy.h>
#endif
#include <assert.h>
#include <stdatomic.h>



typedef struct { _Atomic uint64_t cnt; void *fn; } nvn_slot_t;
nvn_slot_t nvn_slot[NVN_NFUNC];
_Static_assert(sizeof(nvn_slot_t) == 16, "跳板按 16 字节步长寻址");
extern void *nvn_thunk[NVN_NFUNC];
extern const char *nvn_names[NVN_NFUNC];
extern void nvn_unimpl_entry(void);
extern void nvn_prof_entry(void);
void *nvn_real[NVN_NFUNC];
_Atomic uint64_t nvn_prof_ticks[NVN_NFUNC];
static uint8_t g_unimpl_seen[NVN_NFUNC];

uint64_t nvn_unimpl_c(nvn_slot_t *slot, uint64_t x0, uint64_t x1, uint64_t x2) {
    unsigned i = (unsigned)(slot - nvn_slot);
    if (!g_unimpl_seen[i]++) LOG("【红】NVN 未实现被调用：%s (x0=%#llx x1=%#llx x2=%#llx)", nvn_names[i], x0, x1, x2);
    return 0;
}

typedef struct { const char *name; void *fn; } nvn_impl;
#define NVN(name, ret, ...)                                                                       \
    static ret h_##name(__VA_ARGS__);                                                             \
    static const NVNT_##name chk_##name __attribute__((unused)) = h_##name;                       \
    DM_SECT(swnvn) static const nvn_impl e_##name = {#name, (void *)h_##name}; \
    static ret h_##name(__VA_ARGS__)


#define NVN_NOPROTO(name, ret, ...)                                                               \
    static ret h_##name(__VA_ARGS__);                                                             \
    DM_SECT(swnvn) static const nvn_impl e_##name = {#name, (void *)h_##name}; \
    static ret h_##name(__VA_ARGS__)

static int g_log;
uint64_t nvn_new_serial(void) { static _Atomic uint64_t n; return ++n; }
static unsigned g_nsem;
#define OLOG(...) do { if (g_log) LOG("[nvn] " __VA_ARGS__); } while (0)

static void nvn_init_slots(void) {
    static int done; if (done) return; done = 1;
    g_log = getenv("DM_NVN_LOG") != NULL;
    unsigned long sz; const nvn_impl *im;
    DM_SECT_GET(swnvn, const nvn_impl, im, sz);
    unsigned nim = im ? (unsigned)(sz / sizeof *im) : 0, hit = 0;
    for (unsigned i = 0; i < NVN_NFUNC; i++) nvn_slot[i].fn = (void *)nvn_unimpl_entry;
    for (unsigned k = 0; k < nim; k++) {
        unsigned i = 0;
        while (i < NVN_NFUNC && strcmp(nvn_names[i], im[k].name)) i++;
        if (i == NVN_NFUNC) die("NVN 实现 %s 不在 main 的名字表里", im[k].name);
        nvn_slot[i].fn = im[k].fn; hit++;
    }
    g_nsem = hit;
}

static void *nvn_lookup(const char *name) {
    nvn_init_slots();
    for (unsigned i = 0; i < NVN_NFUNC; i++)
        if (!strcmp(nvn_names[i], name)) return nvn_thunk[i];
    LOG("【红】NVN 取址未知名字：%s", name ? name : "(null)");
    return NULL;
}



HLE_FORCE("nvnBootstrapLoader", "NVN 图形接口整块换成宿主空设备（后续 Metal），sdk 的 NVN 驱动不执行",
          void *, h_nvnBootstrapLoader, const char *name) { return nvn_lookup(name); }



#define B(x) ((x) & 0xff)

#include "nvn_obj.h"



NVN(nvnDeviceBuilderSetDefaults, void, NVNdeviceBuilder *b) { memset(b, 0, sizeof(dev_builder_t)); }
NVN(nvnDeviceBuilderSetFlags, void, NVNdeviceBuilder *b, int f) { ((dev_builder_t *)b)->flags = f; }
NVN(nvnDeviceInitialize, NVNboolean, NVNdevice *d, const NVNdeviceBuilder *b) {
    dev_t_ *o = (dev_t_ *)d; memset(o, 0, sizeof *o);
    o->magic = MAGIC('D', 'E', 'V', 0); o->flags = ((const dev_builder_t *)b)->flags;
    OLOG("DeviceInitialize %p flags=%#x", (void *)d, o->flags);
    return 1;
}
NVN(nvnDeviceFinalize, void, NVNdevice *d) { ((dev_t_ *)d)->magic = 0; }
NVN(nvnDeviceGetProcAddress, PFNNVNGENERICFUNCPTRPROC, const NVNdevice *d, const char *name) {
    (void)d; return (PFNNVNGENERICFUNCPTRPROC)nvn_lookup(name);
}



static const int g_devinfo[] = {
    [0x00] = 53, [0x01] = 311,
    [0x02] = 14, [0x03] = 65536, [0x04] = 256, [0x05] = 8, [0x06] = 16, [0x07] = 4, [0x08] = 16, [0x09] = 32,
    [0x0A] = 16, [0x0B] = 4, [0x0C] = 4, [0x0D] = 4, [0x0E] = 16, [0x0F] = 32, [0x10] = 32, [0x11] = 256, [0x12] = 256,
    [0x13] = 4, [0x14] = 8, [0x15] = 512, [0x16] = 256, [0x17] = 2, [0x18] = 256, [0x19] = 4096, [0x1A] = 131072,
    [0x1B] = 16, [0x1C] = 1536, [0x1D] = 1024, [0x1E] = 64, [0x1F] = 1536, [0x20] = 65535, [0x21] = 65535, [0x22] = 65535,
    [0x23] = 8, [0x24] = 1048576, [0x25] = 4096, [0x26] = 16, [0x27] = 4096, [0x28] = 1, [0x29] = 1, [0x2A] = 1,
    [0x2B] = 262144, [0x2C] = 15, [0x2D] = 2048, [0x2E] = 1, [0x2F] = 1, [0x30] = 14, [0x31] = 5, [0x32] = 1, [0x33] = 8,
    [0x34] = 8, [0x35] = 4, [0x36] = 32, [0x37] = 128, [0x38] = 32, [0x39] = 128, [0x3A] = 65536, [0x3B] = 1, [0x3C] = 4,
    [0x3D] = 16384, [0x3E] = 134217728, [0x3F] = 2048, [0x40] = 16384, [0x41] = 16384, [0x42] = 1, [0x43] = 1, [0x44] = 1,
    [0x45] = 1, [0x46] = 5, [0x47] = 1, [0x48] = 1, [0x49] = 1, [0x4A] = 1, [0x4B] = 1, [0x4C] = 1, [0x4D] = 16,
    [0x4E] = 1024, [0x4F] = 32, [0x50] = 4096, [0x51] = 65536, [0x52] = 65536, [0x53] = 256, [0x54] = 16384,
    [0x55] = 262144, [0x56] = 4096, [0x57] = 1, [0x58] = 4, [0x59] = 2, [0x5A] = 0, [0x5B] = 4096, [0x5C] = 16384,
    [0x5D] = 4096, [0x5E] = 128, [0x5F] = 4096,
};
NVN(nvnDeviceGetInteger, void, const NVNdevice *d, NVNdeviceInfo what, int *out) {
    (void)d;
    unsigned w = (unsigned)what;


    if (w >= sizeof g_devinfo / sizeof *g_devinfo) { LOG("【红】nvnDeviceGetInteger 未知项 %#x，给 0", w); *out = 0; return; }
    *out = g_devinfo[w];
    OLOG("DeviceGetInteger %#x -> %d", w, *out);
}
NVN(nvnDeviceGetImageHandle, NVNimageHandle, const NVNdevice *d, int id) { (void)d; return (NVNimageHandle)id; }


NVN(nvnDeviceGetSeparateTextureHandle, NVNseparateTextureHandle, const NVNdevice *d, int id) { (void)d; return (NVNseparateTextureHandle){(uint64_t)id}; }
NVN(nvnDeviceGetSeparateSamplerHandle, NVNseparateSamplerHandle, const NVNdevice *d, int id) { (void)d; return (NVNseparateSamplerHandle){(uint64_t)id << 20}; }
NVN(nvnDeviceInstallDebugCallback, void, NVNdevice *d, const PFNNVNDEBUGCALLBACKPROC cb, void *data, uint32_t enable) {
    dev_t_ *o = (dev_t_ *)d; o->dbg_cb = B(enable) ? (void *)cb : NULL; o->dbg_data = data;
}
NVN(nvnDeviceSetWindowOriginMode, void, NVNdevice *d, NVNwindowOriginMode m) { ((dev_t_ *)d)->origin = m; LOG("[nvn] 窗口原点 %s", m ? "左上（同 D3D / Metal）" : "左下（GL）"); }
NVN(nvnDeviceSetDepthMode, void, NVNdevice *d, NVNdepthMode m) { ((dev_t_ *)d)->depth_mode = m; LOG("[nvn] 深度模式 %s", m ? "近=0（同 D3D / Metal）" : "近=-w（GL）"); }
NVN(nvnDeviceFinalizeCommandHandle, void, NVNdevice *d, NVNcommandHandle h) { (void)d; (void)h; }



NVN(nvnMemoryPoolBuilderSetDefaults, void, NVNmemoryPoolBuilder *b) { memset(b, 0, sizeof(pool_builder_t)); }
NVN(nvnMemoryPoolBuilderSetDevice, void, NVNmemoryPoolBuilder *b, NVNdevice *d) { ((pool_builder_t *)b)->dev = d; }
NVN(nvnMemoryPoolBuilderSetStorage, void, NVNmemoryPoolBuilder *b, void *m, size_t n) { ((pool_builder_t *)b)->mem = m; ((pool_builder_t *)b)->size = n; }
NVN(nvnMemoryPoolBuilderSetFlags, void, NVNmemoryPoolBuilder *b, int f) { ((pool_builder_t *)b)->flags = f; }







static nvn_pool_flags_registry g_pool_flags;
static pthread_mutex_t g_pool_flags_mu = PTHREAD_MUTEX_INITIALIZER;
static void pool_flags_add(const void *owner,const void *mem, uint64_t size, int flags) {
    pthread_mutex_lock(&g_pool_flags_mu);
    nvn_pool_flags_registry_add(&g_pool_flags,owner,mem,size,flags);
    pthread_mutex_unlock(&g_pool_flags_mu);
}
static void pool_flags_del(const void *owner) {
    pthread_mutex_lock(&g_pool_flags_mu);
    nvn_pool_flags_registry_del(&g_pool_flags,owner);
    pthread_mutex_unlock(&g_pool_flags_mu);
}


int nvn_pool_flags(const void *addr, size_t len) {
    pthread_mutex_lock(&g_pool_flags_mu);
    int f=nvn_pool_flags_registry_get(&g_pool_flags,addr,len);
    pthread_mutex_unlock(&g_pool_flags_mu);
    return f;
}
NVN(nvnMemoryPoolInitialize, NVNboolean, NVNmemoryPool *p, const NVNmemoryPoolBuilder *b) {
    const pool_builder_t *s = (const pool_builder_t *)b; pool_t *o = (pool_t *)p;
    o->magic = MAGIC('P', 'O', 'O', 'L'); o->flags = s->flags; o->mem = s->mem; o->size = s->size;
    if (o->mem) { mtl_pool_add(o->mem, o->size); nvn_dirty_add_range(o->mem, o->size); pool_flags_add(p,o->mem, o->size, o->flags); }
    if (!o->mem && !(o->flags & 0x200)) LOG("【红】MemoryPoolInitialize 非虚拟池没有存储 flags=%#x size=%#llx", o->flags, o->size);
    OLOG("MemoryPool %p mem=%p size=%#llx flags=%#x", (void *)p, (void *)o->mem, o->size, o->flags);
    return 1;
}
NVN(nvnMemoryPoolFinalize, void, NVNmemoryPool *p) { pool_flags_del(p); if (((pool_t *)p)->mem) { nvn_dirty_remove_range(((pool_t *)p)->mem); mtl_pool_del(((pool_t *)p)->mem); } ((pool_t *)p)->magic = 0; }
NVN(nvnMemoryPoolFlushMappedRange, void, const NVNmemoryPool *p, ptrdiff_t off, size_t n) { (void)p; (void)off; (void)n; }
NVN(nvnMemoryPoolGetBufferAddress, NVNbufferAddress, const NVNmemoryPool *p) { return (NVNbufferAddress)((const pool_t *)p)->mem; }



NVN(nvnTexturePoolInitialize, NVNboolean, NVNtexturePool *tp, const NVNmemoryPool *p, ptrdiff_t off, int n) {
    *(dpool_t *)tp = (dpool_t){(const pool_t *)p, off, n}; OLOG("TexturePool n=%d", n); mtl_set_tex_pool_n(n); return 1;
}
NVN(nvnTexturePoolFinalize, void, NVNtexturePool *tp) { (void)tp; }
NVN(nvnTexturePoolRegisterTexture, void, const NVNtexturePool *tp, int id, const NVNtexture *t, const NVNtextureView *v) { (void)tp; mtl_reg_tex(id, (const tex_t *)t, (const view_t *)v); }
NVN(nvnSamplerPoolInitialize, NVNboolean, NVNsamplerPool *sp, const NVNmemoryPool *p, ptrdiff_t off, int n) {
    *(dpool_t *)sp = (dpool_t){(const pool_t *)p, off, n}; OLOG("SamplerPool n=%d", n); return 1;
}
NVN(nvnSamplerPoolFinalize, void, NVNsamplerPool *sp) { (void)sp; }
NVN(nvnSamplerPoolRegisterSampler, void, const NVNsamplerPool *sp, int id, const NVNsampler *s) { (void)sp; mtl_reg_samp(id, &((const samp_t *)s)->p); }



#define TB(b) ((tex_builder_t *)(b))
NVN(nvnTextureBuilderSetDefaults, void, NVNtextureBuilder *b) {
    NVNdevice *dev = TB(b)->dev; memset(b, 0, sizeof(tex_builder_t)); TB(b)->dev = dev;
    TB(b)->d.target = NVN_TEXTURE_TARGET_2D; TB(b)->d.width = TB(b)->d.height = TB(b)->d.depth = TB(b)->d.levels = 1;
}
NVN(nvnTextureBuilderSetDevice, void, NVNtextureBuilder *b, NVNdevice *d) { TB(b)->dev = d; }
NVN(nvnTextureBuilderSetFlags, void, NVNtextureBuilder *b, int f) { TB(b)->d.flags = f; }
NVN(nvnTextureBuilderSetTarget, void, NVNtextureBuilder *b, NVNtextureTarget t) { TB(b)->d.target = t; }
NVN(nvnTextureBuilderSetWidth, void, NVNtextureBuilder *b, int v) { TB(b)->d.width = v; }
NVN(nvnTextureBuilderSetHeight, void, NVNtextureBuilder *b, int v) { TB(b)->d.height = v; }
NVN(nvnTextureBuilderSetDepth, void, NVNtextureBuilder *b, int v) { TB(b)->d.depth = v; }
NVN(nvnTextureBuilderSetSize1D, void, NVNtextureBuilder *b, int w) { TB(b)->d.width = w; TB(b)->d.height = TB(b)->d.depth = 1; }
NVN(nvnTextureBuilderSetLevels, void, NVNtextureBuilder *b, int v) { TB(b)->d.levels = v; }
NVN(nvnTextureBuilderSetFormat, void, NVNtextureBuilder *b, NVNformat f) { TB(b)->d.format = f; }
NVN(nvnTextureBuilderSetSamples, void, NVNtextureBuilder *b, int s) { TB(b)->d.samples = s; }
NVN(nvnTextureBuilderSetStride, void, NVNtextureBuilder *b, ptrdiff_t s) { TB(b)->d.stride = (uint32_t)s; }
NVN(nvnTextureBuilderSetStorage, void, NVNtextureBuilder *b, NVNmemoryPool *p, ptrdiff_t off) { TB(b)->pool = (pool_t *)p; TB(b)->off = off; }
NVN(nvnTextureBuilderSetPackagedTextureLayout, void, NVNtextureBuilder *b, const NVNpackagedTextureLayout *l) {
    TB(b)->has_layout = l != NULL; if (l) memcpy(TB(b)->layout, l->layout, 8);
    TB(b)->d.pkg_block_h = l ? l->layout[0] + 1 : 0;
}
NVN(nvnTextureBuilderGetFlags, NVNtextureFlags, const NVNtextureBuilder *b) { return (NVNtextureFlags)((const tex_builder_t *)b)->d.flags; }
static uint64_t g_layout_logged;
static void log_layout(const tex_builder_t *b, const nvn_texlayout *L) {
    if (b->has_layout && g_layout_logged++ < 16)
        LOG("[nvn] 预排布贴图 %ux%ux%u 级%u 格式%#x 目标%d 布局字节 %02x%02x%02x%02x%02x%02x%02x%02x → 块高 log2=%u（取布局第 0 字节）尺寸 %#llx",
            b->d.width, b->d.height, b->d.depth, b->d.levels, b->d.format, b->d.target, b->layout[0], b->layout[1], b->layout[2],
            b->layout[3], b->layout[4], b->layout[5], b->layout[6], b->layout[7], L->tileH, L->size);
}
NVN(nvnTextureBuilderGetStorageSize, size_t, const NVNtextureBuilder *b) {
    nvn_texlayout L; nvn_tex_layout(&((const tex_builder_t *)b)->d, &L);
    if (!L.bpb) LOG("【红】TextureBuilderGetStorageSize 未知格式 %#x", ((const tex_builder_t *)b)->d.format);
    log_layout((const tex_builder_t *)b, &L);
    return L.size;
}
NVN(nvnTextureBuilderGetStorageAlignment, size_t, const NVNtextureBuilder *b) {
    nvn_texlayout L; nvn_tex_layout(&((const tex_builder_t *)b)->d, &L); return L.align;
}
#define TX(t) ((tex_t *)(t))
#define CTX(t) ((const tex_t *)(t))








typedef union snap_slot { tex_t t; prog_t p; union snap_slot *next; } snap_slot;
#define SNAP_KEEP 8
static pthread_mutex_t g_snap_mu = PTHREAD_MUTEX_INITIALIZER;
static snap_slot *g_snap_free;
static struct { snap_slot *s; uint64_t at; } *g_snap_ret; static size_t g_snap_nret, g_snap_capret, g_snap_live, g_snap_slots;
static uint64_t g_snap_sub;
static _Atomic uint64_t g_x_subs;
static snap_slot *snap_alloc(void) {
    if (!g_snap_free) {
        snap_slot *c = calloc(256, sizeof *c); if (!c) { LOG("【红】对象副本分配失败"); abort(); }
        for (int i = 255; i >= 0; i--) { c[i].next = g_snap_free; g_snap_free = &c[i]; }
        g_snap_slots += 256;
        LOG("[nvn] 对象副本槽位扩到 %zu 个（%zu KB）：在用 %zu，其中待回收 %zu，第 %llu 次提交", g_snap_slots, g_snap_slots * sizeof *c >> 10, g_snap_live, g_snap_nret, (unsigned long long)g_snap_sub);
    }
    snap_slot *n = g_snap_free; g_snap_free = n->next; memset(n, 0, sizeof *n); g_snap_live++;
    return n;
}
static void snap_retire(const void *s) {
    if (g_snap_nret == g_snap_capret) { g_snap_capret = g_snap_capret ? g_snap_capret * 2 : 256; g_snap_ret = realloc(g_snap_ret, g_snap_capret * sizeof *g_snap_ret); }
    g_snap_ret[g_snap_nret].s = (snap_slot *)s; g_snap_ret[g_snap_nret++].at = atomic_load(&g_x_subs);
}
static void snap_tick(uint64_t done) {
    pthread_mutex_lock(&g_snap_mu);
    g_snap_sub = done; size_t k = 0;
    for (size_t i = 0; i < g_snap_nret; i++) {
        if (g_snap_ret[i].at + SNAP_KEEP <= g_snap_sub) { snap_slot *n = g_snap_ret[i].s; memset(n, 0, sizeof *n); n->next = g_snap_free; g_snap_free = n; g_snap_live--; }
        else g_snap_ret[k++] = g_snap_ret[i];
    }
    g_snap_nret = k;
    pthread_mutex_unlock(&g_snap_mu);
}
static const tex_t *snap_tex(const NVNtexture *t) {
    tex_t *o = TX(t); if (!o || !o->serial) return o;
    const tex_t *s = __atomic_load_n((const tex_t **)&o->snap, __ATOMIC_ACQUIRE);
    if (s && s->serial == o->serial) return s;
    pthread_mutex_lock(&g_snap_mu);
    s = o->snap;
    if (!s || s->serial != o->serial) { snap_slot *n = snap_alloc(); n->t = *o; n->t.snap = NULL; s = &n->t; __atomic_store_n((const tex_t **)&o->snap, s, __ATOMIC_RELEASE); }
    pthread_mutex_unlock(&g_snap_mu);
    return s;
}
static const prog_t *snap_prog(const NVNprogram *p) {
    prog_t *o = (prog_t *)p; if (!o || !o->serial) return o;
    const prog_t *s = __atomic_load_n((const prog_t **)&o->snap, __ATOMIC_ACQUIRE);
    if (s && s->serial == o->serial) return s;
    pthread_mutex_lock(&g_snap_mu);
    s = o->snap;
    if (!s || s->serial != o->serial) { snap_slot *n = snap_alloc(); n->p = *o; n->p.snap = NULL; s = &n->p; __atomic_store_n((const prog_t **)&o->snap, s, __ATOMIC_RELEASE); }
    pthread_mutex_unlock(&g_snap_mu);
    return s;
}
static void x_drain(void);
static uint64_t x_fence(void);


static void snap_drop(const void **pp, uint64_t ser, uint64_t snap_ser_off) {
    pthread_mutex_lock(&g_snap_mu);
    const uint8_t *s = *pp; uint64_t ss; if (s) memcpy(&ss, s + snap_ser_off, 8);
    if (s && ser && ss == ser) snap_retire(s);
    *pp = NULL;
    pthread_mutex_unlock(&g_snap_mu);
}
NVN(nvnTextureInitialize, NVNboolean, NVNtexture *t, const NVNtextureBuilder *b) {
    const tex_builder_t *s = (const tex_builder_t *)b; tex_t *o = TX(t);
    memset(o, 0, sizeof *o);
    o->magic = MAGIC('T', 'E', 'X', 0); o->d = s->d; o->pool = s->pool; o->off = s->off;
    o->has_layout = s->has_layout; memcpy(o->layout, s->layout, 8);
    nvn_tex_layout(&o->d, &o->L);
    o->mem = s->pool && s->pool->mem ? s->pool->mem + s->off : NULL;
    o->serial = nvn_new_serial();
    OLOG("Texture %p %ux%ux%u lv%u fmt=%#x tgt=%d flags=%#x size=%#llx", (void *)t, o->d.width, o->d.height, o->d.depth,
         o->d.levels, o->d.format, o->d.target, o->d.flags, o->L.size);
    return 1;
}
NVN(nvnTextureFinalize, void, NVNtexture *t) { snap_drop(&TX(t)->snap, TX(t)->serial, offsetof(tex_t, serial)); mtl_tex_release(TX(t)); TX(t)->magic = 0; }
NVN(nvnTextureGetFlags, NVNtextureFlags, const NVNtexture *t) { return (NVNtextureFlags)CTX(t)->d.flags; }
NVN(nvnTextureGetWidth, int, const NVNtexture *t) { return (int)CTX(t)->d.width; }
NVN(nvnTextureGetHeight, int, const NVNtexture *t) { return (int)CTX(t)->d.height; }
NVN(nvnTextureGetDepth, int, const NVNtexture *t) { return (int)CTX(t)->d.depth; }
NVN(nvnTextureGetStorageSize, int, const NVNtexture *t) { return (int)CTX(t)->L.size; }
NVN(nvnTextureCompare, NVNboolean, const NVNtexture *a, const NVNtexture *b) {
    const tex_t *x = CTX(a), *y = CTX(b);
    return !memcmp(&x->d, &y->d, sizeof x->d) && x->mem == y->mem;
}


NVN_NOPROTO(nvnTextureGetRawStorageClass, int, const NVNtexture *t) { (void)t; return 1; }
#define VW(v) ((view_t *)(v))
NVN(nvnTextureViewSetDefaults, void, NVNtextureView *v) { memset(v, 0, sizeof(view_t)); }
NVN(nvnTextureViewSetLevels, void, NVNtextureView *v, int base, int n) { VW(v)->set_levels = 1; VW(v)->base_level = base; VW(v)->nlevels = n; }
NVN(nvnTextureViewSetLayers, void, NVNtextureView *v, int min, int n) { VW(v)->set_layers = 1; VW(v)->min_layer = min; VW(v)->nlayers = n; }
NVN(nvnTextureViewSetFormat, void, NVNtextureView *v, NVNformat f) { VW(v)->set_format = 1; VW(v)->format = f; }
NVN(nvnTextureViewSetTarget, void, NVNtextureView *v, NVNtextureTarget t) { VW(v)->set_target = 1; VW(v)->target = t; }
NVN(nvnTextureViewGetLayers, NVNboolean, const NVNtextureView *v, int *min, int *n) {
    const view_t *o = (const view_t *)v; *min = o->min_layer; *n = o->nlayers; return o->set_layers;
}
NVN(nvnTextureGetViewOffset, ptrdiff_t, const NVNtexture *t, const NVNtextureView *v) {
    const tex_t *o = CTX(t); const view_t *w = (const view_t *)v;
    if (!w) return 0;
    uint64_t off = w->set_levels ? nvn_tex_level_offset(&o->d, (unsigned)w->base_level) : 0;
    if (w->set_layers) off += (uint64_t)w->min_layer * o->L.layer_size;
    return (ptrdiff_t)off;
}


static void read_texels(const tex_t *o, const view_t *v, const NVNcopyRegion *r, uint8_t *dst, ptrdiff_t row, ptrdiff_t img) {
    int bw, bh, bpb = nvn_fmt_bpb(o->d.format, &bw, &bh);
    uint32_t wb = (uint32_t)((r->width + bw - 1) / bw) * bpb, hb = (uint32_t)(r->height + bh - 1) / bh;
    if (!row) row = wb;
    if (!img) img = row * hb;
    unsigned level = v && v->set_levels ? (unsigned)v->base_level : 0, layer0 = v && v->set_layers ? (unsigned)v->min_layer : 0;
    x_drain();
    mtl_read_texels(o, level, layer0, r, dst, (size_t)row, (size_t)img);
}
NVN(nvnTextureReadTexels, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r, void *dst) {
    read_texels(CTX(t), (const view_t *)v, r, dst, 0, 0);
}
NVN(nvnTextureReadTexelsStrided, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r, void *dst, ptrdiff_t row, ptrdiff_t img) {
    read_texels(CTX(t), (const view_t *)v, r, dst, row, img);
}
NVN(nvnTextureInvalidateTexels, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r) { (void)t; (void)v; (void)r; }



#define SB(b) (&((samp_builder_t *)(b))->p)
NVN(nvnSamplerBuilderSetDefaults, void, NVNsamplerBuilder *b) {
    NVNdevice *dev = ((samp_builder_t *)b)->dev; memset(b, 0, sizeof(samp_builder_t)); ((samp_builder_t *)b)->dev = dev;
    samp_par *o = SB(b);
    o->minf = NVN_MIN_FILTER_NEAREST_MIPMAP_LINEAR; o->magf = NVN_MAG_FILTER_LINEAR;
    o->ws = o->wt = o->wr = NVN_WRAP_MODE_REPEAT; o->cmp_func = NVN_COMPARE_FUNC_LESS;
    o->lod_min = -1000.f; o->lod_max = 1000.f; o->aniso = 1.f;
}
NVN(nvnSamplerBuilderSetDevice, void, NVNsamplerBuilder *b, NVNdevice *d) { ((samp_builder_t *)b)->dev = d; }
NVN(nvnSamplerBuilderSetMinMagFilter, void, NVNsamplerBuilder *b, NVNminFilter mi, NVNmagFilter ma) { SB(b)->minf = mi; SB(b)->magf = ma; }
NVN(nvnSamplerBuilderSetWrapMode, void, NVNsamplerBuilder *b, NVNwrapMode s, NVNwrapMode t, NVNwrapMode r) { SB(b)->ws = s; SB(b)->wt = t; SB(b)->wr = r; }
NVN(nvnSamplerBuilderSetLodClamp, void, NVNsamplerBuilder *b, float lo, float hi) { SB(b)->lod_min = lo; SB(b)->lod_max = hi; }
NVN(nvnSamplerBuilderSetLodBias, void, NVNsamplerBuilder *b, float v) { SB(b)->lod_bias = v; }
NVN(nvnSamplerBuilderSetCompare, void, NVNsamplerBuilder *b, NVNcompareMode m, NVNcompareFunc f) { SB(b)->cmp_mode = m; SB(b)->cmp_func = f; }
NVN(nvnSamplerBuilderSetBorderColor, void, NVNsamplerBuilder *b, const float *c) { memcpy(SB(b)->border, c, 16); }
NVN(nvnSamplerBuilderSetMaxAnisotropy, void, NVNsamplerBuilder *b, float v) { SB(b)->aniso = v; }
NVN(nvnSamplerBuilderSetLodSnap, void, NVNsamplerBuilder *b, float v) { (void)b; (void)v; }
NVN(nvnSamplerBuilderGetMinMagFilter, void, const NVNsamplerBuilder *b, NVNminFilter *mi, NVNmagFilter *ma) {
    const samp_par *o = &((const samp_builder_t *)b)->p; *mi = (NVNminFilter)o->minf; *ma = (NVNmagFilter)o->magf;
}
NVN(nvnSamplerBuilderGetWrapMode, void, const NVNsamplerBuilder *b, NVNwrapMode *s, NVNwrapMode *t, NVNwrapMode *r) {
    const samp_par *o = &((const samp_builder_t *)b)->p; *s = (NVNwrapMode)o->ws; *t = (NVNwrapMode)o->wt; *r = (NVNwrapMode)o->wr;
}
NVN(nvnSamplerInitialize, NVNboolean, NVNsampler *s, const NVNsamplerBuilder *b) {
    samp_t *o = (samp_t *)s; o->magic = MAGIC('S', 'M', 'P', 0); o->p = ((const samp_builder_t *)b)->p; return 1;
}
NVN(nvnSamplerFinalize, void, NVNsampler *s) { ((samp_t *)s)->magic = 0; }



NVN(nvnProgramInitialize, NVNboolean, NVNprogram *p, NVNdevice *d) {
    (void)d; memset(p, 0, sizeof(prog_t)); ((prog_t *)p)->magic = MAGIC('P', 'R', 'O', 'G'); return 1;
}
NVN(nvnProgramFinalize, void, NVNprogram *p) { prog_t *o = (prog_t *)p; snap_drop(&o->snap, o->serial, offsetof(prog_t, serial)); mtl_prog_release(o); o->magic = 0; }
NVN(nvnProgramSetShaders, NVNboolean, NVNprogram *p, int n, const NVNshaderData *sd) {
    prog_t *o = (prog_t *)p;
    if (n < 0 || n > 6) { LOG("【红】ProgramSetShaders 阶段数 %d", n); return 0; }
    snap_drop(&o->snap, o->serial, offsetof(prog_t, serial)); mtl_prog_release(o); o->nstage = n; memcpy(o->sd, sd, n * sizeof *sd); o->serial = nvn_new_serial();
    OLOG("ProgramSetShaders %p n=%d data0=%#llx", (void *)p, n, n ? (unsigned long long)sd[0].data : 0ull);
    return 1;
}



NVN(nvnSyncInitialize, NVNboolean, NVNsync *s, NVNdevice *d) { (void)d; *(sync_t *)s = (sync_t){MAGIC('S', 'Y', 'N', 'C'), 0, 0}; return 1; }
NVN(nvnSyncFinalize, void, NVNsync *s) { ((sync_t *)s)->magic = 0; }


NVN(nvnQueueFenceSync, void, NVNqueue *q, NVNsync *s, NVNsyncCondition c, int f) { (void)q; (void)c; (void)f; ((sync_t *)s)->fenced = 1; ((sync_t *)s)->fence = x_fence(); }




uint64_t dm_nvn_frame_count(void);
static _Atomic uint64_t g_gw_sync_n, g_gw_sync_ns, g_gw_sync_lag, g_gw_ev_n, g_gw_ev_ns, g_gw_sync_hit, g_gw_acq_n;
static const void *volatile g_acq_sync;
static void gw_report(void) {
    static uint64_t last; uint64_t f = dm_nvn_frame_count(); if (f < last + 300) return; last = f;
    uint64_t sn = atomic_exchange(&g_gw_sync_n, 0), sns = atomic_exchange(&g_gw_sync_ns, 0), lag = atomic_exchange(&g_gw_sync_lag, 0), hit = atomic_exchange(&g_gw_sync_hit, 0);
    uint64_t en = atomic_exchange(&g_gw_ev_n, 0), ens = atomic_exchange(&g_gw_ev_ns, 0), an = atomic_exchange(&g_gw_acq_n, 0);
    LOG("[nvn] 游戏等 GPU（300 帧）：SyncWait %.2f 次/帧（已完成 %.2f，其中取图同步 %.2f），共 %.2f ms/帧，平均落后最新提交 %.2f 次；读事件 %.2f 次/帧，共 %.2f ms/帧",
        sn / 300.0, hit / 300.0, an / 300.0, sns / 300e6, sn ? (double)lag / sn : 0, en / 300.0, ens / 300e6);
}
static int x_wait_sub(uint64_t sub, uint64_t timeout);
NVN(nvnSyncWait, NVNsyncWaitResult, const NVNsync *s, uint64_t timeout) {
    const sync_t *o = (const sync_t *)s;
    if (!o->fenced) return NVN_SYNC_WAIT_RESULT_TIMEOUT_EXPIRED;
#ifdef __OHOS__
    atomic_fetch_add(&g_gw_sync_n, 1); atomic_fetch_add(&g_gw_sync_lag, x_fence() - o->fence); if ((const void *)s == g_acq_sync) atomic_fetch_add(&g_gw_acq_n, 1); gw_report();
    if (mtl_wait(o->fence, 0)) { atomic_fetch_add(&g_gw_sync_hit, 1); return NVN_SYNC_WAIT_RESULT_ALREADY_SIGNALED; }


    XTRACE_BEGIN("dm-game-syncwait"); XTRACE_COUNT("dm-game-wait-target", o->fence); uint64_t t0 = now_ns(); int ok = !timeout || x_wait_sub(o->fence, timeout);
    if (ok) { uint64_t used = now_ns() - t0; ok = mtl_wait(o->fence, timeout >= 3600000000000ull ? timeout : used >= timeout ? 0 : timeout - used); }
    atomic_fetch_add(&g_gw_sync_ns, now_ns() - t0); XTRACE_END();
    return ok ? NVN_SYNC_WAIT_RESULT_CONDITION_SATISFIED : NVN_SYNC_WAIT_RESULT_TIMEOUT_EXPIRED;
#else
    if (mtl_wait(o->fence, 0)) return NVN_SYNC_WAIT_RESULT_ALREADY_SIGNALED;
    return mtl_wait(o->fence, timeout) ? NVN_SYNC_WAIT_RESULT_CONDITION_SATISFIED : NVN_SYNC_WAIT_RESULT_TIMEOUT_EXPIRED;
#endif
}





__attribute__((weak)) void mtl_event_read(const volatile uint32_t *val) { (void)val; }
NVN(nvnEventBuilderSetDefaults, void, NVNeventBuilder *b) { memset(b, 0, sizeof(ev_builder_t)); }
NVN(nvnEventBuilderSetStorage, void, NVNeventBuilder *b, const NVNmemoryPool *p, int64_t off) { *(ev_builder_t *)b = (ev_builder_t){(const pool_t *)p, off}; }
NVN(nvnEventInitialize, NVNboolean, NVNevent *e, const NVNeventBuilder *b) {
    const ev_builder_t *s = (const ev_builder_t *)b;
    if (!s->pool || !s->pool->mem) { LOG("【红】EventInitialize 没有存储"); return 0; }
    if (mtl_event_read) mtl_event_read((volatile uint32_t *)(s->pool->mem + s->off));
    *(ev_t *)e = (ev_t){MAGIC('E', 'V', 'N', 'T'), (volatile uint32_t *)(s->pool->mem + s->off)}; return 1;
}
NVN(nvnEventFinalize, void, NVNevent *e) { if (mtl_event_read) mtl_event_read(((ev_t *)e)->val); ((ev_t *)e)->magic = 0; }




#ifdef __OHOS__
NVN(nvnEventGetValue, uint32_t, const NVNevent *e) { if (mtl_event_read) { uint64_t t0 = now_ns(); mtl_event_read(((const ev_t *)e)->val); atomic_fetch_add(&g_gw_ev_ns, now_ns() - t0); atomic_fetch_add(&g_gw_ev_n, 1); } return *((const ev_t *)e)->val; }
#else
NVN(nvnEventGetValue, uint32_t, const NVNevent *e) { if (mtl_event_read) mtl_event_read(((const ev_t *)e)->val); return *((const ev_t *)e)->val; }
#endif
static void ev_signal(const ev_t *e, int mode, uint32_t v) { if (mtl_event_read) mtl_event_read(e->val); if (mode == NVN_EVENT_SIGNAL_MODE_ADD) *e->val += v; else *e->val = v; }
NVN(nvnEventSignal, void, NVNevent *e, NVNeventSignalMode m, uint32_t v) { ev_signal((const ev_t *)e, m, v); }



#define QB(b) ((queue_builder_t *)(b))
NVN(nvnQueueBuilderSetDefaults, void, NVNqueueBuilder *b) {
    NVNdevice *dev = QB(b)->dev; memset(b, 0, sizeof(queue_builder_t)); QB(b)->dev = dev;
    QB(b)->cmd_sz = 65536; QB(b)->comp_sz = 262144; QB(b)->ctl_sz = 16384; QB(b)->flush = 4096;
}
NVN(nvnQueueBuilderSetDevice, void, NVNqueueBuilder *b, NVNdevice *d) { QB(b)->dev = d; }
NVN(nvnQueueBuilderSetFlags, void, NVNqueueBuilder *b, int f) { QB(b)->flags = f; }
NVN(nvnQueueBuilderSetCommandMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->cmd_sz = n; }
NVN(nvnQueueBuilderSetComputeMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->comp_sz = n; }
NVN(nvnQueueBuilderSetControlMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->ctl_sz = n; }
NVN(nvnQueueBuilderSetCommandFlushThreshold, void, NVNqueueBuilder *b, size_t n) { QB(b)->flush = n; }


NVN(nvnQueueBuilderGetQueueMemorySize, size_t, const NVNqueueBuilder *b) {
    const queue_builder_t *o = (const queue_builder_t *)b;
    return (o->cmd_sz + o->comp_sz + o->ctl_sz + 4095) & ~(size_t)4095;
}
NVN(nvnQueueBuilderSetQueueMemory, void, NVNqueueBuilder *b, void *m, size_t n) { QB(b)->mem = m; QB(b)->mem_sz = n; }
NVN(nvnQueueInitialize, NVNboolean, NVNqueue *q, const NVNqueueBuilder *b) {
    *(queue_t *)q = (queue_t){MAGIC('Q', 'U', 'E', 'U'), ((const queue_builder_t *)b)->dev, 0};
    OLOG("QueueInitialize %p", (void *)q); return 1;
}
NVN(nvnQueueFinalize, void, NVNqueue *q) { ((queue_t *)q)->magic = 0; }
NVN(nvnQueueGetError, NVNqueueGetErrorResult, NVNqueue *q, NVNqueueErrorInfo *info) {
    (void)q; if (info) memset(info, 0, sizeof *info); return NVN_QUEUE_GET_ERROR_RESULT_GPU_NO_ERROR;
}
NVN(nvnQueueFlush, void, NVNqueue *q) { (void)q; }
NVN(nvnQueueFinish, void, NVNqueue *q) { (void)q; x_drain(); mtl_finish(); }



#define CB(c) ((cmd_t *)(c))
#define OP_RESERVE 24

NVN(nvnCommandBufferInitialize, NVNboolean, NVNcommandBuffer *c, NVNdevice *d) {
    memset(c, 0, sizeof(cmd_t)); CB(c)->magic = MAGIC('C', 'M', 'D', 'B'); CB(c)->dev = d; return 1;
}
NVN(nvnCommandBufferFinalize, void, NVNcommandBuffer *c) { CB(c)->magic = 0; }
NVN(nvnCommandBufferSetMemoryCallback, void, NVNcommandBuffer *c, PFNNVNCOMMANDBUFFERMEMORYCALLBACKPROC cb) { CB(c)->cb = cb; }
NVN(nvnCommandBufferSetMemoryCallbackData, void, NVNcommandBuffer *c, void *d) { CB(c)->cb_data = d; }
static uint8_t *align8(uint8_t *p) { return (uint8_t *)(((uintptr_t)p + 7) & ~(uintptr_t)7); }




static struct { int on; uint64_t bytes, recs, grants, grant_bytes, grant_min, grant_max, cbs, hostseg, op_n[64], op_b[64], frame0, waste; } g_cs = { .on = -1 };
static pthread_mutex_t g_cs_mu = PTHREAD_MUTEX_INITIALIZER;
uint64_t dm_nvn_frame_count(void);
static int cs_on(void) { if (g_cs.on < 0) g_cs.on = getenv("DM_NVN_CMDSTAT") != NULL; return g_cs.on; }
static void cs_op(uint32_t op, uint32_t size) { if (!cs_on() || op >= 64) return; pthread_mutex_lock(&g_cs_mu); g_cs.op_n[op]++; g_cs.op_b[op] += size; pthread_mutex_unlock(&g_cs_mu); }
static void cs_dump(void) {
    if (!cs_on()) return; uint64_t f = dm_nvn_frame_count(); if (f - g_cs.frame0 < 300) return;
    pthread_mutex_lock(&g_cs_mu); double n = (double)(f - g_cs.frame0); char top[700]; int k = 0; uint8_t used[64] = {0};
    for (int r = 0; r < 8; r++) { int bi = -1; for (int i = 0; i < 64; i++) if (!used[i] && g_cs.op_b[i] && (bi < 0 || g_cs.op_b[i] > g_cs.op_b[bi])) bi = i; if (bi < 0) break; used[bi] = 1;
        k += snprintf(top + k, sizeof top - k, " 命令%d：%.0f 条×%.0f 字节=%.1f KB；", bi, g_cs.op_n[bi] / n, (double)g_cs.op_b[bi] / (double)g_cs.op_n[bi], g_cs.op_b[bi] / n / 1024.0); }
    LOG("[nvn] 命令内存 至帧 %llu：每帧录 %.1f KB / %.0f 条；游戏给段 每帧 %.2f 次，每段 均 %.1f KB（最小 %.1f / 最大 %.1f）；补内存回调 每帧 %.2f 次；宿主补段 每帧 %.3f 次；换段丢弃的段尾 每帧 %.1f KB｜每帧%s",
        (unsigned long long)f, g_cs.bytes / n / 1024.0, g_cs.recs / n, g_cs.grants / n, g_cs.grants ? g_cs.grant_bytes / (double)g_cs.grants / 1024.0 : 0, g_cs.grant_min / 1024.0, g_cs.grant_max / 1024.0, g_cs.cbs / n, g_cs.hostseg / n, g_cs.waste / n / 1024.0, top);
    int on = g_cs.on; memset(&g_cs, 0, sizeof g_cs); g_cs.on = on; g_cs.frame0 = f; pthread_mutex_unlock(&g_cs_mu);
}
static void put_hdr(uint8_t *p, uint32_t op, uint32_t size) { op_hdr h = {op, size}; memcpy(p, &h, sizeof h); if (op) cs_op(op, size); }


static void cmd_new_segment(cmd_t *o, uint8_t *mem, size_t n) {
    uint8_t *start = align8(mem), *end = mem + n;
    if (o->recording && o->cur) { if (cs_on() && o->end > o->cur) { pthread_mutex_lock(&g_cs_mu); g_cs.waste += (uint64_t)(o->end - o->cur); pthread_mutex_unlock(&g_cs_mu); } put_hdr(o->cur, OP_JUMP, 16); memcpy(o->cur + 8, &start, 8); }
    o->cur = start; o->end = end;
}






#define HOSTSEG_SZ (1u << 20)
#define HOSTSEG_MAX 64
uint64_t dm_nvn_frame_count(void);
static struct { uint8_t *mem; uint64_t frame; int busy; } g_hostseg[HOSTSEG_MAX];
static pthread_mutex_t g_hostseg_mu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *hostseg_take(void) {
    static unsigned total; uint64_t f = dm_nvn_frame_count(); uint8_t *m = NULL;
    pthread_mutex_lock(&g_hostseg_mu);
    for (int i = 0; i < HOSTSEG_MAX; i++) if (g_hostseg[i].busy && g_hostseg[i].frame + 8 < f) g_hostseg[i].busy = 0;
    for (int i = 0; i < HOSTSEG_MAX && !m; i++) if (!g_hostseg[i].busy) {
        if (!g_hostseg[i].mem) g_hostseg[i].mem = malloc(HOSTSEG_SZ);
        g_hostseg[i].busy = 1; g_hostseg[i].frame = f; m = g_hostseg[i].mem; }
    unsigned t = ++total;
    pthread_mutex_unlock(&g_hostseg_mu);
    if (!m) die("NVN 命令内存：游戏池已空，宿主补段 %d 段也用完（8 帧内）", HOSTSEG_MAX);
    if (t == 1 || t % 256 == 0) LOG("[nvn] 游戏的命令内存池已空，宿主补段（累计 %u 次，帧 %llu）", t, (unsigned long long)f);
    return m;
}
NVN(nvnCommandBufferAddCommandMemory, void, NVNcommandBuffer *c, const NVNmemoryPool *p, ptrdiff_t off, size_t n) {
    const pool_t *pl = (const pool_t *)p;
    if (!pl || n < 64) { if (cs_on()) { pthread_mutex_lock(&g_cs_mu); g_cs.hostseg++; pthread_mutex_unlock(&g_cs_mu); } cmd_new_segment(CB(c), hostseg_take(), HOSTSEG_SZ); return; }
    if (!pl->mem) { LOG("【红】AddCommandMemory 池没有 CPU 存储"); return; }
    if (cs_on()) { pthread_mutex_lock(&g_cs_mu); g_cs.grants++; g_cs.grant_bytes += n; if (!g_cs.grant_min || n < g_cs.grant_min) g_cs.grant_min = n; if (n > g_cs.grant_max) g_cs.grant_max = n; pthread_mutex_unlock(&g_cs_mu); }
    cmd_new_segment(CB(c), pl->mem + off, n);
}
NVN(nvnCommandBufferAddControlMemory, void, NVNcommandBuffer *c, void *m, size_t n) { (void)c; (void)m; (void)n; }


static uint8_t *cmd_alloc(cmd_t *o, uint32_t size) {
    size = (size + 7) & ~7u;
    for (int tries = 0; !o->cur || o->cur + size + OP_RESERVE > o->end; tries++) {
        if (!o->cb || tries == 2) die("NVN 命令内存耗尽且无法补充（cb=%p）", (void *)o->cb);
        if (cs_on()) { pthread_mutex_lock(&g_cs_mu); g_cs.cbs++; pthread_mutex_unlock(&g_cs_mu); }
        o->cb((NVNcommandBuffer *)o, NVN_COMMAND_BUFFER_MEMORY_EVENT_OUT_OF_COMMAND_MEMORY, size + OP_RESERVE, o->cb_data);
    }
    if (cs_on()) { pthread_mutex_lock(&g_cs_mu); g_cs.bytes += size; g_cs.recs++; pthread_mutex_unlock(&g_cs_mu); }
    uint8_t *p = o->cur; o->cur += size; put_hdr(p, 0, size); return p;
}
NVN(nvnCommandBufferBeginRecording, void, NVNcommandBuffer *c) {
    cmd_t *o = CB(c);
    if (o->recording) LOG("【红】BeginRecording 重入");
    o->recording = 1;
    uint8_t *p = cmd_alloc(o, 8); put_hdr(p, OP_BEGIN, 8); o->begin = p;
}
NVN(nvnCommandBufferEndRecording, NVNcommandHandle, NVNcommandBuffer *c) {
    cmd_t *o = CB(c);
    uint8_t *p = cmd_alloc(o, 8); put_hdr(p, OP_END, 8);
    o->recording = 0;
    return (NVNcommandHandle)o->begin;
}
NVN(nvnCommandBufferIsRecording, NVNboolean, const NVNcommandBuffer *c) { return ((const cmd_t *)c)->recording != 0; }
NVN(nvnCommandBufferCopyCommands, void, NVNcommandBuffer *c, int n, const NVNcommandHandle *h) {
    for (int i = 0; i < n; i++) { uint8_t *p = cmd_alloc(CB(c), 16); put_hdr(p, OP_CALL, 16); memcpy(p + 8, &h[i], 8); }
}
NVN(nvnCommandBufferSignalEvent, void, NVNcommandBuffer *c, const NVNevent *e, NVNeventSignalMode m, NVNeventSignalLocation loc, int flags, uint32_t v) {
    (void)loc; (void)flags;
    uint8_t *p = cmd_alloc(CB(c), 24); put_hdr(p, OP_SIGNAL_EVENT, 24);
    const ev_t *ev = (const ev_t *)e; volatile uint32_t *val = ev->val; uint32_t mm = m;
    memcpy(p + 8, &val, 8); memcpy(p + 16, &mm, 4); memcpy(p + 20, &v, 4);
}
NVN(nvnCommandBufferResetCounter, void, NVNcommandBuffer *c, NVNcounterType t) {
    uint8_t *p = cmd_alloc(CB(c), 16); put_hdr(p, OP_RESET_COUNTER, 16); uint64_t tt = t; memcpy(p + 8, &tt, 8);
}
NVN(nvnCommandBufferReportCounter, void, NVNcommandBuffer *c, NVNcounterType t, NVNbufferAddress a) {
    uint8_t *p = cmd_alloc(CB(c), 24); put_hdr(p, OP_REPORT_COUNTER, 24); uint64_t tt = t; memcpy(p + 8, &tt, 8); memcpy(p + 16, &a, 8);
}
NVN(nvnCommandBufferCopyBufferToBuffer, void, NVNcommandBuffer *c, NVNbufferAddress src, NVNbufferAddress dst, size_t n, int flags) {
    (void)flags;
    uint8_t *p = cmd_alloc(CB(c), 32); put_hdr(p, OP_COPY_B2B, 32); uint64_t nn = n;
    memcpy(p + 8, &src, 8); memcpy(p + 16, &dst, 8); memcpy(p + 24, &nn, 8);
}


NVN(nvnCommandBufferUpdateUniformBuffer, void, NVNcommandBuffer *c, NVNbufferAddress buf, size_t bufsz, ptrdiff_t off, size_t n, const void *data) {
    (void)bufsz;
    uint32_t sz = (uint32_t)(24 + n);
    uint8_t *p = cmd_alloc(CB(c), sz); put_hdr(p, OP_UPDATE_UB, (sz + 7) & ~7u);
    uint64_t dst = buf + off, nn = n; memcpy(p + 8, &dst, 8); memcpy(p + 16, &nn, 8); memcpy(p + 24, data, n);
}



static void *rec(NVNcommandBuffer *c, uint32_t op, size_t payload) {
    uint32_t sz = (uint32_t)((8 + payload + 7) & ~(size_t)7);
    uint8_t *p = cmd_alloc(CB(c), sz); put_hdr(p, op, sz);
    memset(p + 8, 0, sz - 8);
    return p + 8;
}
#define REC(c, OP, T) ((T *)rec((c), (OP), sizeof(T)))




#define RECN(c, OP, T, ARR, N) ((T *)rec((c), (OP), offsetof(T, ARR) + sizeof(((T *)0)->ARR[0]) * (size_t)(N)))
static void copy_view(view_t *dst, uint8_t *has, const NVNtextureView *v) { *has = v != NULL; if (v) *dst = *(const view_t *)v; }
NVN(nvnCommandBufferSetRenderTargets, void, NVNcommandBuffer *c, int n, const NVNtexture *const *ct, const NVNtextureView *const *cv,
    const NVNtexture *dt, const NVNtextureView *dv) {
    if (n < 0 || n > 8) { LOG("【红】SetRenderTargets 数量 %d", n); n = n < 0 ? 0 : 8; }
    p_set_rt *r = REC(c, OP_SET_RT, p_set_rt); r->n = n;
    for (int i = 0; i < n; i++) { r->color[i] = ct ? snap_tex(ct[i]) : NULL; copy_view(&r->cview[i], &r->has_cview[i], cv ? cv[i] : NULL); }
    r->depth = snap_tex(dt); copy_view(&r->dview, &r->has_dview, dv);
}
NVN(nvnCommandBufferClearColor, void, NVNcommandBuffer *c, int index, const float *rgba, int mask) {
    p_clear_color *r = REC(c, OP_CLEAR_COLOR, p_clear_color); r->index = index; r->mask = mask; memcpy(r->rgba, rgba, 16);
}
NVN(nvnCommandBufferClearDepthStencil, void, NVNcommandBuffer *c, float depth, uint32_t dmask, int stencil, int smask) {
    p_clear_ds *r = REC(c, OP_CLEAR_DS, p_clear_ds); *r = (p_clear_ds){depth, (int)B(dmask), stencil, smask};
}
NVN(nvnCommandBufferSetViewports, void, NVNcommandBuffer *c, int first, int n, const float *v) {
    if (n < 0 || first + n > 16) { LOG("【红】SetViewports first=%d n=%d", first, n); return; }
    p_viewports *r = REC(c, OP_VIEWPORTS, p_viewports); r->first = first; r->n = n;
    for (int i = 0; i < n; i++) memcpy(r->v[i], v + 4 * i, 16);
}
NVN(nvnCommandBufferSetScissor, void, NVNcommandBuffer *c, int x, int y, int w, int h) { *REC(c, OP_SCISSOR, p_scissor) = (p_scissor){x, y, w, h}; }
NVN(nvnCommandBufferSetDepthRange, void, NVNcommandBuffer *c, float n, float f) { *REC(c, OP_DEPTH_RANGE, p_depth_range) = (p_depth_range){n, f}; }
NVN(nvnCommandBufferBindProgram, void, NVNcommandBuffer *c, const NVNprogram *p, int stages) {
    *REC(c, OP_BIND_PROGRAM, p_bind_program) = (p_bind_program){snap_prog(p), stages};
}
static void bind_state(NVNcommandBuffer *c, int kind, const void *st, size_t n) { p_bind_state *r = REC(c, OP_BIND_STATE, p_bind_state); r->kind = kind; memcpy(r->bytes, st, n); }
NVN(nvnCommandBufferBindBlendState, void, NVNcommandBuffer *c, const NVNblendState *st) { bind_state(c, 0, st, sizeof(blend_st)); }
NVN(nvnCommandBufferBindColorState, void, NVNcommandBuffer *c, const NVNcolorState *st) { bind_state(c, 1, st, sizeof(color_st)); }
NVN(nvnCommandBufferBindChannelMaskState, void, NVNcommandBuffer *c, const NVNchannelMaskState *st) { bind_state(c, 2, st, sizeof(chmask_st)); }
NVN(nvnCommandBufferBindMultisampleState, void, NVNcommandBuffer *c, const NVNmultisampleState *st) { bind_state(c, 3, st, sizeof(ms_st)); }
NVN(nvnCommandBufferBindPolygonState, void, NVNcommandBuffer *c, const NVNpolygonState *st) { bind_state(c, 4, st, sizeof(poly_st)); }
NVN(nvnCommandBufferBindDepthStencilState, void, NVNcommandBuffer *c, const NVNdepthStencilState *st) { bind_state(c, 5, st, sizeof(ds_st)); }
NVN(nvnCommandBufferBindVertexAttribState, void, NVNcommandBuffer *c, int n, const NVNvertexAttribState *st) {
    if (n < 0 || n > 16) { LOG("【红】BindVertexAttribState n=%d", n); return; }
    p_bind_vattr *r = RECN(c, OP_BIND_VATTR, p_bind_vattr, st, n); r->first = 0; r->n = n; memcpy(r->st, st, 4 * n);
}
NVN(nvnCommandBufferBindVertexStreamState, void, NVNcommandBuffer *c, int n, const NVNvertexStreamState *st) {
    if (n < 0 || n > 16) { LOG("【红】BindVertexStreamState n=%d", n); return; }
    p_bind_vstream *r = RECN(c, OP_BIND_VSTREAM, p_bind_vstream, st, n); r->first = 0; r->n = n; memcpy(r->st, st, 8 * n);
}
NVN(nvnCommandBufferBindVertexBuffers, void, NVNcommandBuffer *c, int first, int n, const NVNbufferRange *rg) {
    if (n < 0 || first + n > 16) { LOG("【红】BindVertexBuffers first=%d n=%d", first, n); return; }
    p_bind_vbuf *r = RECN(c, OP_BIND_VBUF, p_bind_vbuf, r, n); r->first = first; r->n = n; memcpy(r->r, rg, sizeof *rg * n);
}
NVN(nvnCommandBufferBindUniformBuffer, void, NVNcommandBuffer *c, NVNshaderStage st, int i, NVNbufferAddress a, size_t n) {
    *REC(c, OP_BIND_UBO, p_bind_buf) = (p_bind_buf){st, i, a, n};
}
NVN(nvnCommandBufferBindUniformBuffers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n, const NVNbufferRange *rg) {
    for (int i = 0; i < n; i++) *REC(c, OP_BIND_UBO, p_bind_buf) = (p_bind_buf){st, first + i, rg[i].address, rg[i].size};
}
NVN(nvnCommandBufferBindStorageBuffer, void, NVNcommandBuffer *c, NVNshaderStage st, int i, NVNbufferAddress a, size_t n) {
    *REC(c, OP_BIND_SSBO, p_bind_buf) = (p_bind_buf){st, i, a, n};
}
NVN(nvnCommandBufferBindStorageBuffers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n, const NVNbufferRange *rg) {
    for (int i = 0; i < n; i++) *REC(c, OP_BIND_SSBO, p_bind_buf) = (p_bind_buf){st, first + i, rg[i].address, rg[i].size};
}
static void bind_handles(NVNcommandBuffer *c, int op, int st, int first, int n, const uint64_t *h) {
    if (n < 0 || n > 32) { LOG("【红】绑定句柄数 %d", n); return; }
    p_bind_handles *r = RECN(c, op, p_bind_handles, h, n); r->stage = st; r->first = first; r->n = n; memcpy(r->h, h, 8 * n);
}


NVN(nvnCommandBufferBindImages, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n, const NVNimageHandle *h) {
    bind_handles(c, OP_BIND_IMG, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateTextures, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n, const NVNseparateTextureHandle *h) {
    bind_handles(c, OP_BIND_TEX, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateSamplers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n, const NVNseparateSamplerHandle *h) {
    bind_handles(c, OP_BIND_SAMP, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateSampler, void, NVNcommandBuffer *c, NVNshaderStage st, int i, NVNseparateSamplerHandle h) {
    bind_handles(c, OP_BIND_SAMP, st, i, 1, &h.value);
}
NVN(nvnCommandBufferSetStencilValueMask, void, NVNcommandBuffer *c, NVNface f, int m) { *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, m, 0, 0, 0}; }
NVN(nvnCommandBufferSetStencilMask, void, NVNcommandBuffer *c, NVNface f, int m) { *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, 0, m, 0, 1}; }
NVN(nvnCommandBufferSetStencilRef, void, NVNcommandBuffer *c, NVNface f, int v) { *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, 0, 0, v, 2}; }
NVN(nvnCommandBufferSetPolygonOffsetClamp, void, NVNcommandBuffer *c, float fa, float un, float cl) {
    *REC(c, OP_POLY_OFFSET, p_poly_offset) = (p_poly_offset){fa, un, cl};
}
NVN(nvnCommandBufferDrawArrays, void, NVNcommandBuffer *c, NVNdrawPrimitive pr, int first, int n) { *REC(c, OP_DRAW, p_draw) = (p_draw){pr, first, n}; }
NVN(nvnCommandBufferDrawElementsInstanced, void, NVNcommandBuffer *c, NVNdrawPrimitive pr, NVNindexType it, int n, NVNbufferAddress ib,
    int bv, int bi, int inst) {
    *REC(c, OP_DRAW_IDX, p_draw_idx) = (p_draw_idx){pr, it, n, bv, bi, inst, ib};
}
NVN(nvnCommandBufferDispatchCompute, void, NVNcommandBuffer *c, int x, int y, int z) {


    *REC(c, OP_DISPATCH, p_dispatch) = (p_dispatch){x, y, z, (uint64_t)__builtin_return_address(0)};
}
NVN(nvnCommandBufferBarrier, void, NVNcommandBuffer *c, int bits) { *(int32_t *)rec(c, OP_BARRIER, 8) = bits; }
NVN(nvnCommandBufferSetTexturePool, void, NVNcommandBuffer *c, const NVNtexturePool *p) { REC(c, OP_SET_TEX_POOL, p_set_pool)->pool = (const dpool_t *)p; }
NVN(nvnCommandBufferSetSamplerPool, void, NVNcommandBuffer *c, const NVNsamplerPool *p) { REC(c, OP_SET_SAMP_POOL, p_set_pool)->pool = (const dpool_t *)p; }

NVN(nvnCommandBufferCopyBufferToTexture, void, NVNcommandBuffer *c, NVNbufferAddress src, const NVNtexture *t, const NVNtextureView *v,
    const NVNcopyRegion *r, int flags) {
    p_copy_b2t *o = REC(c, OP_COPY_B2T, p_copy_b2t); o->src = src; o->tex = snap_tex(t); copy_view(&o->view, &o->has_view, v); o->r = *r; o->flags = flags;
}
NVN(nvnCommandBufferCopyTextureToBuffer, void, NVNcommandBuffer *c, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r,
    NVNbufferAddress dst, int flags) {
    p_copy_b2t *o = REC(c, OP_COPY_T2B, p_copy_b2t); o->src = dst; o->tex = snap_tex(t); copy_view(&o->view, &o->has_view, v); o->r = *r; o->flags = flags;
}


NVN(nvnCommandBufferClearTexture, void, NVNcommandBuffer *c, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r, const float *rgba, int mask) {
    p_clear_tex *o = REC(c, OP_CLEAR_TEX, p_clear_tex); o->tex = snap_tex(t); copy_view(&o->view, &o->has_view, v); o->r = *r; o->mask = mask; o->is_uint = 0; memcpy(o->c.f, rgba, 16);
}
NVN(nvnCommandBufferClearTextureui, void, NVNcommandBuffer *c, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r, const uint32_t *rgba, int mask) {
    p_clear_tex *o = REC(c, OP_CLEAR_TEX, p_clear_tex); o->tex = snap_tex(t); copy_view(&o->view, &o->has_view, v); o->r = *r; o->mask = mask; o->is_uint = 1; memcpy(o->c.u, rgba, 16);
}
NVN(nvnCommandBufferCopyTextureToTexture, void, NVNcommandBuffer *c, const NVNtexture *s, const NVNtextureView *sv, const NVNcopyRegion *sr,
    const NVNtexture *d, const NVNtextureView *dv, const NVNcopyRegion *dr, int flags) {
    p_copy_t2t *o = REC(c, OP_COPY_T2T, p_copy_t2t); o->src = snap_tex(s); o->dst = snap_tex(d);
    copy_view(&o->sv, &o->has_sv, sv); copy_view(&o->dv, &o->has_dv, dv); o->sr = *sr; o->dr = *dr; o->flags = flags;
}
NVN(nvnCommandBufferSetCopyRowStride, void, NVNcommandBuffer *c, ptrdiff_t st) { REC(c, OP_COPY_STRIDE, p_copy_stride)->stride = st; }
NVN(nvnCommandBufferSetCopyImageStride, void, NVNcommandBuffer *c, ptrdiff_t st) { REC(c, OP_COPY_IMAGE_STRIDE, p_copy_stride)->stride = st; }
NVN(nvnCommandBufferSetBlendColor, void, NVNcommandBuffer *c, const float *v) { memcpy(REC(c, OP_BLEND_COLOR, p_blend_color)->rgba, v, 16); }



#define ST(T, x) ((T *)(x))
NVN(nvnBlendStateSetDefaults, void, NVNblendState *s) { *ST(blend_st, s) = (blend_st){0, NVN_BLEND_FUNC_ONE, NVN_BLEND_FUNC_ZERO, NVN_BLEND_FUNC_ONE, NVN_BLEND_FUNC_ZERO, NVN_BLEND_EQUATION_ADD, NVN_BLEND_EQUATION_ADD}; }
NVN(nvnBlendStateSetBlendTarget, void, NVNblendState *s, int t) { ST(blend_st, s)->target = t; }
NVN(nvnBlendStateSetBlendFunc, void, NVNblendState *s, NVNblendFunc sr, NVNblendFunc dr, NVNblendFunc sa, NVNblendFunc da) {
    blend_st *o = ST(blend_st, s); o->src_rgb = sr; o->dst_rgb = dr; o->src_a = sa; o->dst_a = da;
}
NVN(nvnBlendStateSetBlendEquation, void, NVNblendState *s, NVNblendEquation er, NVNblendEquation ea) { ST(blend_st, s)->eq_rgb = er; ST(blend_st, s)->eq_a = ea; }
NVN(nvnColorStateSetDefaults, void, NVNcolorState *s) { *ST(color_st, s) = (color_st){0, NVN_LOGIC_OP_COPY, NVN_ALPHA_FUNC_ALWAYS}; }
NVN(nvnColorStateSetBlendEnable, void, NVNcolorState *s, int i, uint32_t en) {
    color_st *o = ST(color_st, s); if (i < 0 || i > 7) return;
    o->blend_en = B(en) ? (o->blend_en | 1u << i) : (o->blend_en & ~(1u << i));
}
NVN(nvnChannelMaskStateSetDefaults, void, NVNchannelMaskState *s) { ST(chmask_st, s)->mask = 0xffffffffu; }
NVN(nvnChannelMaskStateSetChannelMask, void, NVNchannelMaskState *s, int i, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    if (i < 0 || i > 7) return;
    uint32_t m = (B(r) ? 1 : 0) | (B(g) ? 2 : 0) | (B(b) ? 4 : 0) | (B(a) ? 8 : 0);
    chmask_st *o = ST(chmask_st, s); o->mask = (o->mask & ~(0xfu << (4 * i))) | m << (4 * i);
}
NVN(nvnMultisampleStateSetDefaults, void, NVNmultisampleState *s) { memset(s, 0, sizeof(ms_st)); ST(ms_st, s)->enable = 1; }
NVN(nvnMultisampleStateSetMultisampleEnable, void, NVNmultisampleState *s, uint32_t e) { ST(ms_st, s)->enable = B(e) != 0; }
NVN(nvnMultisampleStateSetSamples, void, NVNmultisampleState *s, int n) { ST(ms_st, s)->samples = (uint8_t)n; }
NVN(nvnMultisampleStateSetAlphaToCoverageEnable, void, NVNmultisampleState *s, uint32_t e) { ST(ms_st, s)->a2c = B(e) != 0; }
NVN(nvnMultisampleStateSetSampleLocationsEnable, void, NVNmultisampleState *s, uint32_t e) { ST(ms_st, s)->sl_en = B(e) != 0; }
NVN(nvnMultisampleStateSetSampleLocations, void, NVNmultisampleState *s, int start, int n, const float *v) { (void)s; (void)start; (void)n; (void)v; }
NVN(nvnPolygonStateSetDefaults, void, NVNpolygonState *s) { *ST(poly_st, s) = (poly_st){NVN_FACE_NONE, NVN_FRONT_FACE_CCW, NVN_POLYGON_MODE_FILL, 0}; }
NVN(nvnPolygonStateSetCullFace, void, NVNpolygonState *s, NVNface f) { ST(poly_st, s)->cull = f; }
NVN(nvnPolygonStateSetFrontFace, void, NVNpolygonState *s, NVNfrontFace f) { ST(poly_st, s)->front = f; }
NVN(nvnPolygonStateSetPolygonMode, void, NVNpolygonState *s, NVNpolygonMode m) { ST(poly_st, s)->mode = m; }
NVN(nvnPolygonStateSetPolygonOffsetEnables, void, NVNpolygonState *s, int e) { ST(poly_st, s)->offset_en = e; }
NVN(nvnDepthStencilStateSetDefaults, void, NVNdepthStencilState *s) {
    *ST(ds_st, s) = (ds_st){0, 1, NVN_DEPTH_FUNC_LESS, 0, NVN_STENCIL_FUNC_ALWAYS, NVN_STENCIL_FUNC_ALWAYS,
                            NVN_STENCIL_OP_KEEP, NVN_STENCIL_OP_KEEP, NVN_STENCIL_OP_KEEP, NVN_STENCIL_OP_KEEP, NVN_STENCIL_OP_KEEP, NVN_STENCIL_OP_KEEP};
}
NVN(nvnDepthStencilStateSetDepthTestEnable, void, NVNdepthStencilState *s, uint32_t e) { ST(ds_st, s)->dt = B(e) != 0; }
NVN(nvnDepthStencilStateSetDepthWriteEnable, void, NVNdepthStencilState *s, uint32_t e) { ST(ds_st, s)->dw = B(e) != 0; }
NVN(nvnDepthStencilStateSetDepthFunc, void, NVNdepthStencilState *s, NVNdepthFunc f) { ST(ds_st, s)->df = f; }
NVN(nvnDepthStencilStateSetStencilTestEnable, void, NVNdepthStencilState *s, uint32_t e) { ST(ds_st, s)->st = B(e) != 0; }
NVN(nvnDepthStencilStateSetStencilFunc, void, NVNdepthStencilState *s, NVNface f, NVNstencilFunc fn) {
    ds_st *o = ST(ds_st, s); if (f & NVN_FACE_FRONT) o->sf_f = fn; if (f & NVN_FACE_BACK) o->sf_b = fn;
}
NVN(nvnDepthStencilStateSetStencilOp, void, NVNdepthStencilState *s, NVNface f, NVNstencilOp fail, NVNstencilOp zfail, NVNstencilOp zpass) {
    ds_st *o = ST(ds_st, s);
    if (f & NVN_FACE_FRONT) { o->fail_f = fail; o->zfail_f = zfail; o->zpass_f = zpass; }
    if (f & NVN_FACE_BACK) { o->fail_b = fail; o->zfail_b = zfail; o->zpass_b = zpass; }
}
NVN(nvnVertexAttribStateSetDefaults, void, NVNvertexAttribState *s) { *ST(va_st, s) = (va_st){0, 0, 0}; }
NVN(nvnVertexAttribStateSetFormat, void, NVNvertexAttribState *s, NVNformat f, ptrdiff_t off) {
    if (off < 0 || off >= (1 << 19)) LOG("【红】顶点属性偏移 %td 超出打包范围", off);
    ST(va_st, s)->fmt = f; ST(va_st, s)->offset = (uint32_t)off;
}
NVN(nvnVertexAttribStateSetStreamIndex, void, NVNvertexAttribState *s, int i) { ST(va_st, s)->stream = i; }
NVN(nvnVertexStreamStateSetDefaults, void, NVNvertexStreamState *s) { *ST(vs_st, s) = (vs_st){0, 0}; }
NVN(nvnVertexStreamStateSetStride, void, NVNvertexStreamState *s, ptrdiff_t st) { ST(vs_st, s)->stride = (uint32_t)st; }
NVN(nvnVertexStreamStateSetDivisor, void, NVNvertexStreamState *s, int d) { ST(vs_st, s)->divisor = d; }

static _Atomic uint64_t g_sub_wall_ns, g_sub_cpu_ns, g_sub_instr;
static _Atomic uint64_t g_cum_sub_instr, g_cum_sub_cpu_ns;
extern int thread_selfcounts(int type, void *buf, size_t nbytes);
static uint64_t thr_instr(void) { uint64_t c[2] = {0, 0}; thread_selfcounts(1, c, sizeof c); return c[0]; }




static void thr_counts(uint64_t *instr, uint64_t *cyc) { uint64_t c[2] = {0, 0}; thread_selfcounts(1, c, sizeof c); *instr = c[0]; *cyc = c[1]; }
static uint64_t thr_sys_ns(void) {
    thread_basic_info_data_t bi; mach_msg_type_number_t n = THREAD_BASIC_INFO_COUNT; mach_port_t me = mach_thread_self(); uint64_t r = 0;
    if (thread_info(me, THREAD_BASIC_INFO, (thread_info_t)&bi, &n) == KERN_SUCCESS) r = (uint64_t)bi.system_time.seconds * 1000000000ull + (uint64_t)bi.system_time.microseconds * 1000ull;
    mach_port_deallocate(mach_task_self(), me); return r;
}
static _Atomic uint64_t g_cum_sub_cyc, g_cum_sub_sys_ns, g_cum_sub_wall_ns;
static int frame_rec(void);
static _Atomic uint64_t g_nvn_ns;





static pthread_mutex_t g_susp_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_susp_cv = PTHREAD_COND_INITIALIZER;
static int g_susp, g_inflight_api;
static void gfx_enter(void) { pthread_mutex_lock(&g_susp_mu); while (g_susp) pthread_cond_wait(&g_susp_cv, &g_susp_mu); g_inflight_api++; pthread_mutex_unlock(&g_susp_mu); }
static void gfx_leave(void) { pthread_mutex_lock(&g_susp_mu); g_inflight_api--; pthread_cond_broadcast(&g_susp_cv); pthread_mutex_unlock(&g_susp_mu); }
void dm_gfx_suspend(int on) {
    pthread_mutex_lock(&g_susp_mu);
    g_susp = on != 0;
    if (on) while (g_inflight_api) pthread_cond_wait(&g_susp_cv, &g_susp_mu);
    else pthread_cond_broadcast(&g_susp_cv);
    pthread_mutex_unlock(&g_susp_mu);
    if (on) { x_drain(); mtl_drain(); LOG("[nvn] 图形挂起：在途 GPU 工作已清空"); } else LOG("[nvn] 图形恢复");
}


typedef struct { const char *who; double comp_ms, period_ms; uint64_t last; double win[120]; int n; } rt_decl;
static rt_decl g_rt_main = {"出图线程"}, g_rt_xlat = {"翻译线程"};
static void set_time_constraint(rt_decl *d, uint64_t period_ns);
static void rt_comp_tick(rt_decl *d, uint64_t period_ns);










enum { XJ_SUBMIT, XJ_PRESENT, XJ_FRAME };
typedef struct { int kind, n; uint64_t *h; const tex_t *tex; double period; int crop[4]; uint64_t frame, sub, host; } xjob;
#define XQ 32
#define XTK 1024
static xjob g_xq[XQ]; static uint64_t g_xq_in, g_xq_done, g_xsub_done;
static struct { uint64_t job, tk; } g_xtk[XTK];
static pthread_mutex_t g_xmu = PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t g_xcv = PTHREAD_COND_INITIALIZER;
#ifdef __OHOS__


static uint64_t g_x_trace_frame;
#endif
static int pace_adapt_enabled(void);
static _Atomic double g_pa_xcpu_ms = -1;
static int g_x_sync; static pthread_t g_x_thr; static mach_port_t g_x_port;
static void x_submit(const uint64_t *h, int n, uint64_t sub, uint64_t host) {
    mtl_host_apply(host);
#ifdef __OHOS__
    { void front_pin_check(void); front_pin_check(); }
#endif
    uint64_t t0 = now_ns(), c0 = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID), i0, y0, s0 = thr_sys_ns(); thr_counts(&i0, &y0);
    mtl_sub_begin();
    mtl_sub_index(h, n);
    for (int i = 0; i < n; i++) { mtl_exec_rest(h + i + 1, n - i - 1); mtl_exec((const uint8_t *)h[i]); }
    mtl_exec_rest(NULL, 0);
    uint64_t f = mtl_sub_commit();
    if (f != sub) { static int red; if (!red++) LOG("【红】翻译队列：第 %llu 次提交的 Metal 栅栏是 %llu，栅栏与提交数对不上，游戏等同步会提前 / 推迟放行", (unsigned long long)sub, (unsigned long long)f); }
    snap_tick(sub);
    uint64_t dw = now_ns() - t0;
    atomic_fetch_add(&g_nvn_ns, dw); atomic_fetch_add(&g_sub_wall_ns, dw);
    uint64_t i1, y1; thr_counts(&i1, &y1); uint64_t c1 = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID), dc = c1 - c0, di = i1 - i0;
    XTRACE_COUNT("dm-x-submit-cpu-us", c0 && c1 >= c0 ? (int64_t)(dc / 1000) : -1);
    atomic_fetch_add(&g_cum_sub_cyc, y1 - y0); atomic_fetch_add(&g_cum_sub_sys_ns, thr_sys_ns() - s0); atomic_fetch_add(&g_cum_sub_wall_ns, dw);
    atomic_fetch_add(&g_sub_cpu_ns, dc); atomic_fetch_add(&g_cum_sub_cpu_ns, dc);
    atomic_fetch_add(&g_sub_instr, di); atomic_fetch_add(&g_cum_sub_instr, di);
}
static void x_run(const xjob *j, uint64_t job) {
    XTRACE_COUNT("dm-x-run-job", job); XTRACE_COUNT("dm-x-run-sub", j->sub); XTRACE_COUNT("dm-x-run-frame", j->frame);
    if (j->kind == XJ_SUBMIT) { XTRACE_BEGIN("dm-x-submit"); x_submit(j->h, j->n, j->sub, j->host); free(j->h); XTRACE_END(); }
    else if (j->kind == XJ_PRESENT) {
#ifdef __OHOS__
        { void front_gtx_frame(void); front_gtx_frame(); }
#endif
        XTRACE_BEGIN("dm-x-present"); uint64_t tk = mtl_present(j->tex, j->period, j->crop); XTRACE_END(); pthread_mutex_lock(&g_xmu); g_xtk[job % XTK].job = job; g_xtk[job % XTK].tk = tk; pthread_mutex_unlock(&g_xmu); }
    else {
        XTRACE_BEGIN("dm-x-frame");


        if (!g_x_sync && pace_adapt_enabled()) {
            static nvn_frame_cpu_window sample; double ms;
            uint64_t cpu = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID);
            if (!cpu || cpu < sample.previous) atomic_store(&g_pa_xcpu_ms, -1);
            if (nvn_frame_cpu_sample(&sample, cpu, &ms)) atomic_store(&g_pa_xcpu_ms, ms);
        }
        mtl_set_frame(j->frame);


        uint64_t pn = (uint64_t)(j->period * 1e9); static uint64_t rt_p;
        if (!g_x_sync && !dm_detclock_enabled()) { if (pn && rt_p != pn) { rt_p = pn; set_time_constraint(&g_rt_xlat, pn); } rt_comp_tick(&g_rt_xlat, pn); }
        XTRACE_END();
    }
}
static void *x_thread(void *a) {
    (void)a;
#ifdef __APPLE__
    pthread_setname_np("nvn-translate");
#else


    pthread_setname_np(pthread_self(), "nvn-translate");
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#ifdef __OHOS__
    { void front_pin_fast(const char *who); void front_gtx_register(int); front_pin_fast("nvn-translate"); front_gtx_register(1); }
#endif
#endif
    g_x_port = pthread_mach_thread_np(pthread_self());
    pthread_mutex_lock(&g_xmu);
    for (;;) {
        while (g_xq_done == g_xq_in) pthread_cond_wait(&g_xcv, &g_xmu);
        uint64_t job = g_xq_done + 1; xjob j = g_xq[job % XQ];
        pthread_mutex_unlock(&g_xmu);
        x_run(&j, job);
        pthread_mutex_lock(&g_xmu);
        g_xq_done = job; XTRACE_COUNT("dm-x-done-job", job);
        if (j.kind == XJ_SUBMIT) { g_xsub_done = j.sub; XTRACE_COUNT("dm-x-sub-done", g_xsub_done); } pthread_cond_broadcast(&g_xcv);
    }
    return NULL;
}
static void x_start(void) {
    const char *e = getenv("DM_NVN_SYNC"); g_x_sync = e && atoi(e);
    if (g_x_sync) { LOG("[nvn] DM_NVN_SYNC=1：翻译在提交线程上同步做（对照档）"); return; }
    pthread_attr_t at; pthread_attr_init(&at);
#ifdef __APPLE__
    pthread_attr_set_qos_class_np(&at, QOS_CLASS_USER_INTERACTIVE, 0);
#endif
    pthread_attr_setstacksize(&at, 8 << 20);
    pthread_t t; if (pthread_create(&g_x_thr, &at, x_thread, NULL)) { LOG("【红】翻译线程起不来，改为同步翻译"); g_x_sync = 1; } else LOG("[nvn] 翻译线程已起：提交 / 出图在翻译线程上按序执行（队列 %d 项）", XQ);
    pthread_attr_destroy(&at);
}
static uint64_t x_push(xjob *j) {
    static pthread_once_t once = PTHREAD_ONCE_INIT; pthread_once(&once, x_start);
    pthread_mutex_lock(&g_xmu);
    while (g_xq_in - g_xq_done >= XQ) pthread_cond_wait(&g_xcv, &g_xmu);
    if (j->kind == XJ_SUBMIT) j->sub = atomic_fetch_add(&g_x_subs, 1) + 1;
    uint64_t job = ++g_xq_in;
#ifdef __OHOS__
    if (j->kind == XJ_FRAME) g_x_trace_frame = j->frame; else j->frame = g_x_trace_frame;
#endif
    XTRACE_COUNT("dm-x-enqueue-job", job); XTRACE_COUNT("dm-x-enqueue-kind", j->kind);
    XTRACE_COUNT("dm-x-enqueue-sub", j->sub); XTRACE_COUNT("dm-x-enqueue-frame", j->frame);
    if (g_x_sync) {
        pthread_mutex_unlock(&g_xmu); x_run(j, job); pthread_mutex_lock(&g_xmu);
        g_xq_done = job; XTRACE_COUNT("dm-x-done-job", job); pthread_cond_broadcast(&g_xcv);
    } else { g_xq[job % XQ] = *j; pthread_cond_broadcast(&g_xcv); }
    pthread_mutex_unlock(&g_xmu);
    return job;
}
static void x_wait(uint64_t job) { pthread_mutex_lock(&g_xmu); while (g_xq_done < job) pthread_cond_wait(&g_xcv, &g_xmu); pthread_mutex_unlock(&g_xmu); }
static void x_drain(void) { pthread_mutex_lock(&g_xmu); uint64_t job = g_xq_in; pthread_mutex_unlock(&g_xmu); x_wait(job); }
static uint64_t x_fence(void) { return atomic_load(&g_x_subs); }



static int x_wait_sub(uint64_t sub, uint64_t timeout) {
    if (g_x_sync) return 1;
    XTRACE_BEGIN("dm-x-wait-sub"); XTRACE_COUNT("dm-x-wait-target", sub);
    pthread_mutex_lock(&g_xmu); int ok = 1;
    XTRACE_COUNT("dm-x-wait-done-in", g_xsub_done);
    if (g_xsub_done < sub) {
        if (timeout >= 3600000000000ull) while (g_xsub_done < sub) pthread_cond_wait(&g_xcv, &g_xmu);
        else { struct timespec d; clock_gettime(CLOCK_REALTIME, &d); uint64_t ns = (uint64_t)d.tv_nsec + timeout % 1000000000ull; d.tv_sec += (time_t)(timeout / 1000000000ull + ns / 1000000000ull); d.tv_nsec = (long)(ns % 1000000000ull);
            while (g_xsub_done < sub) if (pthread_cond_timedwait(&g_xcv, &g_xmu, &d)) { ok = g_xsub_done >= sub; break; } }
    }
    XTRACE_COUNT("dm-x-wait-done-out", g_xsub_done);
    pthread_mutex_unlock(&g_xmu); XTRACE_END(); return ok;
}
static uint64_t x_present_ticket(uint64_t job) {
    x_wait(job);
    pthread_mutex_lock(&g_xmu); uint64_t tk = g_xtk[job % XTK].job == job ? g_xtk[job % XTK].tk : 0; int lost = g_xtk[job % XTK].job != job; pthread_mutex_unlock(&g_xmu);
    if (lost) { static int red; if (!red++) LOG("【红】翻译队列：出图任务 %llu 的票号已被覆盖（票号表 %d 项不够），这次取图不等出图线程", (unsigned long long)job, XTK); }
    return tk;
}
NVN(nvnQueueSubmitCommands, void, NVNqueue *q, int n, const NVNcommandHandle *h) {
    gfx_enter();
    ((queue_t *)q)->submits++;
    xjob j = {XJ_SUBMIT, n, n > 0 ? malloc((size_t)n * sizeof *h) : NULL};
    if (n > 0) memcpy(j.h, h, (size_t)n * sizeof *h);
    j.host = mtl_host_latch();
    x_push(&j);
    gfx_leave();
}







static void nvn_nop(void) {}
static const char *const g_nops[] = {
    "nvnCommandBufferDispatchComputeIndirect",
    "nvnCommandBufferSetDepthBounds",
    "nvnCommandBufferWaitSync",








    "nvnCommandBufferSetShaderScratchMemory",
    "nvnCommandBufferPushDebugGroupDynamic",
    "nvnCommandBufferPopDebugGroup",
    "nvnCommandBufferInsertDebugMarkerDynamic",
    "nvnCommandBufferWaitEvent",


    "nvnWindowFinalize",
};
__attribute__((constructor)) static void nvn_nops_register(void) {


    nvn_init_slots();
    for (unsigned k = 0; k < sizeof g_nops / sizeof *g_nops; k++) {
        unsigned i = 0;
        while (i < NVN_NFUNC && strcmp(nvn_names[i], g_nops[k])) i++;
        if (i == NVN_NFUNC) die("NVN 空实现名 %s 不在 main 名字表里", g_nops[k]);
        if (nvn_slot[i].fn != (void *)nvn_unimpl_entry) die("NVN %s 既有实现又列为空实现", g_nops[k]);
        nvn_slot[i].fn = (void *)nvn_nop;
    }
    unsigned nnop = sizeof g_nops / sizeof *g_nops;


    if (getenv("DM_NVN_PROF"))
        for (unsigned i = 0; i < NVN_NFUNC; i++)
            if (nvn_slot[i].fn != (void *)nvn_unimpl_entry && strcmp(nvn_names[i], "nvnQueuePresentTexture")) { nvn_real[i] = nvn_slot[i].fn; nvn_slot[i].fn = (void *)nvn_prof_entry; }
    LOG("[nvn] 空设备：名字 %u，已实现 %u（语义 %u + 空实现 %u），其余 %u 个接报红桩",
        NVN_NFUNC, g_nsem + nnop, g_nsem, nnop, NVN_NFUNC - g_nsem - nnop);
}



#define WB(b) ((win_builder_t *)(b))
NVN(nvnWindowBuilderSetDefaults, void, NVNwindowBuilder *b) { NVNdevice *dev = WB(b)->dev; memset(b, 0, sizeof(win_builder_t)); WB(b)->dev = dev; WB(b)->interval = 1; }
NVN(nvnWindowBuilderSetDevice, void, NVNwindowBuilder *b, NVNdevice *d) { WB(b)->dev = d; }
NVN(nvnWindowBuilderSetNativeWindow, void, NVNwindowBuilder *b, NVNnativeWindow w) { WB(b)->native = w; }
NVN(nvnWindowBuilderSetTextures, void, NVNwindowBuilder *b, int n, NVNtexture *const *t) {
    if (n < 0 || n > 4) { LOG("【红】WindowBuilderSetTextures 数量 %d", n); n = n < 0 ? 0 : 4; }
    WB(b)->ntex = n; for (int i = 0; i < n; i++) WB(b)->tex[i] = t[i];
}
NVN(nvnWindowBuilderSetPresentInterval, void, NVNwindowBuilder *b, int v) { WB(b)->interval = v; }
NVN(nvnWindowInitialize, NVNboolean, NVNwindow *w, const NVNwindowBuilder *b) {
    const win_builder_t *s = (const win_builder_t *)b; win_t *o = (win_t *)w;
    memset(o, 0, sizeof *o); o->magic = MAGIC('W', 'I', 'N', 'D'); o->native = s->native; o->ntex = s->ntex; o->interval = s->interval;
    memcpy(o->tex, s->tex, sizeof o->tex);
    for (int i = 0; i < o->ntex; i++) if (o->tex[i]) mtl_xv_window_tex(CTX(o->tex[i])->serial);
    LOG("[nvn] WindowInitialize 交换链 %d 张 间隔 %d 首张 %dx%d 格式 %#x", o->ntex, o->interval,
        o->ntex ? (int)CTX(o->tex[0])->d.width : 0, o->ntex ? (int)CTX(o->tex[0])->d.height : 0, o->ntex ? CTX(o->tex[0])->d.format : 0);
    return 1;
}
NVN(nvnWindowSetPresentInterval, void, NVNwindow *w, int v) { if (((win_t *)w)->interval != v) LOG("[nvn] 游戏设出图间隔 %d → %d", ((win_t *)w)->interval, v); ((win_t *)w)->interval = v; }
NVN(nvnWindowSetCrop, void, NVNwindow *w, int x, int y, int ww, int hh) {
    win_t *o = (win_t *)w; int v[4] = {x, y, ww, hh};
    if (memcmp(o->crop, v, sizeof v)) LOG("[nvn] WindowCrop %d,%d %dx%d", x, y, ww, hh);
    memcpy(o->crop, v, sizeof v);
}
static void trow(uint64_t f, const char *name, double v);
static void sleep_until_ns(uint64_t dl);
static uint64_t present_period(const win_t *o);
__attribute__((weak)) void dm_main_yield_stats(uint64_t *n, uint64_t *ns) { *n = 0; *ns = 0; }
__attribute__((weak)) uint32_t dm_guest_work_qos(unsigned *count) { if (count) *count = 0; return QOS_CLASS_UNSPECIFIED; }
__attribute__((weak)) void dm_hle_wake_counts(uint64_t *wakes, uint64_t *skips) { *wakes = 0; *skips = 0; }
extern uint32_t mtl_present_qos(void);
static uint64_t g_frame;
static uint64_t g_pa_wait;
static uint64_t g_acq_wait_ns, g_pq_wait_ns;
uint64_t dm_nvn_frame_count(void) { return g_frame; }
extern uint64_t g_t0_ns;
NVN(nvnWindowAcquireTexture, NVNwindowAcquireTextureResult, NVNwindow *w, NVNsync *s, int *idx) {
    win_t *o = (win_t *)w;
    *idx = o->ntex ? o->next++ % o->ntex : 0;


    uint64_t at = *idx < 4 ? o->avail[*idx] : 0, now = dm_detclock_enabled() ? guest_ns() : now_ns(), wait = at > now ? at - now : 0;
    if (!dm_detclock_enabled()) {
        static uint64_t rt_period; uint64_t period = present_period(o);
        if (period && rt_period != period) { rt_period = period; set_time_constraint(&g_rt_main, period); }
        rt_comp_tick(&g_rt_main, period);
        if (mtl_link_active()) { if (!mtl_link_wait_tick(period ? 2 * period : 33333333ull) && wait) sleep_until_ns(at); }
        else if (wait) sleep_until_ns(at);
    } else {


        static int pace = -1; static uint64_t step, last;
        if (pace < 0) { const char *e = getenv("DM_DETCLOCK_PACE"), *d = getenv("DM_DETCLOCK"); step = d ? strtoull(d, 0, 0) : 0; pace = e && atoi(e) && step; }
        if (pace) { if (last && now_ns() < last + step) sleep_until_ns(last + step); last = now_ns(); }
    }
    g_acq_wait_ns += wait; g_pa_wait += wait; trow(g_frame, "@late_ms", wait ? (now_ns() - at) / 1e6 : 0);


    uint64_t tk = *idx < 4 ? o->ticket[*idx] : 0;
    if (tk) { uint64_t w0 = now_ns(), rt = x_present_ticket(tk); if (rt) mtl_present_wait(rt); g_pq_wait_ns += now_ns() - w0; }
    if (s) { ((sync_t *)s)->fenced = 1; ((sync_t *)s)->fence = x_fence(); g_acq_sync = s; }
    return NVN_WINDOW_ACQUIRE_TEXTURE_RESULT_SUCCESS;
}



static uint64_t g_prev[NVN_NFUNC];
static FILE *g_tsv;
static void dump_frame(void) {
    static int inited, nframes = 3, calls = 1;
    if (!inited) {
        inited = 1;
        const char *e = getenv("DM_NVN_FRAMES"); if (e) nframes = atoi(e);
        const char *c = getenv("DM_NVN_TSV_CALLS"); if (c) calls = atoi(c);
        const char *t = getenv("DM_NVN_TSV"); if (t && !(g_tsv = fopen(t, "w"))) LOG("【红】打不开 %s", t);
        if (g_tsv) fprintf(g_tsv, "# kind: nvn-per-frame  name 为 NVN 函数名时值 = 调用次数；@ 开头为计时埋点（ms / 个）\nframe\tname\tvalue\n");
    }
    uint64_t total = 0; unsigned nfn = 0;
    char line[8192]; size_t o = 0; line[0] = 0;
    for (unsigned i = 0; i < NVN_NFUNC; i++) {
        uint64_t c = atomic_load(&nvn_slot[i].cnt), d = c - g_prev[i];
        g_prev[i] = c;
        if (!d) continue;
        total += d; nfn++;
        if (g_tsv && calls) fprintf(g_tsv, "%llu\t%s\t%llu\n", g_frame, nvn_names[i], d);
        if (g_frame <= (uint64_t)nframes && o < sizeof line - 96) o += snprintf(line + o, sizeof line - o, " %s=%llu", nvn_names[i] + 3, d);
    }
    if (g_tsv) fflush(g_tsv);
    if (g_frame <= (uint64_t)nframes) LOG("[nvn] %s %llu：%u 个函数 %llu 次调用 |%s", g_frame ? "帧" : "初始化", g_frame, nfn, total, line);
    else if (g_frame % 300 == 0) LOG("[nvn] 帧 %llu：%u 个函数 %llu 次调用", g_frame, nfn, total);
    g_frame++; cs_dump();




    if (g_frame % 300 == 0) { task_vm_info_data_t vi; mach_msg_type_number_t cnt = TASK_VM_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &cnt) == KERN_SUCCESS) { unsigned long long mb = (unsigned long long)(vi.phys_footprint >> 20); unsigned long long ht = 0, hf = 0, hm = 0; int dm_guest_heap_stat(unsigned long long *, unsigned long long *, unsigned long long *); int hok = dm_guest_heap_stat(&ht, &hf, &hm);
            void mtl_mem_stat(unsigned long long, unsigned long long, int, unsigned long long, unsigned long long, unsigned long long); mtl_mem_stat(g_frame, mb, hok, ht, hf, hm);
            static long cap = -1; if (cap < 0) { const char *e = getenv("DM_MEM_CAP_GB"); cap = e ? atol(e) : DM_MEM_CAP_DEFAULT_GB; }
            if (cap > 0 && mb > (unsigned long long)cap * 1024) { LOG("【红】内存看门狗：常驻 %llu MB 超过上限 %ld GB（帧 %llu），自行退出", mb, cap, (unsigned long long)g_frame); _exit(86); } } }
    dm_guest_frame_tick();
}


#define TWIN 300
static double g_tw[6][TWIN]; static unsigned g_tn[6];
static double g_pres_iv[TWIN]; static unsigned g_presn;
__attribute__((weak)) double dm_brightness_now(void) { return -1; }
__attribute__((weak)) void dm_perf_present(double iv_ms) { (void)iv_ms; }
uint64_t dm_nvn_sub_cpu_ns(void) { return atomic_load(&g_cum_sub_cpu_ns); }
static uint64_t sub_cyc(void) { return atomic_load(&g_cum_sub_cyc); }
extern uint64_t (*g_ub_sub_cpu_ns)(void), (*g_ub_sub_cyc)(void);
__attribute__((constructor)) static void ub_sub_register(void) { g_ub_sub_cpu_ns = dm_nvn_sub_cpu_ns; g_ub_sub_cyc = sub_cyc; }
__attribute__((weak)) int dm_thermal_now(void) { return -1; }


#define LFN 16
typedef struct { uint64_t f; double fr, nv, t_s, make_ms, up_mb, up_ms; unsigned pso, lib, up, rup; int ls; } lf_rec;
static lf_rec g_lf[LFN];
__attribute__((weak)) void dm_longframe_note(const char *what) { LOG("[长帧] %s", what); }
extern void mtl_create_ns(uint64_t *lib_ns, uint64_t *pso_ns); extern uint64_t mtl_upload_bytes(void); extern void mtl_pkg_stats(uint64_t *uploads, uint64_t *reuploads);
static const char *const g_tname[6] = {"帧时间", "出帧工作", "NVN 提交+出图", "GPU", "翻译层录制期", "翻译层提交期"};
static void tpush(int k, double v) { g_tw[k][g_tn[k]++ % TWIN] = v; }


#define HRN 1024
static double g_hr[HRN]; static uint64_t g_hn, g_hn_read;
static int dcmp(const void *a, const void *b);
void nvn_gpu_recent(double *avg, double *p95, unsigned *n) {
    uint64_t hi = g_hn, lo = g_hn_read > hi ? hi : g_hn_read; if (hi - lo > HRN) lo = hi - HRN; g_hn_read = hi;
    unsigned c = (unsigned)(hi - lo); static double v[HRN]; double s = 0;
    for (unsigned i = 0; i < c; i++) { v[i] = g_hr[(lo + i) % HRN]; s += v[i]; }
    qsort(v, c, sizeof *v, dcmp); *n = c; *avg = c ? s / c : -1; *p95 = c ? v[(c * 95) / 100 < c ? (c * 95) / 100 : c - 1] : -1;
}
static int dcmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static void tsummary(uint64_t frame) {
    char line[1024]; size_t o = 0;
    for (int k = 0; k < 6; k++) {
        unsigned n = g_tn[k] < TWIN ? g_tn[k] : TWIN; if (!n) continue;
        double v[TWIN]; memcpy(v, g_tw[k], n * sizeof *v); qsort(v, n, sizeof *v, dcmp);
        o += snprintf(line + o, sizeof line - o, " %s p50 %.2f / p95 %.2f / 最大 %.2f ms（%u 帧）；", g_tname[k], v[n / 2], v[(n * 95) / 100 < n ? (n * 95) / 100 : n - 1], v[n - 1], n);
    }
    LOG("[nvn] 计时 至帧 %llu：%s", frame, line);
    { unsigned n = g_tn[0] < TWIN ? g_tn[0] : TWIN, over = 0; double v[TWIN];
      if (n) { memcpy(v, g_tw[0], n * sizeof *v); qsort(v, n, sizeof *v, dcmp); for (unsigned i = 0; i < n; i++) over += v[i] > 16.7;
        LOG("[nvn] 帧间隔 至帧 %llu：>16.7ms %u / %u 帧，p99 %.2f ms", frame, over, n, v[(n * 99) / 100 < n ? (n * 99) / 100 : n - 1]); } }
    { unsigned n = g_presn < TWIN ? g_presn : TWIN, k = 0, drop = 0, discard = 0; double v[TWIN];
      for (unsigned i = 0; i < n; i++) { double x = g_pres_iv[i]; if (x < 0) { drop++; discard++; } else if (x > 0) { v[k++] = x; drop += x > 25.0; } }
      task_vm_info_data_t vi; mach_msg_type_number_t vc = TASK_VM_INFO_COUNT; double fp = task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &vc) == KERN_SUCCESS ? vi.phys_footprint / 1048576.0 : -1;
      double br = dm_brightness_now();


      if (k) { qsort(v, k, sizeof *v, dcmp); LOG("[nvn] 上屏 至帧 %llu：%u 帧，掉帧 %u（丢弃 %u），间隔 p5 %.2f / p50 %.2f / p95 %.2f，p99 %.2f ms；footprint %.0f MiB；亮度 %.2f；温控 %d", frame, k + discard, drop, discard,
          v[(k * 5) / 100], v[k / 2], v[(k * 95) / 100 < k ? (k * 95) / 100 : k - 1], v[(k * 99) / 100 < k ? (k * 99) / 100 : k - 1], fp, br, dm_thermal_now()); }
      g_presn = 0; }
    { static uint64_t pi, pc, pd, pf; uint64_t ci = atomic_load(&g_cum_sub_instr), cc = atomic_load(&g_cum_sub_cpu_ns), ps_, dr_; mtl_stats(&ps_, &dr_);
      if (pf && frame > pf && dr_ > pd) LOG("[nvn] 翻译 至帧 %llu：每次绘制 %.0f 条指令，每帧提交 %.2f M 条 / CPU %.3f ms，每帧绘制 %.0f", frame,
          (double)(ci - pi) / (double)(dr_ - pd), (ci - pi) / 1e6 / (double)(frame - pf), (cc - pc) / 1e6 / (double)(frame - pf), (double)(dr_ - pd) / (double)(frame - pf));
      { static uint64_t py, pk, pw; uint64_t cy = atomic_load(&g_cum_sub_cyc), ck = atomic_load(&g_cum_sub_sys_ns), cw = atomic_load(&g_cum_sub_wall_ns);
        if (pf && frame > pf && ci > pi && cy > py) { double nf = (double)(frame - pf), cyc = (double)(cy - py), cpu = (double)(cc - pc);
          qos_class_t qc = QOS_CLASS_UNSPECIFIED; int rp = 0; pthread_get_qos_class_np(pthread_self(), &qc, &rp);
          uint64_t aw = 0, ag = 0; int an = 0; void mtl_alloc_stats(uint64_t *, uint64_t *, int *); mtl_alloc_stats(&aw, &ag, &an);
          static uint64_t pyn, pyns; uint64_t yn = 0, yns = 0; dm_main_yield_stats(&yn, &yns);
          LOG("[nvn] 翻译细项 至帧 %llu：每帧提交 周期 %.2f M（每条指令 %.2f 周期）、CPU %.3f ms 其中内核态 %.3f ms、墙钟 %.3f ms；平均主频 %.2f GHz（周期 / CPU 时间）；本线程 QoS %#x；分配器池 %d 个、累计等 GPU %llu 次；实时声明计算 %.2f ms；主线程让出 每帧 %.0f 次 / %.3f ms", frame,
              cyc / 1e6 / nf, cyc / (double)(ci - pi), cpu / 1e6 / nf, (double)(ck - pk) / 1e6 / nf, (double)(cw - pw) / 1e6 / nf, cpu > 0 ? cyc / cpu : 0, (unsigned)qc, an, (unsigned long long)aw, g_rt_main.comp_ms, (double)(yn - pyn) / nf, (double)(yns - pyns) / 1e6 / nf); pyn = yn; pyns = yns; }
#ifdef __APPLE__
        if (frame % 300 == 0) { unsigned wn = 0; uint32_t wq = dm_guest_work_qos(&wn); thread_extended_info_data_t xi; mach_msg_type_number_t xc = THREAD_EXTENDED_INFO_COUNT; int okx = thread_info(pthread_mach_thread_np(pthread_self()), THREAD_EXTENDED_INFO, (thread_info_t)&xi, &xc) == KERN_SUCCESS;
            thread_time_constraint_policy_data_t tp; mach_msg_type_number_t tc = THREAD_TIME_CONSTRAINT_POLICY_COUNT; boolean_t def = 0; int rt_live = thread_policy_get(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&tp, &tc, &def) == KERN_SUCCESS && !def;
            LOG("[nvn] 线程策略：主线程 实时 计算 %.2f ms / 周期 %.2f ms，实时策略%s，当前优先级 %d（基础 %d，调度类 %d）；工作线程 %u 条 QoS %#x；出图 QoS %#x", g_rt_main.comp_ms, g_rt_main.period_ms, rt_live ? "在生效" : "未生效", okx ? xi.pth_curpri : -1, okx ? xi.pth_priority : -1, okx ? xi.pth_policy : -1, wn, wq, mtl_present_qos());
            if (g_x_port) {
                thread_extended_info_data_t yi; mach_msg_type_number_t yc = THREAD_EXTENDED_INFO_COUNT; int oky = thread_info(g_x_port, THREAD_EXTENDED_INFO, (thread_info_t)&yi, &yc) == KERN_SUCCESS;
                thread_time_constraint_policy_data_t yp; mach_msg_type_number_t ytc = THREAD_TIME_CONSTRAINT_POLICY_COUNT; boolean_t ydef = 0; int y_live = thread_policy_get(g_x_port, THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&yp, &ytc, &ydef) == KERN_SUCCESS && !ydef;
                qos_class_t yq = QOS_CLASS_UNSPECIFIED; int yr = 0; pthread_get_qos_class_np(g_x_thr, &yq, &yr);
                LOG("[nvn] 线程策略：翻译线程 实时 计算 %.2f ms / 周期 %.2f ms，实时策略%s，当前优先级 %d（基础 %d，调度类 %d）；QoS %#x", g_rt_xlat.comp_ms, g_rt_xlat.period_ms, y_live ? "在生效" : "未生效",
                    oky ? yi.pth_curpri : -1, oky ? yi.pth_priority : -1, oky ? yi.pth_policy : -1, (unsigned)yq); } }
#endif
        py = cy; pk = ck; pw = cw; }
      pi = ci; pc = cc; pd = dr_; pf = frame; }
    uint64_t up, re, fa, pr; mtl_pkg_stats(&up, &re); nvn_dirty_stats(&fa, &pr);
    LOG("[nvn] 预排布贴图 首传 %llu 张、池内容变化重传 %llu 次；写保护缺页 %llu 次、宿主写前解保护 %llu 页；漏检 %llu（累计）", up, re, fa, pr, mtl_pkg_miss());
    memset(g_tn, 0, sizeof g_tn);
}




#define SRN 16
static struct { uint64_t f; int n; const char *k[64]; double v[64]; } g_sr[SRN];
static int g_slow = -1;
static int slow_on(void) { if (g_slow < 0) g_slow = getenv("DM_SLOWFRAME") != NULL; return g_slow; }
static int frame_rec(void) { return g_tsv || slow_on(); }
static void trow(uint64_t f, const char *name, double v) {
    if (g_tsv) fprintf(g_tsv, "%llu\t%s\t%.3f\n", f, name, v);
    if (slow_on()) { __typeof__(g_sr[0]) *r = &g_sr[f % SRN]; if (r->f != f) { r->f = f; r->n = 0; } if (r->n < 64) { r->k[r->n] = name; r->v[r->n++] = v; } }
}
static double srow(uint64_t f, const char *name, double dflt) {
    __typeof__(g_sr[0]) *r = &g_sr[f % SRN];
    if (r->f == f) for (int i = 0; i < r->n; i++) if (!strcmp(r->k[i], name)) return r->v[i];
    return dflt;
}


static void slow_emit(uint64_t f) {
    static uint64_t judged, slow;
    double pm = srow(f, "@presented_ms", 0), fr = srow(f, "@frame_ms", 0), lim = 1000.0 / 60 * 1.5;
    int has = mtl_has_window(), bad = has ? (pm > lim || pm < 0) : fr > lim;
    if (has ? pm != 0 : fr != 0) judged++;
    if (bad) {
        slow++;
        char b[4096]; size_t o = snprintf(b, sizeof b, "[slowframe]\t%llu", f);
        __typeof__(g_sr[0]) *r = &g_sr[f % SRN];
        for (int i = 0; i < r->n && o < sizeof b - 64; i++) o += snprintf(b + o, sizeof b - o, "\t%s=%.3f", r->k[i], r->v[i]);
        double m = 0; for (uint64_t p = f >= 3 ? f - 3 : 0; p < f; p++) { double x = srow(p, "@frame_ms", 0); if (x > m) m = x; }
        snprintf(b + o, sizeof b - o, "\t@prev3_max_frame_ms=%.3f", m);
        fprintf(stderr, "%s\n", b);
    }
    if (f % 300 == 0) fprintf(stderr, "[slowframe]\t#\t至帧 %llu\tjudged=%llu\tslow=%llu\t判据=%s\n", f, judged, slow, has ? "上屏间隔" : "出图调用间隔（无窗口）");
}








static void set_time_constraint_comp(rt_decl *d, uint64_t period_ns, uint64_t comp_ns) {
    static mach_timebase_info_data_t tb; if (!tb.denom) mach_timebase_info(&tb);
    if (comp_ns < 1000000ull) comp_ns = 1000000ull; if (comp_ns > period_ns * 9 / 10) comp_ns = period_ns * 9 / 10;
#ifdef __APPLE__
    thread_time_constraint_policy_data_t p = {
        .period = (uint32_t)(period_ns * tb.denom / tb.numer), .computation = (uint32_t)(comp_ns * tb.denom / tb.numer),
        .constraint = (uint32_t)(period_ns * tb.denom / tb.numer), .preemptible = 1};
    kern_return_t kr = thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&p, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
#else


    int front_perf_hint(uint64_t period_ns, uint64_t comp_ns) __attribute__((weak));
    kern_return_t kr = front_perf_hint && !front_perf_hint(period_ns, comp_ns) ? KERN_SUCCESS : KERN_FAILURE;
#endif
    if (kr == KERN_SUCCESS) { d->comp_ms = comp_ns / 1e6; d->period_ms = period_ns / 1e6; }
    LOG("[nvn] %s时间约束策略（周期 %.3f ms、计算 %.3f ms）%s", d->who, period_ns / 1e6, comp_ns / 1e6, kr == KERN_SUCCESS ? "已设" : "设置失败");
}
static int rt_comp_fixed_ms(void) { static int v = -2; if (v == -2) { const char *e = getenv("DM_RT_COMP"); v = e && *e && strcmp(e, "auto") ? atoi(e) : -1; } return v; }
static void set_time_constraint(rt_decl *d, uint64_t period_ns) { int f = rt_comp_fixed_ms(); set_time_constraint_comp(d, period_ns, f > 0 ? (uint64_t)f * 1000000ull : 2000000ull); }


static void rt_comp_tick(rt_decl *d, uint64_t period_ns) {
    if (rt_comp_fixed_ms() > 0 || !period_ns || !d->comp_ms) return;
    uint64_t c = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID); if (!d->last) { d->last = c; return; }
    d->win[d->n++] = (double)(c - d->last) / 1e6; d->last = c;
    if (d->n < 120) return;
    double v[120]; memcpy(v, d->win, sizeof v); qsort(v, 120, sizeof *v, dcmp); d->n = 0;
    double p90 = v[107], comp = p90 * 1.25, cap = period_ns * 0.9 / 1e6; if (comp > cap) comp = cap; if (comp < 1) comp = 1;
    if (comp > d->comp_ms * 1.15 || comp < d->comp_ms * 0.85) set_time_constraint_comp(d, period_ns, (uint64_t)(comp * 1e6));
}
static void sleep_until_ns(uint64_t dl) {
    static mach_timebase_info_data_t tb; if (!tb.denom) mach_timebase_info(&tb);
    uint64_t now = now_ns(); if (dl <= now) return;
    mach_wait_until(mach_absolute_time() + (dl - now) * tb.denom / tb.numer);
}






static _Atomic int g_pa = -1; static int g_pa_k; static double g_pa_ema, g_pa_screen_hz; static unsigned g_pa_up; static uint64_t g_pa_last;
static int pace_adapt_enabled(void) {
    if (g_pa < 0) { const char *e = getenv("DM_PACE_ADAPT"); g_pa = e && *e == '1'; }
    return g_pa;
}
static double present_auto_hz(double screen_hz) {
#ifdef __ANDROID__

    if (mtl_fps_target() == 30) return dm_pace_hz(screen_hz, 30);
#endif


    if (pace_adapt_enabled() && screen_hz > 0) return screen_hz / nvn_adaptive_divisor(screen_hz, g_pa_k, g_pa_screen_hz);
    return dm_pace_hz(screen_hz, 60);
}
static int g_hz = -1, g_display_mode, g_hz_auto;
void nvn_present_config(int hz, int display_mode) { g_hz = hz; g_display_mode = display_mode; g_hz_auto = 0; }
double front_display_refresh(char *name, size_t n);
static void present_cfg(const win_t *o) {
    if (g_hz >= 0) return;


    int scr = mtl_display_hz();
    const char *e = getenv("DM_PRESENT_HZ"); g_hz = e ? atoi(e) : 60; g_hz_auto = !e && scr > 0;
    if (!g_hz) LOG("[nvn] 【警告】DM_PRESENT_HZ=0：不限帧率，帧节奏非真机，界面逻辑可能偏离（仅供性能剖析，不能用于判据 / 对拍）");








    const char *m = getenv("DM_PRESENT_INTERVAL"); g_display_mode = m ? !strcmp(m, "display") : scr > 0;
    char name[128]; front_display_refresh(name, sizeof name);
    if (g_hz_auto) LOG("[nvn] 出图节拍 %.2f Hz，间隔模式 %s（游戏设的出图间隔 %d；屏幕 %s，刷新率 %.2f Hz，取整除的一档）", present_auto_hz(mtl_display_refresh()), g_display_mode ? "display（每节拍一帧，剪口）" : "game（原版语义）", o->interval,
        *name ? name : "主屏", mtl_display_refresh());
    else LOG("[nvn] 出图节拍 %d Hz，间隔模式 %s（游戏设的出图间隔 %d；%s）", g_hz, g_display_mode ? "display（每节拍一帧，剪口）" : "game（原版语义）", o->interval,
        e ? "DM_PRESENT_HZ 钉住" : "无窗口，固定 60 Hz");
}









__attribute__((weak)) uint64_t dm_present_late_count(void) { return 0; } __attribute__((weak)) double xbg_recent_gpu_ms(void) { return -1; }
static void pace_adapt_tick(double screen_hz) {
    if (!pace_adapt_enabled() || screen_hz <= 0) return;



    uint64_t now = now_ns(), cpu = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID); static uint64_t pcpu; g_pa_wait = 0;
    if (!g_pa_last || !pcpu || !cpu || cpu < pcpu) { g_pa_last = now; pcpu = cpu; return; }
    double iv = (now - g_pa_last) / 1e6, busy = (cpu - pcpu) / 1e6; g_pa_last = now; pcpu = cpu;
    if (iv > 250) return;
    g_pa_ema += (busy - g_pa_ema) / 16;


    double need = g_pa_ema, gm = xbg_recent_gpu_ms ? xbg_recent_gpu_ms() : -1, cm = g_x_sync ? -1 : atomic_load(&g_pa_xcpu_ms);
    if (gm > need) need = gm; if (cm > need) need = cm;
    int kmin = nvn_adaptive_min_divisor(screen_hz), kmax = nvn_adaptive_max_divisor(screen_hz);
    g_pa_k = nvn_adaptive_divisor(screen_hz, g_pa_k, g_pa_screen_hz);
    g_pa_screen_hz = screen_hz;
    double per = g_pa_k * 1000.0 / screen_hz;







    static unsigned wn, skip, misses_local; static int probing; static uint64_t late0, stable_since, backoff = 3000000000ull;
    uint64_t lc = dm_present_late_count ? dm_present_late_count() : 0;
    if (!dm_present_late_count && iv > per * 1.25) misses_local++;
    if (!stable_since) { stable_since = now; late0 = lc; }
    if (skip) { skip--; late0 = lc; wn = 0; misses_local = 0; return; }
    if (++wn < 60) return;
    unsigned misses = dm_present_late_count ? (unsigned)(lc - late0) : misses_local; wn = 0; late0 = lc; misses_local = 0;
    if (probing) {
        probing = 0; stable_since = now;
        if (misses >= 4 && g_pa_k < kmax) { g_pa_k++; backoff = backoff * 2 > 60000000000ull ? 60000000000ull : backoff * 2; LOG("[nvn] 出图档 试升失败，退回 %.1f FPS（60 帧错过 %u 帧；下次 %.0f s 后再试）", screen_hz / g_pa_k, misses, backoff / 1e9); }
        else { backoff = 3000000000ull; LOG("[nvn] 出图档 ↑ %.1f FPS 站稳（60 帧错过 %u 帧；游戏 %.1f / 翻译 %.1f / GPU %.1f ms）", screen_hz / g_pa_k, misses, g_pa_ema, cm, gm); }
    } else if (misses >= 6 && g_pa_k < kmax) { g_pa_k++; stable_since = now; LOG("[nvn] 出图档 ↓ %.1f FPS（60 帧错过 %u 帧；游戏 %.1f / 翻译 %.1f / GPU %.1f ms）", screen_hz / g_pa_k, misses, g_pa_ema, cm, gm); }
    else if (g_pa_k > kmin && now - stable_since > backoff) { g_pa_k--; probing = 1; skip = 20; LOG("[nvn] 出图档 试升 %.1f FPS", screen_hz / g_pa_k); }
    (void)need; (void)per;
}
static uint64_t present_period(const win_t *o) {
    present_cfg(o);
    int itv = g_display_mode ? 1 : o->interval > 0 ? o->interval : 1; double hz = g_hz_auto ? present_auto_hz(mtl_display_refresh()) : g_hz;
    return hz > 0 ? (uint64_t)(itv * 1e9 / hz) : 0;
}
int (*g_nvn_loadscreen_up)(void);
NVN(nvnQueuePresentTexture, void, NVNqueue *q, NVNwindow *w, int idx) {
    (void)q; win_t *o = (win_t *)w; o->presents++;
    static uint64_t prev_in, prev_out, prev_pass, prev_draw;


    { static int gate;
      if (!gate && g_nvn_loadscreen_up && g_nvn_loadscreen_up()) { gate = 1; int ms = mtl_warm_wait(15000); if (ms) LOG("[nvn] 进局读取画面等管线预热 %d ms", ms); } }
    present_cfg(o);
    gfx_enter();
    uint64_t t0 = now_ns();
    if (idx >= 0 && idx < o->ntex && idx < 4) {
        xjob j = {XJ_PRESENT, 0, NULL, snap_tex(o->tex[idx]), present_period(o) / 1e9}; memcpy(j.crop, o->crop, sizeof j.crop);
        o->ticket[idx] = x_push(&j); }
    gfx_leave();
    atomic_fetch_add(&g_nvn_ns, now_ns() - t0);
    uint64_t f = g_frame, ps, dr; mtl_stats(&ps, &dr);
    if (nvn_real[0] || nvn_real[1]) {
        static unsigned isub = ~0u; static mach_timebase_info_data_t tb; if (!tb.denom) mach_timebase_info(&tb);
        if (isub == ~0u) for (isub = 0; isub < NVN_NFUNC && strcmp(nvn_names[isub], "nvnQueueSubmitCommands"); isub++) {}
        uint64_t rec = 0, sub = 0;
        static uint64_t acc[NVN_NFUNC];
        for (unsigned i = 0; i < NVN_NFUNC; i++) { uint64_t t = atomic_exchange(&nvn_prof_ticks[i], 0); acc[i] += t; if (i == isub) sub = t; else rec += t; }
        if ((g_frame + 1) % TWIN == 0) {
            char line[1024]; size_t o = 0;
            for (int k = 0; k < 5; k++) { unsigned best = 0; for (unsigned i = 1; i < NVN_NFUNC; i++) if (acc[i] > acc[best]) best = i;
                if (!acc[best]) break; o += snprintf(line + o, sizeof line - o, " %s %.2f ms；", nvn_names[best] + 3, acc[best] * tb.numer / tb.denom / 1e6); acc[best] = 0; }
            LOG("[nvn] 翻译层耗时前 5（近 300 帧合计）：%s", line); memset(acc, 0, sizeof acc);
        }
        trow(g_frame, "@rec_ms", rec * tb.numer / tb.denom / 1e6); trow(g_frame, "@submit_ms", sub * tb.numer / tb.denom / 1e6);
        tpush(4, rec * tb.numer / tb.denom / 1e6); tpush(5, sub * tb.numer / tb.denom / 1e6);
    }
    if (prev_in) {
        double fr = (t0 - prev_in) / 1e6, wk = (t0 - prev_out) / 1e6, nv = atomic_exchange(&g_nvn_ns, 0) / 1e6;
        tpush(0, fr); tpush(1, wk); tpush(2, nv); XTRACE_COUNT("dm-game-frame", f); XTRACE_COUNT("dm-frame-ms", (int64_t)fr);
        trow(f, "@frame_ms", fr); trow(f, "@work_ms", wk); trow(f, "@nvn_ms", nv);


        trow(f, "@wait_ms", g_acq_wait_ns / 1e6); trow(f, "@pq_wait_ms", g_pq_wait_ns / 1e6);
        if (frame_rec()) {
            static pthread_t pth; static uint64_t pcpu, pproc;
            uint64_t c = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID); pthread_t th = pthread_self();
            trow(f, "@cpu_ms", pcpu && pthread_equal(th, pth) ? (c - pcpu) / 1e6 : -1); pth = th; pcpu = c;
            struct rusage ru; getrusage(RUSAGE_SELF, &ru);
            uint64_t pc = (uint64_t)(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000000000ull + (uint64_t)(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1000ull;
            if (pproc) trow(f, "@proc_cpu_ms", (pc - pproc) / 1e6); pproc = pc;
            trow(f, "@call_ms", (t0 - g_t0_ns) / 1e6);
        }
        trow(f, "@passes", (double)(ps - prev_pass)); trow(f, "@draws", (double)(dr - prev_draw));
        static uint64_t pl, pst, pf, pudc; uint64_t l, st, fu; mtl_bw(&l, &st, &fu);
        trow(f, "@rt_load_mb", (l - pl) / 1048576.0); trow(f, "@rt_store_mb", (st - pst) / 1048576.0); trow(f, "@pass_full_rw", (double)(fu - pf));
        uint64_t udc = mtl_udc_saved_bytes(); trow(f, "@unwritten_dc_mb", (udc - pudc) / 1048576.0);
        pl = l; pst = st; pf = fu; pudc = udc;
        static uint64_t pae, par; uint64_t ae_, ar_; mtl_args_stats(&ae_, &ar_);
        trow(f, "@args_enc", (double)(ae_ - pae)); trow(f, "@args_reuse", (double)(ar_ - par)); pae = ae_; par = ar_;
        static uint64_t puc, pus; uint64_t uc_, us_; mtl_use_stats(&uc_, &us_);
        static uint64_t pdo; uint64_t do_ = mtl_ds_omit(); trow(f, "@ds_omit", (double)(do_ - pdo)); pdo = do_;
        trow(f, "@use_call", (double)(uc_ - puc)); trow(f, "@use_skip", (double)(us_ - pus)); puc = uc_; pus = us_;
        static uint64_t pfa, pre_; uint64_t fa_, pr_, up_, re_; nvn_dirty_stats(&fa_, &pr_); mtl_pkg_stats(&up_, &re_);
        trow(f, "@faults", (double)(fa_ - pfa)); trow(f, "@reuploads", (double)(re_ - pre_)); pfa = fa_; pre_ = re_;
        static uint64_t pmc, pmp, pmn; uint64_t mc_, mpg_, mn_; nvn_dirty_mp_stats(&mc_, &mpg_, &mn_);
        trow(f, "@mprotect_calls", (double)(mc_ - pmc)); trow(f, "@mprotect_pages", (double)(mpg_ - pmp)); trow(f, "@mprotect_ms", (mn_ - pmn) / 1e6); pmc = mc_; pmp = mpg_; pmn = mn_;
        double up_ms = mtl_upload_ns_take() / 1e6; trow(f, "@upload_ms", up_ms);
        trow(f, "@upload_gpu_ms", mtl_upload_gpu_ns_take() / 1e6);


        static uint64_t plib, ppso; uint64_t lib_, pso_; mtl_create_stats(&lib_, &pso_);
        trow(f, "@lib_new", (double)(lib_ - plib)); trow(f, "@pso_new", (double)(pso_ - ppso));
        {
            static uint64_t plns, ppns, pup, prup, pub; uint64_t lns, pns, up, rup, ub; mtl_create_ns(&lns, &pns); mtl_pkg_stats(&up, &rup); ub = mtl_upload_bytes();
            lf_rec *r = &g_lf[f % LFN]; r->f = f; r->fr = fr; r->nv = nv; r->ls = g_nvn_loadscreen_up ? g_nvn_loadscreen_up() : 0; r->t_s = (now_ns() - g_t0_ns) / 1e9; r->pso = (unsigned)(pso_ - ppso); r->lib = (unsigned)(lib_ - plib);
            r->make_ms = ((lns - plns) + (pns - ppns)) / 1e6; r->up = (unsigned)(up - pup); r->rup = (unsigned)(rup - prup); r->up_mb = (ub - pub) / 1048576.0; r->up_ms = up_ms;
            plns = lns; ppns = pns; pup = up; prup = rup; pub = ub;
        }
        plib = lib_; ppso = pso_;
        trow(f, "@t_s", (now_ns() - g_t0_ns) / 1e9);


        { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); trow(f, "@mono_s", ts.tv_sec + ts.tv_nsec / 1e9); }


        { static uint64_t pw, pk; uint64_t w, k; dm_hle_wake_counts(&w, &k); trow(f, "@wakes", (double)(w - pw)); trow(f, "@wake_skips", (double)(k - pk)); pw = w; pk = k; }
        trow(f, "@drawable_ms", mtl_drawable_ns_take() / 1e6);
        trow(f, "@sub_wall_ms", atomic_exchange(&g_sub_wall_ns, 0) / 1e6); trow(f, "@sub_cpu_ms", atomic_exchange(&g_sub_cpu_ns, 0) / 1e6); trow(f, "@sub_instr", (double)atomic_exchange(&g_sub_instr, 0));
    } else atomic_store(&g_nvn_ns, 0);
    g_acq_wait_ns = g_pq_wait_ns = 0;
    if (f >= 8) { double g = mtl_gpu_ms(f - 8); if (g >= 0) { tpush(3, g); trow(f - 8, "@gpu_ms", g); g_hr[g_hn++ % HRN] = g; }
        { lf_rec *r = &g_lf[(f - 8) % LFN];
          static int on = -1; if (on < 0) { const char *e = getenv("DM_LONGFRAME"); on = !(e && !strcmp(e, "0")); }


          static double ef = 16.7, eg = 8; int lf = r->fr > 20.0 && r->fr > 1.4 * ef, lg = g > 16.7 && g > 1.3 * eg;
          if (r->f == f - 8) { ef += ((r->fr < 3 * ef ? r->fr : 3 * ef) - ef) / 16; eg += ((g < 3 * eg ? g : 3 * eg) - eg) / 16; }
          if (on && r->f == f - 8 && f - 8 > 0 && (lf || lg)) { char b[512];
              snprintf(b, sizeof b, "帧 %llu（%.2f s）整帧 %.1f ms，提交+出图 %.1f ms，GPU %.1f ms；新建管线 %u 条 + 着色器库 %u 个共 %.1f ms；上传贴图 首传 %u 张 + 重传 %u 次 %.1f MB / %.1f ms",
                       (unsigned long long)r->f, r->t_s, r->fr, r->nv, g, r->pso, r->lib, r->make_ms, r->up, r->rup, r->up_mb, r->up_ms);



              if (!lf) strlcat(b, "；整帧不长，GPU 长", sizeof b);


              const lf_rec *pv = &g_lf[(f - 9) % LFN]; int hidden = r->ls || (f >= 9 && pv->f == f - 9 && pv->ls);
              if (hidden) strlcat(b, "；接读取画面，不可见", sizeof b);
              dm_longframe_note(b); } }
        uint64_t g0, g1; if (frame_rec() && mtl_gpu_span(f - 8, &g0, &g1)) { trow(f - 8, "@gpu_beg_ms", (g0 - g_t0_ns) / 1e6); trow(f - 8, "@gpu_end_ms", (g1 - g_t0_ns) / 1e6); } }
    prev_in = t0; prev_pass = ps; prev_draw = dr;
    dump_frame();
    if (g_frame % TWIN == 0) tsummary(g_frame);
    { static uint64_t pc, py, pf; uint64_t i_, y_, c_ = clock_gettime_nsec_np(CLOCK_THREAD_CPUTIME_ID); thr_counts(&i_, &y_);
      if (g_frame % TWIN == 0) { if (pf && g_frame > pf && c_ > pc) LOG("[nvn] 主线程 至帧 %llu：每帧 CPU %.2f ms，平均主频 %.2f GHz（周期 / CPU 时间）", g_frame, (c_ - pc) / 1e6 / (double)(g_frame - pf), (double)(y_ - py) / (double)(c_ - pc)); pc = c_; py = y_; pf = g_frame; } }
    { xjob j = {XJ_FRAME}; j.frame = g_frame; j.period = present_period(o) / 1e9; x_push(&j); }


    if (g_hz_auto && !dm_detclock_enabled()) pace_adapt_tick(mtl_display_refresh());
    uint64_t period = present_period(o), now = dm_detclock_enabled() ? guest_ns() : now_ns();
    nvn_pacer pc = {o->pace};
    uint64_t slot = now + nvn_pace(&pc, now, period);
    o->pace = pc.target;
    if (idx >= 0 && idx < 4) o->avail[idx] = period ? slot + period : 0;
    static uint64_t prev_slot; if (prev_slot && period) trow(f, "@display_ms", (slot - prev_slot) / 1e6); prev_slot = slot;


    if (o->presents > 9) { uint64_t n = o->presents - 8; double a_ = mtl_presented_time(n), b_ = mtl_presented_time(n - 1);
        if (a_ < 0) trow(f - 8, "@presented_ms", -1); else if (a_ > 0 && b_ > 0) trow(f - 8, "@presented_ms", (a_ - b_) * 1e3);
        g_pres_iv[g_presn++ % TWIN] = a_ < 0 ? -1 : (a_ > 0 && b_ > 0 ? (a_ - b_) * 1e3 : 0);
        { double iv_ = a_ < 0 ? -1 : (a_ > 0 && b_ > 0 ? (a_ - b_) * 1e3 : 0); mtl_ub_present(iv_); }
        dm_perf_present(a_ < 0 ? -1 : (a_ > 0 && b_ > 0 ? (a_ - b_) * 1e3 : 0));
        if (a_ > 0) trow(f - 8, "@pres_at_ms", a_ * 1e3 - g_t0_ns / 1e6); }
    if (f >= 8 && slow_on()) slow_emit(f - 8);
    prev_out = now_ns();
}
