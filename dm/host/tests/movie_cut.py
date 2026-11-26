



import struct, sys
from capstone import *
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
from game_addrs import GA
main, sub = open(sys.argv[1], 'rb').read(), open(sys.argv[2], 'rb').read()
md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
def ins(a): i = next(md.disasm(main[a:a + 4], a)); return f'{i.mnemonic} {i.op_str}'



CHAIN = [
    (GA.AT_MOVIE_EXT_ALLOC_SIZE, 'mov w1, #0x10', 'Extractor 对象 0x10 字节：剪口把状态指针存在第二个字'),
    (GA.AT_MOVIE_DEC_ALLOC_SIZE, 'mov w1, #0x10', 'Decoder 对象 0x10 字节：同上'),
    (GA.AT_MOVIE_EXT_TRACK_COUNT, 'ldr x8, [x8, #0x20]', 'Setup 调 Extractor 槽4 GetTrackCount'),
    (GA.AT_MOVIE_NO_TRACK_BRANCH, f'b.lt #{GA.AT_MOVIE_TRACK_SKIP_TARGET:#x}', '轨数 < 1 跳过选轨（剪口给 0 轨）'),
    (GA.AT_MOVIE_DEC_EVENTS_STORE, 'stp x24, x23, [x19, #0x1b8]', 'DecoderEvents[0..1] = {P+0x18, P+0x40}'),
    (GA.AT_MOVIE_EVENT_INPUT_KIND, 'mov w8, #1', 'P+0x18 的等待项种类 1 = 输入可用'),
    (GA.AT_MOVIE_EVENT_OUTPUT_KIND, 'mov w8, #2', 'P+0x40 的等待项种类 2 = 输出可用'),
    (GA.AT_MOVIE_DEC_START, 'ldr x8, [x8, #0x28]', 'Setup 调 Decoder 槽5 Start'),
    (GA.AT_MOVIE_OPEN_STATUS_CMP, 'cmp w21, #0', 'Open 返回 (Start 状态 == 0)：剪口 Start 成功 ⇒ 在播'),
    (GA.AT_MOVIE_OPEN_RESULT_SET, 'cset w0, eq', ''),
    (GA.AT_MOVIE_FRAME_UPDATE_CALL, f'bl #{GA.MOVIE_PLAYER_UPDATE:#x}', '每帧更新调播放器更新'),
    (GA.AT_MOVIE_UPD_DONE_LOAD, 'ldrb w8, [x8, #0x489]', '帧队列空时返回 !P[+0x489]（播完标志）…'),
    (GA.AT_MOVIE_UPD_PLAYING_SET, 'cset w0, eq', '… 未播完即视为在播'),
    (GA.AT_MOVIE_THREAD_INPUT_CMP, 'cmp w21, #1', '解码线程：种类 1 走输入路径'),
    (GA.AT_MOVIE_THREAD_OUTPUT_CMP, 'cmp w21, #2', '解码线程：种类 2 走输出路径'),
    (GA.AT_MOVIE_DEC_ACQUIRE_IN, 'ldr x8, [x8, #0x48]', '输入：Decoder 槽9 AcquireInputBufferIndex'),
    (GA.AT_MOVIE_DEC_GET_IN_BUF, 'ldr x8, [x8, #0x50]', '输入：Decoder 槽10 GetInputBuffer'),
    (GA.AT_MOVIE_EXT_READ, 'ldr x8, [x8, #0x50]', '输入：Extractor 槽10 Read'),
    (GA.AT_MOVIE_READ_EOS_CMP, 'cmp w23, #2, lsl #12', 'Read 返回 0x2000 = 流尾 …'),
    (GA.AT_MOVIE_LOOP_FLAG_LOAD, 'ldrb w8, [x19, #0x12]', '… 循环标志 …'),
    (GA.AT_MOVIE_EXT_SEEK, 'ldr x8, [x8, #0x40]', '… 循环则 Extractor 槽8 SeekTo（剪口重挂定时）'),
    (GA.AT_MOVIE_EOS_FLAG_SET, 'mov w26, #4', '… 否则送入标志 4（流尾）'),
    (GA.AT_MOVIE_EXT_SAMPLE_TIME, 'ldr x8, [x8, #0x60]', '输入：Extractor 槽12 GetSampleTime'),
    (GA.AT_MOVIE_DEC_SEND_INPUT, 'ldr x8, [x8, #0x58]', '输入：Decoder 槽11 SendInputBufferForDecode'),
    (GA.AT_MOVIE_EXT_ADVANCE, 'ldr x8, [x8, #0x58]', '输入：Extractor 槽11 Advance'),
    (GA.AT_MOVIE_DEC_ACQUIRE_OUT, 'ldr x8, [x8, #0x60]', '输出：Decoder 槽12 AcquireOutputBufferIndex'),
    (GA.AT_MOVIE_OUT_STATUS_TEST, 'tst w0, #0xffffdfff', '状态 0 / 0x2000 才入帧队列（剪口无输出回 0x1008）'),
    (GA.AT_MOVIE_DROP_QUEUE_CMP, 'cmp x9, #2', '丢帧只在队列 ≥2 时（单个流尾项不会被丢）'),
    (GA.AT_MOVIE_DEC_OUT_FORMAT, 'ldr x8, [x8, #0x90]', '取帧：Decoder 槽18 GetOutputBufferFormat（剪口回非 0）'),
    (GA.AT_MOVIE_FETCH_EOS_CMP, 'cmp w27, #4', '标志 4 …'),
    (GA.AT_MOVIE_DONE_FLAG_SET, 'strb w8, [x25, #0x489]', '… 置播完 ⇒ 游戏走正常播完路径'),
    (GA.AT_MOVIE_DEC_STOP, 'ldr x8, [x8, #0x30]', '收尾调 Decoder 槽6 Stop（剪口撤销定时）'),
    (GA.AT_MOVIE_DEC_DELETE, 'ldr x8, [x8, #8]', '收尾调 Decoder 槽1 删除析构'),
    (GA.AT_MOVIE_EXT_DELETE, 'ldr x8, [x8, #8]', '收尾调 Extractor 槽1 删除析构'),
]
bad = 0
for a, want, why in CHAIN:
    got = ins(a); ok = got == want; bad += not ok
    print(f'{"✓" if ok else "✗"} {a:#x}  {got:28s} {"" if ok else "期望 " + want}  {why}')



m0 = struct.unpack_from('<I', sub, 4)[0]; dyn = m0 + struct.unpack_from('<i', sub, m0 + 4)[0]
D = {}; o = dyn
while True:
    t, v = struct.unpack_from('<qQ', sub, o); o += 16
    if t == 0: break
    D.setdefault(t, v)
rel = {}
for k in range(D[8] // 24):
    off, info, add = struct.unpack_from('<QQq', sub, D[7] + 24 * k)
    rel[off] = add if info & 0xffffffff == 1027 else ('sym', info >> 32)
names, sym, symn = {}, {}, {}
for k in range(1, (D[5] - D[6]) // 24):
    nm, info, oth, shn, val, sz = struct.unpack_from('<IBBHQQ', sub, D[6] + 24 * k)
    s = sub[D[5] + nm:sub.index(b'\0', D[5] + nm)].decode(); symn[k] = s
    if shn and val: names.setdefault(val, s); sym[s] = val
def slot_name(r): return symn.get(r[1], '?') if isinstance(r, tuple) else names.get(r, '?')
VT = [('_ZTVN5movie9ExtractorE', 4, '_ZN5movie9Extractor13GetTrackCountEPi'),
      ('_ZTVN5movie9ExtractorE', 15, '_ZN5movie9Extractor13SetDataSourceEPNS_18ClientStreamReaderEPKc'),
      ('_ZTVN5movie7DecoderE', 2, '_ZN5movie7Decoder9ConfigureEPNS_9MediaDataE'),
      ('_ZTVN5movie7DecoderE', 3, '_ZN5movie7Decoder20RegisterClientEventsEPNS_13DecoderEventsE'),
      ('_ZTVN5movie7DecoderE', 5, '_ZN5movie7Decoder5StartEv'),
      ('_ZTVN5movie7DecoderE', 6, '_ZN5movie7Decoder4StopEv'),
      ('_ZTVN5movie9ExtractorE', 8, '_ZN5movie9Extractor6SeekToEl'),
      ('_ZTVN5movie9ExtractorE', 10, '_ZN5movie9Extractor4ReadEPNS_6BufferE'),
      ('_ZTVN5movie9ExtractorE', 11, '_ZN5movie9Extractor7AdvanceEv'),
      ('_ZTVN5movie9ExtractorE', 12, '_ZN5movie9Extractor13GetSampleTimeEPl'),
      ('_ZTVN5movie7DecoderE', 9, '_ZN5movie7Decoder23AcquireInputBufferIndexEPiS1_'),
      ('_ZTVN5movie7DecoderE', 10, '_ZN5movie7Decoder14GetInputBufferEiPNS_6BufferE'),
      ('_ZTVN5movie7DecoderE', 11, '_ZN5movie7Decoder24SendInputBufferForDecodeEiiilj'),
      ('_ZTVN5movie7DecoderE', 12, '_ZN5movie7Decoder24AcquireOutputBufferIndexEPiPlPjS1_'),
      ('_ZTVN5movie7DecoderE', 15, '_ZN5movie7Decoder24ReleaseOutputBufferIndexEi'),
      ('_ZTVN5movie7DecoderE', 18, '_ZN5movie7Decoder21GetOutputBufferFormatEiPNS_9MediaDataE')]
for vt, slot, want in VT:
    got = slot_name(rel.get(sym[vt] + 16 + 8 * slot, 0)); ok = got == want; bad += not ok
    print(f'{"✓" if ok else "✗"} {vt} 槽{slot}  {got}{"" if ok else "  期望 " + want}')
n = len(CHAIN) + len(VT)
print(f'{"全过" if not bad else "红"}：{n - bad}/{n}')
sys.exit(1 if bad else 0)
