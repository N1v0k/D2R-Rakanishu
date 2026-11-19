





















import struct, re, sys, os
from itertools import combinations
import lz4.block
from capstone import *

src, outdir, NAME = sys.argv[1], sys.argv[2], sys.argv[3]
assert sys.argv[4:] in ([], ['--qa-combat']), 'unsupported rewrite option'
QA_COMBAT = sys.argv[4:] == ['--qa-combat']
d = open(src, 'rb').read()
assert d[:4] == b'NSO0'
flags = struct.unpack_from('<I', d, 0xc)[0]
segs = [struct.unpack_from('<III', d, 0x10 + i * 0x10) for i in range(3)]
bss = struct.unpack_from('<I', d, 0x3c)[0]
csz = struct.unpack_from('<III', d, 0x60)
IMG = bytearray(segs[2][1] + segs[2][2] + bss)
for i, (fo, mo, sz) in enumerate(segs):
    raw = d[fo:fo + csz[i]]
    if flags >> i & 1: raw = lz4.block.decompress(raw, uncompressed_size=sz)
    IMG[mo:mo + sz] = raw
TEXT_END = segs[0][2]
RO_OFF, DATA_OFF = segs[1][1], segs[2][1]
KEY_PAGE_OFF = (len(IMG) + 0x3FFF) & ~0x3FFF
IMG.extend(bytes(0x4000))
PATCH_OFF = (len(IMG) + 0xFFFF) & ~0xFFFF







CUTS = {'db0507f36b84200ac05f75d76ed5026f': [


    (0xaa9f88, '9c000094', '011200d4', '提示条追加 bl 0xaaa1f8 → svc #0x90：总数最多 8 条，优先撤宿主可选提示给原版操作留位，防第 9 个内嵌文字控件越界（dm_padmode.c）', '仅宿主'),


    (0x8524c4, '0808a2d2', '0800a4d2', '显存分配器初始化配置：mov x8,#0x10400000（260 MB）→ mov x8,#0x20000000（512 MB），随后 movk #0xa0,lsl48 与 str [sp,#0x238] 不变；'
     '原预算下内部分辨率 > 约 345 万像素时（2736x1260 过、2864x1240 崩）渲染目标放不下，main+0x174b018 返回空、main+0x17417a4 读空崩溃；'
     '只是显存预算，不进游戏逻辑：cvd5 逐拍对拍在 1600x900 与 2868x1320 两档均一致'),
    (0x1979bc, 'e8a79f1a', 'e8d79f1a', '离线放行窗口：读存档设置键（0x1d53e80 处字串）的上次在线时刻，'
     '「当前时刻 − 上次在线 < 0x0009356907420000 ns（= 30 天）」置位 [x20+0x2c]；lt→gt'),
    (0x5b14dc, 'e8a79f1a', 'e8d79f1a', '调 0xddab50（缓冲区 [x10+0x138]+x25、剩余长度、回调 0x527d50、表 0x209cdb8、9 项）返回值 <1 置位，'
     '为真时走 0x5b14f8 分支；lt→gt（业务含义未从字串定出）'),
    (0x5f3184, 'e8a79f1a', 'e8d79f1a', '数据表 [0x2d64a18+idx*16] → +0xf90 记录（0x1fc 字节）+0x4c 下标 → +0xfd0 记录（0x128 字节）+0xf1 有符号字节 <0 置位，'
     '与（w26>0x24）相与后作第 4 参传下游；lt→gt（业务含义未从字串定出）'),
    (0x1d53e80, '55736572204c617374204f6e6c696e6500', '566f6963654c616e677561676500000000',
     '只读字串 "User Last Online" → "VoiceLanguage"：第 1 处改读一个必定存在的设置键，「上次在线」取到小数值，差值恒超 30 天，配合 gt 恒放行'),


    (0x8019ec, '280f0034', 'e8feff34', '前端引导首次进入分支：cbz w8,#0x801bd0（LogoMenuPanel → Logo 视频 → 标题页）改为 cbz w8,#0x8019c8'
     '（落到直达路径的 0x23e5e0(1) → 0xb156a0 进主菜单；跳过 0x8019bc 起置联机模式 [0x2d7badc]=1 的两条，联机模式保持启动初值 0 = 离线，'
     '不触发 0x80ec44 的联网检查、断网弹窗与回标题页）；r4 版目标 0x8019bc 会置联机模式 1，实测仍回标题页并弹断网框'),


    (0x1b0ee60, 'fc6fbaa9fa6701a9', '011000d4c0035fd6', 'TACT BLTE 块 zlib 解码入口（DecodeZlib）→ svc #0x80 ; ret：宿主用 libdeflate 实现'
     '（hle_tact.c，调用号登记在 hle.h SVC_HOST_TACT_ZLIB）；改写器先打剪口再扫描，svc #0x80 自动成为宿主门，门块保存恢复 x30，ret 回原调用者', '仅宿主'),


    (0x5d3a88, 'a27a0194', '211000d4', '收包泵 0x5d3960 投递客户端命令包的调用点 bl 0x632510 → svc #0x81：宿主缺省以原参数原样调用 0x632510（返回值调用方不用）；'
     'DM_C2S_REPLAY 时丢弃客户端自发命令、改由拍入口按文件注入（diag.c，调用号登记在 hle.h SVC_HOST_C2S_DELIVER）', '仅宿主'),
    (0x5d39c8, 'ba750194', '411000d4', '收包泵 0x5d3960 投递系统类消息的调用点 bl 0x6310b0 → svc #0x82：宿主缺省以原参数原样调用 0x6310b0（返回值调用方不用）；'
     'DM_C2S_REPLAY 时，回放文件首拍起丢弃宿主客户端自发的系统类消息（之前的加入消息放行），改由拍入口按文件注入（diag.c，调用号登记在 hle.h SVC_HOST_C2S_SYS）', '仅宿主'),




    (0x211da30, '8007000038040000', '0010000070080000', '输出分辨率上限初值 1920x1080 → 4096x2160（数据段）：宿主按屏幕比例给的宽度（如 iPhone 19.5:9 的 1956x900）'
     '须不超上限才会被选中；Mac 前端仍给原值 1600x900 / 1280x720，行为不变', '仅宿主'),






    (0xe10e48, '09e880522903098a', 'e903192a1f2003d5', '运行模式通知发窗口尺寸事件的宽度：mov w9,#0x740; and x9,x25,x9 → mov w9,w25; nop（取低 32 位，不再用原版两档宽度推出的常数掩码）', '仅宿主'),
    (0xe12e88, '09e880528902098a', 'e903142a1f2003d5', '启动初始化选分辨率（0xe12d70）同一处掩码：mov w9,#0x740; and x9,x20,x9 → mov w9,w20; nop（structck 参数门下游掩码检查查出；启动时事件接收对象通常未建，平时不显形）', '仅宿主'),










    (0xb15e20, '0281002d', '1f2003d5', '经典画面切换写起点 / 目标（stp s2, s0, [x8, #4]）→ nop：目标恒为 1.0（新画面）'),
    (0xb15e24, '1f0d00b9', '1f2003d5', '经典画面切换清进度（str wzr, [x8, #0xc]）→ nop'),




    (0xb1a660, '083a8252', 'e8ff9f52', '界面画布宽上限 mov w8, #4560 → mov w8, #0xffff（画布宽 = 屏宽）', '仅宿主'),


















    (0x7f669c, '370b0036', '59000014', '字符串表装载 tbz w23, #0（是否装 legacy 表 A）→ b 0x7f6800：表 A 无读者（选表标志恒 0 → 表 B），整段 17 个 legacy 文件不装'),


    (0x11f02dc, '05120094', '611000d4', '段读取 0x11f0110 逐块解码 bl 0x11f4af0(ctx, 块, 长度) → svc #0x83：宿主缺省以原参数原样调用并回填 w0（hle.h SVC_HOST_GRANNY_A）', '仅宿主'),






    (0x468f2c, '3d1f0094', '011100d4', '物品落地取掉落音调用点 bl 0x470c20 → svc #0x88：宿主原样代调后按规则把一档物品的掉落音换成提示音（其余原样）', '仅宿主'),
    (0x8fa590, '0d6a0494', 'e11000d4', '物品外观路径调用点 bl 0xa14dc4 → svc #0x87：宿主原样代调后按规则改指地上物品的带光变体（存储 / 穿戴 / 腰带 / 镶嵌不碰）', '仅宿主'),




    (0x5d8040, 'a005fe97', '411100d4', '玩家进局取出生点调用点 bl 0x5596c0 → svc #0x8a：宿主原样代调，进局在城镇时出生点换成小站旁（hle.h SVC_HOST_JOIN_SPAWN）', '仅宿主'),
    (0x5d8400, 'b004fe97', '611100d4', '进局给尸体取落点调用点 bl 0x5596c0 → svc #0x8b：同上，尸体与人取同一个点（hle.h SVC_HOST_JOIN_CORPSE）', '仅宿主'),
    (0x11f0914, '77100094', '811000d4', '段读取 0x11f07d0 逐块解码 bl 0x11f4af0(ctx, 块, 长度) → svc #0x84：同上（hle.h SVC_HOST_GRANNY_B）', '仅宿主'),
]}




OBS = {'db0507f36b84200ac05f75d76ed5026f': [
    (0xb1dc60, 1, 'd10603ff', 'UI 消息总线虚函数实现入口（x1 = 消息 {u64 类型哈希, u64 动作哈希, char* 面板名, …}）：'
     'DM_UIMSG 消息流与 DM_BOOTPROF 启动分段的唯一来源；原指令 sub sp, sp, #0x180'),
    (0x5d3bc0, 2, 'd10503ff', '服务端拍入口（x0 = 游戏对象 G）：DM_TICKS 逐拍 6 格状态哈希（dm/verify/tick_state_format.md，与真值 swtick.py 同一采集点）；'
     '原指令 sub sp, sp, #0x140'),
    (0x6e5b40, 3, 'd10103ff', '客户端命令分派入口 (G, 单位, 包, 长度)：DM_C2S_LOG 命令包序列（dm/verify/command_packet_format.md，与真值同一采集点）；'
     '原指令 sub sp, sp, #0x40'),
    (0x6310b0, 4, 'a9bb67fc', '系统类客户端消息处理入口 (buf = u32 客户端号 + 包, 包长)：包号 ≥ 0x67（加入 / 心跳 0x6d / 0x6f 等），收包泵先抽干此队列；'
     'DM_C2S_LOG 记为 "q":"sys"；原指令 stp x28, x25, [sp, #-0x50]!'),
    (0x317298, 5, 'd10183ff', 'SetLanguage 入口（x1 → u32 新语言号，语言名表 main+0x20ae478）：配合上方语言重载剪口，宿主断言新语言为 zhCN；'
     '原指令 sub sp, sp, #0x60'),
]}
BUILD_ID = d[0x40:0x50].hex()


RULESET = os.environ.get('DM_RULESET', 'native')
assert RULESET in ('native', 'touch-run-v4'), f'未知规则域 {RULESET}'
RULE_BUILD = 'db0507f36b84200ac05f75d76ed5026f'
RULE_RUN = RULESET == 'touch-run-v4' and NAME == 'main'
if RULE_RUN:
    import hashlib
    assert BUILD_ID == RULE_BUILD, f'Run规则构建号不符 {BUILD_ID}'
    RULE_ORIG_SHA = hashlib.sha256(IMG).hexdigest()
cuts = list(CUTS.get(BUILD_ID, []))


PRIVATE_GATES = os.environ.get("DM_PRIVATE_GATES", "1") != "0"
PRIVATE_AIM = RULE_RUN and PRIVATE_GATES
if PRIVATE_AIM:
    cuts.append((0x2faf04, 'fde7fe97', 'c11000d4',
      'skill-aim-v1-private: skill ground direction call only; svc86 delegates original2b4ef8 then sampled-right/original rotation+grid; zero/nonplayer/non-skill retain original; busy/town/skill/server untouched', '仅宿主'))
if RULE_RUN:






    SHADOW_SIDE = 1024
    movz_w14 = lambda n: struct.pack('<I', 0x52800000 | (n << 5) | 14).hex()
    cuts.append((0x168a580, movz_w14(2100), movz_w14(SHADOW_SIDE), f'阴影图边长 mov w14,#2100 → mov w14,#{SHADOW_SIDE}（档位 1）', '仅宿主'))
if RULE_RUN:






    cbz_x8 = lambda at, to: struct.pack('<I', 0xB4000008 | (((to - at) >> 2) & 0x7ffff) << 5).hex()
    str_x19_178 = lambda rt: struct.pack('<I', 0xF9000000 | (0x178 // 8) << 10 | 19 << 5 | rt).hex()
    cuts.append((0xab5c28, str_x19_178(8), str_x19_178(31), '系统通知的确认动作类别 str x8,[x19,#0x178] → str xzr（不带确认动作）', '仅宿主'))
    cuts.append((0x3d853c, cbz_x8(0x3d853c, 0x3d8560), cbz_x8(0x3d853c, 0x3d880c), '通知面板：确认动作类别为空 → 不处理 PLUS', '仅宿主'))
    cuts.append((0x3d8544, cbz_x8(0x3d8544, 0x3d8560), cbz_x8(0x3d8544, 0x3d880c), '通知面板：确认动作哈希为空 → 不处理 PLUS', '仅宿主'))
if RULE_RUN:






    cuts.append((0xc35fb0, struct.pack('<I', 0xB3407D01).hex(), struct.pack('<I', 0xF2800000 | 3000 << 5 | 1).hex(), '技能说明框横坐标给到屏幕外，由原回夹贴右边（不盖技能树）', '仅宿主'))








    cuts.append((0x89c444, struct.pack('<I', 0x1E2C2000).hex(), struct.pack('<I', 0x1E282000).hex(), '镜头缩放下限比较：fcmp s0,s12（1.0）→ fcmp s0,s8（0.5）', '仅宿主'))
    cuts.append((0x89c428, struct.pack('<I', 0x52A7F008).hex(), struct.pack('<I', 0x52A7E008).hex(), '镜头缩放下限值：mov w8,#0x3f800000（1.0）→ #0x3f000000（0.5）', '仅宿主'))







    cuts.append((0x89ccc8, struct.pack('<I', 0x94362F2C).hex(), struct.pack('<I', 0xD40011E1).hex(), '镜头更新取第二个镜头调用点 bl 0x1628978 → svc #0x8f：宿主原样代调，镜头开着时把原版画面往 3D 镜头前方挪（hle.h SVC_HOST_CAM_AHEAD）', '仅宿主'))


    if os.environ.get('DM_TARGET') == 'android':
        cuts.append((0x5d428c, struct.pack('<I', 0x52927C09).hex(), struct.pack('<I', 0x529D4C09).hex(), 'autosave interval low half mov w9,#0x93e0 -> #0xea60: 60 s on Android', '仅宿主'))
        cuts.append((0x5d4290, struct.pack('<I', 0x72A00089).hex(), struct.pack('<I', 0x72A00009).hex(), 'autosave interval high half movk w9,#4,lsl #16 -> #0: 60 s on Android', '仅宿主'))


    cuts.append((0x7d6720, struct.pack('<I', 0x97f796b4).hex(), struct.pack('<I', 0xd4001e01).hex(), 'life write of the damage function bl 0x5bc1f0 -> svc #0xf0: host forwards the call as is and reads the hit for the damage numbers (hle.h SVC_HOST_DAMAGE)', '仅宿主'))





    cuts.append((0x7db850, '00e1ff97', '211e00d4',
      'melee record builder resistance call bl 0x7d3c50 -> svc #0xf1: host snapshots the unreduced record of a player hit on a monster and forwards the call (dm_splash.c, hle.h SVC_HOST_MELEE_RESIST)', '仅宿主'))
    cuts.append((0x7da4e0, 'e8f8ff97', '411e00d4',
      'melee execute result call bl 0x7d8880 -> svc #0xf2: host forwards the call and then splashes the hit to the monsters the cone in front of it reaches (dm_splash.c, hle.h SVC_HOST_MELEE_RESULT)', '仅宿主'))




    cuts.append((0x7b8280, 'ff4305d1e87300fd', '611e00d4c0035fd6',
      'curse/shrine apply entry main+0x7b8280 -> svc #0xf3; ret: host trampoline applies the shrine rules around the original body (dm_shrine.c, hle.h SVC_HOST_SHRINE_APPLY)', '仅宿主'))



    cuts.append((0xa14dc4, 'ffc304d1fc6b00f9', '811e00d4c0035fd6',
      'unit definition path entry main+0xa14dc4 -> svc #0xf4; ret: host draws an owned item with its chosen look (dm_looks.c, hle.h SVC_HOST_LOOK_MODEL)', '仅宿主'))
    cuts.append((0x45b460, 'ffc306d1fc6f15a9', 'a11e00d4c0035fd6',
      'item inventory picture entry main+0x45b460 -> svc #0xf5; ret: host draws an owned item with its chosen look (dm_looks.c, hle.h SVC_HOST_LOOK_ICON)', '仅宿主'))



    cuts.append((0x576f70, 'ffc301d1fc6f01a9', 'c11e00d4c0035fd6',
      'item colour entry main+0x576f70 -> svc #0xf6; ret: host colours an owned item with its chosen colour (dm_looks.c, hle.h SVC_HOST_LOOK_COLOUR)', '仅宿主'))
if RULE_RUN and PRIVATE_GATES:




    run_words = [
        (0x2c620c, 0xb90e511f, 0x52800029, '偏好初始化：mov w9,1（x8仍为2d20000）'),
        (0x2c6210, 0xd00152c8, 0xb90e5109, '偏好初始化：str w9,[x8,e50]；替代同页重复adrp，保原x8'),
        (0x369a90, 0xd100c3ff, None, '客户端RunOff入口尾跳原RunOn；保其player0/合法mode/环境门'),
        (0x369980, 0x340006a0, 0xd503201f, '客户端RunOn仅取消既有player0门之后的零精力拒绝'),
        (0x2b2c74, 0x340000a0, 0xd503201f, '控制器d29资格仅取消stat10零值限制；幅度比较与停杆保原'),
        (0x2b2dc0, 0x3900031f, 0xd503201f, '控制器d29仅取消stat10零值清除；幅度归一化保原'),
        (0x2b30fc, 0x0a090101, 0x52800021, '已通过合法动作/零轴门的移动编码选择位固定Run；幅度不改'),
        (0x2b3104, 0x2a1f03e1, 0x52800021, '无用户走跑偏好时亦固定Run编码位；方向/幅度不改'),
        (0x6dc2c4, 0x1a880502, 0x52800062, 'server42合法移动请求固定Run3，原type0/死亡/锁定/幅度/方向门保留'),
        (0x6dce90, 0x340000e0, 0xd503201f, 'server49仅取消既有player0/mode2门之后的零精力拒绝'),
        (0x6dcec0, 0xa9bf7bfd, None, 'server4A入口尾跳原49；长度/type0/mode门保留'),
    ]
    for off, old, new, why in run_words:
        if new is None:
            target = 0x369900 if off == 0x369a90 else 0x6dce50
            new = 0x14000000 | (((target - off) >> 2) & 0x3ffffff)
        cuts.append((off, struct.pack('<I', old).hex(), struct.pack('<I', new).hex(), 'touch-run-v4：' + why))
cuts = [c if len(c) == 5 else (*c, '全') for c in cuts]
for off, ob, nb, why, scope in cuts:
    ob, nb = bytes.fromhex(ob), bytes.fromhex(nb)
    assert len(ob) == len(nb) and IMG[off:off + len(ob)] == ob, f'剪口原字节不符 {off:#x}：{bytes(IMG[off:off + len(ob)]).hex()}'
    IMG[off:off + len(nb)] = nb


KEYS = {'K18': 508, 'KTLS': 509, 'KTLR': 510, 'KGATE': 511, 'KRES0': 506, 'KRES1': 507, 'KASPECT': 505, 'KASPECTUI': 504}
KEY_INDEX = {k: i for i, k in enumerate(('K18', 'KTLS', 'KTLR', 'KGATE', 'KRES0', 'KRES1', 'KASPECT', 'KASPECTUI'))}

md = Cs(CS_ARCH_ARM64, CS_MODE_ARM); md.detail = True
patch = bytearray(); fix = []; gates = []; stats = {}
def stat(k): stats[k] = stats.get(k, 0) + 1
def here(): return PATCH_OFF + len(patch)
def emit(w, kind=None):
    if kind: fix.append((len(patch), kind))
    patch.extend(struct.pack('<I', w & 0xFFFFFFFF))

def emit_key_addr(reg, kind):
    fix.append((len(patch), kind)); emit(ADRP(reg, here(), KEY_PAGE_OFF)); emit(LDRX(reg, reg, KEY_INDEX[kind]))

def emit_slot_addr(base, key_reg, kind):
    emit_key_addr(key_reg, kind); emit(ADDR(base, base, key_reg))



def B(frm, to):
    off = to - frm; assert -(1 << 27) <= off < (1 << 27) and off % 4 == 0
    return 0x14000000 | ((off >> 2) & 0x3FFFFFF)
def STP(t, t2, n, off): return 0xA9000000 | (((off // 8) & 0x7F) << 15) | (t2 << 10) | (n << 5) | t
def LDP(t, t2, n, off): return 0xA9400000 | (((off // 8) & 0x7F) << 15) | (t2 << 10) | (n << 5) | t
def STP_PRE(t, t2, n, off): return 0xA9800000 | (((off // 8) & 0x7F) << 15) | (t2 << 10) | (n << 5) | t
def LDP_POST(t, t2, n, off): return 0xA8C00000 | (((off // 8) & 0x7F) << 15) | (t2 << 10) | (n << 5) | t
def STUR(t, n, off): return 0xF8000000 | ((off & 0x1FF) << 12) | (n << 5) | t
def LDUR(t, n, off): return 0xF8400000 | ((off & 0x1FF) << 12) | (n << 5) | t
def LDRX(t, n, imm12=0): return 0xF9400000 | (imm12 << 10) | (n << 5) | t
def STRX(t, n, imm12=0): return 0xF9000000 | (imm12 << 10) | (n << 5) | t
def MRS_TPIDRRO(t): return 0xD53BD060 | t
def MRS_TPIDR(t): return 0xD53BD040 | t








TARGET = os.environ.get('DM_TARGET', 'apple')
assert TARGET in ('apple', 'android', 'ohos'), f'DM_TARGET 只能是 apple / android / ohos：{TARGET}'
OHOS_TSD_OFF = 272
def emit_tp(t):
    if TARGET == 'android': emit(MRS_TPIDR(t)); emit(LDUR(t, t, -8))
    elif TARGET == 'ohos': emit(MRS_TPIDR(t)); emit(SUBI(t, t, OHOS_TSD_OFF)); emit(LDRX(t, t))
    else: emit(MRS_TPIDRRO(t))









NOREDZONE = TARGET == 'android'
def STR_PRE(t, n, off): return 0xF8000C00 | ((off & 0x1FF) << 12) | (n << 5) | t
def LDR_POST(t, n, off): return 0xF8400400 | ((off & 0x1FF) << 12) | (n << 5) | t
def stash(S, T, nrz=None):
    emit(STP_PRE(S, T, 31, -16) if (NOREDZONE if nrz is None else nrz) else STP(S, T, 31, -16))
def unstash(S, T, nrz=None):
    emit(LDP_POST(S, T, 31, 16) if (NOREDZONE if nrz is None else nrz) else LDP(S, T, 31, -16))
def uses_sp(ins): return re.search(r'\bw?sp\b', ins.op_str) is not None
def ADRP(dd, pc, target):
    imm = ((target & ~0xFFF) - (pc & ~0xFFF)) >> 12; assert -(1 << 20) <= imm < (1 << 20)
    return 0x90000000 | ((imm & 3) << 29) | (((imm >> 2) & 0x7FFFF) << 5) | dd
def ADDI(dd, n, imm12, sh=0): return 0x91000000 | (sh << 22) | (imm12 << 10) | (n << 5) | dd
def ADDR(dd, n, m): return 0x8B000000 | (m << 16) | (n << 5) | dd
def SUBI(dd, n, imm12, sh=0): return 0xD1000000 | (sh << 22) | (imm12 << 10) | (n << 5) | dd
def BR(n): return 0xD61F0000 | (n << 5)
def BLR(n): return 0xD63F0000 | (n << 5)
NOP = 0xD503201F

def dis(w, addr):
    l = list(md.disasm(struct.pack('<I', w), addr)); assert len(l) == 1, hex(w)
    return l[0]
def regset(ins):
    r, w = ins.regs_access()
    return {ins.reg_name(x) for x in r}, {ins.reg_name(x) for x in w}
def num(name):
    m = re.fullmatch(r'[xw](\d+)', name or ''); return int(m.group(1)) if m else None
def has18(ins): return re.search(r'\b[xw]18\b', ins.op_str) is not None
def regs_in(ins):
    s = {int(n) for n in re.findall(r'\b[xw](\d+)\b', ins.op_str)}
    try:
        rd, wr = regset(ins); s |= {num(n) for n in rd | wr} - {None}
    except Exception: pass
    return s
def pick(avoid, n=2):
    out = [r for r in range(0, 16) if r not in avoid and r != 18][:n]; assert len(out) == n; return out

def subst(ins, S):

    w = struct.unpack('<I', ins.bytes)[0]
    want = re.sub(r'\b([xw])18\b', lambda m: f'{m.group(1)}{S}', ins.op_str)
    pos = [p for p in (0, 5, 10, 16) if (w >> p) & 31 == 18]
    for k in range(len(pos), 0, -1):
        for c in combinations(pos, k):
            nw = w
            for p in c: nw = (nw & ~(31 << p)) | (S << p)
            try: dd = dis(nw, ins.address)
            except AssertionError: continue
            if dd.mnemonic == ins.mnemonic and dd.op_str == want: return nw
    raise RuntimeError(f'subst fail {ins.address:x} {ins.mnemonic} {ins.op_str}')



def sp_delta(ins):

    m = re.search(r'\[sp, #(-?(?:0x)?[0-9a-f]+)\]!', ins.op_str)
    if m: return int(m.group(1), 0)
    m = re.search(r'\[sp\], #(-?(?:0x)?[0-9a-f]+)$', ins.op_str)
    if m: return int(m.group(1), 0)
    assert not re.match(r'(sp|wsp),', ins.op_str), f'writes sp: {ins.address:x} {ins.op_str}'
    return 0














SPOP = r'\[sp(?:, #-?(?:0x)?[0-9a-f]+)?\](?:!|, #-?(?:0x)?[0-9a-f]+)?|sp(?:, #(?:0x)?[0-9a-f]+(?:, lsl #12)?)?$'
def sp_frame(ins, w, dlt):
    if not uses_sp(ins): return 16, [w]
    F, rn = 16, (w >> 5) & 31


    def same(nw, off):
        d, o = dis(nw, ins.address), dis(w, ins.address)
        if d.mnemonic != o.mnemonic and (o.mnemonic, d.mnemonic) != ('mov', 'add'): return False
        return re.sub(SPOP, 'SP', d.op_str) == re.sub(SPOP, 'SP', o.op_str) and (f'#{off:#x}]' in d.op_str or d.op_str.endswith(f'sp, #{off:#x}'))
    if (w >> 27) & 7 == 0b101 and not (w >> 26) & 1 and rn == 31:
        opc, mode, L = w >> 30, (w >> 23) & 7, (w >> 22) & 1
        if opc == 3 or (opc == 1 and not L): return None
        sc = 8 if opc == 2 else 4
        imm = (((w >> 15) & 0x7F) ^ 0x40) - 0x40
        if mode == 2:
            a = imm * sc
            if a < 0: return None
        elif mode in (1, 3):
            a = dlt if mode == 3 else 0
            while a < 16 - F: F += 16
        else: return None
        off = a + F
        if off % sc or (F + dlt) % 8 or not -512 <= F + dlt <= 504: return None
        if mode == 2 and off // sc > 63:



            o = dis(w, ins.address); regs = o.op_str.split(', [')[0].split(', ')
            size, opc = (3 if opc == 2 else 2), ((2 if opc == 1 else 1) if L else 0)
            out = []
            for k, rt in enumerate((w & 31, (w >> 10) & 31)):
                nw = 0x39000000 | (size << 30) | (opc << 22) | (((off + k * sc) // sc) << 10) | (31 << 5) | rt
                d = dis(nw, ins.address)
                if not d.mnemonic.startswith(('ldr', 'str')) or d.op_str != f'{regs[k]}, [sp, #{off + k * sc:#x}]': return None
                out.append(nw)
            return F, out
        if not -64 <= off // sc <= 63: return None
        nw = (w & ~(0x7F << 15) & ~(7 << 23)) | (((off // sc) & 0x7F) << 15) | (2 << 23)
        return (F, [nw]) if same(nw, off) else None
    if dlt: return None
    if (w >> 27) & 7 == 0b111 and not (w >> 26) & 1 and rn == 31:
        size, sc = w >> 30, 1 << (w >> 30)
        if (w >> 24) & 3 == 1:
            if size == 3 and (w >> 22) & 3 == 2: return None
            off = ((w >> 10) & 0xFFF) * sc + F
            if off // sc > 0xFFF: return None
            nw = (w & ~(0xFFF << 10)) | ((off // sc) << 10)
            return (F, [nw]) if same(nw, off) else None
        if (w >> 24) & 3 == 0 and not (w >> 21) & 1 and (w >> 10) & 3 == 0:
            off = ((((w >> 12) & 0x1FF) ^ 0x100) - 0x100) + F
            if not F <= off <= 255: return None
            nw = (w & ~(0x1FF << 12)) | ((off & 0x1FF) << 12)
            return (F, [nw]) if same(nw, off) else None
        return None
    if w & 0xFF800000 == 0x91000000 and rn == 31 and (w & 31) != 31:
        if (w >> 22) & 1: return F, [w, ADDI(w & 31, w & 31, F)]
        off = ((w >> 10) & 0xFFF) + F
        if off > 0xFFF: return None
        nw = (w & ~(0xFFF << 10)) | (off << 10)
        return (F, [nw]) if same(nw, off) else None
    return None

def site_plain(ins):
    S, T = pick(regs_in(ins))
    dlt = sp_delta(ins)
    fr = sp_frame(ins, subst(ins, S), dlt) if NOREDZONE else None
    if fr:
        F, words = fr
        start = here()
        emit(STP_PRE(S, T, 31, -F)); emit_tp(T); emit_slot_addr(T, S, 'K18'); emit(LDRX(S, T))
        for x in words: emit(x)
        emit(STRX(S, T))
        emit(LDP_POST(S, T, 31, F + dlt)); emit(B(here(), ins.address + 4))
        return start
    if NOREDZONE: stat('below_sp')
    save = -16 if dlt == 0 else -112
    assert dlt == 0 or dlt >= -96, f'sp delta {dlt} at {ins.address:x}'
    start = here()
    emit(STP(S, T, 31, save)); emit_tp(T); emit_slot_addr(T, S, 'K18'); emit(LDRX(S, T))
    emit(subst(ins, S))
    emit(STRX(S, T))
    emit(LDP(S, T, 31, save - dlt)); emit(B(here(), ins.address + 4))
    return start

def site_cond(ins):
    w = struct.unpack('<I', ins.bytes)[0]
    S, T = pick(regs_in(ins))
    tgt = ins.operands[-1].imm
    start = here()
    stash(S, T); emit_tp(T); emit_slot_addr(T, S, 'K18'); emit(LDRX(S, T))
    m = ins.mnemonic
    taken = here() + 4 * 3
    off = (taken - here()) >> 2
    nw = ((w & 0xFF000000) | ((off & 0x7FFFF) << 5) | S) if m in ('cbz', 'cbnz') else ((w & 0xFFF80000) | ((off & 0x3FFF) << 5) | S)
    pc = here(); emit(nw)
    dd = dis(nw, pc); assert dd.mnemonic == m and dd.operands[-1].imm == taken
    unstash(S, T); emit(B(here(), ins.address + 4))
    assert here() == taken
    unstash(S, T); emit(B(here(), tgt))
    return start

def site_pcrel(ins):
    tgt = ins.operands[1].imm
    S, T = pick(set())
    start = here()
    stash(S, T); emit_tp(T); emit_slot_addr(T, S, 'K18')
    pc = here(); emit(ADRP(S, pc, tgt))
    if ins.mnemonic == 'adr': emit(ADDI(S, S, tgt & 0xFFF))
    emit(STRX(S, T))
    unstash(S, T); emit(B(here(), ins.address + 4))
    return start

def site_br(ins, base, count):

    start = here()
    if NOREDZONE: emit(STP_PRE(16, 17, 31, -16))
    else: emit(STUR(16, 31, -16)); emit(STUR(17, 31, -24))
    emit_tp(16); emit_slot_addr(16, 17, 'K18'); emit(LDRX(16, 16)); emit(LDRX(17, 31, 1) if NOREDZONE else LDUR(17, 31, -24))
    padbase = start + 4 * 24
    delta = padbase - base
    op = ADDI if delta >= 0 else SUBI; dl = abs(delta)
    ops = []; hi, lo = dl >> 12, dl & 0xFFF
    while hi: h = min(hi, 0xFFF); ops.append(op(16, 16, h, 1)); hi -= h
    if lo: ops.append(op(16, 16, lo, 0))
    assert len(ops) <= 12
    for o in ops: emit(o)
    emit(BR(16))
    while here() < padbase: emit(NOP)
    tails = padbase + 4 * count
    for i in range(count): emit(B(here(), tails + 8 * i))
    for i in range(count):
        emit(LDR_POST(16, 31, 16) if NOREDZONE else 0xF85F0000 | (31 << 5) | 16)
        emit(B(here(), base + 4 * i))
    return start

def site_sysreg_read(ins, kind):

    N = ins.bytes[0] & 31
    tmp = 16 if N != 16 else 17
    start = here()
    emit(STR_PRE(tmp, 31, -16) if NOREDZONE else STUR(tmp, 31, -16)); emit_tp(N); emit_slot_addr(N, tmp, kind); emit(LDRX(N, N)); emit(LDR_POST(tmp, 31, 16) if NOREDZONE else LDUR(tmp, 31, -16))
    emit(B(here(), ins.address + 4))
    return start

def site_tpidr_write(ins):
    N = struct.unpack('<I', ins.bytes)[0] & 31
    (T,) = pick({N}, 1)
    start = here()
    tmp = next(x for x in (16, 17) if x not in (N, T))
    if NOREDZONE:
        emit(STP_PRE(T, tmp, 31, -16)); emit_tp(T); emit_slot_addr(T, tmp, 'KTLS'); emit(STRX(N, T)); emit(LDP_POST(T, tmp, 31, 16))
        emit(B(here(), ins.address + 4))
        return start
    emit(0xF81F0000 | (31 << 5) | T)
    emit(STUR(tmp, 31, -24)); emit_tp(T); emit_slot_addr(T, tmp, 'KTLS'); emit(STRX(N, T)); emit(LDUR(tmp, 31, -24))
    emit(0xF85F0000 | (31 << 5) | T)
    emit(B(here(), ins.address + 4))
    return start

def emit_gate_call():








    emit_tp(30); emit_slot_addr(30, 29, 'KGATE'); emit(LDRX(30, 30)); emit(LDRX(29, 31))

def site_gate(ins, kind, arg):

    start = here()
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call()
    emit(BLR(30))
    ret = here()
    emit(LDP_POST(29, 30, 31, 16))
    emit(B(here(), ins.address + 4))
    gates.append((ret, kind, arg, ins.address))
    return start

def site_obs(off, oid, word):




    ins = dis(word, off)
    assert ins.mnemonic not in ('adr', 'adrp', 'b', 'bl', 'br', 'blr', 'ret', 'cbz', 'cbnz', 'tbz', 'tbnz') and not ins.mnemonic.startswith('b.') \
        and not (ins.mnemonic.startswith('ldr') and '[' not in ins.op_str) and not has18(ins), f'观测门原指令不可搬 {off:#x} {ins.mnemonic} {ins.op_str}'
    start = here()
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call()
    emit(BLR(30))
    ret = here()
    emit(LDP_POST(29, 30, 31, 16))
    emit(word)
    emit(B(here(), off + 4))
    gates.append((ret, 'obs', oid, off))
    return start



EXCL_LD = ('ldxr', 'ldaxr', 'ldxp', 'ldaxp', 'ldxrb', 'ldaxrb', 'ldxrh', 'ldaxrh')
EXCL_ST = ('stxr', 'stlxr', 'stxp', 'stlxp', 'stxrb', 'stlxrb', 'stxrh', 'stlxrh')
def find_windows(insl):
    out = []
    for i, ins in enumerate(insl):
        if ins.mnemonic not in EXCL_LD: continue
        j = i
        while j < len(insl) and insl[j].mnemonic not in EXCL_ST: j += 1
        if j >= len(insl) or j - i >= 16: continue
        if j + 1 < len(insl) and insl[j + 1].mnemonic in ('cbz', 'cbnz'): j += 1
        seg = insl[i:j + 1]
        if any(has18(x) for x in seg): out.append(seg)
    return out

def site_window(seg):
    used = set()
    for x in seg: used |= regs_in(x)
    S, T = pick(used)


    nrz = NOREDZONE and not any(uses_sp(x) for x in seg)
    if NOREDZONE and not nrz: stat('below_sp')
    lo, hi = seg[0].address, seg[-1].address + 4
    start = here()
    stash(S, T, nrz); emit_tp(T); emit_slot_addr(T, S, 'K18'); emit(LDRX(S, T))
    copy0 = here(); n = len(seg)
    exits = copy0 + 4 * n
    ex_targets = []; words = []
    for k, x in enumerate(seg):
        w = struct.unpack('<I', x.bytes)[0]
        if has18(x): w = subst(x, S)
        m = x.mnemonic
        if m in ('cbz', 'cbnz', 'tbz', 'tbnz') or m.startswith('b.'):
            tgt = x.operands[-1].imm
            if lo <= tgt < hi: newt = copy0 + (tgt - lo)
            else: ex_targets.append(tgt); newt = exits + 16 * len(ex_targets)
            pc = copy0 + 4 * k; off = (newt - pc) >> 2
            if m in ('tbz', 'tbnz'): w = (w & 0xFFF8001F) | ((off & 0x3FFF) << 5)
            else: w = (w & 0xFF00001F) | ((off & 0x7FFFF) << 5)
            dd = dis(w, pc); assert dd.operands[-1].imm == newt and dd.mnemonic == m
        else:
            assert m not in ('adr', 'adrp', 'b', 'bl', 'br', 'blr', 'ret') and not (m == 'ldr' and '[' not in x.op_str), hex(x.address)
        words.append(w)
    for w in words: emit(w)








    for t in [hi] + ex_targets:
        emit(STRX(S, T)); unstash(S, T, nrz); emit(B(here(), t)); emit(NOP)
    return start, lo, hi



m0 = struct.unpack_from('<I', IMG, 4)[0]
assert IMG[m0:m0 + 4] == b'MOD0'
ehs = m0 + struct.unpack_from('<i', IMG, m0 + 16)[0]
def rd(o, enc):
    fmt, app = enc & 0xF, enc & 0x70
    v, sz = {0x3: (struct.unpack_from('<I', IMG, o)[0], 4), 0xB: (struct.unpack_from('<i', IMG, o)[0], 4),
             0x4: (struct.unpack_from('<Q', IMG, o)[0], 8)}[fmt]
    if app == 0x10: v += o
    if app == 0x30: v += ehs
    return v, sz
_, penc, cenc, tenc = IMG[ehs:ehs + 4]
o = ehs + 4
_, s = rd(o, penc); o += s
nfde, s = rd(o, cenc); o += s
starts = set()
for _ in range(nfde):
    a, s = rd(o, tenc); o += s
    _, s = rd(o, tenc); o += s
    starts.add(a)
starts = sorted(x for x in starts if x < TEXT_END)



kinds = {}
for o in range(0, TEXT_END, 4):
    w = struct.unpack_from('<I', IMG, o)[0]
    if w & 0xFFE0001F == 0xD4000001: kinds[o] = ('svc', (w >> 5) & 0xFFFF); continue
    if w & 0xFFFFFFE0 == 0xD53BD040: kinds[o] = ('tpidr_r', None); continue
    if w & 0xFFFFFFE0 == 0xD51BD040: kinds[o] = ('tpidr_w', None); continue
    if w & 0xFFFFFFE0 == 0xD53BD060: kinds[o] = ('tpidrro_r', None); continue
    if w & 0xFFFFFFE0 == 0xD53BE020: kinds[o] = ('cntpct', w & 31); continue
    if w & 0xFFFFFFE0 == 0xD53BE040: kinds[o] = ('cntvct', w & 31); continue


    if not any((w >> p) & 31 == 18 for p in (0, 5, 10, 16)):
        continue
    for ins in md.disasm(IMG[o:o + 4], o):
        if has18(ins): kinds[o] = ('x18', None)



import bisect
def func_of(a):
    i = bisect.bisect_right(starts, a) - 1
    return (starts[i], starts[i + 1] if i + 1 < len(starts) else TEXT_END) if i >= 0 else (a, a + 4)
done = set(); windows = []; jumps = {}
funcs = sorted({func_of(a) for a, (k, _) in kinds.items() if k == 'x18'})
for fa, fb in funcs:
    insl = list(md.disasm(bytes(IMG[fa:fb]), fa))
    ws = find_windows(insl)
    for wseg in ws:
        windows.append(wseg); done |= {x.address for x in wseg}
    for j, ins in enumerate(insl):
        if ins.mnemonic == 'br' and ins.op_str == 'x18':
            base = width = None
            ctx = insl[max(0, j - 8):j]
            for p in reversed(ctx):
                if p.mnemonic == 'adr' and p.op_str.startswith('x18,'): base = p.operands[1].imm; break
            for p in reversed(ctx):
                if p.mnemonic == 'add' and p.op_str.startswith('x18, x18,') and 'lsl #2' in p.op_str:
                    idx = num(p.reg_name(p.operands[2].reg))
                    for q in reversed(ctx):
                        if q.mnemonic in ('ldrb', 'ldrsb') and num(q.reg_name(q.operands[0].reg)) == idx: width = 256
                        if q.mnemonic in ('ldrh', 'ldrsh') and num(q.reg_name(q.operands[0].reg)) == idx: width = 65536
                        if width: break
                    break
            assert base and width, f'jump table {ins.address:x}'
            jumps[ins.address] = (base, width)


    inwin = {x.address for wseg in ws for x in wseg}; heads = {wseg[0].address for wseg in ws}
    for x in insl:
        if x.mnemonic in ('b', 'bl', 'cbz', 'cbnz', 'tbz', 'tbnz') or x.mnemonic.startswith('b.'):
            t = x.operands[-1].imm
            assert not (t in inwin and t not in heads), f'branch into window {x.address:x}'

def put_branch(at, to): struct.pack_into('<I', IMG, at, B(at, to))
for wseg in windows:
    st, lo, hi = site_window(wseg); stat('window'); put_branch(lo, st)
for a in sorted(kinds):
    if a in done: continue
    k, arg = kinds[a]
    ins = dis(struct.unpack_from('<I', IMG, a)[0], a)
    if k == 'x18':
        m = ins.mnemonic
        if m == 'br': st = site_br(ins, *jumps[a]); stat('br')
        elif m in ('cbz', 'cbnz', 'tbz', 'tbnz'): st = site_cond(ins); stat('cond')
        elif m in ('adr', 'adrp'): st = site_pcrel(ins); stat('pcrel')
        elif m in ('blr', 'ret', 'b', 'bl') or (m == 'ldr' and '[' not in ins.op_str): raise RuntimeError(f'{a:x} {m} {ins.op_str}')
        else: st = site_plain(ins); stat('plain')
    elif k == 'tpidr_r': st = site_sysreg_read(ins, 'KTLS'); stat(k)
    elif k == 'tpidrro_r': st = site_sysreg_read(ins, 'KTLR'); stat(k)
    elif k == 'tpidr_w': st = site_tpidr_write(ins); stat(k)
    elif k == 'svc': st = site_gate(ins, 'svc', arg); stat(k)
    elif k in ('cntpct', 'cntvct'): st = site_gate(ins, k, arg); stat(k)
    put_branch(a, st)



if RULE_RUN and PRIVATE_GATES:
    off, word = 0x3e443c, 0x3969c268
    assert struct.unpack_from('<I', IMG, off)[0] == word
    assert off < TEXT_END and off not in kinds and not any(seg[0].address <= off < seg[-1].address + 4 for seg in windows)
    start = here()
    emit(STP_PRE(8, 31, 31, -16))
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call(); emit(BLR(30))
    ret = here(); emit(LDP_POST(29, 30, 31, 16))


    emit(0x34000068)
    emit(LDP_POST(8, 31, 31, 16)); emit(B(here(), 0x3e4460))
    emit(LDP_POST(8, 31, 31, 16)); emit(word); emit(B(here(), 0x3e4440))
    gates.append((ret, 'svc', 0x85, off))
    put_branch(off, start); stat('private_skill_render')
    cuts.append((off, struct.pack('<I', word).hex(), struct.pack('<I', B(off,start)).hex(),
                 'private confirmed six HUD only: own image/text skip ->4460; unknown original ->4440; svc85 full800', '仅宿主'))
elif RULE_RUN:
    patch.extend(bytes(0x80))
    stat('private_gates_reserved')

qa_obs = []
if QA_COMBAT:
    assert RULE_RUN and BUILD_ID == RULE_BUILD and TARGET == 'android', 'combat observation requires the supported Android main module'
    qa_obs = [
        (0x7d3c50, 24, "a9ba6ffc", "combat reduction entry: game, attacker, defender, damage"),
        (0x7d51e4, 25, "911003ff", "combat reduction exit before restoring the frame"),
        (0x67acc0, 30, "f81a0ffb", "missile impact entry: game, missile, defender, damage"),
        (0x67b034, 31, "a9457bfd", "missile impact return before frame restoration"),
        (0x787c8c, 33, "6b00011f", "open wounds comparison: w8 roll, w0 chance"),
        (0x787db8, 34, "52800020", "open wounds application result: x0 list"),
        (0x787dd0, 35, "d10143ff", "crushing blow callback entry: game, event, attacker, defender, damage"),
        (0x787e8c, 36, "a9447bfd", "crushing blow callback return"),
        (0x787e50, 37, "6b00011f", "crushing blow comparison: w8 roll, w0 chance"),
        (0x7880fc, 38, "2a1f03e3", "crushing blow life write: x0 target, w2 life"),
        (0x6c90a4, 40, "2a1f03e3", "resource regeneration life write: x21 game, x20 unit, w2 life"),
        (0x787c10, 41, "d10183ff", "open wounds callback entry: game, event, attacker, defender, damage"),
        (0x787dbc, 42, "a9457bfd", "open wounds callback return"),
        (0x7d7cb0, 43, "d10543ff", "poison entry: source, target, rate, duration"),
        (0x7d8064, 44, "a9547bfd", "poison refresh candidate before tail setter: x1 list"),
        (0x7d80a8, 45, "a9547bfd", "poison attachment candidate before tail call: x1 list"),
        (0x7d706c, 46, "f100337b", "poison caller continuation after setter or attachment returns"),
        (0x7d70c8, 47, "b9402679", "optional poison continuation and burn block entry"),
        (0x7d71f0, 48, "2a1f03e4", "burn refresh candidate before setter: x1 list"),
        (0x7d7364, 49, "52800022", "burn attachment candidate before call: x1 list"),
        (0x7d736c, 50, "aa1403e0", "burn block exit after rejection, refresh or attachment"),
        (0x7c0898, 53, "a9497bfd", "summon creator return: x0 monster or null"),
        (0x7db180, 69, "6b08013f", "chance comparison: random w9, clamped probability w8"),
        (0x7db568, 70, "6b16011f", "block comparison: random w8, effective probability w22"),
        (0x7db778, 71, "6b15011f", "missile block comparison: random w8, effective probability w21"),
        (0x3f295c, 72, "6b08007f", "stream description: final min w8, max w3"),
        (0x3f2b0c, 73, "39493698", "ground description: requested level w24"),
        (0x3f2bd0, 74, "6b14007f", "ground description: final min w20, max w3"),
        (0x3f7430, 75, "6b08007f", "aura description: final min w8, max w3"),
        (0x786a90, 76, "d10703ff", "item retaliation entry: game, event, retaliator, victim, incoming damage, packed stat"),
        (0x786c60, 77, "a95b7bfd", "item retaliation return before frame restoration"),
        (0x3fdb50, 78, "52802588", "description dispatch: availability w0, player x21, node x20, record x22, level w24"),
        (0x7cf810, 79, "d101c3ff", "animation schedule entry: game, unit"),
        (0x7cf91c, 80, "2a1f03e7", "animation action schedule: game, unit, event, tick, action flag, ordinal"),
        (0x7cf944, 81, "2a1f03e7", "animation action schedule: game, unit, event, tick, action flag, ordinal"),
        (0x7cf9a8, 82, "2a1f03e7", "animation end schedule before event insertion"),
        (0x7cf9b4, 83, "a9467bfd", "animation schedule return before frame restoration"),
        (0x7cfa18, 84, "2a1f03e7", "zero-rate animation end schedule and tail return"),
        (0x7d82b0, 85, "d10203ff", "monster death entry: game, victim, killer, flag"),
        (0x7d8670, 86, "a9477bfd", "monster death return before frame restoration"),
        (0x7d2028, 87, "294316a6", "standard physical builder call: attacker x20, target x22, damage x21, source w26"),
        (0x7d1bfc, 88, "6b080129", "physical bounds before subtraction: lower w8, upper w9, attacker x21"),
        (0x7d1c80, 89, "a9467bfd", "physical helper result before frame restoration: result w0, sum w20, attacker x21"),
        (0x7cfa30, 90, "d101c3ff", "animation restart entry: game, unit, integer start"),
        (0x7cfbac, 91, "2a1f03e7", "animation restart action: prepared event arguments"),
        (0x7cfc10, 92, "2a1f03e7", "animation restart end tail: prepared event arguments"),
        (0x7cfc40, 93, "d10203ff", "animation sequence entry: game, unit, raw start, action ordinal"),
        (0x7cfd6c, 94, "2a1f03e7", "animation sequence action: prepared event arguments"),
        (0x7cfdf0, 95, "2a1f03e7", "animation sequence action: prepared event arguments"),
        (0x7cfe58, 96, "2a1f03e7", "animation sequence end tail: prepared event arguments"),
        (0x5e6ea0, 99, "d10543ff", "monster decision entry: game, monster"),
        (0x5e74b0, 100, "aa1f03e3", "monster alternate decision callback: x8 callback, x2 input"),
        (0x5e7f9c, 101, "aa1303e1", "monster wrapped decision callback: x8 wrapper, x3 callback"),
        (0x5e7fb4, 102, "aa1303e1", "monster ordinary decision callback: x8 callback, x2 input"),
        (0x5e70b4, 103, "a9547bfd", "monster decision return before frame restoration"),
        (0x5eb960, 104, "29035416", "accepted finite event: x19 game, x23 unit, w24 kind, w26 deadline"),
        (0x5eb9c0, 105, "29035416", "accepted persistent event: x0 linked event"),
    ]
obs = list(OBS.get(BUILD_ID, [])) + qa_obs
if RULE_RUN and PRIVATE_GATES:
    obs.append((0x43ecc8, 6, "b8686953", "private touch-run-v4 PRE-ldr; full800 host ABI"))
    obs.append((0x1237c60, 8, "d10143ff", "ui world anchor: camera world->screen entry (camera, point, out); host swaps x0 to a copy whose projection is shifted by the UI rect y (iPad UI kept 16:9 inside a taller screen), game camera untouched"))


    obs.append((0x5eb2c0, 10, "a9bc5ff8", "server world-sim entry (game G): while frozen the host swaps x0 to a stand-in whose event queue is empty, so no unit / timer callback runs this tick; the real queue is left untouched"))
    obs.append((0x4b7230, 11, "d10183ff", "client per-unit tick update entry (unit): while frozen the host swaps x0 to a stand-in unit carrying only the skip flag ([+0x124] bit 23), so the function takes its own early-out to the epilogue; all six call sites are inside the client tick simulation 0x461b80 / 0x461e60"))
    obs.append((0x4b74d0, 12, "f81e0ff3", "client per-unit per-frame update entry (unit): drives model animation / interpolation from the tick fraction; while frozen the host swaps x0 to a stand-in unit (type 9, no render component) so both the callee 0x2a18d0 and this function take their own early-outs"))
    obs.append((0x2c3c90, 13, "f81c0ff7", "render per-frame entry, called right after the main loop writes the world frame step [0x20cebb8] and world time [0x3114208]: while frozen the host rewrites the step to 0 and the world time to its value at freeze, the same thing the game's own pause path does at 0x2c1574"))
    obs.append((0xadc434, 14, "d10143ff", "TextBoxWidget text-changed entry (widget): reads the raw text at +0x88 and rebuilds the display; host rewrites +0x88 for strings our item-notes generator annotated with colour codes (TextBoxWidget draws U+E07E as '*'), exact-match table only, everything else untouched"))
    obs.append((0x442bc0, 15, "a9ba6ffc", "client event-message entry (x0 = packet; byte 1 = event subtype, jump table 0x1da6752): subtype 0x17 = offline difficulty scale changed -> 0x20f91c chat line + system notification; host rewrites byte 1 past the table (default branch, nothing shown) only for 0x17 and only in a rift game (dm_rift.c); otherwise untouched. orig stp x28, x27, [sp, #-0x60]!"))
    obs.append((0x634000, 16, "d10143ff", "server level change by level number (game, player, level, kind), used by waypoints / portals / quest moves: in a rift game, when the player leaves a rift map for a level that is neither town nor a rift map, the host swaps w2 to the rift's next map so the destination is replaced inside the single native transition"))
    obs.append((0x711040, 17, "d101c3ff", "server quest event 'monster killed' entry (game, monster, attacker), the only entry (callers 0x7d8550 kill, 0x7ab8dc); in a rift game, for a monster that died on a rift map, the host swaps x1 to a stand-in whose +0xc0 is null so the function takes its own early return (0x711064): bosses in the rift are combat only, no quest state / quest notifications"))
    obs.append((0x3d84a0, 18, "d10143ff", "NotificationPanel message handler entry (panel, message): on InputMessage / ControllerActionBegin with action id 9 (the menu key) the native handler re-sends the notice callback and marks the message bus consumed, so the first menu press only dismisses the notice. In touch mode the host swaps x1 to a copy of the message whose action id is not 9: the notice stays until it times out and the menu key opens the menu; controller mode keeps the native behaviour"))
    obs.append((0x463f40, 19, "f81c0ff7", "client per-frame position-error smoothing entry (s0 = frame length in ticks), called by the main loop right after all units were extrapolated in place and before the per-unit error is added back: the native error is only computed on start / stop (0x4639c0, which runs before the tick loop), so a sharp turn inside a tick snaps the extrapolated position back in one frame; the host adds that frame's jump to the local player's error fields (path +0x2a8 / +0x2ac, same clamp as native) and the native code decays it"))
    obs.append((0x9da5b0, 20, "d104c3ff", "model yaw component per-frame update entry (x1 = argument block: +8 component, +0x20 -> +8 unit): the body turn speed is instance data on the component (+0x10..+0x2c min / max degrees per second for controller / mouse, +0x34 spring parameter); the host only reads them for the local player when a test switch is set"))
    obs.append((0x2f7664, 7, "6dbb23e9", "touch target rank: candidate compare entry (closure, unit, kind, s0=score); host raises Boss/elite score, native eligibility unchanged"))
    obs.append((0xb0d560, 21, "fc1a0fe8", "TooltipsPanel add-tooltip entry (panel, text, target position x2 / x3, style, text2): tooltips are immediate-mode (re-submitted by the hovered widget every frame and cleared after drawing, so the host cannot reach the record at its own time); on touch, in panel state, for an item the player just tapped, the host swaps the target position so the game lays the tooltip out beside the panel instead of over the item grid; everything else passes through unchanged"))
    obs.append((0xc76f94, 23, "12001d4a", "text colour decoder RGB assembly: host overrides only tagged comparison segments"))
    obs.append((0x7dd9e0, 9, "d10203ff", "server experience credit to a player (game, player, level, amount), read only: the kill reward 0x7dcb90 reaches it at 0x7dcfac / 0x7dd094 (its frame record at x29, the kill 0x7d82b0's x19 = the dead unit saved at x29 - 8) and by its tail jump 0x7dcf18 (x30 = 0x7d831c, x19 = the dead unit); every other credit (0x7ddc40, 0x7dde80) arrives with another x30. The kill streak takes the amount of the kills it counted"))
    obs.append((0x7ce800, 22, "d100c3ff", "server place-one-preset-unit entry (game, room, record), only called by the room-activation preset loop 0x7ce930 (8 sites): in a rift game at tier 90+ on a rift map, for a monster record that is not a superunique and not scenery / NPC, the host swaps x2 to a copy whose class is -1 so 0x6910d0 takes its own early return (tbnz w2, #31) and nothing is spawned; the real record is untouched; rift guardian / invader / superunique spawns call 0x7ce630 / 0x69b310 directly and never pass here"))

    obs.append((0x7dab50, 26, "a9ba6ffc", "attack roll entry (attacker, defender, bonus, defence mode): the units of the roll"))
    obs.append((0x7db240, 27, "911043ff", "attack roll common return: w0 1 = the roll passed, 0 = failed or an early exit without a roll (the entry tells them apart)"))
    obs.append((0x7db3e0, 28, "d10183ff", "melee result entry (game, attacker, defender, bonus, flags, range): the units of the attack"))
    obs.append((0x7db690, 29, "a9457bfd", "melee result common return: w0 outcome flags (bit 0 hit, 0x10 shield block, 0x80 / 0x100 / bit 15 up avoided)"))
    obs.append((0x7db584, 106, "53017c0a", "melee result after its block and avoidance rolls (also reached from the block roll's success at 0x7db574): w0 raw result, 1 = shield block, other bits avoided (8 = evade, dropped by the conversion that follows)"))
    obs.append((0x67ae60, 32, "7200101f", "missile impact after the avoidance roll 0x7db6d0: w0 bits 0..4 (1 = block, others avoided), x20 defender, x22 the missile's owner"))


    obs.append((0x7bcd40, 51, "d10383ff", "server skill dispatcher entry: game, actor, skill, level"))
    obs.append((0x7bd080, 52, "a94d7bfd", "server skill dispatcher return: w0 result"))
    obs.append((0x768448, 54, "f9402fe2", "proc callback entry before loading skill from original stack+0x58"))
    obs.append((0x768450, 55, "39449a68", "proc callback return"))
    obs.append((0x76865c, 56, "b90053e4", "proc callback entry: game, actor, skill, level"))
    obs.append((0x768664, 57, "b94053e4", "proc callback return"))
    obs.append((0x7c0644, 58, "f8687928", "state callback entry: game, actor, skill, level"))
    obs.append((0x7c064c, 59, "b9012e74", "state callback return"))
    obs.append((0x7c0654, 60, "a9417bfd", "null-actor tail callback: attribution barrier without an unmatched scope"))
    obs.append((0x66813c, 61, "aa1303e1", "missile skill callback before copying missile x19 into x1"))
    obs.append((0x668144, 62, "b9012e75", "missile skill callback return"))
    obs.append((0x67fa74, 63, "f8697949", "missile skill callback entry: game, missile, skill, level"))
    obs.append((0x67fa7c, 64, "b9012e74", "missile skill callback return"))
    obs.append((0x665020, 65, "a9ba6ffc", "missile hit dispatch entry: game, missile"))
    obs.append((0x6658ec, 66, "910683ff", "missile hit dispatch return"))
    obs.append((0x67c3a0, 67, "a9bc5ffc", "missile area dispatch entry: game, missile"))
    obs.append((0x67c478, 68, "911043ff", "missile area dispatch return"))
    obs.append((0x67f678, 97, "aa1403e0", "abyss target enumeration entry: x20 game, x19 missile"))
    obs.append((0x67f688, 98, "9105a377", "abyss target enumeration continuation, including null bypass"))
    obs.append((0x680d1c, 107, "aa1403e0", "missile do dispatch before the do-function call 0x680d24: x20 game, x19 missile"))
    obs.append((0x680d28, 108, "7100081f", "missile do dispatch after the do-function call returned"))

    obs.append((0x68bb38, 39, "2a1f03e3", "monster regeneration life write: x19 game, x20 unit, w2 life"))
for off, oid, ow, why in obs:
    w = struct.unpack_from('<I', IMG, off)[0]
    assert f'{w:08x}' == ow, f'观测门原指令不符 {off:#x}：{w:08x}'

    assert off in set(starts) or (oid, off) in {(6, 0x43ecc8), (23, 0xc76f94), (27, 0x7db240), (29, 0x7db690), (32, 0x67ae60), (106, 0x7db584), (52, 0x7bd080), (54, 0x768448), (55, 0x768450), (56, 0x76865c), (57, 0x768664), (58, 0x7c0644), (59, 0x7c064c), (60, 0x7c0654), (61, 0x66813c), (62, 0x668144), (63, 0x67fa74), (64, 0x67fa7c), (66, 0x6658ec), (68, 0x67c478), (97, 0x67f678), (98, 0x67f688), (107, 0x680d1c), (108, 0x680d28), (39, 0x68bb38)} | {(q[1], q[0]) for q in qa_obs}, f'private obs point not approved {off:#x}'
    assert off < TEXT_END and off not in kinds and not any(lo <= off < hi for wseg in windows for lo, hi in [(wseg[0].address, wseg[-1].address + 4)]), f'观测门与改写点冲突 {off:#x}'
    put_branch(off, site_obs(off, oid, w)); stat('obs')









if RULE_RUN and PRIVATE_GATES:
    off, word, tgt = 0x7cc1e0, 0xd10183ff, 0x634000
    assert struct.unpack_from('<I', IMG, off)[0] == word and off in set(starts) and tgt in set(starts)
    assert off < TEXT_END and off not in kinds and not any(seg[0].address <= off < seg[-1].address + 4 for seg in windows)
    start = here()
    emit(STP_PRE(8, 31, 31, -16))
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call(); emit(BLR(30))
    ret = here(); emit(LDP_POST(29, 30, 31, 16))
    emit(0x34000068)
    emit(LDP_POST(8, 31, 31, 16)); emit(B(here(), tgt))
    emit(LDP_POST(8, 31, 31, 16)); emit(word); emit(B(here(), off + 4))
    gates.append((ret, 'svc', 0x89, off))
    put_branch(off, start); stat('rift_exit')
    cuts.append((off, struct.pack('<I', word).hex(), struct.pack('<I', B(off, start)).hex(),
                 'rift exit redirect: tile-link level change entry -> host swaps (level, kind) and x8!=0 takes the branch to level-number change 0x634000; otherwise original ->0x7cc1e4; svc89', '仅宿主'))
















if RULE_RUN and PRIVATE_GATES:
    off, word, tgt = 0x6fa9d0, 0x394412aa, 0x6fa9d4
    assert struct.unpack_from('<I', IMG, off)[0] == word
    assert off < TEXT_END and off not in kinds and not any(seg[0].address <= off < seg[-1].address + 4 for seg in windows)
    start = here()
    emit(STP_PRE(8, 31, 31, -16))
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call(); emit(BLR(30))
    ret = here(); emit(LDP_POST(29, 30, 31, 16))
    emit(0x34000068)
    emit(LDP_POST(8, 31, 31, 16)); emit(B(here(), tgt))
    emit(LDP_POST(8, 31, 31, 16)); emit(word); emit(B(here(), off + 4))
    gates.append((ret, 'svc', 0x8c, off))
    put_branch(off, start); stat('rift_save_act')
    cuts.append((off, struct.pack('<I', word).hex(), struct.pack('<I', B(off, start)).hex(),
                 'rift save act: save-header writer, right after it loads the current act into w9 -> host swaps w9 to 4 (act V) while the player is in a rift map; x8 stays 0, original insn runs ->0x6fa9d4; svc8c', '仅宿主'))








if RULE_RUN and PRIVATE_GATES:
    off, word, tgt = 0x64a34c, 0x2a1903e9, 0x64a350
    assert struct.unpack_from('<I', IMG, off)[0] == word
    assert off < TEXT_END and off not in kinds and not any(seg[0].address <= off < seg[-1].address + 4 for seg in windows)
    start = here()
    emit(STP_PRE(8, 31, 31, -16))
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call(); emit(BLR(30))
    ret = here(); emit(LDP_POST(29, 30, 31, 16))
    emit(0x34000068)
    emit(LDP_POST(8, 31, 31, 16)); emit(B(here(), tgt))
    emit(LDP_POST(8, 31, 31, 16)); emit(word); emit(B(here(), off + 4))
    gates.append((ret, 'svc', 0x8d, off))
    put_branch(off, start); stat('merc_slot')
    cuts.append((off, struct.pack('<I', word).hex(), struct.pack('<I', B(off, start)).hex(),
                 'merc slots: placement check, monster-owner branch right after the helm type test -> host sets w0=1 for ring/amulet/boots/gloves/belt on a hireling (rift best >= 10 for new equips; always on load); x8 stays 0, original insn runs ->0x64a350; svc8d', '仅宿主'))










if RULE_RUN and PRIVATE_GATES:
    off, word, tgt = 0x3beb58, 0xaa1303e0, 0x3beb5c
    assert struct.unpack_from('<I', IMG, off)[0] == word
    assert off < TEXT_END and off not in kinds and not any(seg[0].address <= off < seg[-1].address + 4 for seg in windows)
    start = here()
    emit(STP_PRE(8, 31, 31, -16))
    emit(STP_PRE(29, 30, 31, -16))
    emit_gate_call(); emit(BLR(30))
    ret = here(); emit(LDP_POST(29, 30, 31, 16))
    emit(0x34000068)
    emit(LDP_POST(8, 31, 31, 16)); emit(B(here(), tgt))
    emit(LDP_POST(8, 31, 31, 16)); emit(word); emit(B(here(), off + 4))
    gates.append((ret, 'svc', 0x8e, off))
    put_branch(off, start); stat('merc_slot_client')
    cuts.append((off, struct.pack('<I', word).hex(), struct.pack('<I', B(off, start)).hex(),
                 'merc slots (client): hireling can-equip check, between "mov w1,#0x25" and the type test -> host swaps w1 to the item\'s own type for ring/amulet/boots/gloves/belt (rift best >= 10); x8 stays 0, original insn runs ->0x3beb5c; svc8e', '仅宿主'))









PARAMS = {'db0507f36b84200ac05f75d76ed5026f': [
    (0xe12d8c, 'd280c813', 0xe12d90, 'f2c07093', 19, 'KRES1', '启动初始化：主机模式输出分辨率 1600x900（{u32 宽, u32 高} 打包）'),
    (0xe12e1c, 'd280a008', 0xe12e20, 'f2c05a08', 8, 'KRES0', '启动初始化：掌机模式输出分辨率 1280x720'),
    (0xe10d60, 'd280c818', 0xe10d80, 'f2c07098', 24, 'KRES1', '运行模式变化通知（0x1f）处理：主机模式 1600x900'),
    (0xe10d5c, 'd280a016', 0xe10d74, 'f2c05a16', 22, 'KRES0', '运行模式变化通知（0x1f）处理：掌机模式 1280x720'),




    (0x1663ee8, '52838e49', 0x1663eec, '72a800e9', 9, 'KASPECTUI', '界面常量缓冲 0x15 号槽 [屏宽, 屏高, 宽高比上限]：界面精灵着色器按它在上限以外画两侧黑柱（带约 20–30 像素向内的柔边，宿主另加余量把柔边推出屏幕）'),
    (0xb152bc, '52838e48', 0xb152c0, '72a800e8', 8, 'KASPECT', '新画面模式视口：边距 = (屏宽 − 屏高 × 上限) / 2，经 0x85bc90 设居中视口'),
    (0x23e888, '52838e48', 0x23e88c, '72a800e8', 8, 'KASPECT', '新画面模式坐标钳制（0x23e760）：x 夹进 [边距, 边距 + 屏高 × 上限]，宽屏两侧的指针 / 触摸不再被夹回中间'),
]}
params = PARAMS.get(BUILD_ID, [])
for oa, wa, ob_, wb, r, key, why in params:
    for o, w0 in ((oa, wa), (ob_, wb)):
        w = struct.unpack_from('<I', IMG, o)[0]
        assert f'{w:08x}' == w0, f'参数位原指令不符 {o:#x}：{w:08x}'
        assert o not in kinds and not any(lo <= o < hi for wseg in windows for lo, hi in [(wseg[0].address, wseg[-1].address + 4)]), f'参数位与改写点冲突 {o:#x}'
    for o in range(oa + 4, ob_, 4):
        ins = dis(struct.unpack_from('<I', IMG, o)[0], o)
        assert r not in regs_in(ins), f'参数位中间用到 x{r}：{o:#x} {ins.mnemonic} {ins.op_str}'
    start = here()
    tmp = 16 if r != 16 else 17
    emit(STR_PRE(tmp, 31, -16) if NOREDZONE else STUR(tmp, 31, -16)); emit_tp(r); emit_slot_addr(r, tmp, key); emit(LDRX(r, r)); emit(LDR_POST(tmp, 31, 16) if NOREDZONE else LDUR(tmp, 31, -16)); emit(B(here(), oa + 4))
    struct.pack_into('<I', IMG, oa, B(oa, start)); struct.pack_into('<I', IMG, ob_, NOP); stat('param')





rule_blocks = []
if RULE_RUN:
    def run_branch(off, old, body, why):
        assert struct.unpack_from('<I', IMG, off)[0] == old, f'Run跳板原字不符 {off:#x}'
        start = here()
        for word in body(start): emit(word)
        new = B(off, start)
        struct.pack_into('<I', IMG, off, new)
        cuts.append((off, struct.pack('<I', old).hex(), struct.pack('<I', new).hex(), why, '全'))
        rule_blocks.append((start, bytes(patch[start - PATCH_OFF:]).hex(), why))
    def cbz(at, target, r, nonzero=False):
        delta = target-at; assert delta%4==0 and -(1<<20)<=delta<(1<<20)
        return (0x35000000 if nonzero else 0x34000000) | (((delta>>2)&0x7ffff)<<5) | r
    run_branch(0x6c6648, 0x34000060, lambda a: [
        0xb9400268, cbz(a+4,a+12,8,True), B(a+8,0x6c664c),
        cbz(a+12,a+20,0), B(a+16,0x6c664c), B(a+20,0x6c6654)],
        'touch-run-v4：Run选择仅player0忽略精力门，非player按原stat返回；w8后继覆盖')
    run_branch(0x6c6290, 0x710006df, lambda a: [
        0xb9400268, cbz(a+4,a+12,8,True), B(a+8,0x6c62ec), 0x710006df, B(a+16,0x6c6294)],
        'touch-run-v4：原合法Run成本段仅player0跳stat10扣除/耗尽降走；非player保持原cmp')




    run_branch(0x4ac368, 0x35000460, lambda a: [
        0xb9400268, cbz(a+4,a+12,8), B(a+8,a+16), B(a+12,0x4ac3f4),
        cbz(a+16,a+24,0), B(a+20,0x4ac3f4), B(a+24,0x4ac36c)],
        'touch-run-v4：client当前受控Run3仅player0跳STA零值自动Stop；原getter值与真实松杆保留，非player保原门')




    run_branch(0x6c6580, 0x51000849, lambda a: [
        cbz(a,a+16,8,True), 0x51000849, cbz(a+8,a+16,9,True),
        0x52800062, 0x51000849, B(a+20,0x6c6584)],
        'touch-run-v4：原通用合法移动仅player0请求Walk2规范为Run3，非player与其它请求保原，w8/原资格不改')

os.makedirs(outdir, exist_ok=True)
P = lambda ext: os.path.join(outdir, f'{NAME}.{ext}')
open(P('patched.bin'), 'wb').write(IMG + bytearray(PATCH_OFF - len(IMG)) + patch)
import hashlib
with open(os.path.join(outdir, 'build_src.sha256'), 'w') as f:
    f.write(hashlib.sha256(open(os.path.abspath(__file__), 'rb').read()).hexdigest() + '  patch_nso.py\n')
if PRIVATE_AIM:
    with open(P('aim.tsv'), 'w') as f:
        f.write('# RULESET\tskill-aim-v1-private\n# ORIGINAL_CALL\t0x2faf04\t97fee7fd\n# ORIGINAL_CALLEE\t0x2b4ef8\n# SKILL_CALLER\t0x2b5b80\n# SVC\t0x86\n# INPUT\tactual GetNpad sampled right; move axis unchanged\n')
        f.write('# IMAGE_SHA256\t'+hashlib.sha256(open(P('patched.bin'),'rb').read()).hexdigest()+'\n')
if RULE_RUN:
    with open(P('rules.tsv'), 'w') as f:
        f.write(f'# RULESET\t{RULESET}\n# BUILD_ID\t{BUILD_ID}\n# ORIGINAL_SHA256\t{RULE_ORIG_SHA}\n# IMAGE_SHA256\t{hashlib.sha256(open(P("patched.bin"), "rb").read()).hexdigest()}\n')
        for off, raw, why in rule_blocks: f.write(f'{off:#x}\t{raw}\t{hashlib.sha256(bytes.fromhex(raw)).hexdigest()}\t{why}\n')
elif os.path.exists(P('rules.tsv')):
    os.unlink(P('rules.tsv'))
with open(P('fixups.tsv'), 'w') as f:
    f.write(f'# TARGET\t{ {"apple": 0, "android": 1, "ohos": 2}[TARGET]:#x}\n')
    f.write(f'# PATCH_OFF\t{PATCH_OFF:#x}\n# KEY_PAGE_OFF\t{KEY_PAGE_OFF:#x}\n# KEY_PAGE_SIZE\t0x4000\n# TEXT_END\t{TEXT_END:#x}\n# RO_OFF\t{RO_OFF:#x}\n# DATA_OFF\t{DATA_OFF:#x}\n')
    for k in KEY_INDEX: f.write(f'# KEY_{k}_IDX\t{KEY_INDEX[k]}\n')
with open(P('gates.tsv'), 'w') as f:
    for ret, k, arg, orig in gates: f.write(f'{ret:#x}\t{k}\t{arg}\t{orig:#x}\n')
with open(P('sites.tsv'), 'w') as f:
    for a in sorted(kinds):
        if a not in done: f.write(f'{a:#x}\t{kinds[a][0]}\n')
    for wseg in windows: f.write(f'{wseg[0].address:#x}\tW\t{wseg[-1].address + 4:#x}\n')
    for a, (b, c) in jumps.items(): f.write(f'{a:#x}\tJ\t{b:#x}\t{c}\n')
with open(P('cuts.tsv'), 'w') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for off, ob, nb, why, scope in cuts: f.write(f'{off:#x}\t{ob}\t{nb}\t{scope}\t{why}\n')
with open(P('params.tsv'), 'w') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for oa, wa, ob_, wb, r, key, why in params: f.write(f'{oa:#x}\t{ob_:#x}\tx{r}\t{key}\t{why}\n')
with open(P('obs.tsv'), 'w') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for off, oid, ow, why in obs: f.write(f'{off:#x}\t{oid}\t{ow}\t{why}\n')
print(NAME, stats, 'cuts', len(cuts), 'obs', len(obs), 'patch', len(patch), 'PATCH_OFF', hex(PATCH_OFF), 'slot_refs', len(fix), 'gates', len(gates))




if NAME == 'main':
    import hashlib, shutil
    BL = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', f'bootlist_{BUILD_ID}.tsv')
    if not os.path.isfile(BL):
        print('bootlist 没有启动块清单，跳过（不预取）。要用的话自己录一份：dm/tools/tact_bootlist.py')
        for n in ('bootlist.tsv', 'bootlist.sha256'):
            if os.path.exists(os.path.join(outdir, n)): os.remove(os.path.join(outdir, n))
    else:
        hd = open(BL, encoding='utf-8').readline().split()
        if hd[-1:] != [BUILD_ID]: sys.exit(f'【红】{BL} 头行构建号 {hd[-1:]} 与 main {BUILD_ID} 不符')
        shutil.copyfile(BL, os.path.join(outdir, 'bootlist.tsv'))
        sha = hashlib.sha256(open(BL, 'rb').read()).hexdigest()
        open(os.path.join(outdir, 'bootlist.sha256'), 'w').write(f'{sha}  bootlist.tsv\n')
        print('bootlist', os.path.relpath(BL), sha[:12])
