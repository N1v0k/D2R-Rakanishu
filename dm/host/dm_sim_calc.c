

#include "dm_sim_core.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

enum { B_ADD, B_SUB, B_MUL, B_DIV, B_MOD, B_LT, B_GT, B_LE, B_GE, B_EQ, B_NE, B_AND, B_OR, B_MIN, B_MAX, B_AVG };
static const char *const k_bin[] = { "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "&&", "||", "min", "max", "avg" };
static const char *const k_var[DM_V_N] = {
    "lvl", "blvl", "ulvl", "edmn", "edmx", "edln", "edns", "edxs", "enma", "exma", "enms", "exms", "edma", "toht", "mana", "len", "rng", "pets",
    "mael", "madm", "math", "macr", "pnma", "pxma", "pnms", "pxms", "skpt", "usmc", "mps", "fnum",
    "clc1", "clc2", "clc3", "clc4", "clc5", "clc6", "clc7", "clc8", "clc9", "clc10",
    "ast1", "ast2", "ast3", "ast4", "ast5", "ast6",
    "pst1", "pst2", "pst3", "pst4", "pst5", "pst6", "pst7", "pst8", "pst9", "pst10", "pst11", "pst12", "pst13", "pst14" };

int dm_sim_calc_var_id(const char *name) {
    for (int i = 0; i < DM_V_N; i++) if (!strcmp(name, k_var[i])) return i;
    return -1;
}

static int emit(dm_sim_data *d, int op, int arg) {
    if (d->ncode + 2 > d->capcode) {
        int cap = d->capcode ? d->capcode * 2 : 4096;
        int32_t *p = realloc(d->code, (size_t)cap * sizeof *p);
        if (!p) return -1;
        d->code = p; d->capcode = cap;
    }
    d->code[d->ncode++] = op; d->code[d->ncode++] = arg;
    return 0;
}

int dm_sim_calc_compile(dm_sim_data *d, const char *pf) {
    if (!d || !pf || !strcmp(pf, "?")) return -2;
    int start = d->ncode, depth = 0;
    char tok[48];
    while (*pf) {
        while (*pf == ' ') pf++;
        if (!*pf) break;
        size_t n = strcspn(pf, " ");
        if (n >= sizeof tok) goto bad;
        memcpy(tok, pf, n); tok[n] = 0; pf += n;
        int op = -1, arg = 0, pops = 0, push = 1;
        char *end;
        errno = 0;
        long v = strtol(tok, &end, 10);
        if (errno || v < INT_MIN || v > INT_MAX) goto bad;
        if (*tok && *end == 0 && (tok[0] != '-' || tok[1])) { op = DM_OP_NUM; arg = (int)v; }
        else if (tok[0] == '@') {
            char *colon = strchr(tok, ':');
            int sk = (int)strtol(tok + 1, &end, 10), var = colon ? dm_sim_calc_var_id(colon + 1) : -1;
            if (!colon || end != colon || var < 0 || sk < 0 || sk >= DM_SIM_MAXSKILL) goto bad;
            op = DM_OP_XVAR; arg = sk << 8 | var;
        } else if (tok[0] == '#') {
            op = DM_OP_STAT; arg = (int)strtol(tok + 1, &end, 10);
            if (!tok[1] || *end || arg < 0 || arg >= 512) goto bad;
        } else if (!strcmp(tok, "neg")) { op = DM_OP_NEG; pops = 1; }
        else if (!strcmp(tok, "!")) { op = DM_OP_NOT; pops = 1; }
        else if (!strcmp(tok, "?:")) { op = DM_OP_TERN; pops = 3; }
        else if (!strcmp(tok, "LN")) { op = DM_OP_LN; pops = 3; }
        else if (!strcmp(tok, "DM")) { op = DM_OP_DM; pops = 3; }
        else {
            for (int i = 0; i < (int)(sizeof k_bin / sizeof *k_bin); i++) if (!strcmp(tok, k_bin[i])) { op = DM_OP_BIN; arg = i; pops = 2; }
            if (op < 0) {
                int var = dm_sim_calc_var_id(tok);
                if (var < 0) goto bad;
                op = DM_OP_VAR; arg = var;
            }
        }
        if (depth < pops) goto bad;
        depth += push - pops;
        if (depth > 60 || emit(d, op, arg)) goto bad;
    }
    if (depth != 1 || emit(d, DM_OP_END, 0)) goto bad;
    return start;
bad:
    d->ncode = start;
    return -2;
}

static int64_t level_ln(int64_t a, int64_t b, int64_t lvl) { return a + (lvl - 1) * b; }


static int64_t level_dm(int64_t a, int64_t b, int64_t lvl) {
    if (lvl < 1) return a;
    int64_t v = (110 * lvl / (lvl + 6)) * (b - a) / 100 + a;
    return b >= a ? (v > b ? b : v) : (v < b ? b : v);
}

int64_t dm_sim_calc_eval(const dm_sim_data *d, int prog, const dm_sim_calc_env *e, int *bad) {
    int local_bad=0;if(!bad)bad=&local_bad;
    if (!d || !e) { *bad=1;return 0; }
    if (prog < 0) { if (prog == -2 && bad) *bad = 1; return 0; }
    if (e->depth > 24) { if (bad) *bad = 1; return 0; }
    if (!d || !e || !e->var || prog >= d->ncode || prog % 2) { if (bad) *bad=1; return 0; }
    int64_t st[64]; int sp = 0;
    for (const int32_t *c = d->code + prog; c[0] != DM_OP_END; c += 2) {
        if (c+1 >= d->code+d->ncode) { if(bad)*bad=1;return 0; }
        int op=c[0], pops=op==DM_OP_BIN?2:op==DM_OP_TERN || op==DM_OP_LN || op==DM_OP_DM?3:op==DM_OP_NEG || op==DM_OP_NOT?1:0;
        if(sp<pops || sp>=63) {if(bad)*bad=1;return 0;}
        int64_t a, b, x;
        switch (c[0]) {
        case DM_OP_NUM: st[sp++] = c[1]; break;
        case DM_OP_VAR: st[sp++] = e->var(e, e->skill, c[1], bad); break;
        case DM_OP_XVAR: {
            dm_sim_calc_env sub = *e; sub.skill = c[1] >> 8; sub.depth = e->depth + 1;
            st[sp++] = e->var(&sub, sub.skill, c[1] & 255, bad); break;
        }
        case DM_OP_STAT: st[sp++] = e->stat ? e->stat(e, c[1]) : 0; break;
        case DM_OP_NEG: st[sp - 1] = -st[sp - 1]; break;
        case DM_OP_NOT: st[sp - 1] = !st[sp - 1]; break;
        case DM_OP_TERN: x = st[sp - 3]; a = st[sp - 2]; b = st[sp - 1]; sp -= 3; st[sp++] = x ? a : b; break;
        case DM_OP_LN: case DM_OP_DM:
            a = st[sp - 3]; b = st[sp - 2]; x = st[sp - 1]; sp -= 3;
            st[sp++] = c[0] == DM_OP_LN ? level_ln(a, b, x) : level_dm(a, b, x); break;
        case DM_OP_BIN:
            a = st[sp - 2]; b = st[sp - 1]; sp -= 2;
            switch (c[1]) {
            case B_ADD: x = a + b; break;
            case B_SUB: x = a - b; break;
            case B_MUL: x = a * b; break;
            case B_DIV: x = b ? a / b : 0; break;
            case B_MOD: x = b ? a % b : 0; break;
            case B_LT: x = a < b; break;
            case B_GT: x = a > b; break;
            case B_LE: x = a <= b; break;
            case B_GE: x = a >= b; break;
            case B_EQ: x = a == b; break;
            case B_NE: x = a != b; break;
            case B_AND: x = a && b; break;
            case B_OR: x = a || b; break;
            case B_MIN: x = a < b ? a : b; break;
            case B_MAX: x = a > b ? a : b; break;
            default: x = (a + b) / 2; break;
            }
            st[sp++] = x; break;
        default: if (bad) *bad = 1; return 0;
        }
        if(sp && (st[sp-1]>INT_MAX || st[sp-1]<INT_MIN)) { if(bad)*bad=1;return 0; }
        if(c+2>=d->code+d->ncode) {if(bad)*bad=1;return 0;}
    }
    return sp == 1 ? st[0] : 0;
}
