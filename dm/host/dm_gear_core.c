#ifdef DM_GEAR_INSTR
#define _POSIX_C_SOURCE 200809L
#endif
#include "dm_gear_core.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef DM_GEAR_INSTR


#include <stdatomic.h>
#include <time.h>
static atomic_long g_mt[4];
static long m_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1000000000L + t.tv_nsec; }
static void m_add(int k, long dt) { atomic_fetch_add_explicit(&g_mt[k], dt, memory_order_relaxed); }
void dm_gear_instr_times(double *target, double *survival, double *group, double *dealt) {
    *target = atomic_load(&g_mt[0]) / 1e9; *survival = atomic_load(&g_mt[1]) / 1e9; *group = atomic_load(&g_mt[2]) / 1e9;
    *dealt = atomic_load(&g_mt[3]) / 1e9;
}
void dm_gear_instr_times_reset(void) { for (int k = 0; k < 4; k++) atomic_store(&g_mt[k], 0); }
#define M_T0 long t0_ = m_now()
#define M_ADD(k) do { long t1_ = m_now(); m_add((k), t1_ - t0_); t0_ = t1_; } while (0)
#else
#define M_T0 ((void)0)
#define M_ADD(k) ((void)0)
#endif

int dm_gear_boss_life(const dm_sim_data *d, int diff, int act) {
    for (int i = 0; i < d->nboss; i++) {
        if (d->boss[i].kind != DM_BK_ACTBOSS || d->boss[i].act != act) continue;
        dm_sim_target b;
        if (!dm_sim_target_mon(d, d->boss[i].mon, diff, 0, DM_TK_ACTBOSS, 1, &b) && b.hp > 0) return b.hp;
    }
    return -1;
}






static double group_damage(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *std, int splash_on, int *best_id) {
    dm_sim_target pack = *std; pack.extra_targets = DM_GEAR_PACK_EXTRA;
    int ids[DM_SIM_MAXBAR + 1] = { 0 }, n = 0;
    for (int i = -1; i < c->nbar && n <= DM_SIM_MAXBAR; i++) {
        int id = i < 0 ? 0 : c->bar[i];
        if (id >= 0 && id < DM_SIM_MAXSKILL && d->skill[id] && (!id || c->lvl[id])) ids[n++] = id;
    }
    dm_sim_skill_out o[DM_SIM_MAXBAR + 1];
    *best_id = -1;
    if (dm_sim_skills_prepared(d, c, &pack, n, ids, o)) return 0;
    double best = 0;
    for (int i = 0; i < n; i++) {
        if (o[i].flags & (DM_SF_UNAVAIL | DM_SF_MODIFIER | DM_SF_UTILITY | DM_SF_NOWEAPON)) continue;
        double H = d->skill[ids[i]]->area ? DM_GEAR_AREA_HITS : 1;
        double v;
        if (splash_on && o[i].splash > 1) {
            double pois = o[i].dps_element[DM_EL_POIS];
            v = fmax(0.0, o[i].dps - o[i].ow_dps - pois) * H * o[i].splash + pois * H * o[i].splash_pois + o[i].ow_dps * DM_GEAR_PACK_UNITS;
        }
        else v = o[i].dps * H;
        double lasts = o[i].mana_seconds * DM_SIM_FPS;
        if (o[i].mana_seconds >= 0 && lasts < DM_GEAR_MANA_FRAMES) v *= lasts / DM_GEAR_MANA_FRAMES;

        double uses = o[i].charges * fmax(1, fmax(o[i].frames, o[i].cooldown));
        if (o[i].charges > 0 && uses < DM_GEAR_MANA_FRAMES) v *= uses / DM_GEAR_MANA_FRAMES;
        if (v > best) { best = v; *best_id = ids[i]; }
    }

    double aura, struck;
    if (!dm_sim_grant_dps(d, c, &pack, &aura, &struck)) best += aura * DM_GEAR_AREA_HITS + struck;
    return best;
}


static int measure_targets(const dm_sim_data *d, const dm_sim_char *c, dm_sim_target *std) {
    if (!d || !d->ok || !c || c->act < 1 || c->act > 5 || dm_sim_target_act(d, c, std) < 0) return -1;
    return dm_gear_boss_life(d, c->diff, c->act);
}

static int measure_survival(const dm_sim_data *d, const dm_sim_char *c, dm_gear_measures *m) {
    dm_sim_target std;
    if (measure_targets(d, c, &std) < 0) return -1;
    M_T0; M_ADD(0);
    dm_sim_rot_out r; dm_sim_surv_out s;
    if (dm_sim_fight(d, c, &std, &r, &s, NULL) || s.ttd == -2) return -1;
    M_ADD(1);
    double cycle = std.cycle > 0 ? std.cycle : 30;
    m->survival_s = s.ttd < 0 ? DM_SIM_HORIZON / (double)DM_SIM_FPS : s.ttd;


    m->hits = s.ttd < 0 ? DM_SIM_HORIZON / cycle : s.hits_to_death - s.spare;
    m->recovery = s.in_dps > 0 ? 100 * s.sustain / s.in_dps : 0;
    return 0;
}

static int measure_damage(const dm_sim_data *d, const dm_sim_char *c, int splash_on, dm_gear_measures *m) {
    dm_sim_target std;
    int boss = measure_targets(d, c, &std);
    if (boss < 0) return -1;
    M_T0; M_ADD(0);
    m->group = group_damage(d, c, &std, splash_on, &m->skill);
    M_ADD(2);
    dm_sim_target lone = std; lone.hp = boss; lone.regen = 0;
    dm_sim_dealt_out dealt;
    if (dm_sim_dealt(d, c, &lone, DM_GEAR_SINGLE_FRAMES, &dealt)) return -1;
    m->single = dealt.life * DM_SIM_FPS / dealt.frames;
    M_ADD(3);
    return 0;
}


int dm_gear_measure_survival(const dm_sim_data *d, const dm_sim_char *c, dm_gear_measures *m) {
    if (!m) return -1;
    return measure_survival(d, c, m);
}
int dm_gear_measure_damage(const dm_sim_data *d, const dm_sim_char *c, int splash_on, dm_gear_measures *m) {
    if (!m) return -1;
    return measure_damage(d, c, splash_on, m);
}
int dm_gear_measure(const dm_sim_data *d, const dm_sim_char *c, int splash_on, dm_gear_measures *m) {
    if (!m) return -1;
    memset(m, 0, sizeof *m);
    return measure_survival(d, c, m) || measure_damage(d, c, splash_on, m) ? -1 : 0;
}

static double percent(double now, double after) { return now > 0 ? 100 * (after - now) / now : 0; }

double dm_gear_steps(double v) {
    double k = trunc(v / DM_GEAR_STEP + (v >= 0 ? 1e-9 : -1e-9));
    return k == 0 ? 0 : k * DM_GEAR_STEP;
}
double dm_gear_hits_key(double hits, double goal) {
    if (goal <= 0) return 0;
    return floor(fmin(hits, goal) * DM_GEAR_HITS_PER + 1e-9) / DM_GEAR_HITS_PER;
}


double dm_gear_damage_key(const dm_gear_measures *now, const dm_gear_measures *after, const dm_gear_goal *goal) {
    double b = goal->boss_share < 0 ? 0 : goal->boss_share > 1 ? 1 : goal->boss_share;
    return dm_gear_steps((1 - b) * percent(now->group, after->group) + b * percent(now->single, after->single));
}
static int level(double v) { double a = fabs(v); int n = a >= 25 ? 3 : a >= 10 ? 2 : a >= 2 ? 1 : 0; return v < 0 ? -n : n; }

void dm_gear_score(const dm_gear_measures *now, const dm_gear_measures *after, const dm_gear_goal *goal, dm_gear_score_out *o) {
    memset(o, 0, sizeof *o);
    o->change[DM_GEAR_M_GROUP] = percent(now->group, after->group);
    o->change[DM_GEAR_M_SINGLE] = percent(now->single, after->single);
    o->change[DM_GEAR_M_SURVIVAL] = percent(now->survival_s, after->survival_s);
    o->change[DM_GEAR_M_RECOVERY] = after->recovery - now->recovery;
    o->hits = percent(now->hits, after->hits);
    o->damage_base = now->group > 0 || now->single > 0;
    if (o->damage_base) o->damage = dm_gear_damage_key(now, after, goal);
    double target = goal->survival_hits;
    int below_now = target > 0 && now->hits < target, below_after = target > 0 && after->hits < target;
    o->survival = dm_gear_steps(0.5 * o->change[DM_GEAR_M_SURVIVAL] + 0.5 * o->change[DM_GEAR_M_RECOVERY]);
    if (o->damage_base) o->dots_damage = level(o->damage);
    if (!below_now && below_after) { o->goal = DM_GEAR_GOAL_MISSED; o->dots_survival = -3; }
    else if (below_now && !below_after) { o->goal = DM_GEAR_GOAL_REACHED; o->dots_survival = 3; }
    else if (below_now) o->dots_survival = level(dm_gear_steps(o->hits));
    else o->dots_survival = level(o->survival);
    o->key[0] = dm_gear_hits_key(after->hits, target);
    o->key[1] = o->damage_base ? o->damage : dm_gear_steps(o->change[DM_GEAR_M_SURVIVAL]);
    for (int k = 0; k < DM_GEAR_M_N; k++) o->key[2] += o->change[k] / DM_GEAR_M_N;
}


static int undo_group(int g, dm_sim_char *x, const dm_sim_char *now) {
#define TAKE(f) do { if (memcmp(&x->f, &now->f, sizeof x->f)) { memcpy(&x->f, &now->f, sizeof x->f); changed = 1; } } while (0)
    int changed = 0;
    switch (g) {
    case DM_GEAR_R_DEFENSE: TAKE(def); TAKE(block); TAKE(dodge); break;
    case DM_GEAR_R_LIFE: TAKE(life); break;
    case DM_GEAR_R_MANA: TAKE(mana); TAKE(manaregen_pct); break;
    case DM_GEAR_R_REGEN: TAKE(liferegen); break;
    case DM_GEAR_R_LEECH: TAKE(ll); TAKE(ml); break;
    case DM_GEAR_R_RESIST: TAKE(res); TAKE(maxres); TAKE(absorb_pct); TAKE(absorb_flat); TAKE(dr_flat); TAKE(mdr_flat); break;
    case DM_GEAR_R_CAST: TAKE(fcr); break;
    case DM_GEAR_R_ATTACK_SPEED: TAKE(ias); break;
    case DM_GEAR_R_HIT_RECOVERY: TAKE(fhr); TAKE(fbr); break;
    case DM_GEAR_R_SKILLS: TAKE(lvl); TAKE(ngrant); TAKE(grant); break;
    case DM_GEAR_R_DAMAGE:
        TAKE(str); TAKE(dex); TAKE(ed); TAKE(nhands); TAKE(hand); TAKE(ar); TAKE(ar_pct); TAKE(ar_bonus); TAKE(damage_mastery_delta); TAKE(ds); TAKE(cs); TAKE(cb); TAKE(ow);
        TAKE(mastery); TAKE(pierce); TAKE(dmg_demon); TAKE(dmg_undead); TAKE(shield_min); TAKE(shield_max); TAKE(kick_min); TAKE(kick_max);
        break;
    }
    return changed;
#undef TAKE
}

int dm_gear_reasons(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_measures *mnow, const dm_gear_measures *mafter,
                    int splash_on, dm_gear_reasons_out *o) {
    if (!d || !now || !after || !mnow || !mafter || !o) return -1;
    dm_gear_goal none = { 0, 0 };
    dm_gear_score_out full; dm_gear_score(mnow, mafter, &none, &full);
    for (int k = 0; k < DM_GEAR_M_N; k++) { o->group[k] = -1; o->share[k] = 0; }
    int runs = 0;
    for (int g = 0; g < DM_GEAR_R_N; g++) {
        dm_sim_char x = *after;
        if (!undo_group(g, &x, now)) continue;
        dm_gear_measures m;
        runs++;
        if (dm_gear_measure(d, &x, splash_on, &m)) continue;
        dm_gear_score_out part; dm_gear_score(mnow, &m, &none, &part);
        for (int k = 0; k < DM_GEAR_M_N; k++) {
            double share = full.change[k] - part.change[k];
            if (full.change[k] == 0 || share * full.change[k] <= 0 || fabs(share) <= fabs(o->share[k])) continue;
            o->group[k] = g; o->share[k] = share;
        }
    }
    return runs;
}



enum { GF_INT, GF_I16, GF_DBL };
typedef struct { int group, type; size_t off, size; } gear_field;
#define GF(g, f, t) { DM_GEAR_R_##g, t, offsetof(dm_sim_char, f), sizeof ((dm_sim_char *)0)->f }
static const gear_field k_fields[] = {
    GF(DEFENSE, def, GF_INT), GF(DEFENSE, block, GF_INT), GF(DEFENSE, dodge, GF_INT), GF(LIFE, life, GF_DBL), GF(MANA, mana, GF_DBL),
    GF(MANA, manaregen_pct, GF_INT), GF(REGEN, liferegen, GF_INT), GF(LEECH, ll, GF_INT), GF(LEECH, ml, GF_INT), GF(RESIST, res, GF_INT),
    GF(RESIST, maxres, GF_INT), GF(RESIST, absorb_pct, GF_INT), GF(RESIST, absorb_flat, GF_INT), GF(RESIST, dr_flat, GF_INT),
    GF(RESIST, mdr_flat, GF_INT), GF(CAST, fcr, GF_INT), GF(ATTACK_SPEED, ias, GF_INT), GF(HIT_RECOVERY, fhr, GF_INT), GF(HIT_RECOVERY, fbr, GF_INT),
    GF(SKILLS, lvl, GF_I16) };
#undef GF
static size_t field_step(const gear_field *f) { return f->type == GF_DBL ? sizeof(double) : f->type == GF_I16 ? sizeof(int16_t) : sizeof(int); }
static double field_at(const gear_field *f, const dm_sim_char *c, size_t i) {
    const char *p = (const char *)c + f->off + i * field_step(f);
    if (f->type == GF_DBL) { double v; memcpy(&v, p, sizeof v); return v; }
    if (f->type == GF_I16) { int16_t v; memcpy(&v, p, sizeof v); return v; }
    int v; memcpy(&v, p, sizeof v); return v;
}
enum { GEAR_BOTH_WAYS = 2 };

static int group_way(int g, const dm_sim_char *now, const dm_sim_char *after) {
    int up = 0, down = 0;
    for (size_t k = 0; k < sizeof k_fields / sizeof *k_fields; k++) {
        const gear_field *f = &k_fields[k];
        if (f->group != g) continue;
        for (size_t i = 0; i < f->size / field_step(f); i++) { double a = field_at(f, now, i), b = field_at(f, after, i); up |= b > a; down |= b < a; }
    }
    return up && down ? GEAR_BOTH_WAYS : up ? 1 : down ? -1 : 0;
}

static void undo_way(int g, int w, dm_sim_char *x, const dm_sim_char *now) {
    for (size_t k = 0; k < sizeof k_fields / sizeof *k_fields; k++) {
        const gear_field *f = &k_fields[k];
        if (f->group != g) continue;
        size_t step = field_step(f);
        for (size_t i = 0; i < f->size / step; i++) {
            double a = field_at(f, now, i), b = field_at(f, x, i);
            if (w > 0 ? b > a : b < a) memcpy((char *)x + f->off + i * step, (const char *)now + f->off + i * step, step);
        }
    }
}


static int element_changes(const double x[DM_EL_N], const double y[DM_EL_N], int e) {
    double moved = 0; for (int k = 0; k < DM_EL_N; k++) moved += fabs(y[k] - x[k]);
    double own = fabs(y[e] - x[e]);
    return own > 0.0005 * fmax(x[e], y[e]) && own >= 0.25 * moved;
}
static void immune_shares(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_char *a, const dm_sim_target *t, int act, int out[DM_EL_N]) {
    int elemental[DM_EL_N] = { 0 }, ids[2 * DM_SIM_MAXBAR + 1], n = 0;
    ids[n++] = 0;
    for (int side = 0; side < 2; side++) {
        const dm_sim_char *x = side ? a : c;
        for (int i = 0; i < x->nbar; i++) { int id = x->bar[i], seen = 0; for (int j = 0; j < n; j++) seen |= ids[j] == id; if (!seen) ids[n++] = id; }
    }
    for (int i = 0; i < n; i++) {
        int id = ids[i]; dm_sim_skill_out x = { 0 }, y = { 0 };
        if (id < 0 || id >= DM_SIM_MAXSKILL || !d->skill[id]) continue;
        if (c->lvl[id] && dm_sim_skill_eval(d, c, t, id, &x)) x.flags |= DM_SF_UNAVAIL;
        if (a->lvl[id] && dm_sim_skill_eval(d, a, t, id, &y)) y.flags |= DM_SF_UNAVAIL;
        if ((x.flags | y.flags) & (DM_SF_UNAVAIL | DM_SF_MODIFIER | DM_SF_UTILITY)) continue;
        for (int e = DM_EL_FIRE; e <= DM_EL_MAG; e++) elemental[e] |= element_changes(x.dps_element, y.dps_element, e);
    }
    dm_sim_rot_out x = { 0 }, y = { 0 };
    if (!dm_sim_rotation(d, c, t, &x) && !dm_sim_rotation(d, a, t, &y) && !((x.flags | y.flags) & DM_SF_UNAVAIL))
        for (int e = DM_EL_FIRE; e <= DM_EL_MAG; e++) elemental[e] |= element_changes(x.dps_element, y.dps_element, e);
    for (int e = 0; e < DM_EL_N; e++) out[e] = act > 0 && elemental[e] ? t->immune_share[e] : 0;
}

int dm_gear_recover_frames(const dm_sim_data *d, const char *token, const char *wclass, int block, int holy, int v) {
    const char *w = wclass && wclass[0] ? wclass : "HTH";
    for (int i = 0; i < d->nanim; i++) {
        const dm_sim_anim *a = &d->anim[i];
        if (!strcmp(a->token, token) && !strcmp(a->mode, block ? "BL" : "GH") && !strcmp(a->wclass, w))
            return a->frames > 0 ? dm_sim_frames_recover(a->frames, a->speed, v, block && holy ? 100 : 50) : -1;
    }
    return -1;
}


static int skill_frames(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, int id, int cast, int v) {
    dm_sim_char x = *c;
    if (cast) x.fcr = v; else x.ias = v;
    dm_sim_skill_out o;
    if (dm_sim_skills_prepared(d, &x, t, 1, &id, &o) || o.flags & (DM_SF_UNAVAIL | DM_SF_NOWEAPON | DM_SF_LENGTH | DM_SF_SEQ) || o.charges > 0 || o.frames <= 0) return -1;
    return o.frames;
}

static int next_step(int v, int top, int frames, int (*f)(const void *, int), const void *arg) {
    if (v >= top) return 0;
    int r = f(arg, top);
    if (r < 0 || r >= frames) return 0;
    int lo = v, hi = top;
    while (hi - lo > 1) { int mid = lo + (hi - lo) / 2, x = f(arg, mid); if (x >= 0 && x < frames) hi = mid; else lo = mid; }
    return hi;
}
typedef struct { const dm_sim_data *d; const dm_sim_char *c; const dm_sim_target *t; int id, cast; } main_arg;
static int main_at(const void *p, int v) { const main_arg *a = p; return skill_frames(a->d, a->c, a->t, a->id, a->cast, v); }
typedef struct { const dm_sim_data *d; const char *token, *wclass; int block, holy; } recover_arg;
static int recover_at(const void *p, int v) { const recover_arg *a = p; return dm_gear_recover_frames(a->d, a->token, a->wclass, a->block, a->holy, v); }
static int holy_shield(const dm_sim_data *d, const dm_sim_char *c) {
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (d->skill[id] && !strcmp(d->skill[id]->aurastate, "holyshield")) return c->active_lvl[id] > 0;
    return 0;
}

enum { CAST_TOP = 200, SPEED_TOP = 1000 };
static void swap_frames(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_measures *mnow, dm_gear_tip *t) {
    memset(&t->main, 0, sizeof t->main); memset(&t->recover, 0, sizeof t->recover); memset(&t->block, 0, sizeof t->block);
    if (now->form || after->form || now->cls < 0 || now->cls > 7 || now->cls != after->cls) return;
    dm_sim_target std;
    int id = mnow->skill;
    if (id >= 0 && id < DM_SIM_MAXSKILL && d->skill[id] && dm_sim_target_act(d, now, &std) >= 0) {
        const dm_sim_skill *sk = d->skill[id];
        int cast = !strcmp(sk->anim, "SC"), attack = !cast && ((sk->fam & DM_FAM_WEAPON) || sk->useattackrate);
        if ((cast || (attack && now->nhands == 1 && after->nhands == 1)) && sk->rollback_param <= 0 && !sk->seqnum) {
            dm_gear_frames *f = &t->main;
            f->skill = id; f->cast = cast;
            f->before = skill_frames(d, now, &std, id, cast, cast ? now->fcr : now->ias);
            f->after = skill_frames(d, after, &std, id, cast, cast ? after->fcr : after->ias);
            f->ok = f->before > 0 && f->after > 0;
            int v = cast ? after->fcr : after->ias;
            if (f->ok && f->before == f->after && v != (cast ? now->fcr : now->ias)) {
                main_arg a = { d, after, &std, id, cast };
                f->next = next_step(v, cast ? CAST_TOP : SPEED_TOP, f->after, main_at, &a);
            }
        }
    }
    if (now->nhands != 1 || after->nhands != 1) return;
    const char *token = d->cls[now->cls].token;
    for (int k = 0; k < 2; k++) {
        dm_gear_frames *f = k ? &t->block : &t->recover;
        if (k && (now->block <= 0 || after->block <= 0)) continue;
        int vn = k ? now->fbr : now->fhr, va = k ? after->fbr : after->fhr;
        f->before = dm_gear_recover_frames(d, token, now->hand[0].wclass, k, k && holy_shield(d, now), vn);
        f->after = dm_gear_recover_frames(d, token, after->hand[0].wclass, k, k && holy_shield(d, after), va);
        f->ok = f->before > 0 && f->after > 0;
        if (f->ok && f->before == f->after && va != vn) {
            recover_arg a = { d, token, after->hand[0].wclass, k, k && holy_shield(d, after) };
            f->next = next_step(va, SPEED_TOP, f->after, recover_at, &a);
        }
    }
}

int dm_gear_tip_rate(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_goal *goal, int splash_on, dm_gear_tip *t) {
    if (!d || !now || !after || !goal || !t) return -1;
    memset(t, 0, sizeof *t);
    if (!dm_sim_grants_ok(d, now, &t->grant) || !dm_sim_grants_ok(d, after, &t->grant)) return DM_GEAR_RATE_GRANTS;
    if (dm_gear_measure(d, now, splash_on, &t->now) || dm_gear_measure(d, after, splash_on, &t->after)) return -1;
    dm_gear_score(&t->now, &t->after, goal, &t->score);
    if (dm_gear_reasons(d, now, after, &t->now, &t->after, splash_on, &t->reasons) < 0) return -1;


    dm_gear_goal none = { 0, 0 };
    dm_gear_measures way[DM_GEAR_R_N][2]; signed char done[DM_GEAR_R_N][2] = { { 0 } };
    for (int k = 0; k < DM_GEAR_M_N; k++) {
        int g = t->reasons.group[k];
        if (g < 0) continue;
        double change = t->score.change[k];
        int w = g == DM_GEAR_R_DAMAGE ? (change > 0 ? 1 : -1) : group_way(g, now, after);
        if (w == GEAR_BOTH_WAYS) {
            double best = 0; w = 0;
            for (int side = 0; side < 2; side++) {
                int s = side ? -1 : 1;
                if (!done[g][side]) { dm_sim_char x = *after; undo_way(g, s, &x, now); done[g][side] = dm_gear_measure(d, &x, splash_on, &way[g][side]) ? -1 : 1; }
                if (done[g][side] < 0) continue;
                dm_gear_score_out part; dm_gear_score(&t->now, &way[g][side], &none, &part);
                double share = change - part.change[k];
                if (share * change > 0 && fabs(share) > fabs(best)) { best = share; w = s; }
            }
        }
        t->dir[k] = w;
    }
    t->goal_hits = goal->survival_hits;
    dm_sim_target std;
    t->act = dm_sim_target_act(d, now, &std);
    immune_shares(d, now, after, &std, t->act, t->immune);
    swap_frames(d, now, after, &t->now, t);
    return 0;
}


static int percent_text(char out[32], double change, int comma) {
    snprintf(out, 32, "%+.1f %%", change);
    if (comma) { char *dot = strchr(out, '.'); if (dot) *dot = ','; }
    return strcmp(out + 1, comma ? "0,0 %" : "0.0 %") != 0;
}
#define DM_GEAR_CC "\xee\x81\xbe"
int dm_gear_tip_text(const dm_gear_tip *t, const dm_gear_words *w, int comma, char line[][DM_GEAR_LINEB], int max) {
    int n = 0;
    char last[DM_GEAR_LINEB - 8] = "";
    int hits = (int)lround(t->goal_hits);
    if (t->score.goal == DM_GEAR_GOAL_MISSED && n < max) { snprintf(line[n], DM_GEAR_LINEB, DM_GEAR_CC "1"); snprintf(line[n] + 4, DM_GEAR_LINEB - 4, w->missed, hits); n++; }
    if (t->score.goal == DM_GEAR_GOAL_REACHED && n < max) { snprintf(line[n], DM_GEAR_LINEB, DM_GEAR_CC "3"); snprintf(line[n] + 4, DM_GEAR_LINEB - 4, w->reached, hits); n++; }

    if (t->goal_hits > 0 && t->now.hits < t->goal_hits && t->after.hits < t->goal_hits && n < max) {
        char a[32], b[32], whole[32];
        snprintf(a, sizeof a, "%ld", lround(t->after.hits)); snprintf(b, sizeof b, "%ld", lround(t->now.hits)); snprintf(whole, sizeof whole, "%s", a);
        if (!strcmp(a, b)) { snprintf(a, sizeof a, "%.1f", t->after.hits); snprintf(b, sizeof b, "%.1f", t->now.hits);
            if (comma) { char *p = strchr(a, '.'); if (p) *p = ','; p = strchr(b, '.'); if (p) *p = ','; } }
        int dots = t->score.dots_survival;
        if (strcmp(a, b)) { snprintf(line[n], DM_GEAR_LINEB, DM_GEAR_CC "%c", dots > 0 ? '3' : dots < 0 ? '1' : '5'); snprintf(line[n] + 4, DM_GEAR_LINEB - 4, w->below, b, a); n++; }
        else if (level(t->score.damage)) { snprintf(line[n], DM_GEAR_LINEB, DM_GEAR_CC "5"); snprintf(line[n] + 4, DM_GEAR_LINEB - 4, w->below_same, whole); n++; }
    }
    for (int k = 0; k < DM_GEAR_M_N && n < max; k++) {
        if ((k == DM_GEAR_M_GROUP || k == DM_GEAR_M_SINGLE) && !t->score.damage_base) continue;
        double v = t->score.change[k];
        char p[32];
        if (!percent_text(p, v, comma)) continue;
        snprintf(line[n++], DM_GEAR_LINEB, DM_GEAR_CC "%c%s %s", v > 0 ? '3' : '1', w->measure[k], p);
        int g = t->reasons.group[k];
        if (g < 0 || !t->dir[k] || n >= max || (g == DM_GEAR_R_DAMAGE && (k == DM_GEAR_M_GROUP || k == DM_GEAR_M_SINGLE))) continue;
        char why[DM_GEAR_LINEB - 8];
        snprintf(why, sizeof why, t->dir[k] > 0 ? w->more : w->less, w->group[g]);
        if (!strcmp(why, last)) continue;
        snprintf(last, sizeof last, "%s", why);
        snprintf(line[n++], DM_GEAR_LINEB, DM_GEAR_CC "5%s", why);
    }
    return n;
}
