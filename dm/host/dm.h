

#pragma once
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif




#if defined(__ANDROID__) || defined(__OHOS__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define DM_PHONE 1
#else
#define DM_PHONE 0
#endif

#define MAX_MOD 8
#define DM_PRE_COMMIT_HOOKS 11
#define LOG(...) do { fprintf(stderr, "[dm] " __VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(...) do { LOG("FATAL " __VA_ARGS__); abort(); } while (0)

typedef struct { uint32_t st_name; uint8_t st_info, st_other; uint16_t st_shndx; uint64_t st_value, st_size; } Elf64_Sym;
typedef struct { int64_t d_tag; union { uint64_t d_val, d_ptr; } d_un; } Elf64_Dyn;
typedef struct { uint64_t r_offset, r_info; int64_t r_addend; } Elf64_Rela;
typedef struct { uint8_t *d; size_t n; } dm_blob;
#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffff)
enum { DT_INIT = 12, DT_RELA = 7, DT_RELASZ = 8, DT_JMPREL = 23, DT_PLTRELSZ = 2, DT_SYMTAB = 6, DT_STRTAB = 5,
       DT_INIT_ARRAY = 25, DT_INIT_ARRAYSZ = 27 };
enum { R_AARCH64_ABS64 = 257, R_AARCH64_GLOB_DAT = 1025, R_AARCH64_JUMP_SLOT = 1026, R_AARCH64_RELATIVE = 1027 };



typedef struct {
    char name[16];
    uint8_t *base;
    size_t size;
    uint64_t text_end, ro_off, data_off, patch_off, key_page_off, key_page_size;
    unsigned key_idx[8];
    const Elf64_Dyn *dyn;
    const Elf64_Sym *symtab;
    const char *strtab;
    unsigned nsym;
    uint32_t *symh; unsigned symh_mask;
    uint64_t init, init_array, init_arraysz;
} module_t;

extern module_t g_mod[MAX_MOD];
extern int g_nmod;


extern pthread_key_t g_key18, g_keytls, g_keytlr, g_keygate;
extern pthread_key_t g_keyres[2], g_keyaspect, g_keyaspectui;

void load_modules(const char *dir, const char **names, int n);
void run_module_inits(void);
void *module_sym(module_t *m, const char *name);
void *guest_sym(const char *name);
module_t *module_of(const void *p);


uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs);


enum { OBS_UIMSG = 1, OBS_TICK = 2, OBS_C2S = 3, OBS_C2S_SYS = 4, OBS_SETLANG = 5, OBS_TARGET_RANK = 7, OBS_UI_PROJECT = 8, OBS_KILL_CREDIT = 9, OBS_FREEZE_WORLD = 10, OBS_FREEZE_CLIENT = 11, OBS_FREEZE_ANIM = 12, OBS_FREEZE_RENDER = 13, OBS_TEXTBOX = 14, OBS_EVENT_MSG = 15, OBS_LEVEL_WARP = 16, OBS_QUEST_KILL = 17, OBS_NOTICE_INPUT = 18, OBS_ERR_SMOOTH = 19, OBS_MODEL_YAW = 20, OBS_TOOLTIP = 21, OBS_PRESET_SPAWN = 22, OBS_COMPARE_COLOUR = 23,
       OBS_ATTACK_ROLL_IN = 26, OBS_ATTACK_ROLL_OUT = 27, OBS_MELEE_RESULT_IN = 28, OBS_MELEE_RESULT_OUT = 29, OBS_MISSILE_AVOID = 32, OBS_MELEE_AVOID = 106,

       OBS_COMBAT_SKILL_IN = 51, OBS_COMBAT_SKILL_OUT = 52, OBS_COMBAT_PROC_STACK_IN = 54, OBS_COMBAT_PROC_STACK_OUT = 55, OBS_COMBAT_PROC_IN = 56, OBS_COMBAT_PROC_OUT = 57,
       OBS_COMBAT_STATE_IN = 58, OBS_COMBAT_STATE_OUT = 59, OBS_COMBAT_UNKNOWN = 60, OBS_COMBAT_MISSILE_STACK_IN = 61, OBS_COMBAT_MISSILE_STACK_OUT = 62,
       OBS_COMBAT_MISSILE_SKILL_IN = 63, OBS_COMBAT_MISSILE_SKILL_OUT = 64, OBS_COMBAT_MISSILE_HIT_IN = 65, OBS_COMBAT_MISSILE_HIT_OUT = 66,
       OBS_COMBAT_MISSILE_AREA_IN = 67, OBS_COMBAT_MISSILE_AREA_OUT = 68, OBS_COMBAT_ABYSS_AREA_IN = 97, OBS_COMBAT_ABYSS_AREA_OUT = 98,
       OBS_MISSILE_DO_IN = 107, OBS_MISSILE_DO_OUT = 108,
       OBS_COMBAT_MON_REGEN = 39,
       OBS_GOLD_PICKUP = 109, OBS_GAMBLE_ITEM = 110,
       OBS_SAVE_SERIALIZE = 111,
       OBS_PRODUCT_MAX = 112 };
#ifdef DM_PLAYER_BUILD
enum { OBS_MAX = OBS_PRODUCT_MAX };
#else
enum {
    OBS_COMBAT_RES_IN = 24, OBS_COMBAT_RES_OUT = 25,
    OBS_COMBAT_MISSILE_IN = 30, OBS_COMBAT_MISSILE_OUT = 31,
    OBS_COMBAT_OW_ROLL = 33, OBS_COMBAT_OW_APPLY = 34,
    OBS_COMBAT_CB_IN = 35, OBS_COMBAT_CB_OUT = 36, OBS_COMBAT_CB_ROLL = 37, OBS_COMBAT_CB_LIFE = 38,
    OBS_COMBAT_UNIT_REGEN = 40,
    OBS_COMBAT_OW_IN = 41, OBS_COMBAT_OW_OUT = 42,
    OBS_COMBAT_POISON_IN = 43, OBS_COMBAT_POISON_REFRESH = 44, OBS_COMBAT_POISON_NEW = 45,
    OBS_COMBAT_POISON_DONE = 46, OBS_COMBAT_BURN_IN = 47,
    OBS_COMBAT_BURN_REFRESH = 48, OBS_COMBAT_BURN_NEW = 49, OBS_COMBAT_BURN_OUT = 50,
    OBS_COMBAT_SUMMON = 53,
    OBS_COMBAT_CHANCE_ROLL = 69, OBS_COMBAT_BLOCK_ROLL = 70, OBS_COMBAT_MISSILE_BLOCK_ROLL = 71,
    OBS_SIM_STREAM_RANGE = 72, OBS_SIM_GROUND_LEVEL = 73, OBS_SIM_GROUND_RANGE = 74, OBS_SIM_AURA_RANGE = 75,
    OBS_COMBAT_REACTION_IN = 76, OBS_COMBAT_REACTION_OUT = 77, OBS_SIM_DESCRIPTION_DISPATCH = 78,
    OBS_COMBAT_ANIMATION_IN = 79, OBS_COMBAT_ANIMATION_ACTION1 = 80, OBS_COMBAT_ANIMATION_ACTION2 = 81,
    OBS_COMBAT_ANIMATION_END = 82, OBS_COMBAT_ANIMATION_OUT = 83, OBS_COMBAT_ANIMATION_TAIL = 84,
    OBS_COMBAT_DEATH_IN = 85, OBS_COMBAT_DEATH_OUT = 86,
    OBS_COMBAT_PHYSICAL_IN = 87, OBS_COMBAT_PHYSICAL_BOUNDS = 88, OBS_COMBAT_PHYSICAL_OUT = 89,
    OBS_COMBAT_RESTART_IN = 90, OBS_COMBAT_RESTART_ACTION = 91, OBS_COMBAT_RESTART_TAIL = 92,
    OBS_COMBAT_SEQUENCE_IN = 93, OBS_COMBAT_SEQUENCE_ACTION1 = 94, OBS_COMBAT_SEQUENCE_ACTION2 = 95,
    OBS_COMBAT_SEQUENCE_TAIL = 96,
    OBS_COMBAT_DECISION_IN = 99, OBS_COMBAT_DECISION_CALL0 = 100, OBS_COMBAT_DECISION_CALL1 = 101,
    OBS_COMBAT_DECISION_CALL2 = 102, OBS_COMBAT_DECISION_OUT = 103,
    OBS_COMBAT_SCHEDULE_FINITE = 104, OBS_COMBAT_SCHEDULE_PERSIST = 105,
    OBS_MAX = OBS_PRODUCT_MAX
};
void dm_combat_fixture_speed(uint8_t *base, uint64_t unit, int side);
void dm_test_give_stats_client(uint8_t *base);
int dm_test_peel_stats(uint8_t *b, uint64_t unit);
void dm_test_peel_register(uint32_t id, int q);
void dm_test_peel_client(uint8_t *base);
void dm_test_give_stats_server(uint8_t *base, uint64_t unit, const char *code);
void dm_combat_fixture_cast(uint64_t game, uint64_t player);
#endif
void gate_obs_set(int id, void (*fn)(const uint64_t *regs));
int gate_obs_present(int id);
void dm_quiet_difficulty_msg(int on);
int gate_svc_present(int num);
uint64_t now_ns(void);
uint64_t guest_ns(void);
int dm_detclock_enabled(void);
void dm_guest_clock_advanced(void);
void guest_clock_freeze(int on);
void guest_clock_freeze_for(int reason, int on);
void dm_guest_frame_tick(void);
int dm_nvn_ruler_truth(void);
uint64_t dm_nvn_frame_count(void);
uint64_t dm_nvn_sub_cpu_ns(void);
int dm_ingame_now(void);
void dm_3d_scale_feed(double iv_ms);
float dm_3d_scale_cur(void);
void dm_backtrace(uint64_t fp);
void crash_watch_progress(uint32_t tick, const char *stage);
void crash_watch_frame(void);
void crash_watch_pregame_done(void);
uint64_t crash_watch_progress_count(void);
void crash_dump_now(const char *why);


extern uint64_t g_t0_ns;
extern uint64_t g_proc_start_ns;
extern struct dm_flags { int thr, svc, fs, movie, dbg, imports, bootprof; } g_flag;


enum { PROF_FS_READ, PROF_WAIT_LOCK, PROF_WAIT_CV, PROF_WAIT_ADDR, PROF_WAIT_SYNC, PROF_SLEEP, PROF_MAIN_WAIT,
       PROF_GFX_PRESENT, PROF_GFX_SHADER, PROF_TACT, PROF_GFX_BCDEC, PROF_N };
void dm_prof_add(int id, uint64_t ns, uint64_t bytes);
void dm_prof_mark(const char *seg);
uint64_t dm_prof_ribbon_ns(void);
void *objmap_get(const void *guest); void objmap_put(const void *guest, void *host); void *objmap_del(const void *guest);
void hle_signal_system_event(void *g);
void hle_bind_system_event(void *g, int autoclear);


void dm_fault_hook_register(int (*fn)(void *addr, int is_write, void *uc));
void dm_guest_write_prep_register(void (*fn)(void *addr, size_t len));
void dm_res_report_register(void (*fn)(uint32_t tick, const char *why));
void (*dm_res_report_cb(void))(uint32_t tick, const char *why);
void dm_guest_write_prepare(void *addr, size_t len);


void dm_gfx_suspend(int on);
void dm_gfx_shot_at(long k, const char *path);
void dm_shot_written(const char *path, int ok);
void dm_host_shutdown(const char *why);
