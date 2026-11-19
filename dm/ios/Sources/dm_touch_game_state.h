#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DM_TG_FACETS=6, DM_TG_SLOTS=12, DM_TG_STRIDE=28,
       DM_TG_MAX_AGE_NS=250000000,
       DM_TG_IDENTITY_UNCONFIRMED=0, DM_TG_IDENTITY_NATIVE=1, DM_TG_IDENTITY_SYNTHETIC=2,
       DM_TG_SOURCE_NONE=0, DM_TG_SOURCE_NATIVE_GETTER=1 };
typedef struct { uint32_t logical, normal, alt; } dm_tg_mapping;
typedef struct {
    uint64_t buttons;
    uint32_t raw_skill_id;
    int slot, valid;
} dm_tg_portal;
typedef struct {
    uint64_t captured_ns, group_started_ns, scene_epoch, world_epoch, binding_identity, caller_identity, hud_identity;
    int ready, secondary_group, identity_scope;
    uint32_t group_seen;

    int supported, valid, source, selected_bank, getter_input_slot, observed_index;
    dm_tg_mapping mapping[DM_TG_FACETS];
    uint32_t raw_skill_id[DM_TG_SLOTS];
    dm_tg_portal portal;
} dm_tg_snapshot;

int dm_tg_parse(dm_tg_snapshot *out, const void *mapping, size_t mapping_bytes,
                const void *bank, size_t bank_bytes, int selected_bank, int getter_input_slot,
                int observed_index, uint32_t observed_id, uint64_t captured_ns);
typedef struct { dm_tg_snapshot pending, completed; uint32_t seen; uint64_t started_ns; } dm_tg_group;

int dm_tg_group_observe(dm_tg_group *group, const dm_tg_snapshot *table, int game,
                        uint64_t scene_epoch, uint64_t caller_identity, uint64_t hud_identity, int identity_scope);

uint64_t dm_tg_portal_mask(const dm_tg_snapshot *snapshot, uint64_t host_now_ns);

int dm_touch_game_attach(void);

void dm_touch_game_live_check(int on);
int dm_touch_game_snapshot(dm_tg_snapshot *out);
int dm_touch_game_skills(int *out, int capacity);
void dm_touch_game_set_scene(int game_no_panel, uint64_t scene_epoch, uint64_t world_epoch);
#ifdef DM_TG_TEST_SYNTHETIC_IDENTITY

int dm_touch_game_test_identity(const uint64_t *regs, uint64_t *caller_identity, uint64_t *hud_identity);
#endif
#ifdef __cplusplus
}
#endif
