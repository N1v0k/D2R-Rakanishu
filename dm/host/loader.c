



#include "dm.h"
#include <TargetConditionals.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#endif
#include "hle.h"

module_t g_mod[MAX_MOD];
int g_nmod;
pthread_key_t g_key18, g_keytls, g_keytlr, g_keygate;
pthread_key_t g_keyres[2], g_keyaspect, g_keyaspectui;
enum { KEY_K18, KEY_KTLS, KEY_KTLR, KEY_KGATE, KEY_KRES0, KEY_KRES1, KEY_KASPECT, KEY_KASPECTUI, KEY_N };

uint64_t resolve_import(module_t *m, const char *name, int weak);
static void tsd_probe(void);
void gate_register(uint64_t ret_addr, const char *kind, int arg, uint64_t orig);

static pthread_key_t key_value(unsigned i) {
    switch (i) {
    case KEY_K18: return g_key18; case KEY_KTLS: return g_keytls; case KEY_KTLR: return g_keytlr;
    case KEY_KGATE: return g_keygate; case KEY_KRES0: return g_keyres[0]; case KEY_KRES1: return g_keyres[1];
    case KEY_KASPECT: return g_keyaspect; default: return g_keyaspectui;
    }
}
static void create_keys(void) {
    if (pthread_key_create(&g_key18, NULL) || pthread_key_create(&g_keytls, NULL) ||
        pthread_key_create(&g_keytlr, NULL) || pthread_key_create(&g_keygate, NULL)) die("创建宿主键失败");


    if (pthread_key_create(&g_keyres[0], NULL) || pthread_key_create(&g_keyres[1], NULL) ||
        pthread_key_create(&g_keyaspect, NULL) || pthread_key_create(&g_keyaspectui, NULL)) die("创建参数位键失败");


    LOG("键号 K18=%lu KTLS=%lu KTLR=%lu KGATE=%lu KRES0=%lu KRES1=%lu KASPECT=%lu KASPECTUI=%lu",
        (unsigned long)(unsigned)g_key18, (unsigned long)(unsigned)g_keytls, (unsigned long)(unsigned)g_keytlr, (unsigned long)(unsigned)g_keygate,
        (unsigned long)(unsigned)g_keyres[0], (unsigned long)(unsigned)g_keyres[1], (unsigned long)(unsigned)g_keyaspect, (unsigned long)(unsigned)g_keyaspectui);
    tsd_probe();
}









#ifdef __ANDROID__
#define DM_PATCH_TARGET 1
static uint64_t g_tsd_off;
static uint8_t *tsd_base(void) { uint64_t tp; __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp)); return ((uint8_t **)tp)[-1]; }


static uint64_t key_index(pthread_key_t k) { return (uint32_t)k & 0x7fffffffu; }
static uint64_t slot_off(pthread_key_t k) { return g_tsd_off + key_index(k) * 16 + 8; }
static void tsd_probe(void) {
    void *const magic = (void *)0x6d4d7d5d4d3d2d1dull, *const magic2 = (void *)0x1d2d3d4d5d7d4d6dull;
    pthread_setspecific(g_key18, magic);
    uint8_t *b = tsd_base(); uint64_t at = 0;
    for (uint64_t o = 8; o < 0x4000 && !at; o += 8) if (*(void **)(b + o) == magic) at = o;
    pthread_setspecific(g_key18, 0);
    if (!at || at < key_index(g_key18) * 16 + 8) die("bionic 键表没找到（键 %#x 设的值不在 bionic_tls 前 16 KB 里）", (unsigned)g_key18);
    g_tsd_off = at - key_index(g_key18) * 16 - 8;
    pthread_setspecific(g_keygate, magic2);
    int ok = *(void **)(b + slot_off(g_keygate)) == magic2;
    pthread_setspecific(g_keygate, 0);
    if (!ok) die("bionic 键表布局不符（键表偏移 %#llx 对键 %#x 不成立）", (unsigned long long)g_tsd_off, (unsigned)g_keygate);
    LOG("bionic 键表偏移 %#llx", (unsigned long long)g_tsd_off);
}
#elif defined(__OHOS__)





#define DM_PATCH_TARGET 2
#define DM_OHOS_TSD_OFF 272
static uint8_t *tsd_base(void) { uint64_t tp; __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp)); return *(uint8_t **)(tp - DM_OHOS_TSD_OFF); }
static uint64_t slot_off(pthread_key_t k) { return (uint64_t)k * 8; }
static void tsd_probe(void) {
    void *const magic = (void *)0x6d4d7d5d4d3d2d1dull;
    pthread_setspecific(g_keygate, magic);
    int ok = *(void **)(tsd_base() + slot_off(g_keygate)) == magic;
    pthread_setspecific(g_keygate, 0);
    if (!ok) die("musl 线程键布局不符（tpidr_el0-%d 处不是 tsd 指针）：系统 libc 变了，需重测偏移并重出改写产物", DM_OHOS_TSD_OFF);
}
#else
#define DM_PATCH_TARGET 0
static uint8_t *tsd_base(void) { uint64_t b; __asm__ volatile("mrs %0, tpidrro_el0" : "=r"(b)); return (uint8_t *)(b & ~7ull); }
static uint64_t slot_off(pthread_key_t k) { return (uint64_t)k * 8; }
static void tsd_probe(void) {}
#endif


static void tsd_check(void) {
    pthread_setspecific(g_key18, (void *)0x1234);
    uint64_t off = *(uint64_t *)(g_mod[0].base + g_mod[0].key_page_off + g_mod[0].key_idx[KEY_K18] * 8);
    if (*(uint64_t *)(tsd_base() + off) != 0x1234) die("TSD 布局不符");
    pthread_setspecific(g_key18, 0);
}

static void build_symhash(module_t *m);
static void parse_dynamic(module_t *m) {
    uint32_t mod0 = *(uint32_t *)(m->base + 4);
    if (memcmp(m->base + mod0, "MOD0", 4)) die("%s: no MOD0", m->name);
    m->dyn = (const Elf64_Dyn *)(m->base + mod0 + *(int32_t *)(m->base + mod0 + 4));
    for (const Elf64_Dyn *d = m->dyn; d->d_tag; d++) switch (d->d_tag) {
        case DT_SYMTAB: m->symtab = (const Elf64_Sym *)(m->base + d->d_un.d_ptr); break;
        case DT_STRTAB: m->strtab = (const char *)(m->base + d->d_un.d_ptr); break;
        case DT_INIT: m->init = d->d_un.d_ptr; break;
        case DT_INIT_ARRAY: m->init_array = d->d_un.d_ptr; break;
        case DT_INIT_ARRAYSZ: m->init_arraysz = d->d_un.d_val; break;
    }
    m->nsym = (unsigned)(((const uint8_t *)m->strtab - (const uint8_t *)m->symtab) / sizeof(Elf64_Sym));
    build_symhash(m);
}

static void *module_sym_linear(module_t *m, const char *name) {
    for (unsigned i = 1; i < m->nsym; i++) {
        const Elf64_Sym *s = &m->symtab[i];
        if (s->st_shndx && !strcmp(m->strtab + s->st_name, name)) return m->base + s->st_value;
    }
    return NULL;
}




static uint32_t sym_hash(const char *s) { uint32_t h = 2166136261u; while (*s) h = (h ^ (uint8_t)*s++) * 16777619u; return h; }
static void build_symhash(module_t *m) {
    unsigned cap = 16; while (cap < m->nsym * 2) cap <<= 1;
    m->symh = calloc(cap, sizeof *m->symh); m->symh_mask = cap - 1;
    for (unsigned i = 1; i < m->nsym; i++) {
        const Elf64_Sym *s = &m->symtab[i]; if (!s->st_shndx) continue;
        const char *nm = m->strtab + s->st_name;
        for (unsigned j = sym_hash(nm) & m->symh_mask;; j = (j + 1) & m->symh_mask) {
            if (!m->symh[j]) { m->symh[j] = i; break; }
            if (!strcmp(m->strtab + m->symtab[m->symh[j]].st_name, nm)) break;
        }
    }
}
static unsigned long g_symchk_n, g_symchk_bad;
void *module_sym(module_t *m, const char *name) {
    unsigned long nq; (void)nq;
    if (!m->symh) return module_sym_linear(m, name);
    void *r = NULL;
    for (unsigned j = sym_hash(name) & m->symh_mask; m->symh[j]; j = (j + 1) & m->symh_mask) {
        const Elf64_Sym *s = &m->symtab[m->symh[j]];
        if (!strcmp(m->strtab + s->st_name, name)) { r = m->base + s->st_value; break; }
    }
    static int chk = -1; if (chk < 0) chk = getenv("DM_SYMCHECK") != NULL;
    if (chk) { void *l = module_sym_linear(m, name); g_symchk_n++; nq = g_symchk_n;
               if (l != r) { g_symchk_bad++; LOG("【红】DM_SYMCHECK：%s 在 %s 哈希 %p ≠ 线性 %p", name, m->name, r, l); } }
    return r;
}
void *guest_sym(const char *name) {
    for (int i = 0; i < g_nmod; i++) { void *p = module_sym(&g_mod[i], name); if (p) return p; }
    return NULL;
}
module_t *module_of(const void *p) {
    for (int i = 0; i < g_nmod; i++)
        if ((const uint8_t *)p >= g_mod[i].base && (const uint8_t *)p < g_mod[i].base + g_mod[i].size) return &g_mod[i];
    return NULL;
}



static uint64_t resolve_sym(module_t *m, const char *name, int weak) {
    for (int i = 0; i < g_nmod; i++) { void *p = module_sym(&g_mod[i], name); if (p) return (uint64_t)p; }
    return resolve_import(m, name, weak);
}



static const hle_force_ent *g_force; static unsigned g_nforce; static unsigned *g_force_hits;
static void force_init(void) {
    unsigned long sz; DM_SECT_GET(swforce, const hle_force_ent, g_force, sz);
    g_nforce = g_force ? (unsigned)(sz / sizeof *g_force) : 0;
    g_force_hits = calloc(g_nforce + 1, sizeof *g_force_hits);
    for (unsigned i = 0; i < g_nforce; i++)
        for (unsigned j = i + 1; j < g_nforce; j++)
            if (!strcmp(g_force[i].name, g_force[j].name)) die("强制接管清单重复：%s", g_force[i].name);
}
static void *force_lookup(const char *name) {
    for (unsigned i = 0; i < g_nforce; i++)
        if (!strcmp(g_force[i].name, name)) {
            if (!g_force_hits[i]++ && g_flag.imports) LOG("force %s  （%s）", name, g_force[i].why);
            void *hlecount_wrap(const char *, const char *, void *); return hlecount_wrap("FORCE", name, g_force[i].fn);
        }
    return NULL;
}
static void force_check(void) {
    int bad = 0;
    for (unsigned i = 0; i < g_nforce; i++)
        if (!g_force_hits[i]) { LOG("强制接管条目无任何引用：%s", g_force[i].name); bad = 1; }
    if (bad) die("强制接管清单有误");
    LOG("force: %u 条全部命中", g_nforce);
}



static void read_layout(module_t *m, const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/%s.fixups.tsv", dir, m->name);
    FILE *f = fopen(path, "r");
    if (!f) die("open %s", path);
    static const char *kn[8] = {"K18", "KTLS", "KTLR", "KGATE", "KRES0", "KRES1", "KASPECT", "KASPECTUI"};
    char line[256], key[32]; unsigned long v, target = 0; unsigned seen = 0;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "# %31s %lx", key, &v) != 2) die("%s: 产物是旧改写器出的，换成当前代号（dm/PRODUCT）的产物", path);
        if (!strcmp(key, "TARGET")) target = v;
        else if (!strcmp(key, "PATCH_OFF")) m->patch_off = v;
        else if (!strcmp(key, "KEY_PAGE_OFF")) m->key_page_off = v;
        else if (!strcmp(key, "KEY_PAGE_SIZE")) m->key_page_size = v;
        else if (!strcmp(key, "TEXT_END")) m->text_end = v;
        else if (!strcmp(key, "RO_OFF")) m->ro_off = v;
        else if (!strcmp(key, "DATA_OFF")) m->data_off = v;
        else for (int k = 0; k < 8; k++) {
            size_t n = strlen(kn[k]);
            if (!strncmp(key, "KEY_", 4) && !strncmp(key + 4, kn[k], n) && !strcmp(key + 4 + n, "_IDX")) {
                m->key_idx[k] = (unsigned)strtoul(strrchr(line, '\t') + 1, 0, 10); seen |= 1u << k;
            }
        }
    }
    fclose(f);
    if (!m->patch_off || m->key_page_off % 0x4000 || m->key_page_size != 0x4000 || seen != 0xff)
        die("%s: 产物是旧改写器出的，换成当前代号（dm/PRODUCT）的产物", path);
    static const char *const tname[] = {"苹果", "安卓", "鸿蒙"}, *const tflag[] = {"apple", "android", "ohos"};
    if (target != DM_PATCH_TARGET) die("%s: 产物是给%s出的（TARGET %lu），本机要 %s 的产物（patch_nso.py 用 DM_TARGET=%s 重出）", path,
                                       target < 3 ? tname[target] : "未知平台", target, tname[DM_PATCH_TARGET], tflag[DM_PATCH_TARGET]);
}

static void install_key_page(module_t *m) {
    if (m->key_page_off + m->key_page_size > m->size) die("%s: 键页越界", m->name);
    uint64_t *page = (uint64_t *)(m->base + m->key_page_off);
    memset(page, 0, m->key_page_size);
    for (unsigned i = 0; i < KEY_N; i++) page[m->key_idx[i]] = slot_off(key_value(i));
    for (int k = 0; k < 8; k++) if (!page[m->key_idx[k]]) die("%s: 键页第 %d 项为 0（宿主键没建）——游戏会读到 TSD 0 号槽", m->name, k);
    if (mprotect(m->base + m->key_page_off, m->key_page_size, PROT_READ)) die("%s 键页 mprotect", m->name);
}

static void register_gates(module_t *m, const char *dir) {
    char path[1024], line[256]; snprintf(path, sizeof path, "%s/%s.gates.tsv", dir, m->name);
    FILE *f = fopen(path, "r");
    int ng = 0;
    if (f) {
        unsigned long ret, orig; char kind[16]; int arg;
        while (fgets(line, sizeof line, f))
            if (sscanf(line, "%lx %15s %d %lx", &ret, kind, &arg, &orig) == 4) { gate_register((uint64_t)m->base + ret, kind, arg, orig); ng++; }
        fclose(f);
    }
    LOG("%s: gates %d patch_off %#llx keys %lu/%lu/%lu/%lu", m->name, ng, m->patch_off, (unsigned long)(unsigned)g_key18, (unsigned long)(unsigned)g_keytls, (unsigned long)(unsigned)g_keytlr, (unsigned long)(unsigned)g_keygate);
}

static void relocate(module_t *m) {
    uint64_t rela = 0, relasz = 0, jmprel = 0, pltsz = 0;
    for (const Elf64_Dyn *d = m->dyn; d->d_tag; d++) switch (d->d_tag) {
        case DT_RELA: rela = d->d_un.d_ptr; break;
        case DT_RELASZ: relasz = d->d_un.d_val; break;
        case DT_JMPREL: jmprel = d->d_un.d_ptr; break;
        case DT_PLTRELSZ: pltsz = d->d_un.d_val; break;
    }
    int cnt[3] = {0};
    const Elf64_Rela *tabs[2] = {(const Elf64_Rela *)(m->base + rela), (const Elf64_Rela *)(m->base + jmprel)};
    uint64_t sizes[2] = {relasz, pltsz};
    for (int t = 0; t < 2; t++)
        for (uint64_t i = 0; i < sizes[t] / sizeof(Elf64_Rela); i++) {
            const Elf64_Rela *r = &tabs[t][i];
            uint64_t *where = (uint64_t *)(m->base + r->r_offset);
            unsigned type = ELF64_R_TYPE(r->r_info), si = ELF64_R_SYM(r->r_info);
            if (type == R_AARCH64_RELATIVE) { *where = (uint64_t)m->base + r->r_addend; cnt[0]++; continue; }
            const Elf64_Sym *s = &m->symtab[si];
            uint64_t v = (uint64_t)force_lookup(m->strtab + s->st_name);
            if (!v) v = s->st_shndx ? (uint64_t)m->base + s->st_value : resolve_sym(m, m->strtab + s->st_name, (s->st_info >> 4) == 2);
            if (type == R_AARCH64_JUMP_SLOT) { *where = v; cnt[2]++; }
            else { *where = v + r->r_addend; cnt[1]++; }
        }
    LOG("%s: reloc relative=%d abs/glob=%d jump=%d", m->name, cnt[0], cnt[1], cnt[2]);
}







#include <CommonCrypto/CommonDigest.h>
void loader_hash_log(void) {
    if (!getenv("DM_LOADHASH")) return;
    for (int k = 0; k < g_nmod; k++) {
        module_t *m = &g_mod[k];
        uint8_t *cp = malloc(m->size); memcpy(cp, m->base, m->size);
        uint64_t rela = 0, relasz = 0, jmprel = 0, pltsz = 0;
        for (const Elf64_Dyn *d = m->dyn; d->d_tag; d++) switch (d->d_tag) {
            case DT_RELA: rela = d->d_un.d_ptr; break; case DT_RELASZ: relasz = d->d_un.d_val; break;
            case DT_JMPREL: jmprel = d->d_un.d_ptr; break; case DT_PLTRELSZ: pltsz = d->d_un.d_val; break;
        }
        const Elf64_Rela *tabs[2] = {(const Elf64_Rela *)(m->base + rela), (const Elf64_Rela *)(m->base + jmprel)};
        uint64_t sizes[2] = {relasz, pltsz}; unsigned nslot = 0;
        for (int t = 0; t < 2; t++)
            for (uint64_t i = 0; i < sizes[t] / sizeof(Elf64_Rela); i++) {
                const Elf64_Rela *r = &tabs[t][i]; uint64_t tok[1] = {0};
                if (ELF64_R_TYPE(r->r_info) != R_AARCH64_RELATIVE) {
                    uint64_t v = *(const uint64_t *)(m->base + r->r_offset); int hit = 0;
                    for (int j = 0; j < g_nmod; j++) if (v >= (uint64_t)g_mod[j].base && v < (uint64_t)g_mod[j].base + g_mod[j].size) { tok[0] = 1 + j; hit = 1; break; }
                    if (!hit) { const char *nm = m->strtab + m->symtab[ELF64_R_SYM(r->r_info)].st_name; uint64_t h = 0xcbf29ce484222325ull;
                                for (; *nm; nm++) h = (h ^ (uint8_t)*nm) * 0x100000001b3ull; tok[0] = h | 1ull << 63; }
                }
                memcpy(cp + r->r_offset, &tok[0], 8);
                nslot++;
            }
        CC_SHA256_CTX c; CC_SHA256_Init(&c); CC_SHA256_Update(&c, cp, (CC_LONG)m->size);
        for (int t = 0; t < 2; t++)
            for (uint64_t i = 0; i < sizes[t] / sizeof(Elf64_Rela); i++) {
                const Elf64_Rela *r = &tabs[t][i]; uint64_t tail = (uint64_t)r->r_addend;
                if (ELF64_R_TYPE(r->r_info) != R_AARCH64_RELATIVE) {
                    uint64_t v = *(const uint64_t *)(m->base + r->r_offset);
                    for (int j = 0; j < g_nmod; j++) if (v >= (uint64_t)g_mod[j].base && v < (uint64_t)g_mod[j].base + g_mod[j].size) { tail = v - (uint64_t)g_mod[j].base; break; }
                }
                CC_SHA256_Update(&c, &tail, 8);
            }
        uint8_t d[32]; CC_SHA256_Final(d, &c); free(cp);
        char hx[65]; for (int i = 0; i < 32; i++) sprintf(hx + 2 * i, "%02x", d[i]);
        LOG("DM_LOADHASH %s 大小 %#zx 重定位槽 %u sha256 %s", m->name, m->size, nslot, hx);
    }
}

#if TARGET_OS_IPHONE








#include <mach/mach.h>
static void remap_at(module_t *m, vm_address_t to, const void *from, unsigned long size, boolean_t copy, vm_prot_t need) {
    vm_address_t a = to; vm_prot_t cur = 0, max = 0;
    kern_return_t kr = vm_remap(mach_task_self(), &a, size, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(), (vm_address_t)from, copy, &cur, &max, VM_INHERIT_NONE);
    if (kr != KERN_SUCCESS || a != to) die("%s vm_remap %p → %#lx（%#lx 字节）失败：%d", m->name, from, (unsigned long)to, size, kr);
    if ((cur & need) != need) die("%s vm_remap 后权限 %#x，缺 %#x", m->name, cur, need);
}
#ifdef DM_SHELL




#include <dlfcn.h>
#include <mach-o/dyld.h>
static const struct mach_header_64 *image_mh(void) {
    static const struct mach_header_64 *mh; if (mh) return mh;
    char exe[4096]; uint32_t n = sizeof exe; if (_NSGetExecutablePath(exe, &n)) die("取不到可执行文件路径");
    char *sl = strrchr(exe, '/'); if (!sl || (size_t)(sl - exe) + 40 > sizeof exe) die("可执行文件路径不对：%s", exe); strcpy(sl, "/Frameworks/Guest.framework/Guest");
    if (!dlopen(exe, RTLD_NOW)) die("这个安装包里还没有游戏程序（%s）：先用 dm/tools/d2rm_pack.py 把你自己的游戏文件做进安装包再装。系统的说法：%s", exe, dlerror());
    for (uint32_t i = 0; i < _dyld_image_count(); i++) if (strstr(_dyld_get_image_name(i), "/Guest.framework/Guest")) mh = (const struct mach_header_64 *)_dyld_get_image_header(i);
    if (!mh) die("Guest 动态库已加载但找不到它的映像头");
    fprintf(stderr, "boot: 模块映像取自 Guest 动态库 %s\n", exe); return mh;
}
#define IMAGE_MH image_mh()
#else
#define IMAGE_MH (&_mh_execute_header)
#endif
static void map_image(module_t *m, const char *dir) {
    (void)dir;
    char sx0[17], sx1[17], dm[17]; snprintf(sx0, sizeof sx0, "__dm%sx0", m->name); snprintf(sx1, sizeof sx1, "__dm%sx1", m->name); snprintf(dm, sizeof dm, "__DM%sW", m->name);
    unsigned long z0, zw, z1;
    const uint8_t *x0 = getsectiondata(IMAGE_MH, "__TEXT", sx0, &z0), *w = getsectiondata(IMAGE_MH, dm, "__img", &zw), *x1 = getsectiondata(IMAGE_MH, "__TEXT", sx1, &z1);
    if (!x0 || !w || !x1) die("内嵌节 %s / %s / %s 缺失", sx0, dm, sx1);
    if (z0 + zw != m->patch_off || (z0 & 0x3FFF) || (zw & 0x3FFF) || (z1 & 0x3FFF) || ((uintptr_t)x0 & 0x3FFF) || ((uintptr_t)w & 0x3FFF) || ((uintptr_t)x1 & 0x3FFF))
        die("%s 内嵌节布局不符（x0 %#lx w %#lx x1 %#lx，PATCH_OFF %#llx）", m->name, z0, zw, z1, (unsigned long long)m->patch_off);
    m->size = m->patch_off + z1;
    vm_address_t base = 0;
    if (vm_allocate(mach_task_self(), &base, m->size, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) die("%s 保留 %#zx 字节地址失败", m->name, m->size);
    remap_at(m, base, x0, z0, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    remap_at(m, base + z0, w, zw, TRUE, VM_PROT_READ | VM_PROT_WRITE);
    remap_at(m, base + m->patch_off, x1, z1, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    m->base = (uint8_t *)base; install_key_page(m);
}
#else
static uint8_t *slurp(const char *path, size_t *n) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open %s", path);
    struct stat st; fstat(fd, &st);
    uint8_t *p = malloc(st.st_size);
    if (read(fd, p, st.st_size) != st.st_size) die("read %s", path);
    close(fd); *n = st.st_size; return p;
}





static void map_image(module_t *m, const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/%s.patched.bin", dir, m->name);
    size_t sz; uint8_t *raw = slurp(path, &sz);
    m->size = (sz + 0x3FFF) & ~0x3FFFull;
    m->base = mmap(NULL, m->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m->base == MAP_FAILED) die("%s mmap", m->name);
    memcpy(m->base, raw, sz); free(raw); install_key_page(m);
    uint64_t rx = (m->text_end + 0x3FFF) & ~0x3FFFull;
    if (mprotect(m->base, rx, PROT_READ | PROT_EXEC)) die("%s mprotect text", m->name);
    uint64_t po = m->patch_off & ~0x3FFFull;
    if (mprotect(m->base + po, m->size - po, PROT_READ | PROT_EXEC)) die("%s mprotect patch", m->name);
}
#endif

void load_modules(const char *dir, const char **names, int n) {




    uint64_t t0 = now_ns(), tm = 0, tp = 0, tg = 0, tr = 0, t;
    for (int i = 0; i < n; i++) {
        module_t *m = &g_mod[i];
        strlcpy(m->name, names[i], sizeof m->name);
        read_layout(m, dir);
    }
    create_keys();
    for (int i = 0; i < n; i++) {
        module_t *m = &g_mod[i];
        t = now_ns(); map_image(m, dir); tm += now_ns() - t;
        if (!i) tsd_check();
        t = now_ns(); parse_dynamic(m); tp += now_ns() - t;
        t = now_ns(); register_gates(m, dir); tg += now_ns() - t;
        LOG("%s: base %p size %#zx", names[i], m->base, m->size);
        g_nmod++;
    }
    force_init();
    t = now_ns(); for (int i = 0; i < g_nmod; i++) relocate(&g_mod[i]); tr = now_ns() - t;
    force_check();
    { void hlecount_seal(void); hlecount_seal(); }
    if (getenv("DM_SYMCHECK")) LOG("DM_SYMCHECK：装载期共核 %lu 次符号查询，哈希与线性不一致 %lu", g_symchk_n, g_symchk_bad);
    if (g_flag.bootprof) LOG("① 细分：映射（读 + 拷 + 封页）%.1f ms、解析动态段 %.1f ms、门注册 %.1f ms、重定位 + 强制接管 %.1f ms，合计 %.1f ms",
                             tm / 1e6, tp / 1e6, tg / 1e6, tr / 1e6, (now_ns() - t0) / 1e6);
}





void run_module_inits(void) {
    for (int i = 0; i < g_nmod; i++) {
        module_t *m = &g_mod[i];
        if (!m->init) continue;
        LOG("%s: DT_INIT +%#llx（init_array %llu 项）", m->name, m->init, m->init_arraysz / 8);
        ((void (*)(void))(m->base + m->init))();
    }
}
