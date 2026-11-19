#define _POSIX_C_SOURCE 200809L
#include "dm_gear_search.h"
#include "dm_gear_bound.h"
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>




enum { SV_STR, SV_DEX, SV_VIT, SV_ENE, SV_HP, SV_MANA, SV_IAS, SV_FCR, SV_FHR, SV_FBR, SV_CS, SV_DODGE, SV_DS, SV_CB, SV_OW, SV_LL, SV_ML, SV_DR, SV_MDR,
       SV_REGEN, SV_MREGEN, SV_PREVENT, SV_DEMON, SV_UNDEAD, SV_RATE, SV_SUNDER, SV_ABS = SV_SUNDER + 6, SV_ABSP = SV_ABS + 6, SV_RES = SV_ABSP + 6,
       SV_MAXRES = SV_RES + 6, SV_MAST = SV_MAXRES + 6, SV_PIERCE = SV_MAST + 6, SV_N = SV_PIERCE + 6 };
static const char *const k_sv[SV_SUNDER] = {
    "strength", "dexterity", "vitality", "energy", "maxhp", "maxmana", "item_fasterattackrate", "item_fastercastrate", "item_fastergethitrate",
    "item_fasterblockrate", "passive_critical_strike", "passive_dodge", "item_deadlystrike", "item_crushingblow", "item_openwounds", "lifedrainmindam",
    "manadrainmindam", "normal_damage_reduction", "magic_damage_reduction", "hpregen", "manarecoverybonus", "item_preventheal",
    "item_demondamage_percent", "item_undeaddamage_percent", "attackrate" };
static int stat_id(const dm_sim_data *d, const char *name) {
    if (!name[0]) return -1;
    for (int i = 0; i < DM_GEAR_NSTAT; i++) if (!strcmp(d->stat_name[i], name)) return i;
    return -1;
}
static void sv_names(int k, char out[48]) {
    static const char *sunder[] = { "damage", "fire", "light", "cold", "poison", "magic" };
    static const char *absorb[] = { "", "fire", "light", "cold", "", "magic" };
    static const char *res[] = { "damageresist", "fireresist", "lightresist", "coldresist", "poisonresist", "magicresist" };
    static const char *max[] = { "", "maxfireresist", "maxlightresist", "maxcoldresist", "maxpoisonresist", "" };
    static const char *mast[] = { "", "passive_fire_mastery", "passive_ltng_mastery", "passive_cold_mastery", "passive_pois_mastery", "passive_mag_mastery" };
    static const char *pierce[] = { "", "passive_fire_pierce", "passive_ltng_pierce", "passive_cold_pierce", "passive_pois_pierce", "passive_mag_pierce" };
    out[0] = 0;
    if (k < SV_SUNDER) { snprintf(out, 48, "%s", k_sv[k]); return; }
    int e = (k - SV_SUNDER) % 6, g = (k - SV_SUNDER) / 6;
    switch (g) {
    case 0: snprintf(out, 48, "item_pierce_%s_immunity", sunder[e]); break;
    case 1: if (absorb[e][0]) snprintf(out, 48, "item_absorb%s", absorb[e]); break;
    case 2: if (absorb[e][0]) snprintf(out, 48, "item_absorb%s_percent", absorb[e]); break;
    case 3: snprintf(out, 48, "%s", res[e]); break;
    case 4: snprintf(out, 48, "%s", max[e]); break;
    case 5: snprintf(out, 48, "%s", mast[e]); break;
    case 6: snprintf(out, 48, "%s", pierce[e]); break;
    }
}
static int sv(const dm_gear_problem *p, const dm_sim_char *c, int k) { int id = p->sv[k]; return id >= 0 ? c->stat_value[id] : 0; }
static void sim_fields(const dm_gear_problem *p, dm_sim_char *c) {
    c->str = sv(p, c, SV_STR); c->dex = sv(p, c, SV_DEX); c->vit = sv(p, c, SV_VIT); c->ene = sv(p, c, SV_ENE);
    c->life = sv(p, c, SV_HP); c->life /= 256; c->mana = sv(p, c, SV_MANA); c->mana /= 256;
    c->ias = sv(p, c, SV_IAS); c->fcr = sv(p, c, SV_FCR); c->fhr = sv(p, c, SV_FHR); c->fbr = sv(p, c, SV_FBR);
    c->cs = sv(p, c, SV_CS); c->dodge = sv(p, c, SV_DODGE); c->ds = sv(p, c, SV_DS); c->cb = sv(p, c, SV_CB); c->ow = sv(p, c, SV_OW);
    c->ll = sv(p, c, SV_LL); c->ml = sv(p, c, SV_ML); c->dr_flat = sv(p, c, SV_DR); c->mdr_flat = sv(p, c, SV_MDR);
    c->liferegen = sv(p, c, SV_REGEN); c->manaregen_pct = sv(p, c, SV_MREGEN); c->prevent_heal = sv(p, c, SV_PREVENT);
    c->dmg_demon = sv(p, c, SV_DEMON); c->dmg_undead = sv(p, c, SV_UNDEAD);
    for (int e = 0; e < DM_EL_N; e++) {
        c->sunder[e] = sv(p, c, SV_SUNDER + e) > 0;
        if (e != DM_EL_PHYS && e != DM_EL_POIS) { c->absorb_flat[e] = sv(p, c, SV_ABS + e); c->absorb_pct[e] = sv(p, c, SV_ABSP + e); }
        c->res[e] = sv(p, c, SV_RES + e); c->maxres[e] = 75 + sv(p, c, SV_MAXRES + e);
        c->mastery[e] = sv(p, c, SV_MAST + e); c->pierce[e] = sv(p, c, SV_PIERCE + e);
    }
}

static void sim_combat_fields(dm_sim_char *c, const dm_ic_cb *v, const int *values) {
    c->def = values[IC_C_DEF]; c->block = values[IC_C_BLK]; c->ar = values[IC_C_AR];
    c->ar_base = v->ar_zero ? 0 : v->tohit + 5 * v->dex + v->thf - 35; c->ar_base_known = 1;
    dm_sim_weapon *w = &c->hand[0];
    int hand = v->mode2 ? 1 : 0;
    w->min = v->mn[hand] + v->bmn + (v->armed ? 0 : 1); w->max = v->mx[hand] + v->bmx + (v->armed ? 0 : 2);
    w->strbonus = v->armed ? (v->strb == 1 ? 100 : v->strb) : 100; w->dexbonus = v->dexb == 1 ? 100 : v->dexb;
    c->ed = v->dpct + v->dmast; w->twohand = v->mode2;
    w->edmin = v->edmin; w->edmax = v->edmax; w->postflat = v->flat;
    c->shield = v->can_block;
    static const int ix[] = { -1, 0, 2, 6, 9, 4 };
    for (int e = 1; e < DM_EL_N; e++) { w->elmin[e] = v->el[ix[e]]; w->elmax[e] = v->el[ix[e] + 1]; }
    c->poison_frames = v->plen_ovr > 0 ? v->plen_ovr : v->plen / (v->pcnt > 0 ? v->pcnt : 1);
    w->elmin[DM_EL_POIS] = w->elmin[DM_EL_POIS] * c->poison_frames / 256; w->elmax[DM_EL_POIS] = w->elmax[DM_EL_POIS] * c->poison_frames / 256;
}

static int leaf_items(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_item *it);
static int set2_bonus(const dm_gear_problem *p, const dm_gear_list *l, int id);
static dm_gear_char *set2_body(const dm_gear_problem *p, const dm_gear_pick *pk);

static void level_reads(const dm_sim_data *d, int32_t prog, uint8_t *moves) {
    if (prog < 0) return;
    for (int32_t i = prog; i + 1 < d->ncode && d->code[i] != DM_OP_END; i += 2) {
        if (d->code[i] != DM_OP_XVAR) continue;
        int id = d->code[i + 1] >> 8, var = d->code[i + 1] & 255;
        if (id > 0 && id < DM_SIM_MAXSKILL && var != DM_V_BLVL && var != DM_V_ULVL) moves[id] = 1;
    }
}
static void skill_moves(dm_gear_problem *p) {
    const dm_sim_data *d = p->d;
    memset(p->moves, 0, sizeof p->moves);
    for (int i = 0; i < d->nskill; i++) for (int k = 0; k < DM_CF_N; k++) level_reads(d, d->skills[i].calc[k], p->moves);
    for (int i = 0; i < d->nmiss; i++) for (int k = 0; k < DM_MF_N; k++) level_reads(d, d->misses[i].calc[k], p->moves);
    for (int id = 1; id < DM_SIM_MAXSKILL; id++) {
        const dm_sim_skill *s = d->skill[id];
        if (!s) continue;
        int pstat = 0;
        for (int k = 0; k < 14; k++) pstat |= s->pstat[k] >= 0;
        if (s->passive || pstat || s->aura || (s->fam & (DM_FAM_AOFF | DM_FAM_ADMG | DM_FAM_ADEF | DM_FAM_B | DM_FAM_PA))) p->moves[id] = 1;
    }
}
int dm_gear_problem_ready(dm_gear_problem *p) {
    if (!p || !p->d || !p->d->ok || !p->t || !p->t->ok || p->ncand < 0 || p->ncand > DM_GEAR_MAXCAND || (p->ncand && !p->cand) || p->nweapon < 1 ||
        !p->weapon || p->unarmed < 0 || p->unarmed >= p->nweapon || p->now.cls < 0 || p->now.cls > 7) return -1;
    for (int i = 0; i < p->ncand; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (c->kind <= DM_GEAR_K_NONE || c->kind >= DM_GEAR_K_N || c->w < 1 || c->h < 1 || c->w > DM_GEAR_GRID_W || c->h > DM_GEAR_GRID_H) return -1;
        if (c->weapon < -1 || c->weapon >= p->nweapon || c->item.setitem < -1 || c->item.setitem >= p->t->nitem) return -1;
        if (c->inv && (c->x < 0 || c->y < 0 || c->x + c->w > DM_GEAR_GRID_W || c->y + c->h > DM_GEAR_GRID_H)) return -1;
        if (c->cls_only < 0 || c->cls_only > 8) return -1;
    }
    for (size_t k = 0; k < sizeof p->sv / sizeof *p->sv; k++) { char name[48] = ""; if (k < SV_N) sv_names((int)k, name); p->sv[k] = (int16_t)stat_id(p->d, name); }
    skill_moves(p);
    p->ready = 1;
    dm_gear_pick now; dm_gear_pick_now(p, &now);
    dm_gear_item items[DM_GEAR_MAXITEMS];
    int n = leaf_items(p, &now, items);
    if (n < 0 || dm_gear_compose(p->t, &p->base, items, n, NULL, &p->nowc)) { p->ready = 0; return -1; }
    for (int k = 0, i = 0; k < DM_GEAR_P_N; k++) { p->off_now[k] = -1; if (now.at[k] >= 0) { if (!p->nowc.active[i]) p->off_now[k] = now.at[k]; i++; } }
    p->hands_now[0] = now.at[DM_GEAR_P_MAIN]; p->hands_now[1] = now.at[DM_GEAR_P_OFF];
    p->set2 = 0;
    for (int i = 0; i < DM_SIM_MAXGRANT; i++) {
        const dm_sim_grant *g = &p->now.grant[i];
        p->set2_now[i] = -1;
        if (i >= p->now.ngrant || !g->set2 || g->kind != DM_GR_BUFF || g->skill <= 0 || g->skill >= DM_SIM_MAXSKILL || !p->d->skill[g->skill] || p->node[g->skill])
            continue;
        const dm_gear_char *body = set2_body(p, &now);
        if (!body) { p->ready = 0; return -1; }
        p->set2_now[i] = (int16_t)set2_bonus(p, &body->raw, g->skill); p->set2 = 1;
    }
    return 0;
}

static int dr(const dm_gear_char *now, const dm_gear_char *c, int stat, int layer) {
    return dm_gear_list_get(&c->raw, stat, layer) - dm_gear_list_get(&now->raw, stat, layer);
}
enum { ST_STR = 0, ST_DEX = 2, ST_CLSK = 83, ST_OSKILL = 97, ST_SKILL = 107, ST_TOHITP = 119, ST_ELEMSK = 126, ST_ALLSK = 127, ST_TABSK = 188, ST_ACP = 171,
       ST_ACP2 = 16 };

static int skill_given(const dm_gear_list *l, int id) { return dm_gear_list_get(l, ST_SKILL, id) > 0 || dm_gear_list_get(l, ST_OSKILL, id) > 0; }


static int new_skill(const dm_gear_problem *p, const dm_gear_list *l) {
    for (int k = 0; k < l->n; k++) {
        int id = l->v[k].layer;
        if (l->v[k].stat != ST_SKILL || l->v[k].value <= 0 || id <= 0 || id >= DM_SIM_MAXSKILL || p->node[id] || !p->d->skill[id] || !p->moves[id]) continue;
        if (p->d->skill[id]->cls == p->now.cls && dm_gear_list_get(&p->nowc.raw, ST_SKILL, id) <= 0) return 1;
    }
    return 0;
}

static int set2_bonus(const dm_gear_problem *p, const dm_gear_list *l, int id) {
    int v = dm_gear_list_get(l, ST_OSKILL, id) + dm_gear_list_get(l, ST_ALLSK, 0);
    if (p->d->skill[id]->cls == p->now.cls) {
        v += dm_gear_list_get(l, ST_CLSK, p->now.cls) + dm_gear_list_get(l, ST_SKILL, id);
        if (p->tab[id] >= 0) v += dm_gear_list_get(l, ST_TABSK, p->tab[id]);
    }
    return v;
}
static void char_from(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_char *comp, const dm_gear_char *body, int main, int off,
                      int kick_min, int kick_max, dm_sim_char *a, unsigned *flags, int *cbv);
void dm_gear_char_from(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_char *comp, const dm_gear_char *body, int main, int off,
                       int kick_min, int kick_max, dm_sim_char *a, unsigned *flags) {
    char_from(p, now, comp, body, main, off, kick_min, kick_max, a, flags, NULL);
}

static void char_from(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_char *comp, const dm_gear_char *body, int main, int off,
                      int kick_min, int kick_max, dm_sim_char *a, unsigned *flags, int *cbv) {
    const dm_sim_data *d = p->d;
    *flags = 0;
    *a = p->now;
    for (int i = 0; i < DM_GEAR_NSTAT; i++) if (d->stat_name[i][0]) a->stat_value[i] += comp->value[i] - now->value[i];
    sim_fields(p, a);



    int all = dr(now, comp, ST_ALLSK, 0), cls = dr(now, comp, ST_CLSK, p->now.cls);
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) {
        if (!p->node[id] || !d->skill[id]) continue;
        int lv = p->now.lvl[id] + all + (d->skill[id]->cls == p->now.cls ? cls : 0) + dr(now, comp, ST_SKILL, id);
        if (p->tab[id] >= 0) lv += dr(now, comp, ST_TABSK, p->tab[id]);
        if (p->elem[id] >= 0) lv += dr(now, comp, ST_ELEMSK, p->elem[id]);
        if (!p->now.blvl[id] && d->skill[id]->cls == p->now.cls && skill_given(&now->raw, id) && !skill_given(&comp->raw, id)) lv = 0;
        a->lvl[id] = (int16_t)(lv < 0 ? 0 : lv > 256 ? 256 : lv);
    }


    for (int i = 0; p->set2 && i < a->ngrant; i++) {
        if (p->set2_now[i] < 0) continue;
        int lv = p->now.grant[i].lvl + set2_bonus(p, &(body ? body : comp)->raw, a->grant[i].skill) - p->set2_now[i];
        a->grant[i].lvl = (int16_t)(lv < 1 ? 1 : lv > 256 ? 256 : lv);
    }
    if (main >= 0 && p->cand[main].weapon < 0) { *flags |= DM_GEAR_L_INPUT; return; }
    if (off >= 0 && p->cand[off].weapon >= 0) { *flags |= DM_GEAR_L_INPUT; return; }
    const dm_gear_weapon *wf = &p->weapon[main >= 0 ? p->cand[main].weapon : p->unarmed];
    a->nhands = 1; a->hand[0] = wf->w; a->mastery_cs = wf->mcrit;

    dm_ic_cb v = p->cb;
#define G(f, st) v.f = p->cb.f + comp->value[st] - now->value[st]
    G(str, 0); G(dex, 2); G(ac, 31); G(acovr, 182); G(toblock, 20); G(tohit, 19); G(mn[0], 21); G(mx[0], 22); G(mn[1], 23); G(mx[1], 24);
    G(bmn, 185); G(bmx, 186); G(dpct, 25); G(edmin, 18); G(edmax, 17); G(plen_ovr, 101); G(plen, 59); G(pcnt, 326);
    for (int i = 0; i < 11; i++) G(el[i], 48 + i);
    for (int i = 0; i < 4; i++) G(mast[i], 329 + i);
    if (p->prog[0]) G(ar_prog, 325);
#undef G
    v.acpct = p->cb.acpct + dr(now, comp, ST_ACP, 0) + dr(now, comp, ST_ACP2, 0);
    v.tohitpct = p->cb.tohitpct + dr(now, comp, ST_TOHITP, 0);
    v.armed = main >= 0; v.mode2 = wf->mode2; v.strb = wf->strb; v.dexb = wf->dexb; v.ar_zero = wf->ar_zero; v.flat = wf->flat;
    v.ar_mast = wf->mth[0] == DM_SIM_NO_NODE ? 0 : wf->mth[0]; v.dmast = wf->mdmg[0] == DM_SIM_NO_NODE ? 0 : wf->mdmg[0];
    v.can_block = off >= 0 && p->cand[off].block;

    if (d->skill[0]) v.ar_skill += (a->lvl[0] - p->now.lvl[0]) * d->skill[0]->levtohit;
    int st325 = d->stat_name[325][0] ? a->stat_value[325] : 0;
    int mth[DM_SIM_MAXSKILL], mdmg[DM_SIM_MAXSKILL];
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) {
        mth[id] = p->node[id] ? wf->mth[id] : DM_SIM_NO_NODE; mdmg[id] = p->node[id] ? wf->mdmg[id] : DM_SIM_NO_NODE;
        if (!p->node[id] || mth[id] == DM_SIM_NO_NODE) continue;
        a->ar_bonus[id] = v.tohitpct + mth[id] + (p->prog[id] ? st325 : 0);
        a->damage_mastery_delta[id] = mdmg[id] - v.dmast;
    }
    dm_sim_passive_shift sh;
    if (dm_sim_passive_levels(d, &p->now, a, mth, mdmg, wf->mcrit, &sh)) *flags |= DM_GEAR_L_PASSIVE;
    else { sim_fields(p, a); v.tohitpct += sh.tohit_pct; v.acpct += sh.armor_pct; v.ar_mast += sh.ar_mastery; v.dmast += sh.damage_mastery; }
    int values[IC_C_N + 1];
    if (!dm_ic_cb_eval(&v, values)) { *flags |= DM_GEAR_L_COMBAT; a->capture_bad |= 3; }
    else { sim_combat_fields(a, &v, values); if (cbv) memcpy(cbv, values, sizeof values); }
    a->shield_min = a->shield_max = 0;
    if (a->shield && off >= 0) {
        a->shield_min = p->cand[off].dmin; a->shield_max = p->cand[off].dmax;
        if (a->active_lvl[DM_SK_SHIELD_BUFF] > 0 && d->skill[DM_SK_SHIELD_BUFF]) {
            int bad = 0;
            a->shield_min += (int)dm_sim_skill_value(d, a, DM_SK_SHIELD_BUFF, a->lvl[DM_SK_SHIELD_BUFF], DM_SV_PMIN, &bad);
            a->shield_max += (int)dm_sim_skill_value(d, a, DM_SK_SHIELD_BUFF, a->lvl[DM_SK_SHIELD_BUFF], DM_SV_PMAX, &bad);
            if (bad) *flags |= DM_GEAR_L_COMBAT;
        }
    }
    a->kick_min = kick_min; a->kick_max = kick_max;
    a->sias = sv(p, a, SV_RATE) - 100 + a->hand[0].wsm;

    a->roll_inputs_bad = 1; a->endpoint_bad = 1;
}


static int ammo_fits(const dm_gear_problem *p, int main, int off) {
    int k = p->cand[main].kind, o = off >= 0 ? p->cand[off].kind : -1;
    return k == DM_GEAR_K_BOW ? o == DM_GEAR_K_ARROWS : k == DM_GEAR_K_XBOW ? o == DM_GEAR_K_BOLTS : 1;
}
static const int k_slot[DM_GEAR_P_N] = { 4, 5, 3, 2, 6, 7, 1, 8, 9, 10 };

static int leaf_items(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_item *it) {
    int n = 0, charms = -1;
    for (int k = 0; k < DM_GEAR_P_N; k++) {
        int c = pk->at[k];
        if (c < -1 || c >= p->ncand) return -1;
        if (c >= 0) { it[n] = p->cand[c].item; it[n].slot = k_slot[k]; n++; }
    }
    for (int i = 0; i < p->ncand; i++) {
        if (!pk->charm[i]) continue;
        if (charms < 0) {
            if (n == DM_GEAR_MAXITEMS) return -1;
            charms = n++; memset(&it[charms], 0, sizeof it[charms]); it[charms].setitem = -1;
        }
        const dm_gear_list *l = &p->cand[i].item.own;
        for (int k = 0; k < l->n; k++) dm_gear_list_add(&it[charms].own, l->v[k].stat, l->v[k].layer, l->v[k].value);
        if (l->full) it[charms].own.full = 1;
    }
    return n;
}

static dm_gear_char *set2_body(const dm_gear_problem *p, const dm_gear_pick *pk) {
    static _Thread_local dm_gear_char body;
    static _Thread_local dm_gear_pick x;
    dm_gear_item items[DM_GEAR_MAXITEMS];
    x = *pk; x.at[DM_GEAR_P_MAIN] = x.at[DM_GEAR_P_OFF] = -1;
    int n = leaf_items(p, &x, items);
    return n < 0 || dm_gear_compose(p->t, &p->base, items, n, NULL, &body) ? NULL : &body;
}
int dm_gear_leaf_char(const dm_gear_problem *p, const dm_gear_pick *pk, dm_sim_char *out, dm_gear_char *comp, unsigned *flags) {
    return dm_gear_leaf_char_cb(p, pk, out, comp, flags, NULL);
}
int dm_gear_pick_compose(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_char *comp, uint8_t active[DM_GEAR_P_N + 1]) {
    if (!p || !p->ready || !pk || !comp) return -1;
    dm_gear_item items[DM_GEAR_MAXITEMS];
    int n = leaf_items(p, pk, items);
    if (n < 0 || dm_gear_compose(p->t, &p->base, items, n, NULL, comp)) return -1;
    int i = 0;
    for (int k = 0; k < DM_GEAR_P_N; k++) active[k] = pk->at[k] < 0 ? 1 : comp->active[i++];
    active[DM_GEAR_P_N] = i < n ? comp->active[i] : 1;
    return 0;
}
int dm_gear_leaf_char_cb(const dm_gear_problem *p, const dm_gear_pick *pk, dm_sim_char *out, dm_gear_char *comp, unsigned *flags, int *cbv) {
    *flags = DM_GEAR_L_INPUT;
    if (!p || !p->ready || !pk || !out) return -1;
    dm_gear_item items[DM_GEAR_MAXITEMS];
    static _Thread_local dm_gear_char own;
    dm_gear_char *c = comp ? comp : &own, *body = NULL;
    int n = leaf_items(p, pk, items);
    if (n < 0 || dm_gear_compose(p->t, &p->base, items, n, NULL, c)) return -1;
    if (p->set2 && (body = set2_body(p, pk)) == NULL) return -1;
    int kick_min = 0, kick_max = 0, boots = pk->at[DM_GEAR_P_BOOTS];
    if (boots >= 0) { kick_min = p->cand[boots].dmin; kick_max = p->cand[boots].dmax; }
    char_from(p, &p->nowc, c, body, pk->at[DM_GEAR_P_MAIN], pk->at[DM_GEAR_P_OFF], kick_min, kick_max, out, flags, cbv);
    if (c->flags & (DM_GEAR_C_ORDER | DM_GEAR_C_UNMAPPED | DM_GEAR_C_FUNC1 | DM_GEAR_C_PCT | DM_GEAR_C_ITEMOP | DM_GEAR_C_FULL)) *flags |= DM_GEAR_L_COMPOSE;
    if (new_skill(p, &c->raw)) *flags |= DM_GEAR_L_SKILL;



    for (int k = 0, i = 0; k < DM_GEAR_P_N; k++) {
        int x = pk->at[k]; if (x < 0) continue;
        if (p->cand[x].cls_only && p->cand[x].cls_only - 1 != p->now.cls && !p->cand[x].worn) *flags |= DM_GEAR_L_REQ;
        if (!c->active[i++]) { int worn_off = 0; for (int j = 0; j < DM_GEAR_P_N; j++) worn_off |= p->off_now[j] == x; if (!worn_off) *flags |= DM_GEAR_L_REQ; }
    }


    int main = pk->at[DM_GEAR_P_MAIN], off = pk->at[DM_GEAR_P_OFF];
    if (main >= 0 && !ammo_fits(p, main, off) && !(main == p->hands_now[0] && off == p->hands_now[1])) *flags |= DM_GEAR_L_AMMO;
    return *flags ? -1 : 0;
}
int dm_gear_leaf_measure(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_measures *m, unsigned *flags) {
    static _Thread_local dm_sim_char c;
    if (dm_gear_leaf_char(p, pk, &c, NULL, flags)) return -1;
    if (dm_gear_measure(p->d, &c, p->splash_on, m)) { *flags |= DM_GEAR_L_MEASURE; return -1; }
    return 0;
}
void dm_gear_pick_now(const dm_gear_problem *p, dm_gear_pick *pk) {
    memset(pk, 0, sizeof *pk);
    for (int k = 0; k < DM_GEAR_P_N; k++) pk->at[k] = -1;
    for (int i = 0; i < p->ncand; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (c->kind == DM_GEAR_K_CHARM) { pk->charm[i] = c->inv && !(c->unrated & DM_GEAR_U_LEVEL); continue; }
        if (!c->worn) continue;
        int at = -1;
        switch (c->kind) {
        case DM_GEAR_K_HEAD: at = DM_GEAR_P_HEAD; break;
        case DM_GEAR_K_AMULET: at = DM_GEAR_P_AMULET; break;
        case DM_GEAR_K_ARMOR: at = DM_GEAR_P_ARMOR; break;
        case DM_GEAR_K_RING: at = pk->at[DM_GEAR_P_RING1] < 0 ? DM_GEAR_P_RING1 : DM_GEAR_P_RING2; break;
        case DM_GEAR_K_BELT: at = DM_GEAR_P_BELT; break;
        case DM_GEAR_K_BOOTS: at = DM_GEAR_P_BOOTS; break;
        case DM_GEAR_K_GLOVES: at = DM_GEAR_P_GLOVES; break;
        case DM_GEAR_K_SHIELD: case DM_GEAR_K_ARROWS: case DM_GEAR_K_BOLTS: at = DM_GEAR_P_OFF; break;
        default: at = pk->at[DM_GEAR_P_MAIN] < 0 ? DM_GEAR_P_MAIN : DM_GEAR_P_OFF; break;
        }
        pk->at[at] = (int16_t)i;
    }
}
int dm_gear_key_cmp(const double a[3], const double b[3]) {
    for (int k = 0; k < 3; k++) { if (a[k] > b[k]) return 1; if (a[k] < b[k]) return -1; }
    return 0;
}


typedef struct { uint8_t cell[DM_GEAR_GRID_H][DM_GEAR_GRID_W]; int n, w[DM_GEAR_MAXCAND], h[DM_GEAR_MAXCAND]; } place_ctx;
static int fits(const place_ctx *g, int x, int y, int w, int h) {
    if (x + w > DM_GEAR_GRID_W || y + h > DM_GEAR_GRID_H) return 0;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) if (g->cell[y + j][x + i]) return 0;
    return 1;
}
static void mark(place_ctx *g, int x, int y, int w, int h, int v) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) g->cell[y + j][x + i] = (uint8_t)v; }

static int place_from(place_ctx *g, int k, int from) {
    if (k == g->n) return 1;
    int start = k && g->w[k] == g->w[k - 1] && g->h[k] == g->h[k - 1] ? from : 0;
    for (int pos = start; pos < DM_GEAR_GRID_W * DM_GEAR_GRID_H; pos++) {
        int x = pos % DM_GEAR_GRID_W, y = pos / DM_GEAR_GRID_W;
        if (!fits(g, x, y, g->w[k], g->h[k])) continue;
        mark(g, x, y, g->w[k], g->h[k], 1);
        int ok = place_from(g, k + 1, pos + 1);
        mark(g, x, y, g->w[k], g->h[k], 0);
        if (ok) return 1;
    }
    return 0;
}
int dm_gear_place(const dm_gear_problem *p, const uint8_t *take) {
    place_ctx g; memcpy(g.cell, p->grid, sizeof g.cell); g.n = 0;
    int area = 0, free_cells = 0;
    for (int i = 0; i < p->ncand; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (!take[i] || !c->inv) continue;
        if (!fits(&g, c->x, c->y, c->w, c->h)) return 0;
        mark(&g, c->x, c->y, c->w, c->h, 1);
    }
    for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) free_cells += !g.cell[y][x];
    for (int i = 0; i < p->ncand; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (!take[i] || c->inv) continue;

        int k = g.n++;
        while (k > 0 && g.w[k - 1] * g.h[k - 1] < c->w * c->h) { g.w[k] = g.w[k - 1]; g.h[k] = g.h[k - 1]; k--; }
        g.w[k] = c->w; g.h[k] = c->h; area += c->w * c->h;
    }

    for (int i = 1; i < g.n; i++) for (int j = i; j > 0 && g.w[j - 1] * g.h[j - 1] == g.w[j] * g.h[j] && g.w[j - 1] < g.w[j]; j--) {
        int tw = g.w[j], th = g.h[j]; g.w[j] = g.w[j - 1]; g.h[j] = g.h[j - 1]; g.w[j - 1] = tw; g.h[j - 1] = th; }
    return area <= free_cells && place_from(&g, 0, 0);
}


enum { LV_HANDS, LV_ARMOR, LV_AMULET, LV_RINGS, LV_HEAD, LV_BELT, LV_BOOTS, LV_GLOVES, LV_CHARMS };
typedef struct { int16_t a, b; } choice;
typedef struct {
    int place;
    int group;
    int n; choice *c;


    dm_gear_list hi, lo;
    int unk;
    int nset; int16_t *set;
    int kick_min, kick_max;
} level;
typedef struct { int n, w, h; int16_t *member; } charm_group;

typedef struct { int a, n; level rest; } ring_group;
typedef struct { uint64_t h1, h2; dm_gear_measures m; int state; } centry;
#define CACHE_BITS 16
#define MAXDEPTH (LV_CHARMS + DM_GEAR_MAXCAND + 1)
typedef struct { int i; double key[3]; dm_gear_bound_out b; int ok; } child;

enum { FROM_START, FROM_SEED, FROM_TREE };





typedef struct task { struct task *next; int top, j; child c; } task;
typedef struct {
    pthread_mutex_t mu;
    int have, from; double best[3];
    int *assign;
    child *top; int ntop;
    atomic_int next, taken;
    atomic_long ppm;
    long before;



    pthread_mutex_t tmu; task *tstack; int splitting; atomic_int *left;
} shared;
struct dm_gear_search {
    dm_gear_problem p;
    uint8_t sc[DM_GEAR_NSTAT];
    int nlev; level *lev;
    int ng; charm_group *grp;
    int nrg; ring_group *rg; int16_t *rg_of;
    int pool;
    uint8_t *sure;
    uint8_t *keep;
    dm_gear_list keep_hi, keep_lo; int keep_unk;
    int now_ok; dm_gear_measures now; dm_gear_pick nowpick;
    int seed_on; dm_gear_pick seed;
    centry *cache;
    int unrated, unrated_worn, dual_pairs, filtered; double raw;

    const dm_gear_goal *goal; const atomic_int *stop; dm_gear_progress *prog; dm_gear_result *res;
    int stopped;
    shared *sh;
    int from, helper, was_wide; long cur_ppm, task_end;



    int threads; const atomic_int *wide; void (*pace)(int wide);
#ifndef DM_PLAYER_BUILD
    long test_run_us;
#endif
    centry *wcache[DM_GEAR_MAXTHREADS];
    int assign[MAXDEPTH];
    int at[MAXDEPTH], of[MAXDEPTH];
    dm_gear_bound_out anc[MAXDEPTH]; int nanc;
    long tripwire, unrated_leaves, skipped;
#ifdef DM_GEAR_INSTR
    int wslot;
    long w_leaves;
#endif
};

static uint64_t mix(uint64_t h) { h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33; return h; }
static void char_hash(const dm_sim_char *c, uint64_t *h1, uint64_t *h2) {
    const unsigned char *b = (const unsigned char *)c; size_t n = sizeof *c;
    uint64_t a = 1469598103934665603ULL, z = 0x9e3779b97f4a7c15ULL;
    for (size_t i = 0; i + 8 <= n; i += 8) { uint64_t w; memcpy(&w, b + i, 8); a = (a ^ w) * 1099511628211ULL; z = mix(z + w); }
    for (size_t i = n & ~(size_t)7; i < n; i++) { a = (a ^ b[i]) * 1099511628211ULL; z = mix(z + b[i]); }
    *h1 = a; *h2 = z;
}

static int stopped(dm_gear_search *s) {
    if (s->wide) {
        int w = atomic_load_explicit(s->wide, memory_order_relaxed) != 0;
        if (w != s->was_wide) { s->was_wide = w; if (s->pace) s->pace(w); }
    }
    if (!s->stopped && s->stop && atomic_load_explicit(s->stop, memory_order_relaxed)) s->stopped = 1;
    return s->stopped;
}
static void inc(atomic_long *x) { atomic_fetch_add_explicit(x, 1, memory_order_relaxed); }

#ifdef DM_GEAR_INSTR




#include <time.h>
enum { T_COMPOSE, T_MLOOK, T_MEASURE, T_PLACE, T_SCORE, T_BOUND, T_BOUNDEVAL, T_LEAFN, T_N };
static atomic_long g_leaf_ns[T_N];
static atomic_long g_leaf_n[T_N];
static void t_add(int k, long ns) { atomic_fetch_add_explicit(&g_leaf_ns[k], ns, memory_order_relaxed); atomic_fetch_add_explicit(&g_leaf_n[k], 1, memory_order_relaxed); }
static double t_now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)(t.tv_sec * 1000000000LL + t.tv_nsec); }

static _Thread_local int g_leaf_fresh;



static _Thread_local dm_sim_char g_mask_ch;
static void mask_hash(const uint8_t sc[DM_GEAR_NSTAT], const dm_sim_char *c, uint64_t *h1, uint64_t *h2) {
    g_mask_ch = *c;
    for (int i = 0; i < DM_GEAR_NSTAT; i++) if (sc[i] == DM_GEAR_SC_IRR) g_mask_ch.stat_value[i] = 0;
    char_hash(&g_mask_ch, h1, h2);
}


static _Thread_local int g_tail_assign[MAXDEPTH];
static _Thread_local int g_tail_have;
static _Thread_local int g_tail_full_n;
static atomic_long g_tail_full_dup, g_tail_mask_dup, g_tail_leaves, g_tail_new;

static atomic_long g_es_fresh, g_es_k0_less, g_es_stopped, g_es_lost_end, g_es_must_run;
static atomic_long g_es_frames_saved, g_es_frame_sum;


static _Thread_local uint64_t g_memo_sim[1u << CACHE_BITS][2];
static _Thread_local uint64_t g_memo_sim8[1u << 13][2];
static atomic_long g_memo_sim_hit, g_memo_sim_hit8, g_memo_sim_probe;

#define WREC_MAX 8192
typedef struct { long dur_ns; long leaves; int top, child, j; } wrec;
typedef struct {
    long enter_ns, exit_ns;
    atomic_long busy_ns;
    wrec *rec; int nrec, ovf;
    int ntake;
} wstat;
static wstat g_w[DM_GEAR_MAXTHREADS];
static void wrec_add(wrec **tab, int cap, int *n, int *ovf, long dur_ns, long leaves, int top, int child, int j) {
    if (*n >= cap) { (*ovf)++; return; }
    wrec *r = &(*tab)[(*n)++];
    r->dur_ns = dur_ns; r->leaves = leaves; r->top = top; r->child = child; r->j = j;
}



static atomic_long g_lk_fresh, g_lk_bcap, g_lk_bothcap, g_lk_bc_less, g_lk_bc_tie, g_lk_bc_more;
static atomic_long g_lk_skip, g_lk_nodmg;
static void lazy_key_note(dm_gear_search *s, const double k[3], const double b[3]) {
    if (!s->goal) return;
    double cap = dm_gear_hits_key(s->goal->survival_hits, s->goal->survival_hits);
    atomic_fetch_add_explicit(&g_lk_fresh, 1, memory_order_relaxed);
    if (!(s->now.group > 0 || s->now.single > 0)) { atomic_fetch_add_explicit(&g_lk_nodmg, 1, memory_order_relaxed); return; }
    if (b[0] != cap) return;
    atomic_fetch_add_explicit(&g_lk_bcap, 1, memory_order_relaxed);
    int c = k[1] < b[1] ? -1 : k[1] > b[1] ? 1 : 0;
    if (k[0] == cap) {
        atomic_fetch_add_explicit(&g_lk_bothcap, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(c < 0 ? &g_lk_bc_less : c > 0 ? &g_lk_bc_more : &g_lk_bc_tie, 1, memory_order_relaxed);
    }
    if (c < 0) atomic_fetch_add_explicit(&g_lk_skip, 1, memory_order_relaxed);
}
void dm_gear_instr_p6_reset(void) {
    for (int k = 0; k < T_N; k++) { atomic_store(&g_leaf_ns[k], 0); atomic_store(&g_leaf_n[k], 0); }
    atomic_store(&g_tail_full_dup, 0); atomic_store(&g_tail_mask_dup, 0); atomic_store(&g_tail_leaves, 0); atomic_store(&g_tail_new, 0);
    atomic_store(&g_es_fresh, 0); atomic_store(&g_es_k0_less, 0); atomic_store(&g_es_stopped, 0);
    atomic_store(&g_es_lost_end, 0); atomic_store(&g_es_must_run, 0);
    atomic_store(&g_es_frames_saved, 0); atomic_store(&g_es_frame_sum, 0);
    atomic_store(&g_memo_sim_hit, 0); atomic_store(&g_memo_sim_hit8, 0); atomic_store(&g_memo_sim_probe, 0);
    for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) {
        g_w[i].enter_ns = g_w[i].exit_ns = 0; atomic_store(&g_w[i].busy_ns, 0);
        g_w[i].nrec = g_w[i].ovf = g_w[i].ntake = 0;
    }
    atomic_store(&g_lk_fresh, 0); atomic_store(&g_lk_bcap, 0); atomic_store(&g_lk_bothcap, 0);
    atomic_store(&g_lk_bc_less, 0); atomic_store(&g_lk_bc_tie, 0); atomic_store(&g_lk_bc_more, 0);
    atomic_store(&g_lk_skip, 0); atomic_store(&g_lk_nodmg, 0);
    g_tail_have = 0; g_tail_full_n = 0; g_leaf_fresh = 0;
    dm_gear_instr_times_reset();
    dm_sim_instr_p6_reset();
}

static int tail_first(const dm_gear_search *s) {
    for (int l = 0; l < s->nlev; l++)
        if (s->lev[l].place == DM_GEAR_P_BELT || s->lev[l].place == DM_GEAR_P_BOOTS || s->lev[l].place == DM_GEAR_P_GLOVES) return l;
    for (int l = 0; l < s->nlev; l++) if (s->lev[l].place == -3) return l;
    return s->nlev;
}

static void tail_check(dm_gear_search *s) {
    int tf = tail_first(s);
    if (g_tail_have && !memcmp(g_tail_assign, s->assign, sizeof *g_tail_assign * (size_t)tf)) return;
    memcpy(g_tail_assign, s->assign, sizeof *g_tail_assign * (size_t)tf);
    g_tail_have = 1; g_tail_full_n = 0;
    dm_sim_instr_tail_new();
    atomic_fetch_add_explicit(&g_tail_new, 1, memory_order_relaxed);
}

static void early_stop_note(dm_gear_search *s, const dm_gear_measures *m, const double okey[3], const double best0[3]) {
    atomic_fetch_add_explicit(&g_es_fresh, 1, memory_order_relaxed);
    if (!s->goal || s->now.group <= 0 || s->now.single <= 0) { atomic_fetch_add_explicit(&g_es_must_run, 1, memory_order_relaxed); return; }
    if (okey[0] < best0[0]) { atomic_fetch_add_explicit(&g_es_k0_less, 1, memory_order_relaxed); return; }
    if (okey[0] > best0[0]) { atomic_fetch_add_explicit(&g_es_must_run, 1, memory_order_relaxed); return; }
    if (okey[1] > best0[1]) { atomic_fetch_add_explicit(&g_es_must_run, 1, memory_order_relaxed); return; }

    const dm_gear_instr_cp *cps; int ncp;
    dm_sim_instr_dealt_cps(&cps, &ncp);
    int kill, dframe; long iters;
    dm_sim_instr_dealt_last(&kill, &dframe, &iters);
    double b = s->goal->boss_share;
    double gch = 100 * (m->group - s->now.group) / s->now.group;
    double boss = dm_gear_boss_life(s->p.d, s->p.now.diff, s->p.now.act);
    int stop_at = -1;
    if (boss > 0) for (int k = 0; k < ncp; k++) {
        double fc = cps[k].frame > 0 ? cps[k].frame : 1;
        double by_time = cps[k].removed + (cps[k].bound > 0 ? cps[k].bound : 0);
        double sup = fmax(boss * 25 / fc, by_time * 25 / DM_GEAR_SINGLE_FRAMES);
        double sch = 100 * (sup - s->now.single) / s->now.single;
        double k1 = dm_gear_steps((1 - b) * gch + b * sch);
        if (k1 < best0[1]) { stop_at = cps[k].frame; break; }
    }
    if (stop_at >= 0) {
        atomic_fetch_add_explicit(&g_es_stopped, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_es_frame_sum, stop_at, memory_order_relaxed);
        long saved = (kill >= 0 ? kill : dframe) - stop_at;
        if (saved > 0) atomic_fetch_add_explicit(&g_es_frames_saved, saved, memory_order_relaxed);
    } else if (okey[1] < best0[1] || okey[0] < best0[0]) atomic_fetch_add_explicit(&g_es_lost_end, 1, memory_order_relaxed);
    else atomic_fetch_add_explicit(&g_es_must_run, 1, memory_order_relaxed);
}
void dm_gear_instr_p6_report(FILE *f, double total_s, int threads) {
    fputs("\nleaf time split (all threads, seconds; of the fresh leaves' measure the parts are split further below):\n", f);
    static const char *const names[T_N] = { "compose + stat calc", "identity hash + cache", "dm_gear_measure (all)", "charm place",
                                            "score", "node_bound total", "bound eval (inside node_bound)", "fresh rated leaves" };
    for (int k = 0; k < T_N; k++)
        fprintf(f, "  %-34s %9.2f s  %10ld calls\n", names[k], atomic_load(&g_leaf_ns[k]) / 1e9, atomic_load(&g_leaf_n[k]));
    double t_surv, t_group, t_dealt, t_target;
    dm_gear_instr_times(&t_target, &t_surv, &t_group, &t_dealt);
    fprintf(f, "  %-34s %9.2f s\n  %-34s %9.2f s\n  %-34s %9.2f s\n  %-34s %9.2f s\n",
            "part: standard target lookup", t_target, "part: survival fight (run + def)", t_surv, "part: group measure", t_group,
            "part: single target fight", t_dealt);
    long runs[3], iters[3], frames[3];
    dm_sim_instr_fights(runs, iters, frames);
    fputs("\nfight runs and loop work (runs, loop iterations, simulated frames) and runs per second per thread:\n", f);
    static const char *const fk[3] = { "group skill preparations", "survival fight", "single target fight" };
    for (int k = 0; k < 3; k++)
        fprintf(f, "  %-24s runs %9ld  iterations %11ld  frames %12ld  %8.1f runs/s/thread\n", fk[k], runs[k], iters[k], frames[k],
                runs[k] / (total_s * (threads > 0 ? threads : 1)));
    long fresh[3], dw[3], dt[3], ovf;
    dm_sim_instr_inputs(fresh, dw, dt, &ovf);
    fputs("\nidentical fight inputs (identity = whole leaf character, stats no measure reads masked out):\n"
          "  kind                 distinct  repeated in walk  repeated in one tail\n", f);
    for (int k = 0; k < 3; k++)
        fprintf(f, "  %-20s %9ld %16ld %20ld\n", fk[k], fresh[k], dw[k], dt[k]);
    fprintf(f, "  same character (no masking): repeated in one tail %10ld (in walk: runs minus fresh, printed above)\n", atomic_load(&g_tail_full_dup));
    fprintf(f, "  masked identity repeated in one tail, character new: %10ld; fresh leaves %10ld in %8ld tails (input table overflow drops %8ld)\n",
            atomic_load(&g_tail_mask_dup), atomic_load(&g_tail_leaves), atomic_load(&g_tail_new), ovf);
    fprintf(f, "\nsimulated masked-identity measure cache: %8ld hits in %9ld leaves the character cache missed (that size %d), %8ld at 8192 entries\n"
               "  (a hit could only be served by an exact key stored in the entry, not by the 128-bit hash alone)\n",
            atomic_load(&g_memo_sim_hit), atomic_load(&g_memo_sim_probe), 1 << CACHE_BITS, atomic_load(&g_memo_sim_hit8));
    fputs("\nearly stop of the single target fight against the incumbent key (fresh rated leaves):\n", f);
    fprintf(f, "  fresh rated leaves %9ld; decided by survival alone (key[0] below): %8ld\n"
               "  decided mid-fight at a checkpoint: %8ld (mean decision frame %.0f, frames saved %11ld)\n"
               "  lost only at the fight's end without a mid-fight checkpoint: %8ld; could win or tie (full fight needed): %8ld\n",
            atomic_load(&g_es_fresh), atomic_load(&g_es_k0_less), atomic_load(&g_es_stopped),
            atomic_load(&g_es_stopped) ? (double)atomic_load(&g_es_frame_sum) / (double)atomic_load(&g_es_stopped) : 0.0,
            atomic_load(&g_es_frames_saved), atomic_load(&g_es_lost_end), atomic_load(&g_es_must_run));

    long t0 = 0;
    for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) if (g_w[i].enter_ns && (!t0 || g_w[i].enter_ns < t0)) t0 = g_w[i].enter_ns;
    fputs("\nworker hand-out (level 0 children taken: a cut one ends there, the rest is split into level 1 tasks every worker takes;\n"
          "  busy = inside the tasks walked, idle = the worker's loop time minus busy, gone = left the loop before the run ended):\n", f);
    double busy_sum = 0, crit = 0; int crit_top = -1, crit_child = -1, crit_slot = -1;
    for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) {
        if (!g_w[i].enter_ns) continue;
        double span = (g_w[i].exit_ns - g_w[i].enter_ns) / 1e9, busy = atomic_load(&g_w[i].busy_ns) / 1e9;
        busy_sum += busy;
        fprintf(f, "  slot %d (%s): loop %7.2f s  busy %7.2f s  idle %6.2f s  gone %7.2f s   %d children taken, %d tasks\n",
                i, i ? "helper" : "run thread", span, busy, span - busy, total_s - (g_w[i].exit_ns - t0) / 1e9, g_w[i].ntake, g_w[i].nrec);
        for (int r = 0; r < g_w[i].nrec; r++) {
            double d = g_w[i].rec[r].dur_ns / 1e9;
            fprintf(f, "      top %3d task %3d (child %3d): %8.2f s  leaves %9ld\n", g_w[i].rec[r].top, g_w[i].rec[r].j, g_w[i].rec[r].child,
                    d, g_w[i].rec[r].leaves);
            if (d > crit) { crit = d; crit_top = g_w[i].rec[r].top; crit_child = g_w[i].rec[r].child; crit_slot = i; }
        }
        if (g_w[i].ovf) fprintf(f, "      (task table overflow %d)\n", g_w[i].ovf);
    }
    fprintf(f, "  busy sum %.2f s of %.2f thread-seconds; longest single task: %.2f s (top %d child %d, slot %d)\n",
            busy_sum, total_s * (threads > 0 ? threads : 1), crit, crit_top, crit_child, crit_slot);
    long lf = atomic_load(&g_lk_fresh);
    fputs("\nlazy key (fresh rated leaves, the incumbent read at the leaf's start; key[0] never rises above the goal cap):\n", f);
    fprintf(f, "  fresh rated leaves %9ld; no damage base: %8ld; incumbent below the cap (survival can still win): %9ld\n"
               "  incumbent at the cap: %9ld; of all fresh leaves, damage-only key[1] < b1 (skippable by the rule): %9ld (%.1f %%)\n"
               "  leaf also at the cap: %9ld; of those key[1] < b1 (loses whatever survival says): %9ld, key[1] == b1 (needs key[2]): %9ld, key[1] > b1: %9ld\n",
            lf, atomic_load(&g_lk_nodmg), lf - atomic_load(&g_lk_nodmg) - atomic_load(&g_lk_bcap),
            atomic_load(&g_lk_bcap), atomic_load(&g_lk_skip), lf ? 100.0 * (double)atomic_load(&g_lk_skip) / (double)lf : 0.0,
            atomic_load(&g_lk_bothcap), atomic_load(&g_lk_bc_less), atomic_load(&g_lk_bc_tie), atomic_load(&g_lk_bc_more));
}
#endif


static void make_pick(const dm_gear_search *s, dm_gear_pick *pk) {
    memset(pk, 0, sizeof *pk);
    memcpy(pk->charm, s->keep, (size_t)s->p.ncand);
    for (int k = 0; k < DM_GEAR_P_N; k++) pk->at[k] = -1;
    for (int l = 0; l < s->nlev; l++) {
        const level *L = &s->lev[l]; const choice *c = &L->c[s->assign[l]];
        if (L->place == -1) { pk->at[DM_GEAR_P_MAIN] = c->a; pk->at[DM_GEAR_P_OFF] = c->b; }
        else if (L->place == -2) { pk->at[DM_GEAR_P_RING1] = c->a; pk->at[DM_GEAR_P_RING2] = c->b; }
        else if (L->place == -3) { const charm_group *g = &s->grp[L->group]; for (int i = 0; i < c->a; i++) pk->charm[g->member[i]] = 1; }
        else pk->at[L->place] = c->a;
    }
}
#ifdef DM_GEAR_INSTR


static void tail_note(uint64_t h1, uint64_t h2, uint64_t a1, uint64_t a2) {
    static _Thread_local uint64_t tf[1024][2], tm[1024][2];
    int n = g_tail_full_n;
    int dfull = 0, dmask = 0;
    for (int k = 0; k < n; k++) { dfull |= tf[k][0] == h1 && tf[k][1] == h2; dmask |= tm[k][0] == a1 && tm[k][1] == a2; }
    if (dfull) atomic_fetch_add_explicit(&g_tail_full_dup, 1, memory_order_relaxed);
    if (dmask && !dfull) atomic_fetch_add_explicit(&g_tail_mask_dup, 1, memory_order_relaxed);
    if (n < 1024) { tf[n][0] = h1; tf[n][1] = h2; tm[n][0] = a1; tm[n][1] = a2; }
    g_tail_full_n = n + 1;
}
#endif


#define CS_MEASURED 1
#define CS_UNRATED 2
#define CS_DAMAGE 3





static int leaf_can_only_lose(const dm_gear_search *s, const dm_gear_measures *m, const double b[3]) {
    if (!b || !s->goal || !(s->now.group > 0 || s->now.single > 0)) return 0;
    double cap = dm_gear_hits_key(s->goal->survival_hits, s->goal->survival_hits);
    return b[0] == cap && dm_gear_damage_key(&s->now, m, s->goal) < b[1];
}


static int measure_cached(dm_gear_search *s, const dm_gear_pick *pk, dm_gear_measures *m, const double *b, int *skipped) {
    *skipped = 0;
    static _Thread_local dm_sim_char c;
    unsigned flags;
#ifdef DM_GEAR_INSTR
    g_leaf_fresh = 0;
    double t_ = t_now_ns();
#endif
    if (dm_gear_leaf_char(&s->p, pk, &c, NULL, &flags)) return -1;
#ifdef DM_GEAR_INSTR
    { double t1 = t_now_ns(); t_add(T_COMPOSE, (long)(t1 - t_)); t_ = t1; }
#endif
    uint64_t h1, h2; char_hash(&c, &h1, &h2);
    centry *e = &s->cache[h1 & ((1u << CACHE_BITS) - 1)];
    if (s->prog) inc(&s->prog->runs);
#ifndef DM_PLAYER_BUILD
    for (long w = 0; w < s->test_run_us && !stopped(s); w += 1000) nanosleep(&(struct timespec){ 0, 1000000 }, NULL);
#endif
    if (e->state && e->h1 == h1 && e->h2 == h2) {
#ifdef DM_GEAR_INSTR
        t_add(T_MLOOK, (long)(t_now_ns() - t_));
#endif
        if (e->state == CS_MEASURED) { *m = e->m; return 0; }
        if (e->state == CS_UNRATED) return -1;
        *m = e->m;
        if (leaf_can_only_lose(s, &e->m, b)) { *skipped = 1; return 0; }
        int rc = dm_gear_measure_survival(s->p.d, &c, m);
        if (rc) { e->state = CS_UNRATED; return -1; }
        e->m = *m; e->state = CS_MEASURED; return 0;
    }
    if (s->prog) inc(&s->prog->fresh);
#ifdef DM_GEAR_INSTR
    t_add(T_MLOOK, (long)(t_now_ns() - t_));

    if (s->from == FROM_TREE) tail_check(s);
    uint64_t a1, a2;
    mask_hash(s->sc, &c, &a1, &a2);
    dm_sim_instr_set_leaf(a1, a2);
    g_leaf_fresh = 1;
    {
        uint64_t *slot = g_memo_sim[a1 & ((1u << CACHE_BITS) - 1)];
        if (slot[0] == a1 && slot[1] == a2) atomic_fetch_add_explicit(&g_memo_sim_hit, 1, memory_order_relaxed);
        else { slot[0] = a1; slot[1] = a2; }
        uint64_t *slot8 = g_memo_sim8[a1 & ((1u << 13) - 1)];
        if (slot8[0] == a1 && slot8[1] == a2) atomic_fetch_add_explicit(&g_memo_sim_hit8, 1, memory_order_relaxed);
        else { slot8[0] = a1; slot8[1] = a2; }
        atomic_fetch_add_explicit(&g_memo_sim_probe, 1, memory_order_relaxed);
    }
    atomic_fetch_add_explicit(&g_tail_leaves, 1, memory_order_relaxed);
    tail_note(h1, h2, a1, a2);
    t_ = t_now_ns();
    memset(m, 0, sizeof *m);
    int rc = dm_gear_measure_damage(s->p.d, &c, s->p.splash_on, m);
    int skip = !rc && leaf_can_only_lose(s, m, b);
    if (!rc && !skip) rc = dm_gear_measure_survival(s->p.d, &c, m);
    if (skip) s->skipped++;
    t_add(T_MEASURE, (long)(t_now_ns() - t_));
    t_add(T_LEAFN, 0);
#else
    memset(m, 0, sizeof *m);
    int rc = dm_gear_measure_damage(s->p.d, &c, s->p.splash_on, m);
    int skip = !rc && leaf_can_only_lose(s, m, b);
    if (!rc && !skip) rc = dm_gear_measure_survival(s->p.d, &c, m);
    if (skip) s->skipped++;
#endif
    e->h1 = h1; e->h2 = h2;
    if (skip) { e->state = CS_DAMAGE; e->m = *m; *skipped = 1; return 0; }
    e->state = rc ? CS_UNRATED : CS_MEASURED; if (!rc) e->m = *m;
    return rc;
}
static int tie_before(dm_gear_search *s, const int *a, const int *b);

static void leaf(dm_gear_search *s) {
    if (s->prog) inc(&s->prog->leaves);
#ifdef DM_GEAR_INSTR
    s->w_leaves++;
#endif
    dm_gear_pick pk; make_pick(s, &pk);
    dm_gear_measures m;


    double best_at[3]; int have_at = 0;
    if (s->from == FROM_TREE) {
        pthread_mutex_lock(&s->sh->mu);
        have_at = s->sh->have;
        memcpy(best_at, s->sh->best, sizeof best_at);
        pthread_mutex_unlock(&s->sh->mu);
    }
    int skipped = 0;
#ifdef DM_GEAR_INSTR
    double t_ = t_now_ns();
    int fits = dm_gear_place(&s->p, pk.charm);
    t_add(T_PLACE, (long)(t_now_ns() - t_));
    if (!fits || measure_cached(s, &pk, &m, have_at ? best_at : NULL, &skipped)) { s->unrated_leaves++; return; }
#else
    if (!dm_gear_place(&s->p, pk.charm) || measure_cached(s, &pk, &m, have_at ? best_at : NULL, &skipped)) { s->unrated_leaves++; return; }
#endif
    double tol = 1e-9;
    if (skipped) {

        for (int k = 0; k < s->nanc; k++) {
            const dm_gear_bound_out *b = &s->anc[k];
            if (m.group > b->group * (1 + tol) + tol || m.single > b->single * (1 + tol) + tol) { s->tripwire++; break; }
        }
        return;
    }
    for (int k = 0; k < s->nanc; k++) {
        const dm_gear_bound_out *b = &s->anc[k];
        if (m.group > b->group * (1 + tol) + tol || m.single > b->single * (1 + tol) + tol || m.hits > b->hits * (1 + tol) + tol ||
            m.survival_s > b->survival_s * (1 + tol) + tol || m.recovery > b->recovery * (1 + tol) + tol) { s->tripwire++; break; }
    }
#ifdef DM_GEAR_INSTR
    dm_gear_score_out o;
    t_ = t_now_ns();
    dm_gear_score(&s->now, &m, s->goal, &o);
    t_add(T_SCORE, (long)(t_now_ns() - t_));
    if (g_leaf_fresh && have_at) { early_stop_note(s, &m, o.key, best_at); lazy_key_note(s, o.key, best_at); }
#else
    dm_gear_score_out o;
    dm_gear_score(&s->now, &m, s->goal, &o);
#endif
    shared *h = s->sh;
    pthread_mutex_lock(&h->mu);
    int c = h->have ? dm_gear_key_cmp(o.key, h->best) : 1;

    while (c == 0 && s->from == FROM_TREE && h->from == FROM_TREE) {
        static _Thread_local int other[MAXDEPTH];
        memcpy(other, h->assign, sizeof *other * (size_t)s->nlev);
        pthread_mutex_unlock(&h->mu);
        int before = tie_before(s, s->assign, other);
        pthread_mutex_lock(&h->mu);
        c = dm_gear_key_cmp(o.key, h->best);
        if (c == 0 && h->from == FROM_TREE && !memcmp(other, h->assign, sizeof *other * (size_t)s->nlev)) { c = before; break; }
    }
    if (c == 0 && s->from == FROM_TREE && h->from == FROM_SEED) c = 1;
    if (c > 0) {
        h->have = 1; h->from = s->from; memcpy(h->best, o.key, sizeof h->best);
        if (s->from == FROM_TREE) memcpy(h->assign, s->assign, sizeof *h->assign * (size_t)s->nlev);
        s->res->best = pk; s->res->after = m; s->res->score = o;
        if (s->prog) { atomic_store_explicit(&s->prog->found, 1, memory_order_relaxed); if (s->prog->better) s->prog->better(s->res, s->prog->ctx); }
    }
    pthread_mutex_unlock(&h->mu);
}

static void list_max(dm_gear_list *dst, const dm_gear_list *src, int lo) {
    for (int k = 0; k < src->n; k++) {
        const dm_gear_v *v = &src->v[k]; int cur = dm_gear_list_get(dst, v->stat, v->layer);
        if (lo ? v->value < cur : v->value > cur) dm_gear_list_add(dst, v->stat, v->layer, v->value - cur);
    }
}

static void cand_bound(const dm_gear_search *s, int c, int decided, dm_gear_list *hi, dm_gear_list *lo, int *unk) {
    const dm_gear_list *l = &s->p.cand[c].item.own;
    int exact = decided && s->sure[c];
    for (int k = 0; k < l->n; k++) {
        int st = l->v[k].stat, ly = l->v[k].layer, v = l->v[k].value, cls = s->sc[st];
        if (cls == DM_GEAR_SC_IRR || cls == DM_GEAR_SC_OUT || !v) continue;
        if (cls == DM_GEAR_SC_UNKNOWN && !exact) *unk = 1;
        dm_gear_list_add(hi, st, ly, exact ? v : v > 0 ? v : 0);
        if (cls == DM_GEAR_SC_SKILL || cls == DM_GEAR_SC_UNKNOWN) dm_gear_list_add(lo, st, ly, exact ? v : v < 0 ? v : 0);
    }
}
static void choice_bound(const dm_gear_search *s, const level *L, const choice *c, int decided, dm_gear_list *hi, dm_gear_list *lo, int *unk) {
    if (L->place == -3) {
        const charm_group *g = &s->grp[L->group];
        for (int i = 0; i < c->a; i++) cand_bound(s, g->member[i], decided, hi, lo, unk);
        return;
    }
    if (c->a >= 0) cand_bound(s, c->a, decided, hi, lo, unk);
    if (c->b >= 0) cand_bound(s, c->b, decided, hi, lo, unk);
}
static void bonus_bound(const dm_gear_search *s, const dm_gear_bonus *b, dm_gear_list *hi, dm_gear_list *lo, int *unk) {

    if (b->unmapped) return;
    int cls = s->sc[b->stat], v = b->value;
    if (cls == DM_GEAR_SC_IRR || cls == DM_GEAR_SC_OUT || !v) return;
    if (cls == DM_GEAR_SC_UNKNOWN) *unk = 1;
    if (v > 0) dm_gear_list_add(hi, b->stat, b->layer, v);
    if (v < 0 && (cls == DM_GEAR_SC_SKILL || cls == DM_GEAR_SC_UNKNOWN)) dm_gear_list_add(lo, b->stat, b->layer, v);
}




static void sets_bound(const dm_gear_search *s, const int16_t *items, int n, const int *pieces, dm_gear_list *hi, dm_gear_list *lo, int *unk) {
    const dm_gear_tables *t = s->p.t;
    uint8_t seen[DM_GEAR_MAXSET] = { 0 };
    for (int i = 0; i < n; i++) {
        const dm_gear_item *it = &s->p.cand[items[i]].item; int row = it->setitem;
        if (row < 0) continue;
        int set = t->item[row].set, func = t->item[row].func, most = pieces ? pieces[set] : INT_MAX;

        static _Thread_local dm_gear_list green, conv; green.n = green.full = conv.n = conv.full = 0;
        if (it->green_read) {
            for (int g = 0; g < 5; g++) for (int k = 0; k < it->green[g].n; k++) {
                if (pieces && (func == 0 || (func == 2 && most < g + 2))) continue;
                dm_gear_bonus b = { 0, 0, (int16_t)it->green[g].v[k].stat, (int16_t)it->green[g].v[k].layer, 0, it->green[g].v[k].value };
                bonus_bound(s, &b, hi, lo, unk);
                if (b.value > 0) dm_gear_list_add(&green, b.stat, b.layer, b.value);
            }
        } else for (int k = 0; k < t->nitemb; k++) if (t->itemb[k].owner == row && !(pieces && (func == 0 || (func == 2 && most < t->itemb[k].level + 1)))) {
            bonus_bound(s, &t->itemb[k], hi, lo, unk);
            if (!t->itemb[k].unmapped && t->itemb[k].value > 0) dm_gear_list_add(&green, t->itemb[k].stat, t->itemb[k].layer, t->itemb[k].value);
        }
        dm_gear_item_conversions(t, s->p.base.clvl, &green, &conv);
        for (int k = 0; k < conv.n; k++) if (conv.v[k].value > 0 && s->sc[conv.v[k].stat] != DM_GEAR_SC_IRR) dm_gear_list_add(hi, conv.v[k].stat, conv.v[k].layer, conv.v[k].value);
        if (seen[set]) continue;
        seen[set] = 1;
        for (int k = 0; k < t->nsetb; k++)
            if (t->setb[k].owner == set && most >= (t->setb[k].level ? t->setb[k].level : t->pieces[set])) bonus_bound(s, &t->setb[k], hi, lo, unk);
    }
}

static void charms_bound(const dm_gear_search *s, int from_level, int cells, dm_gear_list *hi, dm_gear_list *lo, int *unk) {
    typedef struct { double density; int value, area, n; } part;
    dm_gear_list keys = { 0 };
    for (int l = from_level; l < s->nlev; l++) {
        const level *L = &s->lev[l]; if (L->place != -3) continue;
        const dm_gear_list *own = &s->p.cand[s->grp[L->group].member[0]].item.own;
        for (int k = 0; k < own->n; k++) {
            int st = own->v[k].stat, cls = s->sc[st]; if (cls == DM_GEAR_SC_IRR || cls == DM_GEAR_SC_OUT || !own->v[k].value) continue;
            if (cls == DM_GEAR_SC_UNKNOWN) *unk = 1;
            if (own->v[k].value < 0) dm_gear_list_add(lo, st, own->v[k].layer, own->v[k].value * s->grp[L->group].n);
            if (own->v[k].value > 0 && !dm_gear_list_get(&keys, st, own->v[k].layer)) dm_gear_list_add(&keys, st, own->v[k].layer, 1);
        }
    }
    part parts[DM_GEAR_MAXCAND];
    for (int k = 0; k < keys.n; k++) {
        int st = keys.v[k].stat, ly = keys.v[k].layer, np = 0;
        for (int l = from_level; l < s->nlev; l++) {
            const level *L = &s->lev[l]; if (L->place != -3) continue;
            const charm_group *g = &s->grp[L->group];
            int v = dm_gear_list_get(&s->p.cand[g->member[0]].item.own, st, ly);
            if (v > 0) { parts[np].value = v; parts[np].area = g->w * g->h; parts[np].n = g->n; parts[np].density = (double)v / parts[np].area; np++; }
        }
        for (int i = 1; i < np; i++) for (int j = i; j > 0 && parts[j - 1].density < parts[j].density; j--) { part x = parts[j]; parts[j] = parts[j - 1]; parts[j - 1] = x; }
        double left = cells, sum = 0;
        for (int i = 0; i < np && left > 0; i++) {
            double take = fmin(parts[i].n, left / parts[i].area);
            sum += take * parts[i].value; left -= take * parts[i].area;
        }
        int add = (int)ceil(sum - 1e-9);
        if (add > 0) dm_gear_list_add(hi, st, ly, add);
    }
}




static int never_met(const dm_gear_search *s, const int16_t *dec, int n, const dm_gear_list *hi) {
    int need = 0;
    for (int k = 0; k < n; k++) need |= !s->sure[dec[k]];
    if (!need || hi->full) return 0;
    static _Thread_local dm_gear_item item;
    static _Thread_local dm_gear_char most;
    unsigned char on = 1;
    memset(&item, 0, sizeof item); item.setitem = -1; item.own = *hi;
    if (dm_gear_compose(s->p.t, &s->p.base, &item, 1, &on, &most) || (most.flags & DM_GEAR_C_FULL)) return 0;
    int str = most.value[ST_STR], dex = most.value[ST_DEX];
    for (int k = 0; k < n; k++) {
        int x = dec[k], worn_off = 0;
        if (s->sure[x]) continue;
        for (int j = 0; j < DM_GEAR_P_N; j++) worn_off |= s->p.off_now[j] == x;
        const dm_gear_item *it = &s->p.cand[x].item;
        if (!worn_off && ((it->req_str && str < it->req_str) || (it->req_dex && dex < it->req_dex) || s->p.base.clvl < it->req_lvl)) return 1;
    }
    return 0;
}


static void set_pieces(const dm_gear_search *s, const int16_t *dec, int n, const level *const *open, const int *holds, int nopen, int *pieces) {
    const dm_gear_tables *t = s->p.t;
    memset(pieces, 0, sizeof *pieces * DM_GEAR_MAXSET);
    int16_t rows[2 * DM_GEAR_P_N]; int nr = 0;
    for (int k = 0; k < n; k++) {
        int r = s->p.cand[dec[k]].item.setitem, dup = 0;
        if (r < 0) continue;
        for (int j = 0; j < nr; j++) dup |= rows[j] == r;
        if (!dup) { rows[nr++] = (int16_t)r; pieces[t->item[r].set]++; }
    }
    for (int o = 0; o < nopen; o++) {
        const level *L = open[o]; if (!L->nset) continue;
        int add[DM_GEAR_MAXSET] = { 0 };
        for (int k = 0; k < L->nset; k++) {
            int r = s->p.cand[L->set[k]].item.setitem, dup = 0;
            for (int j = 0; j < nr; j++) dup |= rows[j] == r;
            for (int j = 0; j < k && !dup; j++) dup |= s->p.cand[L->set[j]].item.setitem == r;
            if (!dup) add[t->item[r].set]++;
        }
        for (int st = 0; st < t->nset; st++) pieces[st] += add[st] < holds[o] ? add[st] : holds[o];
    }
}




typedef struct { int a; const level *rest; } half_level;
static int node_bound(dm_gear_search *s, const int *assign, int depth, int coarse, const half_level *half, dm_gear_bound_out *b) {
    static _Thread_local dm_gear_bound_in in;
    static _Thread_local int pieces[DM_GEAR_MAXSET];
#ifdef DM_GEAR_INSTR
    double tb_ = t_now_ns();
#define NB_RETURN(x) do { t_add(T_BOUND, (long)(t_now_ns() - tb_)); return (x); } while (0)
#else
#define NB_RETURN(x) return (x)
#endif
    memset(&in, 0, sizeof in);
    int16_t items[DM_GEAR_P_N + DM_GEAR_MAXCAND], dec[2 * DM_GEAR_P_N]; int nitems = 0, ndec = 0, cells = s->pool;
    const level *open[DM_GEAR_P_N + 1]; int holds[DM_GEAR_P_N + 1], nopen = 0;
    in.splash_on = s->p.splash_on;
    in.main = in.off = -1;
    int boots_decided = 0;
    for (int l = 0; l < s->nlev; l++) {
        const level *L = &s->lev[l];
        if (half && l == depth) {
            cand_bound(s, half->a, 1, &in.hi, &in.lo, &in.varied);
            dec[ndec++] = (int16_t)half->a;
            if (s->p.cand[half->a].item.setitem >= 0) items[nitems++] = (int16_t)half->a;
            L = half->rest;
        }
        if (l < depth) {
            const choice *c = &L->c[assign[l]];
            choice_bound(s, L, c, 1, &in.hi, &in.lo, &in.varied);
            if (L->place == -1) { in.main = c->a; in.off = c->b; in.hands = 1; }
            if (L->place == DM_GEAR_P_BOOTS) { boots_decided = 1; if (c->a >= 0) { in.kick_min = s->p.cand[c->a].dmin; in.kick_max = s->p.cand[c->a].dmax; } }
            if (L->place == -3) { const charm_group *g = &s->grp[L->group]; cells -= c->a * g->w * g->h; }
            else {
                if (c->a >= 0) { dec[ndec++] = c->a; if (s->p.cand[c->a].item.setitem >= 0) items[nitems++] = c->a; }
                if (c->b >= 0) { dec[ndec++] = c->b; if (s->p.cand[c->b].item.setitem >= 0) items[nitems++] = c->b; }
            }
        } else if (L->place != -3) {
            for (int k = 0; k < L->hi.n; k++) dm_gear_list_add(&in.hi, L->hi.v[k].stat, L->hi.v[k].layer, L->hi.v[k].value);
            for (int k = 0; k < L->lo.n; k++) dm_gear_list_add(&in.lo, L->lo.v[k].stat, L->lo.v[k].layer, L->lo.v[k].value);
            in.varied |= L->unk;
            for (int k = 0; k < L->nset; k++) items[nitems++] = L->set[k];
            if (L->place == DM_GEAR_P_BOOTS && !boots_decided) { in.kick_min = L->kick_min; in.kick_max = L->kick_max; }
            open[nopen] = L; holds[nopen++] = (L->place == -1 || L->place == -2) && !(half && l == depth) ? 2 : 1;
        }
    }
    if (cells < 0) NB_RETURN(1);
    for (int k = 0; k < s->keep_hi.n; k++) dm_gear_list_add(&in.hi, s->keep_hi.v[k].stat, s->keep_hi.v[k].layer, s->keep_hi.v[k].value);
    for (int k = 0; k < s->keep_lo.n; k++) dm_gear_list_add(&in.lo, s->keep_lo.v[k].stat, s->keep_lo.v[k].layer, s->keep_lo.v[k].value);
    in.varied |= s->keep_unk;
    charms_bound(s, depth + (half != NULL), cells, &in.hi, &in.lo, &in.varied);
    if (!coarse) set_pieces(s, dec, ndec, open, holds, nopen, pieces);
    sets_bound(s, items, nitems, coarse ? NULL : pieces, &in.hi, &in.lo, &in.varied);
    if (!coarse && never_met(s, dec, ndec, &in.hi)) NB_RETURN(1);
    in.tight = !coarse;
    if (s->prog) inc(&s->prog->bounds);
#ifdef DM_GEAR_INSTR
    double t_ = t_now_ns();
    int rc = dm_gear_bound_eval(&s->p, &s->p.nowc, &in, b);
    t_add(T_BOUNDEVAL, (long)(t_now_ns() - t_));
    NB_RETURN(rc ? -1 : 0);
#else
    NB_RETURN(dm_gear_bound_eval(&s->p, &s->p.nowc, &in, b) ? -1 : 0);
#endif
}
#undef NB_RETURN

static int child_order(const void *x, const void *y) {
    const child *a = x, *b = y;
    int c = dm_gear_key_cmp(b->key, a->key);
    return c ? c : a->i - b->i;
}

static int prune(const dm_gear_search *s, const double key[3]) {
    shared *h = s->sh;
    pthread_mutex_lock(&h->mu);
    int r = h->have && dm_gear_key_cmp(key, h->best) < 0;
    pthread_mutex_unlock(&h->mu);
    return r;
}


static void child_key(dm_gear_search *s, const int *assign, int depth, int coarse, dm_gear_bound_out *b, double key[3], int *ok) {
    *ok = 0;
    if (depth + 1 < s->nlev) {
        int rc = node_bound(s, assign, depth + 1, coarse, NULL, b);
        if (rc == 1) { key[0] = key[1] = key[2] = -INFINITY; *ok = 2; return; }
        if (!rc) { dm_gear_bound_key(&s->now, b, s->goal, key); *ok = 1; return; }
    }
    key[0] = key[1] = key[2] = INFINITY; *b = (dm_gear_bound_out){ INFINITY, INFINITY, INFINITY, INFINITY, INFINITY };
}


static int tie_before(dm_gear_search *s, const int *a, const int *b) {
    int l = 0;
    while (l < s->nlev && a[l] == b[l]) l++;
    if (l == s->nlev) return 0;
    int path[MAXDEPTH], ok; double ka[3], kb[3]; dm_gear_bound_out bo;
    memcpy(path, a, sizeof *path * (size_t)(l + 1));
    child_key(s, path, l, 1, &bo, ka, &ok);
    path[l] = b[l];
    child_key(s, path, l, 1, &bo, kb, &ok);
    int c = dm_gear_key_cmp(ka, kb);
    return c > 0 || (c == 0 && a[l] < b[l]);
}


#define PROG_DEPTH 6
static int walked(const dm_gear_search *s, int depth) {
    double w = 0, part = 1;
    for (int d = 1; d <= depth; d++) { if (s->of[d] <= 0) break; part /= s->of[d]; w += part * s->at[d]; }
    long v = (long)(w * 1e6);
    return v < 0 ? 0 : v > 999999 ? 999999 : (int)v;
}




static void share_to(dm_gear_search *s, long v) {
    shared *h = s->sh;
    if (v > s->task_end) v = s->task_end;
    if (v > s->cur_ppm) { atomic_fetch_add_explicit(&h->ppm, v - s->cur_ppm, memory_order_relaxed); s->cur_ppm = v; }
    long all = h->ntop > 0 ? atomic_load_explicit(&h->ppm, memory_order_relaxed) / h->ntop : 0;
    if (all > 999999) all = 999999;
    int v6 = (int)all, was = atomic_load_explicit(&s->prog->share, memory_order_relaxed);
    while (v6 > was && !atomic_compare_exchange_weak_explicit(&s->prog->share, &was, v6, memory_order_relaxed, memory_order_relaxed)) {}
}

static child *children(dm_gear_search *s, int depth) {
    const level *L = &s->lev[depth];
    child *ch = malloc(sizeof *ch * (size_t)L->n);
    if (!ch) { s->stopped = 1; return NULL; }

    uint8_t cut[2 * DM_GEAR_MAXCAND] = { 0 };
    if (L->place == -2 && s->nrg && depth + 1 < s->nlev) {
        for (int g = 0; g < s->nrg; g++) {
            if (s->rg[g].n < 2) continue;
            half_level half = { s->rg[g].a, &s->rg[g].rest };
            dm_gear_bound_out b; double key[3];
            int rc = node_bound(s, s->assign, depth, 0, &half, &b);
            if (rc == 0) dm_gear_bound_key(&s->now, &b, s->goal, key);
            cut[g] = rc == 1 || (rc == 0 && prune(s, key));
            if (stopped(s)) { free(ch); return NULL; }
        }
    }
    for (int i = 0; i < L->n; i++) {
        ch[i].i = i;
        if (L->place == -2 && s->rg_of && s->rg_of[i] >= 0 && cut[s->rg_of[i]]) {
            ch[i].ok = 2; ch[i].key[0] = ch[i].key[1] = ch[i].key[2] = -INFINITY;
            ch[i].b = (dm_gear_bound_out){ INFINITY, INFINITY, INFINITY, INFINITY, INFINITY };
            continue;
        }
        s->assign[depth] = i;
        child_key(s, s->assign, depth, 0, &ch[i].b, ch[i].key, &ch[i].ok);
        if (stopped(s)) { free(ch); return NULL; }
    }
    qsort(ch, (size_t)L->n, sizeof *ch, child_order);
    return ch;
}
#ifdef DM_GEAR_INSTR




#define INSTR_DEPTH 24
#define INSTR_RATIO_N 1000
#define INSTR_KEY_N 4
enum { IN_VISIT, IN_BOUND, IN_NOBOUND, IN_CUT_NONE, IN_CUT_K0, IN_CUT_K1, IN_CUT_K2, IN_TIE, IN_BETTER, IN_W_K0TIE, IN_W_K1TIE, IN_W_K2INF, IN_W_K2FIN, IN_N };
static atomic_long g_instr[INSTR_DEPTH][IN_N];
static atomic_long g_instr_rn[INSTR_DEPTH];
static double g_instr_rsum[INSTR_DEPTH], g_instr_rmax[INSTR_DEPTH];
static struct { int depth, cut; double key[3], best[3]; } g_instr_sample[INSTR_DEPTH][INSTR_KEY_N];
static atomic_int g_instr_sn[INSTR_DEPTH];
static pthread_mutex_t g_instr_mu = PTHREAD_MUTEX_INITIALIZER;
static int instr_depth(int d) { return d < INSTR_DEPTH ? d : INSTR_DEPTH - 1; }

static int instr_class(const dm_gear_search *s, const double key[3], int *below, double *ratio) {
    shared *h = s->sh;
    pthread_mutex_lock(&h->mu);
    int r = -2;
    double best[3];
    *below = 0; *ratio = 0;
    if (h->have) {
        memcpy(best, h->best, sizeof best);
        r = -1;
        for (int k = 0; k < 3; k++) if (key[k] != best[k]) { r = k; *below = key[k] < best[k]; break; }
        if (r >= 0 && best[r] > 0) *ratio = key[r] / best[r]; else if (r == -1) *ratio = 1;
    }
    pthread_mutex_unlock(&h->mu);
    return r;
}


static void instr_ties(dm_gear_search *s, int depth, const double key[3]) {
    shared *h = s->sh;
    pthread_mutex_lock(&h->mu);
    int d = instr_depth(depth), k0 = h->have && key[0] == h->best[0], k1 = k0 && key[1] == h->best[1], inf;
    inf = k1 && isinf(key[2]);
    pthread_mutex_unlock(&h->mu);
    if (!k0) return;
    atomic_fetch_add_explicit(&g_instr[d][IN_W_K0TIE], 1, memory_order_relaxed);
    if (!k1) return;
    atomic_fetch_add_explicit(&g_instr[d][IN_W_K1TIE], 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_instr[d][inf ? IN_W_K2INF : IN_W_K2FIN], 1, memory_order_relaxed);
}

static void instr_child(dm_gear_search *s, int depth, const child *c, int cut) {
    int d = instr_depth(depth);
    if (c->ok == 1) atomic_fetch_add_explicit(&g_instr[d][IN_BOUND], 1, memory_order_relaxed);
    else if (c->ok == 0) atomic_fetch_add_explicit(&g_instr[d][IN_NOBOUND], 1, memory_order_relaxed);
    if (cut == 3) { atomic_fetch_add_explicit(&g_instr[d][IN_CUT_NONE], 1, memory_order_relaxed); return; }
    static const int bucket[3] = { IN_CUT_K0, IN_CUT_K1, IN_CUT_K2 };
    int below; double ratio;
    int r = instr_class(s, c->key, &below, &ratio);
    if (cut == -1) {

        atomic_fetch_add_explicit(&g_instr[d][r >= 0 && r < 3 && below ? bucket[r] : IN_CUT_K2], 1, memory_order_relaxed);
        return;
    }
    int t = r == -1 ? IN_TIE : IN_BETTER;
    atomic_fetch_add_explicit(&g_instr[d][t], 1, memory_order_relaxed);
    if (c->ok == 1) instr_ties(s, depth, c->key);
    if (c->ok == 1 && atomic_load_explicit(&g_instr_sn[d], memory_order_relaxed) < INSTR_KEY_N) {
        shared *h = s->sh;
        pthread_mutex_lock(&h->mu);
        int n = atomic_load_explicit(&g_instr_sn[d], memory_order_relaxed);
        if (n < INSTR_KEY_N) {
            g_instr_sample[d][n].depth = depth; g_instr_sample[d][n].cut = r;
            memcpy(g_instr_sample[d][n].key, c->key, sizeof c->key);
            if (h->have) memcpy(g_instr_sample[d][n].best, h->best, sizeof h->best);
            else g_instr_sample[d][n].best[0] = g_instr_sample[d][n].best[1] = g_instr_sample[d][n].best[2] = NAN;
            atomic_store_explicit(&g_instr_sn[d], n + 1, memory_order_relaxed);
        }
        pthread_mutex_unlock(&h->mu);
    }
    if (c->ok == 1 && ratio > 0) {
        pthread_mutex_lock(&g_instr_mu);
        if (g_instr_rn[d] < INSTR_RATIO_N) { g_instr_rn[d]++; g_instr_rsum[d] += ratio; if (ratio > g_instr_rmax[d]) g_instr_rmax[d] = ratio; }
        pthread_mutex_unlock(&g_instr_mu);
    }
}
void dm_gear_instr_reset(void) {
    for (int d = 0; d < INSTR_DEPTH; d++) {
        for (int c = 0; c < IN_N; c++) atomic_store_explicit(&g_instr[d][c], 0, memory_order_relaxed);
        pthread_mutex_lock(&g_instr_mu);
        g_instr_rn[d] = 0; g_instr_rsum[d] = 0; g_instr_rmax[d] = 0;
        pthread_mutex_unlock(&g_instr_mu);
        atomic_store_explicit(&g_instr_sn[d], 0, memory_order_relaxed);
    }
}
void dm_gear_instr_report(FILE *f) {
    fprintf(f, "depth    nodes bounded no-bound    cut none  k0  k1  k2  walked   tie better  bound/best  n   mean    max  walk: k0tie k1tie k2inf k2fin\n");
    for (int d = 0; d < INSTR_DEPTH; d++) {
        long g[IN_N];
        long tot = 0;
        for (int c = 0; c < IN_N; c++) { g[c] = atomic_load(&g_instr[d][c]); tot += g[c]; }
        if (!tot) continue;
        long rn = atomic_load(&g_instr_rn[d]);
        double mean = rn > 0 ? g_instr_rsum[d] / (double)rn : 0;
        fprintf(f, "%-6d %7ld %7ld %8ld %7ld %5ld %3ld %3ld %3ld %7ld %5ld %6ld %10ld %6.2f %7.2f %8ld %6ld %6ld %6ld\n",
                (int)d, g[IN_VISIT], g[IN_BOUND], g[IN_NOBOUND],
                g[IN_CUT_NONE] + g[IN_CUT_K0] + g[IN_CUT_K1] + g[IN_CUT_K2], g[IN_CUT_NONE], g[IN_CUT_K0], g[IN_CUT_K1], g[IN_CUT_K2],
                g[IN_TIE] + g[IN_BETTER], g[IN_TIE], g[IN_BETTER], rn, mean, g_instr_rmax[d],
                g[IN_W_K0TIE], g[IN_W_K1TIE], g[IN_W_K2INF], g[IN_W_K2FIN]);
    }
    fputs("walked children with a bound, first per depth: the child's bound key, the best at that moment, and the first key component where\n"
          "they differ (their index; -1 a full tie) - the component that keeps the child from being cut:\n", f);
    for (int d = 0; d < INSTR_DEPTH; d++) {
        int n = atomic_load(&g_instr_sn[d]);
        for (int k = 0; k < n; k++) {
            const double *key = g_instr_sample[d][k].key, *best = g_instr_sample[d][k].best;
            fprintf(f, "depth %-3d child %d  bound %.6f / %.6f / %.6f   best %.6f / %.6f / %.6f   differs at %d\n",
                    g_instr_sample[d][k].depth, k, key[0], key[1], key[2], best[0], best[1], best[2], g_instr_sample[d][k].cut);
        }
    }
}




static const char *instr_level_name(int place) {
    static const char *const n[DM_GEAR_P_N] = { "main/off", "main/off", "armor", "amulet", "rings", "rings", "head", "belt", "boots", "gloves" };
    if (place == -1) return "hands";
    if (place == -2) return "rings";
    if (place == -3) return "charms";
    return place >= 0 && place < DM_GEAR_P_N ? n[place] : "?";
}
static void instr_terms_print(FILE *f, const dm_gear_instr_terms *ts, const dm_gear_instr_terms *tg) {
    if (!ts->have) return;
    fprintf(f, "            single %9.1f = actions %8.1f + pets %6.1f + auras %6.1f + item auras %6.1f + wounds %5.1f + cb/sf %7.1f + kill boost %7.1f\n",
            ts->single, ts->lp, ts->pets, ts->auras, ts->item_auras, ts->ow, ts->share, ts->boost);
    fprintf(f, "            group  %9.1f = best skill %8.1f (skill %d) + item auras %6.1f + struck %6.1f\n",
            tg->group, tg->g_best, tg->gwin >= 0 && tg->gwin < tg->ns ? tg->sk[tg->gwin].id : -1, tg->g_item, tg->g_struck);
}
void dm_gear_instr_dump_path(FILE *f, dm_gear_search *s, const dm_gear_pick *best, int nsample) {
    if (!s || !best || !s->goal || !s->now_ok) { fprintf(f, "path dump: needs a run's goal and a rated start\n"); return; }
    const dm_gear_goal *goal = s->goal;
    int path[MAXDEPTH];
    for (int l = 0; l < s->nlev; l++) {
        const level *L = &s->lev[l];
        path[l] = -1;
        for (int i = 0; i < L->n; i++) {
            const choice *c = &L->c[i];
            int hit;
            if (L->place == -1) hit = c->a == best->at[DM_GEAR_P_MAIN] && c->b == best->at[DM_GEAR_P_OFF];
            else if (L->place == -2) hit = c->a == best->at[DM_GEAR_P_RING1] && c->b == best->at[DM_GEAR_P_RING2];
            else if (L->place == -3) {
                const charm_group *g = &s->grp[L->group]; int cnt = 0;
                for (int k = 0; k < g->n; k++) cnt += best->charm[g->member[k]] != 0;
                hit = cnt == c->a;
            } else hit = c->a == best->at[L->place];
            if (hit) { path[l] = i; break; }
        }
        if (path[l] < 0) { fprintf(f, "path dump: the leaf is not a tree leaf (level %d)\n", l); return; }
    }
    dm_gear_measures m; unsigned flags;
    if (dm_gear_leaf_measure(&s->p, best, &m, &flags)) { fprintf(f, "path dump: the leaf has no rating (%u)\n", flags); return; }
    dm_gear_score_out so; dm_gear_score(&s->now, &m, goal, &so);
    fprintf(f, "leaf: now %.1f/%.1f -> measured group %.1f single %.1f survival %.1f hits %.2f recovery %.3f; key %.2f/%.2f/%.4f\n",
            s->now.group, s->now.single, m.group, m.single, m.survival_s, m.hits, m.recovery, so.key[0], so.key[1], so.key[2]);
    for (int l = 0; l < s->nlev; l++)
        fprintf(f, "level %d: %s, %d children, path child %d\n", l, instr_level_name(s->lev[l].place), s->lev[l].n, path[l]);
    dm_gear_instr_cap(1);

    for (int l = 0; l < s->nlev; l++) s->assign[l] = path[l];
    dm_gear_bound_out bl; dm_gear_instr_terms ts, tg;
    int rl = node_bound(s, s->assign, s->nlev, 0, NULL, &bl);
    if (rl) fprintf(f, "\nleaf's own bound: none (rc %d)\n", rl);
    else {
        fprintf(f, "\nleaf's own bound: group %.1f single %.1f\n", bl.group, bl.single);
        if (!dm_gear_instr_get_terms(&ts, &tg)) instr_terms_print(f, &ts, &tg);
    }
    for (int d = 0; d + 1 < s->nlev; d++) {
        const level *L = &s->lev[d];
        fprintf(f, "\ndepth %d (%s, %d children): the leaf's key %.2f/%.2f/%.4f\n", d, instr_level_name(L->place), L->n,
                so.key[0], so.key[1], so.key[2]);
        for (int l = 0; l < d; l++) s->assign[l] = path[l];
        for (int i = 0; i < L->n; i++) {
            s->assign[d] = i;
            dm_gear_bound_out b; double key[3];
            int rc = node_bound(s, s->assign, d + 1, 0, NULL, &b);
            if (rc == 1) { fprintf(f, "  child %3d%s: no leaf rated\n", i, i == path[d] ? " <path>" : ""); continue; }
            if (rc) { fprintf(f, "  child %3d%s: no bound\n", i, i == path[d] ? " <path>" : ""); continue; }
            dm_gear_bound_key(&s->now, &b, goal, key);
            if (i != path[d] && i >= nsample && i < L->n - nsample && L->n > 4 * nsample) {
                fprintf(f, "  child %3d: key %8.2f/%8.2f/%9.4f single %9.1f group %9.1f\n", i, key[0], key[1], key[2], b.single, b.group);
                continue;
            }
            fprintf(f, "  child %3d%s: key %8.2f/%8.2f/%9.4f\n", i, i == path[d] ? " <path>" : "", key[0], key[1], key[2]);
            if (dm_gear_instr_get_terms(&ts, &tg)) continue;
            instr_terms_print(f, &ts, &tg);
            if (i != path[d]) continue;
            for (int k = 0; k < ts.ns; k++)
                fprintf(f, "            skill %4d lv %3d..%3d boss %8.1f pack %8.1f share %7.1f f %5.1f af %5.1f cd %5.1f mana %6.1f pet/s %6.1f aura/s %6.1f area %d mix %6.1f\n",
                        ts.sk[k].id, ts.sk[k].lo, ts.sk[k].hi, ts.sk[k].d, ts.sk[k].dg, ts.sk[k].p, ts.sk[k].f, ts.sk[k].af, ts.sk[k].cd, ts.sk[k].c,
                        ts.sk[k].pet * DM_SIM_FPS, ts.sk[k].aura * DM_SIM_FPS, ts.sk[k].area, ts.sk[k].mix);
            fprintf(f, "            mana stock %.0f (+%.0f/s regen, +%.0f/s potion flow, belt mana total %.0f); boss %.0f life, window %.0f frames; crush %d, static share %.3f, aura state %d\n",
                    ts.stock, ts.regen, ts.pot_rate * DM_SIM_FPS, ts.pot_all, ts.boss, ts.win, ts.crush, ts.field, ts.astate);
            fprintf(f, "            LP solution at frame %.0f: kill in window %d, landed %.1f (cast credit %.1f), mana %.0f of %.0f at that frame, frames %.0f of %.0f\n",
                    ts.lp_t, ts.kill, ts.lp_val, ts.credit, ts.lp_mana,
                    ts.stock + ts.regen * ts.lp_t / DM_SIM_FPS + fmin(ts.pot_rate * ts.lp_t, ts.pot_all), ts.lp_frames, ts.lp_t);
        }
    }
    dm_gear_instr_cap(0);
}
#define INSTR_CUT(s, d, c, how) instr_child((s), (d), (c), (how))
#define INSTR_WALK(s, d, c) instr_child((s), (d), (c), 0)
#define INSTR_VISIT(d) atomic_fetch_add_explicit(&g_instr[instr_depth(d)][IN_VISIT], 1, memory_order_relaxed)
#else
#define INSTR_CUT(s, d, c, how) ((void)0)
#define INSTR_WALK(s, d, c) ((void)0)
#define INSTR_VISIT(d) ((void)0)
#endif

static void dfs(dm_gear_search *s, int depth) {
    if (stopped(s)) return;
    if (depth == s->nlev) { leaf(s); return; }
    const level *L = &s->lev[depth];
    child *ch = children(s, depth);
    if (!ch) return;
    INSTR_VISIT(depth);
    for (int k = 0; k < L->n && !stopped(s); k++) {
        if (ch[k].ok == 2) { INSTR_CUT(s, depth, &ch[k], 3); if (s->prog) inc(&s->prog->pruned); continue; }
        if (ch[k].ok && prune(s, ch[k].key)) { INSTR_CUT(s, depth, &ch[k], -1); if (s->prog) inc(&s->prog->pruned); continue; }
        INSTR_WALK(s, depth, &ch[k]);
        if (s->prog) { atomic_store_explicit(&s->prog->place, depth, memory_order_relaxed); atomic_store_explicit(&s->prog->choice, ch[k].i, memory_order_relaxed); }
        s->at[depth] = k; s->of[depth] = L->n;
        if (depth < PROG_DEPTH && s->prog) share_to(s, walked(s, depth));
        s->assign[depth] = ch[k].i;
        s->anc[s->nanc++] = ch[k].b;
        dfs(s, depth + 1);
        s->nanc--;
    }
    free(ch);
}



static void top_done(dm_gear_search *s) {
    if (!s->prog || stopped(s)) return;
    long t = atomic_fetch_add_explicit(&s->sh->taken, 1, memory_order_relaxed) + 1;
    long runs = atomic_load_explicit(&s->prog->runs, memory_order_relaxed);
    double mean = (double)(runs - s->sh->before) / t;
    atomic_store_explicit(&s->prog->estimate, runs + (long)(mean * (s->sh->ntop - t)), memory_order_relaxed);
}

static void top_walked(dm_gear_search *s) {
    if (!s->prog || stopped(s)) return;
    share_to(s, 1000000);
    top_done(s);
}





static void work(dm_gear_search *s) {
    shared *h = s->sh;
    const level *L0 = &s->lev[0];
#ifdef DM_GEAR_INSTR
    if (g_w[s->wslot].rec) g_w[s->wslot].enter_ns = (long)t_now_ns();
#endif
    while (!stopped(s)) {
        if (s->helper && !s->was_wide) break;
        task *t = NULL;
        int k = -1;
        pthread_mutex_lock(&h->tmu);
        if (h->tstack) { t = h->tstack; h->tstack = t->next; }
        else if (atomic_load_explicit(&h->next, memory_order_relaxed) < h->ntop) { k = atomic_fetch_add_explicit(&h->next, 1, memory_order_relaxed); h->splitting++; }
        else if (h->splitting == 0) { pthread_mutex_unlock(&h->tmu); break; }
        pthread_mutex_unlock(&h->tmu);
        if (!t && k < 0) { sched_yield(); continue; }
        if (k >= 0) {
            const child *c = &h->top[k];
            s->cur_ppm = 0; s->task_end = 1000000;
            INSTR_VISIT(0);
#ifdef DM_GEAR_INSTR
            g_w[s->wslot].ntake++;
#endif
            if (c->ok == 2 || (c->ok && prune(s, c->key))) { INSTR_CUT(s, 0, c, c->ok == 2 ? 3 : -1); if (s->prog) inc(&s->prog->pruned);
                pthread_mutex_lock(&h->tmu); h->splitting--; pthread_mutex_unlock(&h->tmu);
                top_walked(s); }
            else if (s->nlev < 2) {
                pthread_mutex_lock(&h->tmu); h->splitting--; pthread_mutex_unlock(&h->tmu);
                INSTR_WALK(s, 0, c);
                if (s->prog) { atomic_store_explicit(&s->prog->place, 0, memory_order_relaxed); atomic_store_explicit(&s->prog->choice, c->i, memory_order_relaxed); }
                s->at[0] = k; s->of[0] = L0->n; s->assign[0] = c->i;
                s->nanc = 0; s->anc[s->nanc++] = c->b;
                dfs(s, 1);
                top_walked(s);
            } else {
                INSTR_WALK(s, 0, c);
                s->assign[0] = c->i;
                child *ch = children(s, 1);
                if (!ch) { pthread_mutex_lock(&h->tmu); h->splitting--; pthread_mutex_unlock(&h->tmu); break; }
                task *list = NULL; int npushed = 0;
                for (int i = s->lev[1].n - 1; i >= 0; i--) {
                    task *x = malloc(sizeof *x);
                    if (!x) { s->stopped = 1; break; }
                    x->top = k; x->j = i; x->c = ch[i]; x->next = list; list = x; npushed++;
                }
                free(ch);
                task *tl = list; while (tl && tl->next) tl = tl->next;
                pthread_mutex_lock(&h->tmu);
                if (npushed) { atomic_store_explicit(&h->left[k], npushed, memory_order_relaxed); if (tl) tl->next = h->tstack; h->tstack = list; }
                h->splitting--;
                INSTR_VISIT(1);
                pthread_mutex_unlock(&h->tmu);
                if (!npushed) top_walked(s);
            }
            continue;
        }
        child cc = t->c;
        int topk = t->top;
        const child *c = &cc;
#ifdef DM_GEAR_INSTR
        int tj = t->j;
        long tk0 = (long)t_now_ns(); long lv0 = s->w_leaves;
#endif


        int n1 = s->lev[1].n;
        s->cur_ppm = (long)((double)t->j * 1e6 / (double)n1);
        s->task_end = (long)((double)(t->j + 1) * 1e6 / (double)n1);
        if (c->ok == 2 || (c->ok && prune(s, c->key))) { INSTR_CUT(s, 1, c, c->ok == 2 ? 3 : -1); if (s->prog) inc(&s->prog->pruned); }
        else {
            INSTR_WALK(s, 1, c);
            if (s->prog) { atomic_store_explicit(&s->prog->place, 1, memory_order_relaxed); atomic_store_explicit(&s->prog->choice, c->i, memory_order_relaxed); }
            s->at[0] = topk; s->of[0] = L0->n;
            s->at[1] = t->j; s->of[1] = s->lev[1].n;
            s->assign[0] = h->top[topk].i; s->assign[1] = c->i;
            s->nanc = 0;
            s->anc[s->nanc++] = h->top[topk].b;
            s->anc[s->nanc++] = c->b;
            dfs(s, 2);
        }
        free(t);
        if (s->prog && !stopped(s)) share_to(s, s->task_end);
#ifdef DM_GEAR_INSTR
        { long dur = (long)t_now_ns() - tk0;
          atomic_fetch_add_explicit(&g_w[s->wslot].busy_ns, dur, memory_order_relaxed);
          wrec_add(&g_w[s->wslot].rec, WREC_MAX, &g_w[s->wslot].nrec, &g_w[s->wslot].ovf, dur, s->w_leaves - lv0, topk, c->i, tj); }
#endif
        if (atomic_fetch_sub_explicit(&h->left[topk], 1, memory_order_acq_rel) == 1) top_done(s);
    }
#ifdef DM_GEAR_INSTR
    if (g_w[s->wslot].rec) g_w[s->wslot].exit_ns = (long)t_now_ns();
#endif
}
static void *helper(void *x) {
    dm_gear_search *w = x;
    if (w->pace) w->pace(1);
    work(w);
    return NULL;
}


static int same_facts(const dm_gear_problem *p, const dm_gear_cand *a, const dm_gear_cand *b) {
    if (a->kind != b->kind || a->block != b->block || a->cls_only != b->cls_only || (a->weapon < 0) != (b->weapon < 0)) return 0;
    return a->weapon < 0 || a->weapon == b->weapon || !memcmp(&p->weapon[a->weapon], &p->weapon[b->weapon], sizeof *p->weapon);
}

static int same_item(const dm_gear_search *s, int ia, int ib) {
    const dm_gear_cand *a = &s->p.cand[ia], *b = &s->p.cand[ib];
    if (!same_facts(&s->p, a, b) || a->item.setitem != b->item.setitem || a->item.green_read || b->item.green_read) return 0;
    if (a->item.req_str != b->item.req_str || a->item.req_dex != b->item.req_dex || a->item.req_lvl != b->item.req_lvl || a->dmin != b->dmin || a->dmax != b->dmax)
        return 0;
    for (int pass = 0; pass < 2; pass++) {
        const dm_gear_list *x = pass ? &b->item.own : &a->item.own, *y = pass ? &a->item.own : &b->item.own;
        for (int k = 0; k < x->n; k++)
            if (s->sc[x->v[k].stat] != DM_GEAR_SC_IRR && x->v[k].value != dm_gear_list_get(y, x->v[k].stat, x->v[k].layer)) return 0;
    }
    return 1;
}
static int is_hand(int kind) { return kind >= DM_GEAR_K_WEAPON1; }
static int is_main(int kind) { return kind >= DM_GEAR_K_WEAPON1 && kind <= DM_GEAR_K_XBOW; }
static int off_fits(int main_kind, int off_kind) {
    if (off_kind < 0) return 1;
    if (main_kind == DM_GEAR_K_WEAPON2) return 0;
    if (main_kind == DM_GEAR_K_BOW) return off_kind == DM_GEAR_K_ARROWS;
    if (main_kind == DM_GEAR_K_XBOW) return off_kind == DM_GEAR_K_BOLTS;
    return off_kind == DM_GEAR_K_SHIELD;
}
static int add_choice(level *L, int cap, int a, int b) {
    if (L->n >= cap) return -1;
    L->c[L->n].a = (int16_t)a; L->c[L->n].b = (int16_t)b; L->n++;
    return 0;
}
static void level_summary(dm_gear_search *s, level *L) {
    L->hi.n = L->lo.n = 0; L->unk = 0; L->nset = 0; L->kick_min = L->kick_max = 0;
    int16_t seen[2 * DM_GEAR_MAXCAND]; int ns = 0;
    for (int i = 0; i < L->n; i++) {
        static _Thread_local dm_gear_list hi, lo; hi.n = hi.full = lo.n = lo.full = 0;
        choice_bound(s, L, &L->c[i], 0, &hi, &lo, &L->unk);
        list_max(&L->hi, &hi, 0); list_max(&L->lo, &lo, 1);
        int16_t m[2] = { L->c[i].a, L->c[i].b };
        for (int k = 0; k < 2 && L->place != -3; k++) {
            if (m[k] < 0) continue;
            const dm_gear_cand *c = &s->p.cand[m[k]];
            if (L->place == DM_GEAR_P_BOOTS) { if (c->dmin > L->kick_min) L->kick_min = c->dmin; if (c->dmax > L->kick_max) L->kick_max = c->dmax; }
            if (c->item.setitem < 0) continue;
            int dup = 0; for (int j = 0; j < ns; j++) dup |= seen[j] == m[k];
            if (!dup) seen[ns++] = m[k];
        }
    }
    if (ns) { L->set = malloc(sizeof *L->set * (size_t)ns); if (L->set) { memcpy(L->set, seen, sizeof *seen * (size_t)ns); L->nset = ns; } }
}

static int now_choice(const dm_gear_search *s, int l, const dm_gear_pick *pk);
dm_gear_search *dm_gear_search_new(const dm_gear_problem *prob) {
    if (!prob || !prob->ready) return NULL;
    dm_gear_search *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->p = *prob;
    const dm_gear_problem *p = &s->p;
    int n = p->ncand;
    dm_gear_stat_classes(p, s->sc);
    s->cache = calloc((size_t)1 << CACHE_BITS, sizeof *s->cache);
    s->sure = calloc((size_t)n + 1, 1); s->keep = calloc((size_t)n + 1, 1);
    uint8_t *gone = calloc((size_t)n + 1, 1), *fixed = calloc((size_t)n + 1, 1);
    s->lev = calloc(LV_CHARMS + (size_t)n + 1, sizeof *s->lev);
    s->grp = calloc((size_t)n + 1, sizeof *s->grp);
    if (!s->cache || !s->sure || !s->keep || !gone || !fixed || !s->lev || !s->grp) { free(gone); free(fixed); dm_gear_search_free(s); return NULL; }

    int str = dm_gear_list_get(&p->base.base, ST_STR, 0), dex = dm_gear_list_get(&p->base.base, ST_DEX, 0);
    for (int i = 0; i < n; i++) { int v = dm_gear_list_sum(&p->cand[i].item.own, ST_STR), w = dm_gear_list_sum(&p->cand[i].item.own, ST_DEX); if (v < 0) str += v; if (w < 0) dex += w; }
    for (int k = 0; k < p->t->nsetb; k++) if (p->t->setb[k].value < 0 && (p->t->setb[k].stat == ST_STR || p->t->setb[k].stat == ST_DEX)) { if (p->t->setb[k].stat == ST_STR) str += p->t->setb[k].value; else dex += p->t->setb[k].value; }
    for (int k = 0; k < p->t->nitemb; k++) if (p->t->itemb[k].value < 0 && (p->t->itemb[k].stat == ST_STR || p->t->itemb[k].stat == ST_DEX)) { if (p->t->itemb[k].stat == ST_STR) str += p->t->itemb[k].value; else dex += p->t->itemb[k].value; }
    for (int i = 0; i < n; i++) { const dm_gear_item *it = &p->cand[i].item; s->sure[i] = p->base.clvl >= it->req_lvl && (!it->req_str || str >= it->req_str) && (!it->req_dex || dex >= it->req_dex); }

    double kinds[DM_GEAR_K_N] = { 0 };
    int nweap[2] = { 0, 0 };
    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i];
        kinds[c->kind] += 1;

        if (p->charms_fixed && c->kind == DM_GEAR_K_CHARM) {
            if (c->unrated) s->unrated++;
            if (c->inv && !(c->unrated & DM_GEAR_U_LEVEL)) { if (c->unrated) s->unrated_worn++; fixed[i] = 1; } else gone[i] = 1;
            continue;
        }


        unsigned unrated = c->unrated | (!c->worn && new_skill(p, &c->item.own) ? DM_GEAR_U_SKILL : 0);
        if (unrated) { s->unrated++; if ((c->worn && c->kind != DM_GEAR_K_CHARM) || (c->kind == DM_GEAR_K_CHARM && c->inv && !(unrated & DM_GEAR_U_LEVEL))) { s->unrated_worn++; fixed[i] = 1; } else gone[i] = 1; continue; }
        if (c->kind == DM_GEAR_K_WEAPON1 && c->weapon >= 0) nweap[p->weapon[c->weapon].w.kind == DM_WK_CLAW]++;
    }
    if (p->now.cls == 4) s->dual_pairs = (nweap[0] + nweap[1]) * (nweap[0] + nweap[1] - 1);
    if (p->now.cls == 6) s->dual_pairs = nweap[1] * (nweap[1] - 1);


    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (gone[i] || fixed[i] || c->worn || c->item.setitem >= 0 || is_hand(c->kind) || c->dmin || c->dmax) continue;
        int any = 0;
        for (int k = 0; k < c->item.own.n && !any; k++) any = c->item.own.v[k].value && s->sc[c->item.own.v[k].stat] != DM_GEAR_SC_IRR;
        if (!any) { gone[i] = 1; s->filtered++; }
    }




    int most[2], stat2[2] = { ST_STR, ST_DEX };
    most[0] = dm_gear_list_get(&p->base.base, ST_STR, 0); most[1] = dm_gear_list_get(&p->base.base, ST_DEX, 0);
    for (int a = 0; a < 2; a++) {
        int pl = -1, shift = 3;
        for (int k = 0; k < p->t->nop; k++) if (p->t->op[k].op == 2 && p->t->op[k].level && p->t->op[k].ntg == 1 && p->t->op[k].tg[0] == stat2[a]) { pl = p->t->op[k].stat; shift = p->t->op[k].param; }
        long long per = 0;
        for (int i = 0; i < n; i++) {
            int v = dm_gear_list_sum(&p->cand[i].item.own, stat2[a]); if (v > 0) most[a] += v;
            if (pl >= 0) { int w = dm_gear_list_sum(&p->cand[i].item.own, pl); if (w > 0) per += w; }
            if (p->cand[i].item.green_read) for (int g = 0; g < 5; g++) for (int k = 0; k < p->cand[i].item.green[g].n; k++) {
                const dm_gear_v *v2 = &p->cand[i].item.green[g].v[k];
                if (v2->value > 0 && v2->stat == stat2[a]) most[a] += v2->value;
                if (v2->value > 0 && v2->stat == pl) per += v2->value;
            }
        }
        for (int k = 0; k < p->t->nsetb; k++) { const dm_gear_bonus *x = &p->t->setb[k]; if (x->value > 0 && x->stat == stat2[a]) most[a] += x->value; if (x->value > 0 && x->stat == pl) per += x->value; }
        for (int k = 0; k < p->t->nitemb; k++) { const dm_gear_bonus *x = &p->t->itemb[k]; if (x->value > 0 && x->stat == stat2[a]) most[a] += x->value; if (x->value > 0 && x->stat == pl) per += x->value; }
        if (per > 0 && shift >= 0 && shift < 31) most[a] += (int)((per * p->base.clvl + (1LL << shift) - 1) >> shift);
    }
    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i]; const dm_gear_item *it = &c->item;
        if (gone[i] || fixed[i] || c->worn || c->kind == DM_GEAR_K_CHARM) continue;
        if (p->base.clvl < it->req_lvl || (it->req_str && most[0] < it->req_str) || (it->req_dex && most[1] < it->req_dex) ||
            (c->cls_only && c->cls_only - 1 != p->now.cls)) { gone[i] = 1; s->filtered++; }
    }



    for (int b = 0; b < n; b++) {
        const dm_gear_cand *cb = &p->cand[b];
        if (gone[b] || fixed[b] || cb->worn || cb->kind == DM_GEAR_K_CHARM) continue;
        int need = cb->kind == DM_GEAR_K_RING ? 2 : 1, have = 0;
        for (int a = 0; a < n && have < need; a++) {
            if (a == b || gone[a] || fixed[a] || p->cand[a].kind != cb->kind) continue;
            if (same_item(s, a, b) && (p->cand[a].worn || a < b)) have++;
        }
        if (have >= need) { gone[b] = 1; s->filtered++; }
    }

    level *L = &s->lev[s->nlev++]; L->place = -1;
    int nm = 0, no = 0, nk[DM_GEAR_K_N] = { 0 };
    for (int i = 0; i < n; i++) { int k = p->cand[i].kind; nk[k]++; nm += is_main(k); no += is_hand(k) && !is_main(k); }
    int cap = (nm + 1) * (no + 1);
    L->c = malloc(sizeof *L->c * (size_t)cap);
    if (!L->c) goto fail;
    dm_gear_pick now; dm_gear_pick_now(p, &now);
    int wm = now.at[DM_GEAR_P_MAIN], wo = now.at[DM_GEAR_P_OFF];
    if ((wm >= 0 && fixed[wm]) || (wo >= 0 && fixed[wo])) add_choice(L, cap, wm, wo);
    else for (int m = -1; m < n; m++) {
        if (m >= 0 && (gone[m] || fixed[m] || !is_main(p->cand[m].kind))) continue;
        for (int o = -1; o < n; o++) {
            if (o >= 0 && (gone[o] || fixed[o] || !is_hand(p->cand[o].kind) || is_main(p->cand[o].kind))) continue;
            if (!off_fits(m >= 0 ? p->cand[m].kind : -1, o >= 0 ? p->cand[o].kind : -1)) continue;
            if (m >= 0 && !ammo_fits(p, m, o) && !(m == wm && o == wo)) continue;
            add_choice(L, cap, m, o);
        }
    }
    s->raw = 0;
    for (int m = -1; m < n; m++) for (int o = -1; o < n; o++)
        s->raw += (m < 0 || is_main(p->cand[m].kind)) && (o < 0 || (is_hand(p->cand[o].kind) && !is_main(p->cand[o].kind))) &&
                  off_fits(m >= 0 ? p->cand[m].kind : -1, o >= 0 ? p->cand[o].kind : -1);

    static const struct { int lv, place, kind; } one[] = {
        { LV_ARMOR, DM_GEAR_P_ARMOR, DM_GEAR_K_ARMOR }, { LV_AMULET, DM_GEAR_P_AMULET, DM_GEAR_K_AMULET }, { LV_RINGS, -2, DM_GEAR_K_RING },
        { LV_HEAD, DM_GEAR_P_HEAD, DM_GEAR_K_HEAD }, { LV_BELT, DM_GEAR_P_BELT, DM_GEAR_K_BELT }, { LV_BOOTS, DM_GEAR_P_BOOTS, DM_GEAR_K_BOOTS },
        { LV_GLOVES, DM_GEAR_P_GLOVES, DM_GEAR_K_GLOVES } };
    for (size_t q = 0; q < sizeof one / sizeof *one; q++) {
        L = &s->lev[s->nlev++]; L->place = one[q].place;
        cap = (nk[one[q].kind] + 1) * (L->place == -2 ? nk[one[q].kind] + 1 : 1);
        L->c = malloc(sizeof *L->c * (size_t)cap);
        if (!L->c) goto fail;
        if (L->place == -2) {
            int r1 = now.at[DM_GEAR_P_RING1], r2 = now.at[DM_GEAR_P_RING2];
            int f1 = r1 >= 0 && fixed[r1], f2 = r2 >= 0 && fixed[r2], fix = f1 ? r1 : f2 ? r2 : -1;
            if (f1 && f2) add_choice(L, cap, r1, r2);
            else if (fix >= 0) {
                add_choice(L, cap, fix, -1);
                for (int b = 0; b < n; b++) if (!gone[b] && !fixed[b] && p->cand[b].kind == DM_GEAR_K_RING) add_choice(L, cap, fix, b);
            } else {
                add_choice(L, cap, -1, -1);
                for (int a = 0; a < n; a++) {
                    if (gone[a] || fixed[a] || p->cand[a].kind != DM_GEAR_K_RING) continue;
                    add_choice(L, cap, a, -1);
                    for (int b = a + 1; b < n; b++) if (!gone[b] && !fixed[b] && p->cand[b].kind == DM_GEAR_K_RING) add_choice(L, cap, a, b);
                }
            }
        } else {
            int w = now.at[L->place];
            if (w >= 0 && fixed[w]) add_choice(L, cap, w, -1);
            else {
                add_choice(L, cap, -1, -1);
                for (int i = 0; i < n; i++) if (!gone[i] && !fixed[i] && p->cand[i].kind == one[q].kind) add_choice(L, cap, i, -1);
            }
        }
        s->raw *= one[q].kind == DM_GEAR_K_RING ? 1 + kinds[DM_GEAR_K_RING] + kinds[DM_GEAR_K_RING] * (kinds[DM_GEAR_K_RING] - 1) / 2 : 1 + kinds[one[q].kind];
    }


    s->pool = 0;
    for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) s->pool += !p->grid[y][x];
    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (c->kind != DM_GEAR_K_CHARM) continue;
        s->raw *= 2;
        if (gone[i] || fixed[i]) { if (fixed[i]) { s->pool -= c->w * c->h; s->keep[i] = 1; } continue; }
        int g = c->inv ? s->ng : 0;
        for (; g < s->ng; g++) {
            const dm_gear_cand *m = &p->cand[s->grp[g].member[0]];
            if (!m->inv && m->w == c->w && m->h == c->h && same_item(s, s->grp[g].member[0], i)) break;
        }
        if (g == s->ng) { s->grp[g].member = malloc(sizeof *s->grp[g].member * (size_t)n); if (!s->grp[g].member) goto fail; s->grp[g].w = c->w; s->grp[g].h = c->h; s->ng++; }
        charm_group *G = &s->grp[g];
        G->member[G->n++] = (int16_t)i;
    }
    for (int g = 0; g < s->ng; g++) {
        L = &s->lev[s->nlev++]; L->place = -3; L->group = g;
        L->c = malloc(sizeof *L->c * (size_t)(s->grp[g].n + 1));
        if (!L->c) goto fail;
        for (int k = 0; k <= s->grp[g].n; k++) add_choice(L, s->grp[g].n + 1, k, -1);
    }


    dm_gear_pick_now(p, &s->nowpick);
    for (int l = 0; l < s->nlev; l++) {
        level *V = &s->lev[l]; int i = now_choice(s, l, &s->nowpick);
        if (i > 0) { choice c = V->c[i]; memmove(&V->c[1], &V->c[0], sizeof *V->c * (size_t)i); V->c[0] = c; }
    }
    for (int l = 0; l < s->nlev; l++) level_summary(s, &s->lev[l]);

    for (int l = 0; l < s->nlev; l++) {
        level *R = &s->lev[l]; if (R->place != -2) continue;
        s->rg = calloc((size_t)R->n + 1, sizeof *s->rg); s->rg_of = malloc(sizeof *s->rg_of * ((size_t)R->n + 1));
        if (!s->rg || !s->rg_of) goto fail;
        for (int i = 0; i < R->n; i++) {
            int a = R->c[i].a, g = 0;
            s->rg_of[i] = -1;
            if (a < 0) continue;
            while (g < s->nrg && s->rg[g].a != a) g++;
            if (g == s->nrg) {
                s->rg[g].a = a; s->rg[g].rest.place = DM_GEAR_P_RING2; s->nrg++;
                s->rg[g].rest.c = malloc(sizeof *s->rg[g].rest.c * (size_t)R->n);
                if (!s->rg[g].rest.c) goto fail;
            }
            ring_group *G = &s->rg[g];
            G->rest.c[G->rest.n].a = R->c[i].b; G->rest.c[G->rest.n].b = -1; G->rest.n++; G->n++;
            s->rg_of[i] = (int16_t)g;
        }
        for (int g = 0; g < s->nrg; g++) level_summary(s, &s->rg[g].rest);
    }

    for (int i = 0; i < n; i++) if (s->keep[i]) { s->sure[i] = 1; cand_bound(s, i, 1, &s->keep_hi, &s->keep_lo, &s->keep_unk); }
    free(gone); free(fixed);

    unsigned flags;
    dm_gear_pick_now(p, &s->nowpick);
    s->now_ok = !dm_gear_leaf_measure(p, &s->nowpick, &s->now, &flags);
    return s;
fail:
    free(gone); free(fixed); dm_gear_search_free(s);
    return NULL;
}
void dm_gear_search_free(dm_gear_search *s) {
    if (!s) return;
    for (int l = 0; s->lev && l < s->nlev; l++) { free(s->lev[l].c); free(s->lev[l].set); }
    for (int g = 0; s->grp && g < s->ng; g++) free(s->grp[g].member);
    for (int g = 0; s->rg && g < s->nrg; g++) { free(s->rg[g].rest.c); free(s->rg[g].rest.set); }
    free(s->rg); free(s->rg_of);
    for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) free(s->wcache[i]);
    free(s->lev); free(s->grp); free(s->cache); free(s->sure); free(s->keep); free(s);
}
void dm_gear_search_wide(dm_gear_search *s, int threads, const atomic_int *wide, void (*pace)(int wide)) {
    if (!s) return;
    s->threads = threads < 1 ? 1 : threads > DM_GEAR_MAXTHREADS ? DM_GEAR_MAXTHREADS : threads;
    s->wide = wide; s->pace = pace;
}
size_t dm_gear_search_bytes(const dm_gear_search *s) {
    size_t b = sizeof *s + ((size_t)1 << CACHE_BITS) * sizeof *s->cache + (size_t)s->p.ncand;
    for (int g = 0; g < s->nrg; g++) b += sizeof s->rg[g] + sizeof *s->rg[g].rest.c * (size_t)s->rg[g].rest.n + sizeof *s->rg_of;
    for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) if (s->wcache[i]) b += ((size_t)1 << CACHE_BITS) * sizeof *s->cache;
    for (int l = 0; l < s->nlev; l++) b += sizeof s->lev[l] + sizeof *s->lev[l].c * (size_t)s->lev[l].n + sizeof *s->lev[l].set * (size_t)s->lev[l].nset;
    for (int g = 0; g < s->ng; g++) b += sizeof s->grp[g] + sizeof *s->grp[g].member * (size_t)s->p.ncand;
    return b;
}


static int now_choice(const dm_gear_search *s, int l, const dm_gear_pick *pk) {
    const level *L = &s->lev[l];
    for (int i = 0; i < L->n; i++) {
        const choice *c = &L->c[i];
        if (L->place == -1 && c->a == pk->at[DM_GEAR_P_MAIN] && c->b == pk->at[DM_GEAR_P_OFF]) return i;
        if (L->place == -2) {
            int x = pk->at[DM_GEAR_P_RING1], y = pk->at[DM_GEAR_P_RING2];
            if ((c->a == x && c->b == y) || (c->a == y && c->b == x)) return i;
        }
        if (L->place == -3) {
            const charm_group *g = &s->grp[L->group]; int k = 0;
            for (int m = 0; m < g->n; m++) k += pk->charm[g->member[m]];
            if (c->a == k) return i;
        }
        if (L->place >= 0 && c->a == pk->at[L->place]) return i;
    }
    return -1;
}
#ifndef DM_PLAYER_BUILD
void dm_gear_search_test_run_us(dm_gear_search *s, long run_us) { if (s) s->test_run_us = run_us; }
#endif
void dm_gear_search_seed(dm_gear_search *s, const dm_gear_pick *pk) {
    if (!s) return;
    s->seed_on = pk != NULL;
    if (pk) s->seed = *pk;
}
static uint64_t fp_bytes(uint64_t h, const void *x, size_t n) {
    const unsigned char *b = x; size_t i = 0;
    for (; i + 8 <= n; i += 8) { uint64_t w; memcpy(&w, b + i, 8); h = (h ^ w) * 1099511628211ULL; h ^= h >> 29; }
    for (; i < n; i++) h = (h ^ b[i]) * 1099511628211ULL;
    return h;
}
uint64_t dm_gear_cand_hash(const dm_gear_problem *p, int i) {
    if (!p || i < 0 || i >= p->ncand) return 0;
    static _Thread_local dm_gear_cand c;
    c = p->cand[i]; c.item.id = 0; c.item.slot = 0; c.inv = c.x = c.y = c.worn = 0; c.weapon = c.weapon >= 0;
    uint64_t h = fp_bytes(1469598103934665603ULL, &c, sizeof c);
    const dm_gear_cand *o = &p->cand[i];
    if (o->weapon >= 0 && o->weapon < p->nweapon) h = fp_bytes(h, &p->weapon[o->weapon], sizeof *p->weapon);
    return mix(h);
}
uint64_t dm_gear_problem_hash(const dm_gear_problem *p, const dm_gear_goal *goal) {
    if (!p || !goal) return 0;
    uint64_t h = 1469598103934665603ULL, g = p->d ? p->d->generation : 0;
    uintptr_t at[2] = { (uintptr_t)p->d, (uintptr_t)p->t };
    h = fp_bytes(h, &g, sizeof g); h = fp_bytes(h, at, sizeof at);
    h = fp_bytes(h, &p->base, sizeof p->base); h = fp_bytes(h, &p->now, sizeof p->now); h = fp_bytes(h, &p->cb, sizeof p->cb);
    h = fp_bytes(h, p->node, sizeof p->node); h = fp_bytes(h, p->prog, sizeof p->prog); h = fp_bytes(h, p->tab, sizeof p->tab); h = fp_bytes(h, p->elem, sizeof p->elem);
    int n[4] = { p->ncand, p->nweapon, p->unarmed, p->charms_fixed };
    h = fp_bytes(h, n, sizeof n); h = fp_bytes(h, &p->splash_on, sizeof p->splash_on); h = fp_bytes(h, p->grid, sizeof p->grid);
    if (p->ncand > 0 && p->cand) h = fp_bytes(h, p->cand, sizeof *p->cand * (size_t)p->ncand);
    if (p->nweapon > 0 && p->weapon) h = fp_bytes(h, p->weapon, sizeof *p->weapon * (size_t)p->nweapon);
    h = fp_bytes(h, &goal->survival_hits, sizeof goal->survival_hits); h = fp_bytes(h, &goal->boss_share, sizeof goal->boss_share);
    h = mix(h);
    return h ? h : 1;
}
int dm_gear_search_run(dm_gear_search *s, const dm_gear_goal *goal, const atomic_int *stop, dm_gear_progress *prog, dm_gear_result *res) {
    if (!s || !goal || !res) return DM_GEAR_S_INPUT;
    memset(res, 0, sizeof *res);
    res->unrated = s->unrated; res->unrated_worn = s->unrated_worn; res->dual_pairs = s->dual_pairs; res->filtered = s->filtered;
    res->charm_groups = s->ng; res->raw_combinations = s->raw;
    if (!s->now_ok) return res->status = DM_GEAR_S_NOREF;
    shared h = { .from = FROM_START };
    h.assign = malloc(sizeof *h.assign * (size_t)(s->nlev + 1));
    if (!h.assign) return res->status = DM_GEAR_S_INPUT;
    if (pthread_mutex_init(&h.mu, NULL)) { free(h.assign); return res->status = DM_GEAR_S_INPUT; }
    if (pthread_mutex_init(&h.tmu, NULL)) { pthread_mutex_destroy(&h.mu); free(h.assign); return res->status = DM_GEAR_S_INPUT; }
    s->goal = goal; s->stop = stop; s->prog = prog; s->res = res; s->stopped = 0; s->nanc = 0; s->tripwire = 0; s->unrated_leaves = 0; s->skipped = 0;
    s->sh = &h; s->from = FROM_START; s->helper = 0; s->cur_ppm = 0; s->task_end = 1000000;
    s->was_wide = s->wide && atomic_load_explicit(s->wide, memory_order_relaxed) != 0;
    if (s->pace) s->pace(s->was_wide);
    res->now = s->now;

    for (int l = 0; l < s->nlev; l++) { int i = now_choice(s, l, &s->nowpick); s->assign[l] = i < 0 ? 0 : i; }
    dm_gear_score_out o; dm_gear_score(&s->now, &s->now, goal, &o);
    h.have = 1; memcpy(h.best, o.key, sizeof h.best); res->best = s->nowpick; res->after = s->now; res->score = o;
    if (prog) { atomic_store_explicit(&prog->found, 1, memory_order_relaxed); if (prog->better) prog->better(res, prog->ctx); }
    for (int l = 0; l < s->nlev && !stopped(s); l++) {
        int keep = s->assign[l];
        for (int i = 0; i < s->lev[l].n && !stopped(s); i++) {
            if (i == keep) continue;
            double was[3]; memcpy(was, h.best, sizeof was);
            s->assign[l] = i; leaf(s);
            if (dm_gear_key_cmp(h.best, was) > 0) keep = i;
        }
        s->assign[l] = keep;
    }


    if (s->seed_on && !stopped(s)) {
        int ok = 1;
        for (int l = 0; l < s->nlev && ok; l++) { int i = now_choice(s, l, &s->seed); if (i < 0) ok = 0; else s->assign[l] = i; }
        if (ok) {
            long ul = s->unrated_leaves; double was[3]; memcpy(was, h.best, sizeof was);
            s->from = FROM_SEED; leaf(s); s->unrated_leaves = ul;
            res->seeded = dm_gear_key_cmp(h.best, was) > 0;
        }
    }

    s->from = FROM_TREE;
#ifdef DM_GEAR_INSTR
    s->wslot = 0;
#endif
    if (s->nlev == 0) dfs(s, 0);
    else if (!stopped(s) && (h.top = children(s, 0))) {
        h.ntop = s->lev[0].n;
        h.before = prog ? atomic_load_explicit(&prog->runs, memory_order_relaxed) : 0;
        h.left = h.ntop > 0 ? calloc((size_t)h.ntop, sizeof *h.left) : NULL;
        if (!h.left) { s->stopped = 1; free(h.top); h.top = NULL; }
#ifdef DM_GEAR_INSTR
        for (int i = 0; i < DM_GEAR_MAXTHREADS; i++) if (!g_w[i].rec) g_w[i].rec = calloc(WREC_MAX, sizeof *g_w[i].rec);
#endif
        pthread_t th[DM_GEAR_MAXTHREADS]; dm_gear_search *w[DM_GEAR_MAXTHREADS]; int nw = 0;
        int want = s->was_wide ? s->threads - 1 : 0;
        if (want > h.ntop - 1) want = h.ntop - 1;
        for (int i = 0; i < want; i++) {
            if (!s->wcache[i]) s->wcache[i] = calloc((size_t)1 << CACHE_BITS, sizeof *s->cache);
            dm_gear_search *x = s->wcache[i] ? malloc(sizeof *x) : NULL;
            if (!x) break;
            *x = *s; x->cache = s->wcache[i]; x->helper = 1;
            x->tripwire = x->unrated_leaves = x->skipped = 0;
#ifdef DM_GEAR_INSTR
            x->wslot = i + 1;
#endif
            if (pthread_create(&th[nw], NULL, helper, x)) { free(x); break; }
            w[nw++] = x;
        }
        work(s);
        for (int i = 0; i < nw; i++) {
            pthread_join(th[i], NULL);
            s->stopped |= w[i]->stopped; s->tripwire += w[i]->tripwire; s->unrated_leaves += w[i]->unrated_leaves; s->skipped += w[i]->skipped;
            free(w[i]);
        }
        pthread_mutex_lock(&h.tmu);
        while (h.tstack) { task *x = h.tstack; h.tstack = x->next; free(x); }
        pthread_mutex_unlock(&h.tmu);
        free(h.left);
        free(h.top);
    }
    pthread_mutex_destroy(&h.mu);
    pthread_mutex_destroy(&h.tmu);
    free(h.assign);
    s->sh = NULL;



    if (prog && !s->stopped) atomic_store_explicit(&prog->share, 1000000, memory_order_relaxed);
    res->tripwire = s->tripwire; res->leaves_unrated = s->unrated_leaves; res->leaves_partial = s->skipped;
    return res->status = s->stopped ? DM_GEAR_S_ABORTED : DM_GEAR_S_DONE;
}
