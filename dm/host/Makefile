DM_ROOT ?= $(HOME)/dm-work




MACOS_MIN ?= 26.0
CFLAGS=-O1 -g -Wall -Wno-unused-function -mmacosx-version-min=$(MACOS_MIN)
MOVIE_FFMPEG ?= 0
MOVIE_PREFIX ?=
ifeq ($(MOVIE_FFMPEG),1)
ifneq ($(MOVIE_PREFIX),)

CFLAGS += -DDM_MOVIE_FFMPEG=1 -I$(MOVIE_PREFIX)/include
MOVIE_LIBS = $(MOVIE_PREFIX)/lib/libavformat.a $(MOVIE_PREFIX)/lib/libavcodec.a $(MOVIE_PREFIX)/lib/libswscale.a $(MOVIE_PREFIX)/lib/libavutil.a
else
MOVIE_PACKAGES = libavformat libavcodec libavutil libswscale
ifeq ($(shell pkg-config --exists $(MOVIE_PACKAGES) && echo yes),)
$(error MOVIE_FFMPEG=1 requires pkg-config and FFmpeg development libraries, or MOVIE_PREFIX pointing to a static FFmpeg build)
endif
CFLAGS += -DDM_MOVIE_FFMPEG=1 $(shell pkg-config --cflags $(MOVIE_PACKAGES))
MOVIE_LIBS = $(shell pkg-config --libs $(MOVIE_PACKAGES))
endif
endif




SRC=\
	app_mac.m \
	crash.c \
	data_build.c \
	diag.c \
	entry.S \
	front_mac.m \
	game_fields.c \
	hle.c \
	loader.c \
	main.c \
	memstat.m \
	romfs_validate.c \
	dm_actions.c \
	dm_autobuff.c \
	dm_beltauto.c \
	dm_bossrush.c \
	dm_bright.c \
	dm_cam.c \
	dm_cloudsync.c \
	dm_cursor.c \
	dm_cursor_item.c \
	dm_custom.c \
	dm_tab_text.c \
	dm_dropfilter.c \
	dm_drophaptic.c \
	dm_elysium.c \
	dm_freetp.c \
	dm_hud.c \
	dm_i18n.c \
	dm_itembuild_core.c \
	dm_itemcmp.c \
	dm_itemcmp_core.c \
	dm_itemjob.c \
	dm_gear_compose.c \
	dm_dmgnum.c \
	dm_book.c \
	dm_book_core.c \
	dm_feats.c \
	dm_feats_core.c \
	dm_feats_read.c \
	dm_feats_store.c \
	dm_fightlog.c \
	dm_fightlog_core.c \
	dm_streak.c \
	dm_streak_core.c \
	dm_streak_hist.c \
	dm_fightlog_store.c \
	dm_fightlog_hist.c \
	dm_fightlog_text.c \
	dm_fightlog_ui.c \
	dm_kills.c \
	dm_looks.c \
	dm_looks_core.c \
	dm_looks_store.c \
	dm_looks_ui.c \
	dm_lootfilter.c \
	dm_movie_decode.c \
	dm_namegen.c \
	dm_padmode.c \
	dm_pages.c \
	dm_sertrack.c \
	dm_potpick.c \
	dm_pouch.c \
	dm_cloudmerge.c \
	dm_respec.c \
	dm_resume.c \
	dm_resume_core.c \
	dm_rift.c \
	dm_rift_ui.c \
	dm_runeword.c \
	dm_recipes.c \
	dm_effects.c \
	dm_rwlv_core.c \
	dm_shoprefresh.c \
	dm_shrine.c \
	dm_shrine_layout.c \
	dm_sim.c \
	dm_sim_ui.c \
	dm_sim_calc.c \
	dm_sim_core.c \
	dm_sim_data.c \
	dm_sim_report.c \
	dm_sim_skill.c \
	dm_sim_timeline.c \
	dm_splash.c \
	dm_splash_rule.c \
	dm_build.c \
	dm_build_ui.c \
	dm_gear_bound.c \
	dm_gear_core.c \
	dm_gear_dump.c \
	dm_gear_live.c \
	dm_gear_pages.c \
	dm_gear_pages_core.c \
	dm_gear_search.c \
	dm_gear_tab.c \
	dm_gear_order.c \
	dm_gear_equip.c \
	dm_gear_equip_core.c \
	dm_gear_equip_rules.c \
	dm_gear_equip_tab.c \
	dm_gear_equip_want.c \
	dm_glossary.c \
	dm_textbox.c \
	dm_textbox_core.c \
	dm_textinput.c \
	dm_tipx.c \
	dm_tipx_core.c \
	dm_townwp.c \
	dm_turnsmooth.c \
	dm_uiscreen.c \
	dm_unsocket.c \
	dm_unsocket_core.c \
	dm_update.c \
	dm_vault.c \
	dm_vault_ui.c \
	dm_world.c \
	dm_prof.c \
	thread.c \
	touch_mac.c \
	../ios/Sources/cloud_ios.m \
	../ios/Sources/dm_gamepad.m \
	../ios/Sources/dm_rift_hud.m \
	../ios/Sources/dm_touch_controls.c \
	$(sort $(wildcard hle_*.c))




GFX=$(sort $(wildcard ../gfx/*.c ../gfx/nvn/*.c ../gfx/nvn/*.m ../gfx/nvn/*.S ../gfx/tex/*.c ../gfx/tex/*.m))
GFX_H=$(sort $(wildcard ../gfx/nvn/*.h ../gfx/nvn/include/*.h ../gfx/tex/*.h))


LIBDEFLATE=../third_party/libdeflate/lib/deflate_decompress.c ../third_party/libdeflate/lib/zlib_decompress.c ../third_party/libdeflate/lib/adler32.c ../third_party/libdeflate/lib/utils.c ../third_party/libdeflate/lib/arm/cpu_features.c
.DEFAULT_GOAL := dmhost
print-macos-min:
	@echo $(MACOS_MIN)


MACOS_MIN_M3 ?= 15.0
CFLAGS_M3=$(subst -mmacosx-version-min=$(MACOS_MIN),-mmacosx-version-min=$(MACOS_MIN_M3),$(CFLAGS)) -DDM_NVN_M3=1 -DAPP_NAME='"D2R Mobile M3"'
print-macos-min-m3:
	@echo $(MACOS_MIN_M3)



GAME_VERSION:=$(firstword $(subst -, ,$(shell head -1 $(dir $(lastword $(MAKEFILE_LIST)))../PRODUCT)))
ROMFS?=$(or $(DM_ROMFS_SRC),$(DM_ROOT)/data/switch/$(GAME_VERSION)/Update/RomFS)
GENH=dm_vault_tab.h dm_hud_exp.h dm_bright_levels.inc
NEED_ROMFS=@test -f "$(ROMFS)/data/build_manifest.psv" || { echo "🔴 没找到游戏文件：$(ROMFS)/data/build_manifest.psv。把 Switch 版 $(GAME_VERSION) 的 Update/RomFS 放到这里，或用环境变量 DM_ROMFS_SRC 指过去（见 dm/README.md「从你的游戏文件生成资源」）"; exit 1; }
dm_vault_tab.h: ../ios/tools/gen_vault_table.py
	$(NEED_ROMFS)
	python3 ../ios/tools/gen_vault_table.py "$(ROMFS)" > $@.tmp && mv $@.tmp $@
dm_hud_exp.h: ../ios/tools/gen_hud_exp.py ../ios/tools/romfs_art.py
	$(NEED_ROMFS)
	python3 ../ios/tools/gen_hud_exp.py "$(ROMFS)" > $@.tmp && mv $@.tmp $@
dm_bright_levels.inc: ../tools/gen_bright_levels.py ../ios/tools/touch_layout.py
	$(NEED_ROMFS)
	python3 ../tools/gen_bright_levels.py "$(ROMFS)" > $@.tmp && mv $@.tmp $@
dmhost: $(SRC) $(GFX) $(LIBDEFLATE) dm.h hle.h dm_input.h front.h touch_mac.h dm_cursor.h dm_movie_decode.h dm_i18n_tab.h app_mac_overlay.h ../ios/Sources/dm_touch_controls.[ch] ../ios/Sources/dm_rift_hud.h $(GFX_H) $(GENH)
	clang $(CFLAGS) -I. -I../gfx/nvn/include -fobjc-arc -o $@ $(SRC) $(GFX) $(LIBDEFLATE) $(MOVIE_LIBS) -framework Cocoa -framework CoreText -framework QuartzCore -framework Metal -framework AudioToolbox -framework CoreAudio -framework ImageIO -framework UniformTypeIdentifiers -framework GameController -framework Network


dmhost_m3: $(SRC) $(GFX) $(LIBDEFLATE) dm.h hle.h dm_input.h front.h touch_mac.h dm_cursor.h dm_movie_decode.h dm_i18n_tab.h app_mac_overlay.h ../ios/Sources/dm_touch_controls.[ch] ../ios/Sources/dm_rift_hud.h $(GFX_H) $(GENH)
	clang $(CFLAGS_M3) -I. -I../gfx/nvn/include -fobjc-arc -o $@ $(SRC) $(GFX) $(LIBDEFLATE) $(MOVIE_LIBS) -framework Cocoa -framework CoreText -framework QuartzCore -framework Metal -framework AudioToolbox -framework CoreAudio -framework ImageIO -framework UniformTypeIdentifiers -framework GameController -framework Network
