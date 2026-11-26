





#pragma once
#include "dm.h"
#include "game_addrs.h"
#include "dm_sect.h"
typedef struct { const char *name; void *fn; } hle_ent;
#define HLE(mangled, ret, cname, ...)                                                             \
    static ret cname(__VA_ARGS__);                                                                \
    DM_SECT(swhle) static const hle_ent hle_ent_##cname = {mangled, (void *)cname}; \
    static ret cname(__VA_ARGS__)

typedef uint32_t Result;
#define RES_OK 0u
#define RES_FAIL 0x2A2u



void *objmap_get(const void *guest);
void objmap_put(const void *guest, void *host);
void *objmap_del(const void *guest);
uint64_t now_ns(void);
uint64_t guest_ns(void);
int dm_detclock_enabled(void);
void dm_guest_clock_advanced(void);
uint64_t dm_guest_frame(void);
uint64_t dm_server_tick(void);
void guest_clock_freeze(int on);
void guest_clock_freeze_for(int reason, int on);







typedef struct { const char *name; void *fn; const char *why; } hle_force_ent;
#define HLE_FORCE(mangled, why, ret, cname, ...)                                                   \
    static ret cname(__VA_ARGS__);                                                                \
    DM_SECT(swforce) static const hle_force_ent hle_force_##cname = {mangled, (void *)cname, why}; \
    static ret cname(__VA_ARGS__)



int dm_opmode(void);
void dm_3d_scale_hot(int hot);
ssize_t dm_data_pread(int fd, void *buf, size_t n, off_t off);
void dm_data_note_fd(int fd, const char *hostpath);
int dm_overlay_file(const char *rel, char *out, size_t n);
void dm_set_opmode(int m);


void dm_focus_set(int st);


void dm_audio_suspend(int on);
uint64_t dm_audio_render_calls(void);




//   0x80   TACT BLTE 块 zlib 解码 → libdeflate           hle_tact.c      GA_TACT_DECODE_ZLIB（DecodeZlib 入口）
//   0x81   客户端命令包投递（收包泵 → 服务端收包）         diag.c          GA_AT_RECV_CMD_CALL（bl GA_C2S_RECV 调用点；缺省原样转调，DM_C2S_REPLAY 时丢弃）
//   0x82   系统类客户端消息投递（收包泵 → GA_C2S_SYS_HANDLE）       diag.c          GA_AT_RECV_SYS_MSG_CALL（bl GA_C2S_SYS_HANDLE 调用点；缺省原样转调，DM_C2S_REPLAY 时文件首拍起丢弃）
//   0x83   Granny 段逐块解码（段读取 GA_GRANNY_SECTION_READ）           hle_granny.c    GA_AT_GRANNY_SECTION_DECODE（bl GA_GRANNY_BLOCK_DECODE 调用点；缺省按预解码候选替换，核不上即原样转调）
//   0x84   Granny 修正表逐块解码（段读取 GA_GRANNY_FIXUP_READ）       hle_granny.c    GA_AT_GRANNY_FIXUP_DECODE（bl GA_GRANNY_BLOCK_DECODE 调用点；同上）
//   0x85   私有技能 HUD 自绘查询（只读，写 x8）          hle_tact.c      GA_AT_SKILL_HUD_DRAW（分支门：x8≠0 跳 GA_AT_SKILL_HUD_DRAW_SKIP，否则执行原指令回 GA_AT_SKILL_HUD_DRAW_RET；仅触屏私有门）
//   0x86   私有技能地面方向（转调原 GA_SKILL_GROUND_DIR 再按右摇杆改向） hle_private_aim.c GA_AT_SKILL_AIM_DIR_CALL（bl GA_SKILL_GROUND_DIR 调用点；仅触屏私有门）
//   0x87   地上物品外观路径（转调原 GA_UNIT_MODEL_PATH 再按规则改指带光变体） dm_dropfilter.c GA_AT_ITEM_MODEL_PATH_CALL（bl GA_UNIT_MODEL_PATH 调用点；只改模式 3 / 5 的物品，输出在 x8 字符串）
//   0x88   物品落地掉落音（转调原 GA_ITEM_DROP_SOUND 再把一档物品换成提示音） dm_dropfilter.c GA_AT_DROP_SOUND_CALL（bl GA_ITEM_DROP_SOUND 调用点；写 x0 = 声音号）
//   0x89   秘境出口改道（秘境局里把目的地换成下一张图，写 x2 / x3 / x8） dm_rift.c      GA_LEVEL_WARP_TILE（分支门：x8≠0 跳 GA_LEVEL_CHANGE，否则执行原指令回 GA_AT_LEVEL_WARP_TILE_RET）
//   0x8a   进局给玩家取出生点（转调原 GA_LEVEL_SPAWN_POINT；进局在城镇时换成小站旁）  dm_townwp.c     GA_AT_JOIN_SPAWN_CALL（bl GA_LEVEL_SPAWN_POINT 调用点；玩家进局函数 0x5d7f70 里）
//   0x8b   进局给尸体取落点（同上，与玩家取同一个点）                      dm_townwp.c     GA_AT_JOIN_CORPSE_CALL（bl GA_LEVEL_SPAWN_POINT 调用点；同一函数里）
//   0x8c   秘境里存档的当前幕（人在秘境图里把 w9 换成 4 = 第五幕，写 x9；x8 恒 0） dm_rift.c      GA_AT_SAVE_HDR_DIFFICULTY（分支门：x8≠0 跳 GA_AT_SAVE_HDR_DIFFICULTY_RET，否则执行原指令回 GA_AT_SAVE_HDR_DIFFICULTY_RET）
//   0x8d   佣兵加装备位（佣兵 + 秘境 ≥10 层 + 腰带 / 鞋 / 手套 / 戒指 / 项链时把 w0 换成 1，写 x0；x8 恒 0） dm_rift.c      GA_AT_MERC_PLACE_HELM_TEST（分支门：x8≠0 跳 GA_AT_MERC_PLACE_HELM_TEST_RET，否则执行原指令回 GA_AT_MERC_PLACE_HELM_TEST_RET）
//   0x8e   佣兵加装备位的客户端校验（秘境 ≥10 层 + 物品是腰带 / 鞋 / 手套 / 戒指 / 项链时把 w1 换成物品自己的类型号，写 x1；x8 恒 0） dm_rift.c      GA_AT_MERC_WEAR_TYPE_ARG（分支门：x8≠0 跳 GA_AT_MERC_WEAR_TYPE_ARG_RET，否则执行原指令回 GA_AT_MERC_WEAR_TYPE_ARG_RET）
//   0x8f   原版画面往 3D 镜头前方挪（转调原 GA_CAM_SECOND_GET，再把镜头位置与偏移加上挪动量） dm_cam.c      GA_AT_CAM_SECOND_CAM_CALL（bl GA_CAM_SECOND_GET 调用点；镜头每帧更新 0x89c370 里）
//   0x90   提示条追加容量保护（最多 8 个内嵌文字控件） dm_padmode.c    GA_AT_LEGEND_APPEND_CALL（bl GA_LEGEND_APPEND 调用点；满时优先撤宿主可选提示）
//   0xf0 起留给本移植（安卓）自己的宿主调用，上游按 0x91、0x92… 往上排，合并时不撞号
//   0xf0   伤害数字（转调原 GA_STAT_SET，先读这一下伤害） dm_dmgnum.c      GA_AT_DAMAGE_LIFE_CALL（bl GA_STAT_SET 调用点；伤害函数 GA_DAMAGE_APPLY 里扣血那一下）
//   0xf1   近战溅射 A：记数构建里的算抗性调用点（设置开着时为玩家的命存储未算抗性的快照，再原样转调） dm_splash.c  GA_AT_MELEE_RECORD_RESIST_CALL（0x7db7d0 里 bl GA_COMBAT_REDUCE 调用点）
//   0xf2   近战溅射 B：命中执行末尾的结果落地调用点（先原样转调，再对身前锥形范围内的邻近怪逐只走 抗性→伤害→死亡） dm_splash.c  GA_AT_MELEE_EXEC_RESULT_CALL（0x7da000 里 bl GA_HIT_RESULT_APPLY 调用点）
//   0xf1   Melee splash A: the resistance call inside the record builder (snapshots the unreduced record of a player hit while the setting is on, then forwards) dm_splash.c  GA_AT_MELEE_RECORD_RESIST_CALL (bl GA_COMBAT_REDUCE call site in 0x7db7d0)
//   0xf2   Melee splash B: the result execution call at the end of the melee execute (forwards first, then splashes the cone of neighbours: resistance, damage, death each) dm_splash.c  GA_AT_MELEE_EXEC_RESULT_CALL (bl GA_HIT_RESULT_APPLY call site in 0x7da000)
//   0xf3   Shrine stacking and the stamina shrine's faster run/walk: entry of the state-with-duration applier the shrine handlers and curse skills end in (the gate replays the two displaced instructions, tail-calls the rest of the body and runs the rules around it; switches off = forward only) dm_shrine.c  GA_STATE_APPLY_TIMED (entry: svc #0xf3; ret)
//   0xf4   Item looks, 3D model: entry of the unit -> definition path lookup (the gate replays the two displaced instructions and runs the body on a copy of the item header carrying the chosen look; no look = forward only) dm_looks.c  GA_UNIT_MODEL_PATH (entry: svc #0xf4; ret)
//   0xf5   Item looks, inventory picture: entry of the item -> picture lookup (same gate) dm_looks.c  GA_ITEM_ICON (entry: svc #0xf5; ret)
//   0xf6   Item colour: entry of the item -> colour lookup the figure, the inventory picture and the world ask (the gate replays the two displaced instructions; an owned item with a chosen colour or a look gets that colour, every other unit the original) dm_looks.c  GA_ITEM_COLOUR (entry: svc #0xf6; ret)
//   0xf8   Unsocketing, inserts beyond the cube: the runner's free-cell search for a returned insert (forward; in a lent run with inserts for the inventory, the cube first, then the inventory page, and x26 names the page the game then places on) dm_unsocket.c  GA_AT_TRANSMUTE_INSERT_FIND_CELL (bl GA_INV_FIND_FREE call site)
//   0xf9   Unsocketing, an insert the runner could not place: its deletion (forward; in a lent run the insert goes back into its item with the game's socketing instead) dm_unsocket.c  GA_AT_TRANSMUTE_INSERT_DELETE (bl GA_ITEM_FREE call site)
//   0xf7   Unsocketing: entry of the transmute function (request check mode 0 and queued run mode 1; the gate replays the two displaced instructions, lends the socket-clearing row to the Ral recipe for that one call and sets the durability after the run; anything else = forward only) dm_unsocket.c  GA_CUBE_TRANSMUTE (entry: svc #0xf7; ret)


//   No.    Purpose                                      File            Cut (main offset)
//   0x80   TACT BLTE block zlib decode → libdeflate           hle_tact.c      GA_TACT_DECODE_ZLIB (DecodeZlib entry)
//   0x81   Client command packet delivery (receive pump → server receive)         diag.c          GA_AT_RECV_CMD_CALL (bl GA_C2S_RECV call site; forwarded as is by default, dropped under DM_C2S_REPLAY)
//   0x82   System-class client message delivery (receive pump → GA_C2S_SYS_HANDLE)       diag.c          GA_AT_RECV_SYS_MSG_CALL (bl GA_C2S_SYS_HANDLE call site; forwarded as is by default, dropped from the file's first tick on under DM_C2S_REPLAY)
//   0x83   Granny section per-block decode (section read GA_GRANNY_SECTION_READ)           hle_granny.c    GA_AT_GRANNY_SECTION_DECODE (bl GA_GRANNY_BLOCK_DECODE call site; by default replaced with the pre-decoded candidate, forwarded as is if it does not check out)
//   0x84   Granny fixup table per-block decode (section read GA_GRANNY_FIXUP_READ)       hle_granny.c    GA_AT_GRANNY_FIXUP_DECODE (bl GA_GRANNY_BLOCK_DECODE call site; same as above)
//   0x85   Private skill HUD self-draw query (read-only, writes x8)          hle_tact.c      GA_AT_SKILL_HUD_DRAW (branch gate: x8≠0 jumps to GA_AT_SKILL_HUD_DRAW_SKIP, otherwise executes the original instruction and returns to GA_AT_SKILL_HUD_DRAW_RET; touch-only private gate)
//   0x86   Private skill ground direction (forwards to the original GA_SKILL_GROUND_DIR, then redirects by the right stick) hle_private_aim.c GA_AT_SKILL_AIM_DIR_CALL (bl GA_SKILL_GROUND_DIR call site; touch-only private gate)
//   0x87   Ground item appearance path (forwards to the original GA_UNIT_MODEL_PATH, then repoints by rule to the lit variant) dm_dropfilter.c GA_AT_ITEM_MODEL_PATH_CALL (bl GA_UNIT_MODEL_PATH call site; only items in mode 3 / 5 are changed, output in the x8 string)
//   0x88   Item landing drop sound (forwards to the original GA_ITEM_DROP_SOUND, then swaps tier-one items to a notification sound) dm_dropfilter.c GA_AT_DROP_SOUND_CALL (bl GA_ITEM_DROP_SOUND call site; writes x0 = sound id)
//   0x89   Rift exit rerouting (in a rift run the destination is replaced with the next map, writes x2 / x3 / x8) dm_rift.c      GA_LEVEL_WARP_TILE (branch gate: x8≠0 jumps to GA_LEVEL_CHANGE, otherwise executes the original instruction and returns to GA_AT_LEVEL_WARP_TILE_RET)
//   0x8a   Spawn point for the player on run entry (forwards to the original GA_LEVEL_SPAWN_POINT; replaced with a point next to the waypoint when entering in town)  dm_townwp.c     GA_AT_JOIN_SPAWN_CALL (bl GA_LEVEL_SPAWN_POINT call site; in the player join function 0x5d7f70)
//   0x8b   Drop point for the corpse on run entry (same as above, the same point as the player gets)                      dm_townwp.c     GA_AT_JOIN_CORPSE_CALL (bl GA_LEVEL_SPAWN_POINT call site; in the same function)
//   0x8c   Current act saved while in a rift (when the character is in a rift map w9 is replaced with 4 = Act V, writes x9; x8 always 0) dm_rift.c      GA_AT_SAVE_HDR_DIFFICULTY (branch gate: x8≠0 jumps to GA_AT_SAVE_HDR_DIFFICULTY_RET, otherwise executes the original instruction and returns to GA_AT_SAVE_HDR_DIFFICULTY_RET)
//   0x8d   Extra mercenary equipment slots (mercenary + rift level ≥10 + belt / boots / gloves / ring / amulet: w0 is replaced with 1, writes x0; x8 always 0) dm_rift.c      GA_AT_MERC_PLACE_HELM_TEST (branch gate: x8≠0 jumps to GA_AT_MERC_PLACE_HELM_TEST_RET, otherwise executes the original instruction and returns to GA_AT_MERC_PLACE_HELM_TEST_RET)
//   0x8e   Client-side check for the extra mercenary equipment slots (rift level ≥10 + the item is a belt / boots / gloves / ring / amulet: w1 is replaced with the item's own type number, writes x1; x8 always 0) dm_rift.c      GA_AT_MERC_WEAR_TYPE_ARG (branch gate: x8≠0 jumps to GA_AT_MERC_WEAR_TYPE_ARG_RET, otherwise executes the original instruction and returns to GA_AT_MERC_WEAR_TYPE_ARG_RET)
//   0x8f   Original picture moved ahead of the 3D camera (forwards to the original GA_CAM_SECOND_GET, then adds the shift to the camera position and offset) dm_cam.c      GA_AT_CAM_SECOND_CAM_CALL (bl GA_CAM_SECOND_GET call site; in the per-frame camera update 0x89c370)
//   0x90   Legend append capacity guard (at most eight inline text widgets) dm_padmode.c    GA_AT_LEGEND_APPEND_CALL (bl GA_LEGEND_APPEND call site; evicts optional host hints first at capacity)
//   0xf0 and up are reserved for this port's (Android) own host calls; upstream counts up from 0x91, 0x92... so merges do not collide
//   0xf0   Damage numbers (forwards to the original GA_STAT_SET, reads the hit first) dm_dmgnum.c      GA_AT_DAMAGE_LIFE_CALL (bl GA_STAT_SET call site; life write of the damage function GA_DAMAGE_APPLY)
enum { SVC_HOST_TACT_ZLIB = 0x80, SVC_HOST_C2S_DELIVER = 0x81, SVC_HOST_C2S_SYS = 0x82, SVC_HOST_GRANNY_A = 0x83, SVC_HOST_GRANNY_B = 0x84, SVC_HOST_PRIVATE_SKILL_RENDER = 0x85, SVC_HOST_PRIVATE_SKILL_AIM = 0x86, SVC_HOST_GROUND_ASSET = 0x87, SVC_HOST_DROP_SOUND = 0x88, SVC_HOST_RIFT_EXIT = 0x89, SVC_HOST_JOIN_SPAWN = 0x8a, SVC_HOST_JOIN_CORPSE = 0x8b, SVC_HOST_SAVE_ACT = 0x8c, SVC_HOST_MERC_SLOT = 0x8d, SVC_HOST_MERC_SLOT_CL = 0x8e, SVC_HOST_CAM_AHEAD = 0x8f, SVC_HOST_LEGEND_APPEND = 0x90, SVC_HOST_DAMAGE = 0xf0, SVC_HOST_MELEE_RESIST = 0xf1, SVC_HOST_MELEE_RESULT = 0xf2, SVC_HOST_SHRINE_APPLY = 0xf3, SVC_HOST_LOOK_MODEL = 0xf4, SVC_HOST_LOOK_ICON = 0xf5, SVC_HOST_LOOK_COLOUR = 0xf6, SVC_HOST_TRANSMUTE = 0xf7, SVC_HOST_TRANSMUTE_SPILL = 0xf8, SVC_HOST_TRANSMUTE_KEEP = 0xf9 };   // 0x87：地上物品取模型路径（dm_dropfilter.c 掉落光）   // 0x83/0x84：Granny 逐块解码调用点（hle_granny.c） | 0x87: fetch the model path for ground items (dm_dropfilter.c drop light)   // 0x83/0x84: Granny per-block decode call sites (hle_granny.c)
