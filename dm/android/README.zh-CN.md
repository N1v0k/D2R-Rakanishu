# 安卓移植

English: [README.md](README.md)

这是 ARM64 安卓版。APK 接入了公用宿主、Vulkan 回放引擎、AAudio、与 iOS 同一套画法的透明触摸覆盖层及系统输入框，已在真机上玩过（营地、战斗、各面板、NPC 对话），实测数据在 `docs/android-notes/`。玩家装发布页的 APK、导入自己的游戏内容即可，见下面「玩家怎么装」。

## 源码结构

- `app/`：Java Activity、全屏 SurfaceView、触摸覆盖层及 Gradle 应用工程。
- `jni_android.c`、`front_android.c`：启动、资源路径、生命周期及平台前端接口。
- `touch_android*`：接入公用 `dm/ios/Sources/dm_touch_controls.c` 状态机；`text_input_jni.c` 将客体同步输入请求接到安卓系统对话框。
- `vulkan_present.*`：安卓窗口引用、交换链及呈现。
- `../gfx/vulkan/`：NVN 回放适配器和 Vulkan 引擎。
- `compat/`：宿主 Apple 平台接口的安卓实现。
- `tools/`：源码清单解析、ELF 汇编转换及本地化资源生成。

## 玩家怎么装

发布页的 `D2RMobile-android.apk` 不含任何游戏内容（由 `dm/android/tools/shell_apk.sh` 出：三张从游戏文件生成的表换成替身，运行时读导入的 `derived.tsv`，做法同 iOS 壳）。游戏内容用你自己的游戏文件在电脑上做：

```sh
python3 dm/tools/d2rm_android.py <Switch 版 3.3.94012 的 Update 目录> <输出目录>
```

它改写游戏程序（安卓目标）、把全部着色器翻成 Vulkan 包、生成触屏覆盖层与几张数据表，写出 `<输出目录>/android/`。需要的工具见脚本开头（Python 包、.NET SDK、Vulkan SDK 里的 `spirv-opt` / `spirv-val` / `glslc`），缺什么它会逐项报。

把这个 `android` 文件夹整个拷到手机的 `Download/S2X/D2R Mobile/r4k4n1shu/` 下，游戏资源（`RomFS/data` 整个目录）拷到共享的 `Download/S2X/D2R Mobile/data`。打开 App：发现新的导入包就拷进内部目录 `files/dm/`（显示进度，第一次一两分钟），然后启动。导入包要与 App 同一版本（`pack_info.txt` 里的产物代号与 App 编入的相同），不同时 App 会说明。导入完，手机上的 `android` 文件夹可以删掉；导入包里有游戏内容，只给自己用。

## 本地构建

当前配置使用 JDK 17 或更新版本、Gradle 9.8.1、Android SDK 36、NDK 29.0.14206865、CMake 3.31.6 和 Python 3。ARM64 设备要求 Android 12/API 31 或更新系统及具备所需设备能力的 Vulkan 1.1。这些是构建和清单要求，不代表符合条件的机型都已验证。首台目标是 vivo X200 Ultra（Android 16/API 36、Adreno 830）。

按根目录准备说明提供自己的游戏文件。参考 `dm/host/Makefile` 的规则，在仓外生成 `dm_vault_tab.h`、`dm_hud_exp.h`、`dm_bright_levels.inc`。`ANDROID_HOME` 与 `ANDROID_SDK_ROOT` 必须指向同一个 SDK，`JAVA_HOME` 指向 JDK。在仓库根目录执行：

```sh
gradle -p dm/android :app:assembleDebug :app:lintDebug \
  -PdmGenDir=/absolute/path/to/private/generated/headers \
  -PdmPython=/absolute/path/to/python
```

APK 位于 `dm/android/app/build/outputs/apk/debug/app-debug.apk`，原生链接启用 `--no-undefined`。`-PdmPrebuiltJni=...` 只供使用已有库打包，不是当前原生源码编译通过的证据。

在 Linux 或 WSL 上，可用 `dm/android/docker/` 在容器里跑同一构建，上述版本均已钉死，不需要在 Windows 侧装 Python、NDK 或 CMake：

```sh
docker build -t dm-android-build:1 dm/android/docker
DM_GEN_DIR=/绝对路径/私有生成表头目录 dm/android/docker/build.sh
```

脚本以当前用户身份运行，调试签名钥匙保存在 `~/.android-dm`；挂载说明见脚本开头。

## 仓外运行材料

APK 和 Git 仓库均不含游戏程序、着色器、游戏美术或存档。这些由导入包提供（见「玩家怎么装」），App 导入到下表的内部目录。

| 位置 | 内容 |
| --- | --- |
| 应用内部 `files/dm/out/` | 使用 `DM_TARGET=android` 生成的 `main`、`sdk` 改写产物，含 patched 模块、fixups 和 gates |
| 应用内部 `files/dm/shaders/` | 与游戏及 Vulkan 后端匹配的仓外 `.xgs` 包 |
| 应用内部 `files/dm/overlay/` | 必需 CASC 触摸覆盖层，含 `overlay.manifest`，由匹配源码及用户资源生成 |
| 应用内部 `files/dm/overlay/touchart/`、`files/dm/overlay/hud_v4_ui/` | 由 `stage_touch_assets.py` 放入的技能图集及其银色剪影、地图图标、圆框、底栏素材，以及技能钮底座（仓库的 `dm/ios/Sources/ControlArt/dm_mobile_socket.png`）。触屏控件与 iOS 同一套画法（`dm/ios/Sources/DmTouchControlArt.h`、`DmTouchOverlay.m`），摇杆用代码画 |
| 应用内部 `files/dm/overlay_variants/` | 可选：走覆盖层的自定义开关（目前只有「物品注释与旧译名」）每种组合一份完整覆盖层，各带 `overlay.manifest`；App 启动时把与开关相符的那份叠在 `overlay/` 前面 |
| 共享 `Download/S2X/D2R Mobile/data/` | 用户资源档案及 `.build.config`，需要安卓“所有文件访问”授权；没有这个目录、而早先的 `Download/data/` 在时，App 沿用 `Download/data/` |
| 应用内部 `files/dm/derived.tsv`、`files/dm/pack_info.txt` | 从游戏文件生成的数据表（不含游戏内容的发布包启动时读）；导入包的产物代号与生成时刻 |
| 应用内部 `files/save/` | 存档，升级时必须保留 |
| 应用内部 `files/temp/`、`files/dm/boot.log` | 临时文件及追加写入的启动日志 |

内部没有主程序时，启动器回退到应用外部私有目录下的 `dm/`。启动前会检查必要路径，但不能据此证明着色器身份及资源兼容性。调试包可使用 `run-as` 导入内部文件，升级使用相同签名覆盖安装，不以卸载或清数据作为升级步骤。真机验证前备份已有存档，不向 PR 上传 APK 或上述私人文件。

覆盖层生成器 `dm/ios/tools/gen_romfs_overlay.py` 支持用 `DM_DESWIZZLE_LIB` 指定本地编译的纹理处理库。生成的精简 CASC 只含变更并引用原资源，不得替换原始资源目录。

### 着色器包

`files/dm/shaders/` 里，游戏用到的每个着色器各有一个 `<键>.xgs`；键是该着色器控制段 SHA-256 的前 16 位十六进制。包是 XGS1 文件：文件头、各绑定、顶点输入位置、c1 字节，然后是 SPIR-V（`dm/gfx/vulkan/engine/xbg_game.h` 的 `xbg_shader_header`）。目前测试用的一套共 6,931 个包：

- **6,927 个是游戏自己着色器的翻译。** 它们出自游戏文件，所以不进 Git、PR 和 APK。`dm/gfx/tools/dmsh_vk.py` 从头做出它们：用 `dm/gfx/tools/dmsh_xlate` 翻成面向 Vulkan 的 SPIR-V，绑定号改成单描述符集的编号（原号 × 6 + 阶段号）、补上「未定格式」存储图像读写的能力、裁掉多余能力，导出 `<键>.vk.json`，再用 `pack_game_shaders.py` 的打包函数写成包。做出的 6,927 个包与 biglawine 鸿蒙着色器流程做出的测试包逐字节相同（`dmsh_vk.py --compare <参考目录> <输出目录>`）。
- **4 个是 main 程序本体内嵌的查询核**（`q_val0`、`q_val8`、`q_dlt0`、`q_dlt8`；Mac 与 iOS 用 `dm/gfx/nvn/nvn_builtin.h` 里的 MSL 版）。它们是手写的，能从源码做：`python3 dm/gfx/tools/pack_builtin_queries.py --output <新目录>` 用 `glslc`（NDK 的 `shader-tools/` 下就有）编译 `dm/gfx/vulkan/kernels/builtin_queries.comp`，写出这 4 个文件。

一套包还可以附带 `vk-pipeline-list.seed`。首次启动、还没有 `files/dm/vk-pipeline-list.bin` 时，引擎在启动期间编好种子里列出的管线，种子覆盖的地方第一次去就不会因为现编管线而卡。`dm/android/tools/merge_warm_lists.py` 把几台设备的 `files/dm/vk-pipeline-list.bin` 合成一个种子。种子只能和录它时的那套包一起用，所以随包发放：`dm/tools/d2rm_android.py --warm-seed <文件>` 把它放进导入包（在耗时的步骤之前先核种子），并记在 `pack_info.txt` 的 `WARM_SEED=` 一行。种子放在仓库外。种子只对首次启动有用：手机上已经有 `files/dm/vk-pipeline-list.bin` 的，重新导入后仍用自己的清单。工具的检查随 `make -C dm/host/tests run` 跑（也可以 `python3 dm/tools/tests/test_warm_seed.py`）。实测见 `docs/android-notes/2026-10-22-first-play-stutter.zh-CN.md`。

## 验证与剩余工作

执行根目录贡献检查及 `:app:lintDebug`。纯 Java 输入测试不需要手机：

```sh
javac -encoding UTF-8 -d /tmp/dm-input-tests \
  dm/android/app/src/main/java/com/xiaoboluo/d2r/TextInputRules.java \
  dm/android/tests/TextInputRulesTest.java
java -cp /tmp/dm-input-tests com.xiaoboluo.d2r.TextInputRulesTest
```

真机验收须分别记录安装、真实 GPU 出图/帧率、建角、进局、移动停止、NPC 交互、背包/技能、声音、后台恢复及保存重进。无窗口宿主帧率不等于渲染帧率。本后端尚未接入 Metal GPU 计时及实际扫描显示时间，缺失读数不能当作零耗时。

Vulkan 当前每次全画，并使用同步宿主翻译（`DM_NVN_SYNC=1`）；上游新增异步路径需独立审计对象生命周期。呈现层仍逐帧等待队列。iOS 技能图、空技能槽菜单、专用数量/属性加点浮层及商店拖动跟随图尚未完整实现，通用控件和系统输入框不代表完整 iOS 功能对齐。提交上游集成前，还须在 Apple 构建机上验证 macOS/iOS 回归。

覆盖层生成器将源码指纹和逐文件哈希写入 `overlay.manifest`，APK 编入同一生成器输入指纹。缺失、不完整或不匹配时，启动界面提示并在宿主线程启动前退出启动流程。须部署完整生成目录，不可把新清单贴到旧资源上，也不可在手机启动时补清单；生成器或布局修改后应重新生成。这项校验不负责认证第三方下载来源。

`gen_romfs_overlay.py` 显式请求只生成 overlay 所需资产，跳过 iOS 技能图集及未使用的技能剪影；完整 `gen_assets.ensure()` 仍要求这些产物。技能图集改核尺寸和解码 RGBA 像素，保留原 sprite SHA256/MD5。生成子进程显式使用 UTF-8/LF，并保留已有只读资源适配器；Windows/Linux 的字节一致性仍须以同一游戏输入和工具版本实测。

Vulkan 兼容路径按实际格式能力选择，不根据 Android 版本推断。启动日志记录 `format_feature_flags2` 与 `depth16_as_d24`；仅 D16 缺线性过滤且 X8_D24 满足全部用途时拓宽深度格式。Vulkan 1.1 版本号本身不等于兼容通过；vivo 应保持 `1 / 0`，Redmi K50 Ultra 仍需单独真机验证。
# 当期iOS触控

安卓复用上游触控状态机及只读技能绑定观测桥。用自己的匹配RomFS在仓外生成绑定索引和图集PNG：

```
python dm/ios/tools/native_art/generate.py <RomFS> --output-dir <仓外native-art>
```

只把`dm_touch_native_art.c`复制到现有`DM_GEN_DIR`。CMake要求该仓外索引与原有宿主生成头同时存在。生成新的overlay和HUD后执行：

```
python dm/android/tools/stage_touch_assets.py --native-art <仓外native-art>/TouchArt --hud <assets>/NativeHUD/v4/hud_v4_ui --overlay <候选overlay>
```

导入完整且已生成最终manifest的候选overlay。技能PNG和HUD图像留在外部资源载荷，不加Git或APK assets；绘制层从已校验导入的`files/dm/overlay`读取。设置`DM_NATIVE_ART_DIR=<仓外native-art>`可让素材采图测试核验仓外索引和像素。五个导航键保留，等完整替代背包流程通过真机验证后再调整。

## 测试钩子与日志轮转

可调试的 qa APK（`shell_apk.sh` 缺省出的包，CMake Debug）带几个给外部测试工具用的钩子。它们只记录状态，不从外面读任何命令。player 变体与发布构建一个都没有：`test_hooks.c` 只在 `DM_ANDROID_TEST_HOOKS`（qa 的 Debug 构建）下编译，宿主只留着一直为 NULL 的钩子位。

- **界面层日志行**（调试包缺省开）：Java 界面层每次状态变化在 `boot.log` 记一行 `[qa] overlay <名> <状态>`，这个状态的第一帧交给合成器之后（帧提交回调）再记 `[qa] drawn <名> <状态>`（再过一个垂直同步上屏）。名字与状态：`app_menu`（快捷菜单）closed / open / paused / hint，`tech_page` closed / open，`recipe_book` closed / loading / open / inserting，`pad_layout` touch / pad；构建指南页照样报 `build_book`（`TestHooks.overlay(视图, 名, 状态)`）。`pad_layout` 没有自己的 drawn 行：游戏是异步重建 HUD 的，已有的 `手柄 / 触屏布局：切到…版——完成` 一行就是建好的那一刻。
- **状态文件**（缺省关；`files/dm/env` 里写 `DM_QA_STATE=1`）：内部存储里的 `files/dm/state.json`（权限 0600，整份原子替换）。低优先级线程每秒至多写两次、内容变了才写，另外每 5 秒一次心跳。字段：`session`（pid、启动时刻）、`seq`、`mono_ms`（CLOCK_MONOTONIC，可对 `/proc/uptime`）、`wall_ms`、`frame`、`screen`（boot / frontend / loading / ingame）、`paused`、`hud`（区域、幕、位置、等级、生命、法力、腰带……与 `dm_hud：` 行同一组数）、`panels`（非常驻面板栈，同 `[hud] 面板栈` 行）、`tooltip`（`null` 或屏上说明框的文字，颜色码原样）、`overlays`（上面的状态）与 `age_ms`（写入时 HUD、说明框、界面层各自有多旧；HUD 一直变旧 = 拍停了，比如暂停）。
- `DM_QA=0` 关掉界面层日志行与状态文件。
- **脚本出错**（`DM_KEYS`、`DM_TEST_CURSOR`）：`DM_KEYS_SOFT=1`（安卓缺省）时解析不过的脚本整段作废（`【红】hid 脚本作废：…`），运行中监督超时则中止整个脚本、松开它按着的东西（`【红】hid 脚本中止：…`），游戏照常；这个模式下数字与长度从严核。`DM_KEYS_SOFT=0`（Mac / iOS 缺省）照旧终止进程。

`boot.log`（所有构建）：启动时到 8 MiB 的文件改名为 `boot.log.1`（只留一份）；超过 32 MiB 的先改名，由后台线程只把最后 8 MiB 拷进 `boot.log.1`；会话中写到 32 MiB 也换文件。日志以 0600 新建。跨 App 启动跟读日志的工具要料到文件会被换掉（一直开着旧文件的跟读者要切到新文件）。图形引擎的 `NVN PERF` 行按窗口抽样：每 60 帧一个窗口的 `NVN PERF v=1` 汇总（fps、提交 CPU、GPU）一个不少，整窗口的明细只留头三个、之后每 30 秒一个，后面各跟一行 `NVN PERF sampling thinned=… dropped=…`；只在出事时才打的行（上传、着色器复用、evguard、新建管线）和不认识的键一律保留。`DM_LOG_PERF_EVERY=<秒>` 改间隔，`0` 全记。纯逻辑的单测是 `dm/android/tests/test_log_policy.c` 与 `test_qa_state.c`（宿主 `cc`），脚本处理是 `dm/host/tests/t_hid_script.c`。

## 快捷菜单与 Extras

腰带第一格（五芒星）、手柄按住菜单键 0.5 秒、肩键动作「快捷菜单」都打开**快捷菜单**：一列原版手柄暂停菜单样子的按钮——暂停、配方（配方书能用时才有）、构建、人数 ◀ n ▶、关闭。人数到 1 / 8 停住，每改一档当场请求宿主，继续后的第一拍落地；菜单显示的总是请求的那个。画的东西全来自玩家的游戏文件，经导入包（`appart/quick/`、`appart/ui_colors.txt`，`dm/android/tools/gen_app_art.py`）；游戏起来之前 App 逐个解码核对（`AppArt.verify`），`pack_info.txt` 里没有 `CAPS=quickmenu1` 的包同「版本不对」一样不收。

很少改的设置不在快捷菜单里：帧率与状态行在选项页 **Video**；肩键（`sw_custom.txt` 键 `shoulder_l` / `shoulder_r`，动作 0–14，13 = 配方书预留）与**技术**行（带「打开」按钮）在 **Extras** 最后。肩键一节只在有肩键的手机上显示（第一次 F7 / F8，或按 nubia / REDMAGIC 估计）。旧的 Java 偏好迁移一次，留着给退回旧版用。更新后第一次打开会出一次提示说东西搬到了哪里（`shared_prefs/dm_quick.xml`，`hint` 1 = 该出，2 = 出过了）。

## 存档导出与导入

`SaveTransfer.java` 每个进程在宿主第一次启动之前做一次（重新接上正在跑的宿主时不做，游戏运行时从不做），在单独的线程里；结果在主菜单用游戏样式的提示框显示（`TransferNotice.java`）。触发：`Download/S2X/D2R Mobile/r4k4n1shu/` 里的文件或文件夹 `export`、文件夹 `save-import`，以及技术页的「导出 / 导入」按钮——先存档、结束进程，再由 `Restart`（单独进程 `:restart`）重新打开游戏。

- 导出：`files/save` 经 `.part` 拷到 `save-export/<时间>/`，另加 `export-source.txt`（`files/save-transfer-id` 里的安装标识）和 `_app/`（`sw_custom.txt`、`stash_backup/`、跟随的构建指南 `builds.properties`、记录表）。
- 导入：角色与 Settings 只在手机上没有时导入；共享仓库与编年史仓库按模式各是一份：手机上没有就整份拷进来（编年史仓库只在干净恢复时：没有同名仓库、没有编年史仓库、没有带 `.swpage` / `.swpouch` 的角色），否则打成一个收件箱条目 `_cloudinbox/<毫秒>-import/mine.pack`，由 `dm_cloudmerge.c` 在局内像 iCloud 分叉一样并回。备份里没并完的收件箱条目一起带来，并完的不带；带本机安装标识的备份从不合并。
- 不重复：`files/save-transfer/ledger` 记下每份拷进来或放进收件箱的部分（`stash`、`vault`、`inbox` 加 SHA-256），记过的不再拷、不再并。记录表随导出带走，导入时合并。
- 防中断：先全部放进 `files/save-transfer/stage`，再原子写 `files/save-transfer/journal`，按步重放（搬移、导入角色的 elysium 标记、记录表、构建指南、文件夹改名）直到做完，下次启动也接着做；没有日志的 stage 直接丢掉。没做完的导出经 `files/save-transfer/export` 删掉。
- 改动之前先检查：最多 4096 个文件，每个文件 16 MiB、每个包 64 个文件（原生读包的上限），`.d2s`、`.d2i`、`.swvault` 格式有效。

测试：`dm/android/tests/SaveTransferTest.java`（在 `run.sh` 里；用系统属性 `dm.transfer.kill` 在每个发布边界模拟中断）与 `dm/host/tests/t_cloudmerge.c`（带编年史仓库的 `-import` 条目、金币只给一次、配方锁、中断点 `cm:1` / `cm:2` / `cm:4`、超过 64 个收件箱条目）。

## 3D比例与帧率

玩家在选项页**Video**页签里用游戏自己的滑块设（存于`sw_custom.txt`，键`s3d`、`fps`、`res_pct`；`dm/host/dm_custom.c`，尺寸与格在`dm/host/dm_res_scale.h`）。3D清晰度（75–100 %）与帧率立即生效（帧率5帧一格，从30到比屏最高刷新率低的那一格，最后Max；App向显示请求能被帧率整除的最小刷新率，`vlog`里的`Frame rate vote`、`boot.log`里的`display: refresh`）；输出分辨率（物理屏的百分比，5 %一格，至少720行；滑块旁边写最终尺寸）不比这次启动用的尺寸大时点「应用」当场切（App推性能模式变化通知，游戏在启动时建好的窗口里改出图区域，出图跟着换交换链；`boot.log`里的`res live:`，120帧内按上屏的交换链核对，核不上就退回；`files/dm/env`里`DM_RES_LIVE=0`关掉），更大的重启App后生效。缺省：安卓3D比例0.85（其他手机平台0.75）、动态比例开启、下限0.60，帧率按屏幕（刷新率高于130 Hz为72，其余60），输出900行。测量用标记：`gfx-frame-rate-policy=highest`（改投能整除的最大刷新率）、`gfx-frame-rate-strategy=always`（允许非无缝的刷新率切换）。

测量时游戏的`files/dm/env`里写`DM_3D_SCALE`、`DM_3D_SCALE_DYN`、`DM_3D_SCALE_FLOOR`仍有效并钉住数值（Video那一行只显示）；`DM_GAME_RES=<宽>x<高>`同样钉住输出分辨率。固定比例A/B对照示例：

```
DM_3D_SCALE=0.75
DM_3D_SCALE_DYN=0
DM_3D_SCALE_FLOOR=0.60
```

## 比 16:9 窄的屏（真全屏）

屏幕比 16:9 窄 0.01 以上（4:3 平板、折叠屏内屏）时，局内画面跟屏幕比例、3D 铺满整屏；游戏界面按 16:9 贴底（`dm/host/dm_uiscreen.c`，图形层下移界面那一趟），前端各页与读取画面是居中留边的 16:9。16:9 及更宽的屏不变。启动时按屏幕形状决定（`front_android.c`，`boot:` 行末尾写「真全屏」），运行中窗口换形状（折叠 / 展开）时跟着变。

出图尺寸同样按 Video 页：百分比的基准是整块屏（100 % = 面板），宽跟屏幕比例。出图宽不低于 1280，所以 16:9 界面至少 720 行、游戏不必再抬高它（4:3：至少 960 行；auto = 900 行与这个下限中较大的那个；滑块最低一格也守这个下限）。设备缺省与 3D 清晰度同宽屏。窗口比启动时游戏建的窗口贴图大（展开）时，画面当场缩进那个尺寸，重启后更清晰。

真全屏的启动不论多高都记进防崩计数：连续两次没出满 300 帧，这个产物代号就关掉真全屏（`sw_custom.txt` 旁的 `res_full_off`，换了产物代号的新版本再试），下次按 16:9 留边启动。`DM_ANDROID_FULL=0` 关掉它（测量用，`files/dm/env`，qa）；玩家在游戏文件夹（`Download/S2X/D2R Mobile`）放一个名为 `fullscreen-off` 的空文件也一样。

我们自己的各层经 `GameRect.java` 跟 `front_android_view_rect` 走：触控键与底栏、药水键、镜头键在 16:9 块里；状态块与状态效果在画面顶部、游戏上提过去的头像下面；我们的页（配方书、构建指南、术语表、技术页、快捷菜单与暂停画面、存档转移提示）在画面里居中的 16:9 块里，同游戏自己的菜单；伤害数字、整理键、镜头箭头与瞄准环用整个画面。块上方的触点照样点世界。

## 构建变体

QA 与 player 的构建、签名和备份说明见[构建变体](README.md#build-variants)及[签名与传输目录](README.md#signing-and-transfer-storage)。

## arm64翻译器与x86_64模拟器

APK可原样在x86_64安卓模拟器上运行，由模拟器的arm64翻译器（API 35 `google_apis`镜像里的libndk_translation / Berberis）执行`libdm.so`。靠两点：

- **安卓不用LSE原子。** NVN计数跳板（`dm/gfx/nvn/nvn_thunks.S`，由`dm/gfx/tools/gen_protos.py`生成）、`DM_NVN_PROF`计时包装（`nvn_entry.S`）与`DM_HLECOUNT`计数桩（`dm/host/hle_count.c`）在安卓上用ARMv8.0的`ldxr`/`stxr`循环计数（`DM_NVN_COUNT_LLSC=1`，由`CMakeLists.txt`给汇编设）。翻译器把`stadd`等ARMv8.1 LSE原子执行错，ARMv8.0手机根本没有这些指令。苹果与鸿蒙照旧用`stadd`。
- **翻译器下关掉写保护跟踪。** App启动时读一次`ro.product.cpu.abilist`（没有时读`ro.product.cpu.abi`）。首选ABI是`x86`或`x86_64`时，这个只有arm64的库只可能经原生桥（native bridge）翻译运行；模拟器的翻译器处理不了写保护缺页，所以`DM_GFX_VCACHE`、`DM_GFX_TEX_DIRTY`与`DM_GFX_WTRACK`缺省为0。`files/dm/env`里给的值照旧生效，引擎的标记文件（`gfx-vcache`、`gfx-tex-dirty`、`gfx-wtrack`、`gfx-no-wtrack`）仍优先于环境。`boot.log`记下判断：`boot: cpu translated (…)`；手机上是`boot: cpu native (…)`，其他首选ABI记`boot: cpu unknown (no change)`，两者都什么都不改。诊断用的标记`gfx-event-guard`会故意缺页，在翻译器下仍然不行。回归测试位于`dm/android/tests/test_cpu_translation.c`（判断与env/标记优先级）和`dm/gfx/vulkan/engine/tests/wtrack_switch_test.cpp`，任何C / C++编译器都能构建运行。`dm/android/tests/test_hlecount_stub.c`在设备或模拟器上检查安卓的`DM_HLECOUNT`计数桩。

安卓上`DM_NVN_PROF`不能经`files/dm/env`打开：计时包装由库构造函数（`dm/gfx/nvn/hle_nvn.c`的`nvn_nops_register`）装上，它在库载入时就运行，早于`nativeStart`读env文件。包装本身已用LL/SC。

## 自选图形驱动

`libdm.so` 不再链接 `libvulkan.so`。`tools/gen_vk_shim.py` 在构建时从 NDK 的 `vulkan_core.h` / `vulkan_android.h` 为每个 `vk*` 函数生成一个转发函数（构建目录里的 `vk_shim.c`）；`vk_loader.c` 在第一次调用时经所选加载器的 `vkGetInstanceProcAddr` 取地址（全局函数用 `NULL` 实例，其余用实例；换加载器或新建实例后重取）。没有自选驱动时，第一次调用打开系统的 `libvulkan.so`，同以前一样。引擎（`dm/gfx`）不改；`CMakeLists.txt` 像鸿蒙构建去掉 `log` 那样从引擎的链接库里去掉 `vulkan`。宿主测试：`tests/test_vk_loader.sh`（两个假加载器、8 个线程、换加载器、缺函数）。

第一阶段还没有选驱动的界面。玩家把恰好一份驱动 ZIP（Eden / yuzu / Skyline 驱动管理的格式：`meta.json` 加一份 `.so`）放进 App 自己的文件夹 `Android/data/<包名>/files/drivers/`（经 USB 或 adb；别的 App 写不进去）。那里可选一个 `tu_debug.txt`，写名单 `sysmem`、`gmem`、`nolrz`、`noubwc`、`flushall`、`syncdraw` 里的 `TU_DEBUG` 值（逗号分隔）。下一次启动时 `DriverStore.java`（在工作线程上）拷一份 ZIP，只读检查、什么都不执行（`DriverZip.java`：上限、路径、`meta.json`；`DriverElf.java`：AArch64 `ET_DYN`、边界、页对齐、导出 `HMI`），把那一份 `.so` 只读放进 `files/drivers/<sha256>/`，每次载入前重算 sha256。没有 `/dev/kgsl-3d0` 或首选 ABI 是 x86 的设备一律拒绝。libadrenotools（`dm/third_party/adrenotools`）单独在 `libdm_driverload.so` 里，只在用自选驱动时由 `vk_driver.c` dlopen；它的两个钩子库要用时才从 APK 拷到 `files/drivers/hooks/`，所以 `useLegacyPackaging` 照旧关着。adrenotools 载不进驱动时会悄悄退回系统驱动，所以 `vk_driver.c` 在 `vkCreateInstance` 之后核对 `/proc/self/maps` 里有那份 `.so`。

启动保护（`DriverGuard.java`，状态在 `files/drivers/guard.txt`）：建设备（`vkCreateDevice`）之前的干净失败在同一进程里换回系统加载器（`files/drivers/attempt.fallback`）；出够 300 帧算启动完成（`attempt.ok`）；连着两次没启动完就锁住这个包、重新用系统驱动。两种情况技术页都会自己打开一次，显示红字提示。把 ZIP 拿走（启动一次）再放回去就能重试。选着 ZIP 时，App 图标长按有快捷方式「用系统驱动启动」（只管一次启动）。技术页显示真正在跑的驱动和驱动包。在跑的驱动身份（设备、API、driverID、名字）记在 `files/drivers/active.txt` 和 `boot.log`（`vk driver: …`）。只在 qa 调试包：`files/drivers/test-fail` = `open`、`verify` 或 `hooks` 注入失败。字串在 `res/values*/driver.xml`（不在 `ui_text.tsv` 里，导入包不变）。单测：`DriverGuardTest`、`DriverZipTest`、`DriverElfTest`、`DriverTextTest`（在 `tests/run.sh` 里）；打包由 `tools/check_driver_libs.py` 核对（`shell_apk.sh` 跑）。
