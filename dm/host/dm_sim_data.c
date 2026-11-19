

#include "dm_sim_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define MAXF 80
typedef struct { char type[8]; int n; char name[MAXF][64]; } colset;
typedef struct { colset cs[32]; int ncs; const colset *cur; char *f[MAXF]; int nf; dm_sim_data *d; int bad;
                 int cap_skill, cap_miss, cap_mon, cap_anim, cap_state, cap_area, cap_spawn, cap_su, cap_boss; } parser;

static const char *const k_stat_names[DM_ST_N] = {
    "strength", "dexterity", "vitality", "energy", "maxhp", "maxmana", "hpregen", "item_manaregen", "armorclass", "item_armor_percent",
    "tohit", "item_tohit_percent", "toblock", "damagepercent", "item_deadlystrike", "item_crushingblow", "item_openwounds", "passive_critical_strike",
    "lifedrainmindam", "manadrainmindam", "item_fasterattackrate", "item_fastercastrate", "item_fastergethitrate", "item_fasterblockrate", "attackrate",
    "fireresist", "lightresist", "coldresist", "poisonresist", "damageresist", "magicresist", "maxfireresist", "maxlightresist", "maxcoldresist",
    "maxpoisonresist", "normal_damage_reduction", "magic_damage_reduction", "passive_fire_mastery", "passive_ltng_mastery", "passive_cold_mastery",
    "passive_pois_mastery", "passive_mag_mastery", "passive_fire_pierce", "passive_ltng_pierce", "passive_cold_pierce", "passive_pois_pierce",
    "passive_mag_pierce", "passive_dmg_pierce", "item_preventheal", "item_maxhp_percent" };
static const char *const k_cf[DM_CF_N] = {
    "calc1", "calc2", "calc3", "calc4", "calc5", "calc6", "calc7", "calc8", "calc9", "calc10", "ToHitCalc", "DmgSymPerCalc", "EDmgSymPerCalc",
    "ELenSymPerCalc", "auralencalc", "aurarangecalc", "petmax", "prgcalc1", "prgcalc2", "prgcalc3", "aurastatcalc1", "aurastatcalc2",
    "aurastatcalc3", "aurastatcalc4", "aurastatcalc5", "aurastatcalc6", "passivecalc1", "passivecalc2", "passivecalc3", "passivecalc4",
    "passivecalc5", "passivecalc6", "passivecalc7", "passivecalc8", "passivecalc9", "passivecalc10", "passivecalc11", "passivecalc12",
    "passivecalc13", "passivecalc14", "sumsk1calc", "sumsk2calc", "sumsk3calc", "sumsk4calc", "sumsk5calc", "ddam1", "ddam2", "perdelay", "strikeperiod", "burnlen" };
static const char *const k_mf[DM_MF_N] = { "DmgCalc1", "Range", "DmgSymPerCalc", "EDmgSymPerCalc", "SrvCalc1" };
static const char *const k_role[DM_ROLE_N] = { "dmgpct", "flat", "elpct", "sias", "hits", "attacks", "after1pct", "nmiss", "jumps", "petlvl",
                                               "pethp", "ll", "dur", "charges", "firepct", "coldpct", "ltngpct" };

static const char *fld(parser *p, const char *name) {
    const colset *c = p->cur;
    if (!c) { p->bad = 1; return ""; }
    for (int i = 0; i < c->n; i++)
        if (!strcmp(c->name[i], name)) {
            if (i + 1 < p->nf) return p->f[i + 1];
            break;
        }
    if (!p->bad) snprintf(p->d->err, sizeof p->d->err, "%s lacks %s", c->type, name);
    p->bad = 1;
    return "";
}
static int fi(parser *p, const char *name) {
    const char *s = fld(p, name); char *end; errno = 0;
    long v = strtol(s, &end, 10);
    if (!*s || *end || errno || v < INT_MIN || v > INT_MAX) p->bad = 1;
    return p->bad ? 0 : (int)v;
}
static int bounded(parser *p,const char *name,size_t width) {
    int v=fi(p,name);
    if((width==1 && (v<INT8_MIN || v>INT8_MAX)) || (width==2 && (v<INT16_MIN || v>INT16_MAX)))p->bad=1;
    return p->bad?0:v;
}
static int16_t narrow16(parser *p,int32_t value) {
    if(value<INT16_MIN || value>INT16_MAX){p->bad=1;return 0;}
    return (int16_t)value;
}
static void fs(parser *p, const char *name, char *dst, size_t cap) {
    const char *s = fld(p, name);
    snprintf(dst, cap, "%s", strcmp(s, "-") ? s : "");
}
static int flist(parser *p, const char *name, int32_t *out, int n) {
    const char *s = fld(p, name); int k=0;
    while(*s && k<n) {
        if(*s=='-' && (s[1]==',' || !s[1])){out[k++]=-1;s++;}
        else {
            char *end;errno=0;long v=strtol(s,&end,10);
            if(end==s || errno || v<INT_MIN || v>INT_MAX || (*end && *end!=',')){p->bad=1;break;}
            out[k++]=(int32_t)v;s=end;
        }
        if(*s==','){s++;if(!*s)p->bad=1;}else break;
    }
    if(*s || (k!=n && strcmp(name,"monsters")))p->bad=1;
    for(int i=k;i<n;i++)out[i]=0;
    return k;
}
static void flist16(parser *p, const char *name, int16_t *out, int n) {
    int32_t tmp[32];
    flist(p, name, tmp, n);
    for (int i = 0; i < n; i++) out[i] = narrow16(p,tmp[i]);
}

int dm_sim_fam_parse(const char *s, uint32_t *out) {
    static const struct { const char *n; uint32_t bit; } k[] = {
        { "W1", DM_FAM_W1 }, { "W2", DM_FAM_W2 }, { "W3", DM_FAM_W3 }, { "W4", DM_FAM_W4 }, { "W5", DM_FAM_W5 }, { "Z1", DM_FAM_Z1 },
        { "Z2", DM_FAM_Z2 }, { "Z3", DM_FAM_Z3 }, { "DoT", DM_FAM_DOT }, { "Pa", DM_FAM_PA }, { "A+", DM_FAM_AOFF }, { "A~", DM_FAM_ADMG },
        { "A-", DM_FAM_ADEF }, { "L", DM_FAM_L }, { "P", DM_FAM_P }, { "F", DM_FAM_F }, { "K", DM_FAM_K }, { "B", DM_FAM_B }, { "U", DM_FAM_U } };
    *out = 0;
    if (!strcmp(s, "-")) return 0;
    while (*s) {
        int hit = 0;
        for (size_t i = 0; i < sizeof k / sizeof *k; i++) {
            size_t n = strlen(k[i].n);
            if (!strncmp(s, k[i].n, n) && (s[n] == 0 || s[n] == '+')) { *out |= k[i].bit; s += n; hit = 1; break; }
        }
        if (!hit) return -1;
        if (*s == '+') { s++; if (!*s) return -1; }
    }
    return 0;
}

static void *grow(void *p, int n, int *cap, size_t sz) {
    if (n < *cap) return p;
    int c = *cap ? *cap * 2 : 64;
    void *q = realloc(p, (size_t)c * sz);
    if (q) *cap = c;
    return q;
}

static int skill_row(parser *p) {
    dm_sim_data *d = p->d;
    void *q = grow(d->skills, d->nskill, &p->cap_skill, sizeof *d->skills);
    if (!q) return -1;
    d->skills = q;
    dm_sim_skill *s = &d->skills[d->nskill];
    memset(s, 0, sizeof *s);
    int id = fi(p, "id");
    if (id < 0 || id >= DM_SIM_MAXSKILL) { snprintf(d->err, sizeof d->err, "skill id %d", id); return -1; }
    s->id = (int16_t)id; s->cls = bounded(p, "cls", sizeof s->cls);
    if (d->skill[id] || s->cls < -1 || s->cls > 7 || dm_sim_fam_parse(fld(p, "fam"), &s->fam)) { snprintf(d->err, sizeof d->err, "skill %d family", id); return -1; }
    fs(p, "name", s->name, sizeof s->name); fs(p, "anim", s->anim, sizeof s->anim); fs(p, "range", s->range, sizeof s->range);
    fs(p, "auratargetstate", s->auratargetstate, sizeof s->auratargetstate);
    s->hitfactor = bounded(p, "hitfactor", sizeof s->hitfactor);s->frameburn = bounded(p, "frameburn", sizeof s->frameburn);s->timed = bounded(p, "timed", sizeof s->timed);
    s->burnmiss = bounded(p, "burnmiss", sizeof s->burnmiss);s->burnfactor = bounded(p, "burnfactor", sizeof s->burnfactor);s->unavailable = bounded(p, "unavailable", sizeof s->unavailable);
    s->area = bounded(p, "area", sizeof s->area);
    fs(p, "passivestate", s->passivestate, sizeof s->passivestate);
    fs(p, "pettype", s->pettype, sizeof s->pettype); fs(p, "aurastate", s->aurastate, sizeof s->aurastate);
    s->descdam = bounded(p, "descdam", sizeof s->descdam); s->srvstfunc = bounded(p, "srvstfunc", sizeof s->srvstfunc); s->srvdofunc = bounded(p, "srvdofunc", sizeof s->srvdofunc); s->seqnum = bounded(p, "seqnum", sizeof s->seqnum); s->aura = bounded(p, "aura", sizeof s->aura);
    s->passive = bounded(p, "passive", sizeof s->passive); s->summon = bounded(p, "summon", sizeof s->summon); s->localdelay = bounded(p, "localdelay", sizeof s->localdelay); s->globaldelay = bounded(p, "globaldelay", sizeof s->globaldelay);
    s->mana = bounded(p, "mana", sizeof s->mana); s->lvlmana = bounded(p, "lvlmana", sizeof s->lvlmana); s->minmana = bounded(p, "minmana", sizeof s->minmana); s->manashift = bounded(p, "manashift", sizeof s->manashift);
    s->tohit = bounded(p, "ToHit", sizeof s->tohit); s->levtohit = bounded(p, "LevToHit", sizeof s->levtohit); s->hitshift = bounded(p, "HitShift", sizeof s->hitshift); s->srcdam = bounded(p, "SrcDam", sizeof s->srcdam);
    s->mindam = bounded(p, "MinDam", sizeof s->mindam); s->maxdam = bounded(p, "MaxDam", sizeof s->maxdam); flist(p, "MinLevDam", s->minlev, 5); flist(p, "MaxLevDam", s->maxlev, 5);
    s->etype = bounded(p, "EType", sizeof s->etype); s->emin = bounded(p, "EMin", sizeof s->emin); s->emax = bounded(p, "EMax", sizeof s->emax); flist(p, "EMinLev", s->eminlev, 5);
    flist(p, "EMaxLev", s->emaxlev, 5); s->elen = bounded(p, "ELen", sizeof s->elen); flist(p, "ELevLen", s->elevlen, 3); s->kick = bounded(p, "Kick", sizeof s->kick);
    s->periodic = bounded(p, "periodic", sizeof s->periodic); s->perdelay = bounded(p, "perdelay", sizeof s->perdelay); s->alwayshit = bounded(p, "alwayshit", sizeof s->alwayshit); s->useattackrate = bounded(p, "UseAttackRate", sizeof s->useattackrate); s->weapsel = (int8_t)bounded(p, "weapsel", sizeof s->weapsel);
    if (s->weapsel < 0 || s->weapsel > 4) { snprintf(d->err, sizeof d->err, "skill %d weapsel", id); return -1; }
    s->requiresweapon = bounded(p, "requiresweapon", sizeof s->requiresweapon); s->reqlevel = bounded(p, "reqlevel", sizeof s->reqlevel); s->maxlvl = bounded(p, "maxlvl", sizeof s->maxlvl); s->missile = bounded(p, "missile", sizeof s->missile);
    s->descmissile = bounded(p, "descmissile", sizeof s->descmissile); flist(p, "params", s->param, 20);
    flist16(p, "sumskills", s->sumskill, 5);
    flist16(p, "variants", s->variants, 4);
    flist16(p, "astats", s->astat, 6); flist16(p, "pstats", s->pstat, 14);
    for (int i = 0; i < DM_ROLE_N; i++) s->role[i] = -1;
    const char *r = fld(p, "roles");
    while (r && *r && *r != '-') {
        char name[16]; int k = 0;
        while (*r && *r != ':' && k < 15) name[k++] = *r++;
        name[k] = 0;
        if (*r != ':') break;
        r++;
        int param = *r == 'P';
        int v = atoi(r + param);
        if ((param && (v < 1 || v > 20)) || (!param && (v < 1 || v > 10))) return -1;
        if (param && !strcmp(name, "rollback")) s->rollback_param = (int8_t)v;
        for (int i = 0; i < DM_ROLE_N && !param; i++) if (!strcmp(name, k_role[i]) && s->role[i] < 0) s->role[i] = (int8_t)(v - 1);
        r = strchr(r, ',');
        if (r) r++;
    }
    for (int i = 0; i < DM_CF_N; i++) s->calc[i] = -1;
    if (p->bad || s->manashift < 0 || s->manashift > 16 || s->hitshift < 0 || s->hitshift > 16) return -1;
    d->nskill++;
    return 0;
}

static int miss_row(parser *p) {
    dm_sim_data *d = p->d;
    void *q = grow(d->misses, d->nmiss, &p->cap_miss, sizeof *d->misses);
    if (!q) return -1;
    d->misses = q;
    dm_sim_miss *m = &d->misses[d->nmiss];
    memset(m, 0, sizeof *m);
    int id = fi(p, "id");
    if (id < 0 || id >= DM_SIM_MAXMISS) { snprintf(d->err, sizeof d->err, "missile id %d", id); return -1; }
    m->typefilter = bounded(p, "typefilter", sizeof m->typefilter);m->hittypes = bounded(p, "sHitPar2", sizeof m->hittypes);m->otherpct = bounded(p, "dParam1", sizeof m->otherpct);
    m->apply_mastery=bounded(p,"ApplyMastery",sizeof m->apply_mastery);
    m->srcmissdamage=bounded(p,"SrcMissDmg",sizeof m->srcmissdamage);
    m->missile_skill=bounded(p,"MissileSkill",sizeof m->missile_skill);
    m->srvdofunc=bounded(p,"pSrvDoFunc",sizeof m->srvdofunc);
    m->initsteps=bounded(p,"InitSteps",sizeof m->initsteps);
    m->accel=bounded(p,"Accel",sizeof m->accel);m->maxvel=bounded(p,"MaxVel",sizeof m->maxvel);
    m->id = (int16_t)id; m->vel = bounded(p, "Vel", sizeof m->vel); m->nexthit = bounded(p, "NextHit", sizeof m->nexthit); m->nextdelay = bounded(p, "NextDelay", sizeof m->nextdelay); m->srcdamage = bounded(p, "SrcDamage", sizeof m->srcdamage);
    m->hitshift = bounded(p, "HitShift", sizeof m->hitshift); m->mindam = bounded(p, "MinDamage", sizeof m->mindam); m->maxdam = bounded(p, "MaxDamage", sizeof m->maxdam); flist(p, "MinLevDam", m->minlev, 5);
    flist(p, "MaxLevDam", m->maxlev, 5); m->etype = bounded(p, "EType", sizeof m->etype); m->emin = bounded(p, "EMin", sizeof m->emin); m->emax = bounded(p, "EMax", sizeof m->emax);
    flist(p, "EMinLev", m->eminlev, 5); flist(p, "EMaxLev", m->emaxlev, 5); m->elen = bounded(p, "ELen", sizeof m->elen); flist(p, "ELevLen", m->elevlen, 3);
    m->skill = bounded(p, "skill", sizeof m->skill); flist16(p, "sub", m->sub, 3); flist16(p, "hitsub", m->hitsub, 4); m->explosion = bounded(p, "explosion", sizeof m->explosion);
    flist(p, "params", m->param, 5);
    for (int i = 0; i < DM_MF_N; i++) m->calc[i] = -1;
    if (p->bad) return -1;
    d->nmiss++;
    return 0;
}

static int mon_row(parser *p, int detail) {
    dm_sim_data *d = p->d;
    int idx = fi(p, "idx");
    if (idx < 0 || idx > 4000) { snprintf(d->err, sizeof d->err, "monster index %d", idx); return -1; }
    if (!detail) {
        int cap = p->cap_mon;
        if (idx >= cap) {
            int c = cap ? cap : 256;
            while (c <= idx) c *= 2;
            void *q = realloc(d->mon, (size_t)c * sizeof *d->mon);
            if (!q) return -1;
            memset((dm_sim_mon *)q + cap, 0, (size_t)(c - cap) * sizeof *d->mon);
            d->mon = q; p->cap_mon = c;
        }
        if (idx >= d->nmon) d->nmon = idx + 1;
        dm_sim_mon *m = &d->mon[idx];
        fs(p, "id", m->id, sizeof m->id); fs(p, "namekey", m->namekey, sizeof m->namekey);
        const char *fl = fld(p, "flags");
        static const char k_flags[] = "bpmrudnskehctx";
        for (; *fl; fl++) { const char *x = strchr(k_flags, *fl); if (x && *fl != '-') m->flags |= 1 << (x - k_flags); }
        m->lvl[0] = bounded(p, "lvl", sizeof m->lvl[0]); m->lvl[1] = bounded(p, "lvlN", sizeof m->lvl[1]); m->lvl[2] = bounded(p, "lvlH", sizeof m->lvl[2]);
        const char *el = fld(p, "el");
        for (int k = 0; k < 3; k++) {
            char *end;errno=0;long type=strtol(el,&end,10);
            if(errno || end==el || *end!=',' || type < -1 || type>6){p->bad=1;break;}
            m->eltype[k]=(int8_t)type;snprintf(m->elmode[k],3,"%.2s",end+1);
            el=strchr(end,';');if(!el){if(k!=2)p->bad=1;break;}el++;
        }
        m->crit = bounded(p, "crit", sizeof m->crit); m->regen = bounded(p, "regen", sizeof m->regen);
        int32_t g[2]; flist(p, "grp", g, 2); m->grpmin = g[0]; m->grpmax = g[1];
        int32_t a[3];
        if (strcmp(fld(p, "a1"), "-")) { flist(p, "a1", a, 3); for (int k = 0; k < 3; k++) m->a1[k] = narrow16(p,a[k]); }
        if (strcmp(fld(p, "a2"), "-")) { flist(p, "a2", a, 3); for (int k = 0; k < 3; k++) m->a2[k] = narrow16(p,a[k]); }
        m->skilldamage = bounded(p, "skilldamage", sizeof m->skilldamage);
        flist16(p, "skills", m->skills, 8);
        return p->bad ? -1 : 0;
    }
    int df = fi(p, "diff");
    if (idx >= d->nmon || df < 0 || df > 2) { snprintf(d->err, sizeof d->err, "mond %d/%d", idx, df); return -1; }
    dm_sim_mond *x = &d->mon[idx].d[df];
    flist(p, "s1", x->s1, 3);
    x->hpmin = bounded(p, "hpmin", sizeof x->hpmin); x->hpmax = bounded(p, "hpmax", sizeof x->hpmax); x->ac = bounded(p, "ac", sizeof x->ac); x->a1min = bounded(p, "a1min", sizeof x->a1min); x->a1max = bounded(p, "a1max", sizeof x->a1max);
    x->a1th = bounded(p, "a1th", sizeof x->a1th); x->a2min = bounded(p, "a2min", sizeof x->a2min); x->a2max = bounded(p, "a2max", sizeof x->a2max); x->a2th = bounded(p, "a2th", sizeof x->a2th); flist(p, "res", x->res, 6);
    x->block = bounded(p, "block", sizeof x->block); x->drain = bounded(p, "drain", sizeof x->drain); x->coldeffect = bounded(p, "coldeffect", sizeof x->coldeffect); x->aidel = bounded(p, "aidel", sizeof x->aidel);
    flist(p, "elpct", x->elpct, 3); flist(p, "elmin", x->elmin, 3); flist(p, "elmax", x->elmax, 3); flist(p, "eldur", x->eldur, 3);
    return p->bad ? -1 : 0;
}

#define PUSH(arr, n, cap) do { void *q_ = grow(d->arr, d->n, &cap, sizeof *d->arr); if (!q_) return -1; d->arr = q_; } while (0)

static int record(parser *p, const char *type) {
    dm_sim_data *d = p->d;
    if (!strcmp(type, "diff")) {
        int i = fi(p, "idx");
        if (i < 0 || i > 2) return -1;
        dm_sim_diffc *x = &d->diff[i];
        x->rp = bounded(p, "ResistPenalty", sizeof x->rp); x->msb = bounded(p, "MonsterSkillBonus", sizeof x->msb); x->freezediv = bounded(p, "MonsterFreezeDivisor", sizeof x->freezediv);
        x->colddiv = bounded(p, "MonsterColdDivisor", sizeof x->colddiv); x->lsd = bounded(p, "LifeStealDivisor", sizeof x->lsd); x->msd = bounded(p, "ManaStealDivisor", sizeof x->msd);
        x->ubonus = bounded(p, "UniqueDamageBonus", sizeof x->ubonus); x->cbonus = bounded(p, "ChampionDamageBonus", sizeof x->cbonus); x->pvspe = bounded(p, "PlayerDamagePercentVSPrimeEvil", sizeof x->pvspe);
        x->mercboss = bounded(p, "MercenaryDamagePercentVSBoss", sizeof x->mercboss); x->pevspet = bounded(p, "PrimeEvilDamagePercentVSPet", sizeof x->pevspet);
        x->pevsmerc = bounded(p, "PrimeEvilDamagePercentVSMercenary", sizeof x->pevsmerc); x->sfmin = bounded(p, "StaticFieldMin", sizeof x->sfmin); x->cedmg = bounded(p, "MonsterCEDamagePercent", sizeof x->cedmg);
    } else if (!strcmp(type, "class")) {
        int i = fi(p, "idx");
        if (i < 0 || i >= 8) return -1;
        dm_sim_class *x = &d->cls[i];
        fs(p, "code", x->code, sizeof x->code); fs(p, "token", x->token, sizeof x->token);
        x->tohitf = bounded(p, "ToHitFactor", sizeof x->tohitf); x->blockf = bounded(p, "BlockFactor", sizeof x->blockf); x->mincast = bounded(p, "MinimumCastingDelay", sizeof x->mincast); x->manaregen = bounded(p, "ManaRegen", sizeof x->manaregen);
        x->lifevit = bounded(p, "LifePerVitality", sizeof x->lifevit); x->manamag = bounded(p, "ManaPerMagic", sizeof x->manamag);
        x->hppot = bounded(p, "HealthPotionPercent", sizeof x->hppot); x->mppot = bounded(p, "ManaPotionPercent", sizeof x->mppot);
        if (i + 1 > d->ncls) d->ncls = i + 1;
    } else if (!strcmp(type, "potion")) {
        static const char *const codes[DM_POT_N] = { "hp1", "hp2", "hp3", "hp4", "hp5", "mp1", "mp2", "mp3", "mp4", "mp5", "rvs", "rvl" };
        const char *code = fld(p, "code"); int k = -1;
        for (int i = 0; i < DM_POT_N; i++) if (!strcmp(code, codes[i])) k = i;
        if (k < 0 || d->potions_loaded >> k & 1) return -1;
        dm_sim_potion *x = &d->potion[k];
        x->kind = bounded(p, "kind", sizeof x->kind); x->life = fi(p, "life"); x->mana = fi(p, "mana"); x->len = fi(p, "len");
        int want = k < DM_POT_MP1 ? DM_PK_HEAL : k < DM_POT_RVS ? DM_PK_MANA : DM_PK_REJUV;
        int ok = x->kind == want && x->life >= 0 && x->mana >= 0 && x->life <= 100000 && x->mana <= 100000 &&
                 (want == DM_PK_REJUV ? x->len == 0 && x->life > 0 && x->life <= 100 && x->mana > 0 && x->mana <= 100 :
                  x->len > 0 && x->len <= 1000 && (want == DM_PK_HEAL ? x->life > 0 && !x->mana : x->mana > 0 && !x->life));
        if (!ok) { snprintf(d->err, sizeof d->err, "potion %s effect", code); return -1; }
        d->potions_loaded |= 1u << k;
    } else if (!strcmp(type, "anim")) {
        PUSH(anim, nanim, p->cap_anim);
        dm_sim_anim *x = &d->anim[d->nanim];
        fs(p, "token", x->token, sizeof x->token); fs(p, "mode", x->mode, sizeof x->mode); fs(p, "wclass", x->wclass, sizeof x->wclass);
        x->frames = bounded(p, "frames", sizeof x->frames); x->speed = bounded(p, "speed", sizeof x->speed); x->action = bounded(p, "action", sizeof x->action);
        d->nanim++;
    } else if (!strcmp(type, "stat")) {
        const char *n = fld(p, "name");
        int id = fi(p, "id");
        if (id < 0 || id >= 512) return -1;
        snprintf(d->stat_name[id], sizeof d->stat_name[id], "%s", n);
        for (int i = 0; i < DM_ST_N; i++) if (!strcmp(n, k_stat_names[i])) d->stat[i] = (int16_t)fi(p, "id");
    } else if (!strcmp(type, "state")) {
        PUSH(state, nstate, p->cap_state);
        dm_sim_state *x = &d->state[d->nstate];
        fs(p, "name", x->name, sizeof x->name); x->id = bounded(p, "id", sizeof x->id); x->sunderfull = bounded(p, "sunderfull", sizeof x->sunderfull); x->sunderreduce = bounded(p, "sunderresreduce", sizeof x->sunderreduce);
        d->nstate++;
    } else if (!strcmp(type, "skill")) {
        if (skill_row(p)) return -1;
    } else if (!strcmp(type, "calc") || !strcmp(type, "mcalc")) {
        int mc = type[0] == 'm';
        int id = fi(p, mc ? "missile" : "skill");
        const char *field = fld(p, "field");
        int prog = dm_sim_calc_compile(d, fld(p, "postfix"));
        if (mc) {
            dm_sim_miss *m = id >= 0 && id < DM_SIM_MAXMISS ? d->miss[id] : NULL;
            for (int i = 0; m && i < DM_MF_N; i++) if (!strcmp(field, k_mf[i])) m->calc[i] = prog;
        } else {
            dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
            for (int i = 0; s && i < DM_CF_N; i++) if (!strcmp(field, k_cf[i])) s->calc[i] = prog;
        }

        if (prog == -2 && strcmp(fld(p, "postfix"), "?")) { snprintf(d->err, sizeof d->err, "bad calc program %d.%s", id, field); return -1; }
    } else if (!strcmp(type, "miss")) {
        if (miss_row(p)) return -1;
    } else if (!strcmp(type, "monlvl")) {
        int L = fi(p, "level");
        static const char *const k[12] = { "hp", "hpN", "hpH", "ac", "acN", "acH", "th", "thN", "thH", "dm", "dmN", "dmH" };
        if (L < 0 || L > 110) return -1;
        for (int i = 0; i < 12; i++) d->monlvl[L][i] = fi(p, k[i]);
    } else if (!strcmp(type, "umod")) {
        const char *names[]={"strong","resist","fire","lightning","cold","stoneskin","ghostly","spectralhit"};
        const char *name=fld(p,"name");int id=fi(p,"id");
        for(int i=0;i<8;i++)if(!strcmp(name,names[i]))d->umod[i]=narrow16(p,id);
    } else if (!strcmp(type, "uconst")) {
        int i = fi(p, "idx");
        if (i >= 0 && i < 64) { d->uconst[i] = fi(p, "value"); if (i + 1 > d->nuconst) d->nuconst = i + 1; }
    } else if (!strcmp(type, "mon")) {
        if (mon_row(p, 0)) return -1;
    } else if (!strcmp(type, "mond")) {
        if (mon_row(p, 1)) return -1;
    } else if (!strcmp(type, "area")) {
        PUSH(area, narea, p->cap_area);
        dm_sim_area *x = &d->area[d->narea];
        x->id = bounded(p, "id", sizeof x->id); x->act = bounded(p, "act", sizeof x->act); fs(p, "namekey", x->namekey, sizeof x->namekey);
        x->mlvl[0] = bounded(p, "mlvl", sizeof x->mlvl[0]); x->mlvl[1] = bounded(p, "mlvlN", sizeof x->mlvl[1]); x->mlvl[2] = bounded(p, "mlvlH", sizeof x->mlvl[2]); x->rotw = bounded(p, "rotw", sizeof x->rotw);
        d->narea++;
    } else if (!strcmp(type, "spawn")) {
        PUSH(spawn, nspawn, p->cap_spawn);
        dm_sim_spawn *x = &d->spawn[d->nspawn];
        int32_t m[25];
        x->area = bounded(p, "area", sizeof x->area); x->list = bounded(p, "list", sizeof x->list); x->n = (int16_t)flist(p, "monsters", m, 25);
        for (int i = 0; i < 25; i++) x->mon[i] = narrow16(p,m[i]);
        d->nspawn++;
    } else if (!strcmp(type, "su")) {
        PUSH(su, nsu, p->cap_su);
        dm_sim_su *x = &d->su[d->nsu];
        int32_t g[3];
        x->idx = bounded(p, "idx", sizeof x->idx); fs(p, "namekey", x->namekey, sizeof x->namekey); x->mon = bounded(p, "mon", sizeof x->mon);
        flist(p, "mods", g, 3); for (int i = 0; i < 3; i++) x->mods[i] = narrow16(p,g[i]);
        flist(p, "grp", g, 2); x->grpmin = narrow16(p,g[0]); x->grpmax = narrow16(p,g[1]);
        x->area = bounded(p, "area", sizeof x->area); x->area_guess = bounded(p, "area_guess", sizeof x->area_guess); x->act = bounded(p, "act", sizeof x->act);
        d->nsu++;
    } else if (!strcmp(type, "boss")) {
        PUSH(boss, nboss, p->cap_boss);
        dm_sim_boss *x = &d->boss[d->nboss];
        static const char *const k[] = { "actboss", "quest", "uber", "ancient", "clone" };
        const char *kind = fld(p, "kind");
        x->mon = bounded(p, "mon", sizeof x->mon); x->act = bounded(p, "act", sizeof x->act); x->area = bounded(p, "area", sizeof x->area); x->kind = -1;
        for (int i = 0; i < 5; i++) if (!strcmp(kind, k[i])) x->kind = (int16_t)i;
        if (x->kind < 0) return -1;
        d->nboss++;
    } else if (!strcmp(type, "std")) {
        int df = fi(p, "diff"), act = fi(p, "act");
        if (df < 0 || df > 2 || act < 0 || act > 5) return -1;
        dm_sim_std *x = &d->std[df][act];
        int32_t v[6];
        x->diff = (int16_t)df; x->act = (int16_t)act; x->n = bounded(p, "n", sizeof x->n); x->mlvl = bounded(p, "mlvl", sizeof x->mlvl); x->hp = bounded(p, "hp", sizeof x->hp); x->ac = bounded(p, "ac", sizeof x->ac);
        x->ar = bounded(p, "ar", sizeof x->ar); x->dmin = bounded(p, "dmin", sizeof x->dmin); x->dmax = bounded(p, "dmax", sizeof x->dmax); x->elavg = bounded(p, "elavg", sizeof x->elavg); x->elshare = bounded(p, "elshare", sizeof x->elshare); x->elpct = bounded(p, "elpct", sizeof x->elpct);
        flist(p, "res", v, 6); for (int i = 0; i < 6; i++) x->res[i] = narrow16(p,v[i]);
        flist(p, "immune", v, 6); for (int i = 0; i < 6; i++) x->immune[i] = narrow16(p,v[i]);
        x->ranged = bounded(p, "ranged", sizeof x->ranged); x->undead = bounded(p, "undead", sizeof x->undead); x->demon = bounded(p, "demon", sizeof x->demon); x->block = bounded(p, "block", sizeof x->block); x->cycle = bounded(p, "cycle", sizeof x->cycle);
        x->regen = bounded(p, "regen", sizeof x->regen); x->drain = bounded(p, "drain", sizeof x->drain);
        x->lmin = bounded(p, "lmin", sizeof x->lmin); x->lmax = bounded(p, "lmax", sizeof x->lmax);
        x->bhp = fi(p, "bhp"); x->bac = fi(p, "bac"); x->bar = fi(p, "bar"); x->bdmin = fi(p, "bdmin"); x->bdmax = fi(p, "bdmax"); x->bel = fi(p, "bel");

        if (x->n > 0 && (x->lmin < 1 || x->lmax < x->lmin || x->lmax > 110 || x->bhp <= 0 || x->bdmax < x->bdmin))
            { snprintf(d->err, sizeof d->err, "std %d/%d level range or values", df, act); return -1; }
        const int32_t base[] = { x->bhp, x->bac, x->bar, x->bdmin, x->bdmax, x->bel };
        for (int i = 0; i < 6; i++) if (base[i] < 0 || base[i] > 100000000) { snprintf(d->err, sizeof d->err, "std %d/%d values", df, act); return -1; }
        d->nstd++;
    } else return -1;
    return p->bad ? -1 : 0;
}

void dm_sim_free(dm_sim_data *d) {
    free(d->anim); free(d->state); free(d->skills); free(d->misses); free(d->mon); free(d->area); free(d->spawn); free(d->su); free(d->boss);
    free(d->code);
    memset(d, 0, sizeof *d);
}

int dm_sim_load(dm_sim_data *d, const char *text, size_t n, const char *set) {
    memset(d, 0, sizeof *d);
    if (!text || !set || n > 16000000 || memchr(text, 0, n)) return -1;
    const char *body = memchr(text, '\n', n), *tail = NULL;
    for (size_t i = 0; i + 5 < n; i++) if (!memcmp(text+i, "\nend\t", 5)) tail=text+i+1;
    char version[80], sha[80], header[256]; unsigned checksum;
    if (!body || body-text >= 255) return -1;
    memcpy(header,text,(size_t)(body-text));header[body-text]=0;
    if (!body || !tail || tail <= body || sscanf(header,"DM_SIM\t%79s\t%79s\t%x",version,sha,&checksum)!=3 ||
        strcmp(version,"4") || strcmp(sha,"c6447f6d43be17a8c3793593d3f9676015b2ab6211efe37face9ab183768d4be")) {
        snprintf(d->err,sizeof d->err,"invalid fingerprint or envelope"); return -1;
    }
    uint32_t crc=~0u;
    for (const char *b=body+1;b<tail;b++) { crc^=(unsigned char)*b; for(int k=0;k<8;k++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u); }
    if ((crc^~0u)!=checksum) { snprintf(d->err,sizeof d->err,"payload checksum mismatch");return -1; }
    for (int i = 0; i < DM_ST_N; i++) d->stat[i] = -1;
    parser *p = calloc(1, sizeof *p);
    char *buf = p ? malloc(n + 1) : NULL;
    if (!buf) { free(p); snprintf(d->err, sizeof d->err, "no memory"); return -1; }
    p->d = d;
    memcpy(buf, text, n); buf[n] = 0;
    int lines = 0, in_set = 0, seen_set = 0, end = -1, head = 0, rc = -1;
    char *save = buf;
    for (char *line = buf; line && *line; line = save) {
        char *nl = strchr(line, '\n');
        save = nl ? nl + 1 : NULL;
        if (nl) *nl = 0;
        lines++;
        p->nf = 0;
        for (char *s = line; p->nf < MAXF;) {
            p->f[p->nf++] = s;
            char *t = strchr(s, '\t');
            if (!t) break;
            *t = 0; s = t + 1;
        }
        const char *type = p->f[0];
        if (lines == 1) {
            if (strcmp(type, "DM_SIM") || p->nf < 3 || atoi(p->f[1]) != 4) { snprintf(d->err, sizeof d->err, "not a sim.tsv of format 4"); goto out; }
            snprintf(d->sha, sizeof d->sha, "%s", p->f[2]); head = 1;
            continue;
        }
        if (!strcmp(type, "end")) { end = p->nf == 2 ? atoi(p->f[1]) : -1; if (save && *save) goto out; break; }
        if (!strcmp(type, "set")) { in_set = p->nf > 1 && !strcmp(p->f[1], set); seen_set |= in_set; p->ncs = 0; continue; }
        if (!in_set) continue;
        if (!strcmp(type, "cols")) {
            if (p->nf < 2 || p->ncs >= 32) { snprintf(d->err, sizeof d->err, "bad cols line %d", lines); goto out; }
            colset *c = &p->cs[p->ncs++];
            snprintf(c->type, sizeof c->type, "%s", p->f[1]);
            c->n = 0;
            for (int i = 2; i < p->nf && c->n < MAXF; i++) snprintf(c->name[c->n++], sizeof c->name[0], "%s", p->f[i]);
            continue;
        }
        p->cur = NULL;
        for (int i = p->ncs - 1; i >= 0; i--) if (!strcmp(p->cs[i].type, type)) { p->cur = &p->cs[i]; break; }
        if (!p->cur) { snprintf(d->err, sizeof d->err, "record %s without cols (line %d)", type, lines); goto out; }
        if(p->nf != p->cur->n+1) { snprintf(d->err,sizeof d->err,"column count at line %d",lines);goto out; }
        int had_skills = d->nskill, had_miss = d->nmiss;
        if (record(p, type)) { if (!d->err[0]) snprintf(d->err, sizeof d->err, "bad %s record (line %d)", type, lines); goto out; }
        if (d->nskill > had_skills) for (int i = 0; i < d->nskill; i++) d->skill[d->skills[i].id] = &d->skills[i];
        if (d->nmiss > had_miss) for (int i = 0; i < d->nmiss; i++) d->miss[d->misses[i].id] = &d->misses[i];
    }
    if (!head || end != lines) { snprintf(d->err, sizeof d->err, "truncated (end %d, lines %d)", end, lines); goto out; }
    if (!seen_set) { snprintf(d->err, sizeof d->err, "no table set %s", set); goto out; }
    if (d->potions_loaded != (1u << DM_POT_N) - 1) { snprintf(d->err, sizeof d->err, "incomplete potion table in set %s", set); goto out; }
    for (int i = 0; i < d->ncls; i++) if (d->cls[i].hppot <= 0 || d->cls[i].mppot <= 0) { snprintf(d->err, sizeof d->err, "class %d potion factors", i); goto out; }
    if (d->ncls < 7 || !d->skill[0] || d->nmon < 100 || d->nstd < 3 || d->nanim < 100) {
        snprintf(d->err, sizeof d->err, "incomplete set %s (classes %d, monsters %d, std %d)", set, d->ncls, d->nmon, d->nstd); goto out;
    }
    int classes[8]={0};
    for(int i=0;i<d->nskill;i++) if(d->skills[i].cls>=0)classes[d->skills[i].cls]++;
    for(int i=0;i<(!strcmp(set,"rotw")?8:7);i++) if(classes[i]!=30) { snprintf(d->err,sizeof d->err,"incomplete skill class %d",i);goto out; }
    for(int i=0;i<3;i++) if(!d->std[i][0].n || d->diff[i].lsd<1 || d->diff[i].msd<1)goto out;
    snprintf(d->set_name,sizeof d->set_name,"%s",set);
    d->ok = 1; rc = 0;
out:
    free(buf); free(p);
    if (rc) { char err[sizeof d->err]; memcpy(err, d->err, sizeof err); dm_sim_free(d); memcpy(d->err, err, sizeof err); }
    return rc;
}

int dm_sim_skill_by_name(const dm_sim_data *d, const char *name) {
    for (int i = 0; i < d->nskill; i++) if (!strcmp(d->skills[i].name, name)) return d->skills[i].id;
    return -1;
}
