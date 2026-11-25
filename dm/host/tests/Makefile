DM_ROOT ?= $(HOME)/dm-work


PY ?= $(DM_ROOT)/build/cache/dm/venv/bin/python
EXEFS ?= $(DM_ROOT)/data/switch/3.3.94012/Update/ExeFS
ROMFS ?= $(or $(DM_ROMFS_SRC),$(DM_ROOT)/data/switch/3.3.94012/Update/RomFS)
B = build
MOVIE_PACKAGES = libavformat libavcodec libavutil libswscale
MOVIE_TEST_FLAGS ?= -O1
.PHONY: movie-real


run: \
	$(B)/main.orig.bin \
	$(B)/subsdk0.orig.bin \
	$(B)/t_altstack \
	$(B)/t_book \
	$(B)/t_audio \
	$(B)/t_audio_spsc \
	$(B)/t_cloudmerge \
	$(B)/t_cloudsync \
	$(B)/t_condvar \
	$(B)/t_condvar_old \
	$(B)/t_crash_hang \
	$(B)/t_custom_extras \
	$(B)/t_custom_relayout \
	$(B)/t_custom_video \
	$(B)/t_custom_video_voice \
	$(B)/t_dmgnum \
	$(B)/t_splash \
	$(B)/t_splash_rule \
	$(B)/t_shrine \
	$(B)/t_shrine_layout \
	$(B)/t_doorspot \
	$(B)/t_dump_reader \
	$(B)/t_dump_reader_san \
	$(B)/t_effects \
	$(B)/t_feats \
	$(B)/t_feats_read \
	$(B)/t_feats_store \
	$(B)/t_fightlog \
	$(B)/t_resume \
	$(B)/t_streak \
	$(B)/t_fightlog_key \
	$(B)/t_fightlog_gates \
	$(B)/t_fightlog_hist \
	$(B)/t_fightlog_sim \
	$(B)/t_fs \
	$(B)/t_fs_case \
	$(B)/t_hand_guard \
	$(B)/t_hid_receipt \
	$(B)/t_hid_script \
	$(B)/t_hud_dead \
	$(B)/t_hud_players \
	$(B)/t_i18n \
	$(B)/t_item_text \
	$(B)/t_itemjob \
	$(B)/t_legend \
	$(B)/t_loadwatch \
	$(B)/t_looks_core \
	$(B)/t_looks_store \
	$(B)/t_padkeys \
	$(B)/t_minimap_lift \
	$(B)/t_loader_weak \
	$(B)/t_mac_overlay \
	$(B)/t_modetx \
	$(B)/t_movie \
	$(B)/t_namegen \
	$(B)/t_oe \
	$(B)/t_padmap \
	$(B)/t_portrait \
	$(B)/t_pouch_count \
	$(B)/t_recipes_text \
	$(B)/t_recipe_job \
	$(B)/t_res_scale \
	$(B)/t_rift_clock \
	$(B)/t_rift_hud \
	$(B)/t_rift_next \
	$(B)/t_rift_oldrun \
	$(B)/t_rift_wave \
	$(B)/t_interaction_diag \
	$(B)/t_itemcmp \
	$(B)/t_loader_weak \
	$(B)/t_map_diag \
	$(B)/t_warp_diag \
	$(B)/t_romfs_validate \
	$(B)/t_rw_stale \
	$(B)/t_rwlv \
	$(B)/t_skill_hit \
	$(B)/t_ui_reload \
	$(B)/t_swapskill_core \
	$(B)/t_swkbd \
	$(B)/t_target_rank \
	$(B)/t_textbox \
	$(B)/t_tab_text \
	$(B)/t_thread_leak \
	$(B)/t_touch \
	$(B)/t_touch_mac \
	$(B)/t_ui_move \
	$(B)/t_unit_level \
	$(B)/t_unsocket \
	$(B)/t_vault_icons \
	$(B)/t_vault_tiers \
	$(B)/t_widestash_restore \
	$(B)/t_freetp_zr \
	$(B)/t_widget
	python3 native_write_scan.py --selftest ..
	python3 widget_cache_scan.py --selftest ..
	python3 mode_scan.py ../..
	python3 ../../ios/tools/dm_i18n.py --check
	python3 ../../i18n/test_plural.py
	python3 ../../i18n/scan_literals.py
	python3 ../../tools/tests/test_warm_seed.py
	python3 ../../tools/tests/test_patched_ready.py
	python3 ../../tools/tests/test_pack_steps.py
	python3 ../../tools/tests/test_pack_shaders.py
	$(B)/t_thread_leak
	$(B)/t_loader_weak
	$(B)/t_mac_overlay
	$(PY) test_mac_features.py
	$(B)/t_rift_clock
	$(B)/t_rift_hud
	$(B)/t_rift_next
	$(B)/t_rift_oldrun
	$(B)/t_interaction_diag
	$(B)/t_loader_weak
	$(B)/t_map_diag
	$(B)/t_warp_diag
	$(B)/t_rift_wave
	$(B)/t_textbox
	$(B)/t_tab_text
	$(B)/t_itemcmp
	$(B)/t_rwlv
	$(B)/t_touch
	$(B)/t_hid_script 2>/dev/null
	$(B)/t_hid_receipt 2>/dev/null
	$(B)/t_touch_mac
	$(B)/t_oe 2>/dev/null
	$(B)/t_fs 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_fs_case 2>/dev/null
	DM_ROMFS="$(DM_ROOT)/data/switch/3.3.94012/Update/RomFS" $(B)/t_romfs_validate
	$(MAKE) dump-reader
	$(B)/t_audio
	$(B)/t_audio_spsc
	$(B)/t_swapskill_core
	$(B)/t_swkbd
	$(B)/t_altstack
	$(B)/t_crash_hang
	$(B)/t_movie
	$(B)/t_condvar
	$(B)/t_condvar_old
	$(B)/t_target_rank 2>/dev/null
	$(B)/t_hud_dead
	$(B)/t_loadwatch
	$(B)/t_namegen
	$(B)/t_minimap_lift
	$(B)/t_skill_hit 2>/dev/null
	$(B)/t_ui_reload 2>/dev/null
	$(B)/t_rw_stale 2>/dev/null
	$(B)/t_pouch_count 2>/dev/null
	$(B)/t_widestash_restore
	$(B)/t_freetp_zr
	$(B)/t_hand_guard 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_recipe_job
	$(B)/t_modetx
	$(B)/t_ui_move
	$(B)/t_unsocket
	$(B)/t_legend
	$(B)/t_looks_core
	$(B)/t_looks_store
	$(B)/t_padkeys
	$(B)/t_vault_icons
	$(B)/t_vault_tiers 2>/dev/null
	$(B)/t_padmap
	$(B)/t_unit_level
	$(B)/t_widget
	$(B)/t_portrait
	TMPDIR="$(abspath $(B))" $(B)/t_custom_relayout 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_custom_video 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_custom_video_voice 2>/dev/null
	$(B)/t_res_scale
	TMPDIR="$(abspath $(B))" $(B)/t_custom_extras 2>/dev/null
	$(B)/t_hud_players
	$(B)/t_doorspot
	$(B)/t_dmgnum 2>/dev/null
	$(B)/t_splash
	$(B)/t_splash_rule
	$(B)/t_shrine 2>/dev/null
	$(B)/t_shrine_layout
	TMPDIR="$(abspath $(B))" $(B)/t_recipes_text 2>/dev/null
	$(B)/t_item_text 2>/dev/null
	$(B)/t_itemjob
	$(B)/t_effects 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_fightlog 2>/dev/null
	$(B)/t_fightlog_key 2>/dev/null
	$(B)/t_fightlog_gates 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_fightlog_hist 2>/dev/null
	$(B)/t_fightlog_sim 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_streak 2>/dev/null
	$(B)/t_feats
	$(B)/t_feats_read
	TMPDIR="$(abspath $(B))" $(B)/t_feats_store 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_book 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_resume
	TMPDIR="$(abspath $(B))" $(B)/t_cloudsync 2>/dev/null
	TMPDIR="$(abspath $(B))" $(B)/t_cloudmerge 2>/dev/null
	for m in en zh none sysen syses systw sysru sysko syspt tags game plural; do TMPDIR="$(abspath $(B))" $(B)/t_i18n $$m || exit 1; done
	$(PY) movie_cut.py $(B)/main.orig.bin $(B)/subsdk0.orig.bin
	$(PY) oe_notify.py $(B)/main.orig.bin
$(B)/t_i18n: t_i18n.c ../dm_i18n.c ../dm_i18n.h ../dm_i18n_tab.h ../dm_plural.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_i18n.c ../dm_i18n.c
$(B)/t_mac_overlay: t_mac_overlay.m ../app_mac_overlay.h | $(B)
	clang -O1 -Wall -Werror -fobjc-arc -framework Foundation -o $@ t_mac_overlay.m
$(B)/t_hud_dead: t_hud_dead.c ../dm_hud.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_hud_dead.c
$(B)/t_looks_core: t_looks_core.c ../dm_looks_core.c ../dm_looks_core.h | $(B)
	clang -O1 -Wall -Werror -I.. -Drealloc=t_realloc -o $@ t_looks_core.c ../dm_looks_core.c
$(B)/t_padkeys: t_padkeys.c ../dm_padkeys.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_padkeys.c
$(B)/t_looks_store: t_looks_store.c t_fs_stub.c ../../android/compat/clonefile.c ../hle_fs.c ../hle.h ../dm.h ../dm_i18n.c ../dm_looks_store.c ../dm_looks_core.c ../dm_looks_core.h ../dm_looks.h ../dm_sertrack.c | $(B)
	if [ "$$(uname)" = Darwin ]; then clang -O1 -Wall -Wno-unused-function -I.. -o $@ t_looks_store.c t_fs_stub.c ../dm_i18n.c ../dm_looks_store.c ../dm_looks_core.c ../dm_sertrack.c; \
	else c=../../android/compat; clang -std=gnu11 -O1 -Wall -Wno-unused-function -Wno-misleading-indentation -Wno-format -D_GNU_SOURCE -D__ANDROID__ -D__OHOS__ -I$$c/include \
	  -include $$c/include/dm_android_compat.h -I.. -o $@ t_looks_store.c t_fs_stub.c ../dm_i18n.c ../dm_looks_store.c ../dm_looks_core.c ../dm_sertrack.c $$c/clonefile.c -lpthread; fi
$(B)/t_loadwatch: t_loadwatch.c ../dm_loadwatch.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_loadwatch.c
$(B)/t_itemcmp: t_itemcmp.c ../dm_itemcmp_core.c ../dm_itemcmp_core.h ../dm_tipx_core.c ../dm_tipx_core.h ../dm_splash_rule.c ../dm_splash_rule.h | $(B)
	clang -O1 -Wall -Werror -o $@ t_itemcmp.c ../dm_itemcmp_core.c ../dm_tipx_core.c ../dm_splash_rule.c -lm
$(B)/t_hud_players: t_hud_players.c ../dm_hud.c ../dm_hud.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_hud_players.c -lpthread
$(B)/t_textbox: t_textbox.c ../dm_textbox_core.c ../dm_textbox_core.h | $(B)
	clang -O1 -Wall -Werror -o $@ t_textbox.c ../dm_textbox_core.c
$(B)/t_rwlv: t_rwlv.c ../dm_rwlv_core.c ../dm_rwlv_core.h | $(B)
	clang -O1 -Wall -Werror -o $@ t_rwlv.c ../dm_rwlv_core.c
$(B)/t_pouch_count: t_pouch_count.c ../dm_pouch.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_pouch_count.c
$(B)/t_rw_stale: t_rw_stale.c ../dm_runeword.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_rw_stale.c ../dm_itemjob.c
$(B)/t_doorspot: t_doorspot.c ../dm_doorspot.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_doorspot.c
$(B)/t_fightlog: t_fightlog.c ../dm_fightlog_core.c ../dm_fightlog_store.c ../dm_fightlog_text.c ../dm_fightlog_hist.c ../dm_fightlog.h ../dm_fightlog_text.h ../dm_fightlog_hist.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_fightlog.c -lpthread
$(B)/t_swapskill_core: t_swapskill_core.c ../dm_swapskill_core.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_swapskill_core.c
$(B)/t_resume: t_resume.c ../dm_resume_core.c ../dm_resume_core.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_resume.c
.PHONY: resume-test
resume-test: $(B)/t_resume
	TMPDIR="$(abspath $(B))" $(B)/t_resume
$(B)/t_feats: t_feats.c ../dm_feats_core.c ../dm_feats.h ../dm_streak_core.c ../dm_streak_hist.c ../dm_streak.h ../dm_fightlog.h ../dm_rift.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_feats.c -lpthread
$(B)/t_feats_read: t_feats_read.c ../dm_feats_read.c ../dm_feats.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_feats_read.c
$(B)/t_feats_store: t_feats_store.c ../dm_feats_core.c ../dm_feats_store.c ../dm_feats.h ../dm.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_feats_store.c -lpthread

.PHONY: feats-test
feats-test: $(B)/t_feats $(B)/t_feats_read $(B)/t_feats_store $(B)/t_fightlog_gates
	$(B)/t_feats
	$(B)/t_feats_read
	TMPDIR="$(abspath $(B))" $(B)/t_feats_store 2>/dev/null
	$(B)/t_fightlog_gates 2>/dev/null
$(B)/t_streak: t_streak.c ../dm_streak_core.c ../dm_streak_hist.c ../dm_streak.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_streak.c -lpthread
$(B)/t_fightlog_gates: t_fightlog_gates.c ../dm_fightlog.c ../dm_fightlog_core.c ../dm_fightlog_store.c ../dm_fightlog_text.c ../dm_fightlog_hist.c ../dm_feats_core.c ../dm_feats.h ../dm_fightlog.h ../dm_fightlog_hist.h ../dm.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_fightlog_gates.c ../dm_feats_core.c -lpthread
$(B)/t_fightlog_hist: t_fightlog_hist.c ../dm_fightlog_hist.c ../dm_fightlog_hist.h ../dm_fightlog.h ../dm_fightlog_text.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_fightlog_hist.c -lpthread
$(B)/t_fightlog_key: t_fightlog_key.c ../dm_fightlog_core.c ../dm_fightlog_text.c ../dm_fightlog_ui.c ../dm_fightlog_hist.c ../dm_fightlog.h ../dm_fightlog_text.h ../dm_sim_ui.h ../dm_sim_core.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_fightlog_key.c -lpthread
$(B)/t_fightlog_sim: t_fightlog_sim.c ../dm_fightlog_core.c ../dm_fightlog_text.c ../dm_fightlog_ui.c ../dm_fightlog_hist.c ../dm_fightlog_hist.h ../dm_fightlog.h ../dm_fightlog_text.h ../dm_sim_ui.h ../dm_sim_core.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_fightlog_sim.c -lpthread
$(B)/t_effects: t_effects.c ../dm_effects.c ../dm_effects.h ../dm.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_effects.c -lpthread
$(B)/t_res_scale: t_res_scale.c ../dm_res_scale.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_res_scale.c
$(B)/t_widget: t_widget.c ../dm_widget.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_widget.c
$(B)/t_portrait: t_portrait.c ../dm_portrait.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_portrait.c -lm
$(B)/t_custom_relayout: t_custom_relayout.c t_custom_carousel_fake.h ../dm_custom.c ../dm_tab_text.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ t_custom_relayout.c ../dm_tab_text.c
$(B)/t_custom_extras: t_custom_extras.c t_custom_carousel_fake.h ../dm_custom.c ../dm_tab_text.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ t_custom_extras.c ../dm_tab_text.c
$(B)/t_custom_video: t_custom_video.c ../dm_custom.c ../dm_tab_text.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ t_custom_video.c ../dm_tab_text.c

$(B)/t_custom_video_voice: t_custom_video.c ../dm_custom.c ../dm_tab_text.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -DDM_MOVIE_FFMPEG=1 -I.. -o $@ t_custom_video.c ../dm_tab_text.c
$(B)/t_unit_level: t_unit_level.c ../dm_unit.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_unit_level.c
$(B)/t_padmap: t_padmap.c ../dm_padmap.h ../dm_input.h | $(B)
	clang -O1 -Wall -Werror -o $@ t_padmap.c
$(B)/t_minimap_lift: t_minimap_lift.c ../dm_cursor.c ../dm_cursor.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_minimap_lift.c
$(B)/t_skill_hit: t_skill_hit.c ../dm_cursor.c ../dm_cursor.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_skill_hit.c
$(B)/t_unsocket: t_unsocket.c ../dm_unsocket_core.c ../dm_unsocket.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_unsocket.c ../dm_unsocket_core.c
$(B)/t_ui_reload: t_ui_reload.c ../dm_cursor.c ../dm_cursor.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_ui_reload.c
$(B)/t_widestash_restore: t_widestash_restore.c ../dm_cursor.c ../dm_cursor.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_widestash_restore.c
$(B)/t_freetp_zr: t_freetp_zr.c ../dm_freetp.c ../hle.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_freetp_zr.c
$(B)/t_hand_guard: t_hand_guard.c ../dm_cursor.c ../dm_cursor.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -Wl,-dead_strip -o $@ t_hand_guard.c
$(B)/t_recipe_job: t_recipe_job.c ../dm_recipes.c ../dm_recipes.h ../dm.h ../dm_hud.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_recipe_job.c ../dm_itemjob.c
$(B)/t_modetx: t_modetx.c ../dm_modetx.h | $(B)
	clang -O1 -Wall -Werror -o $@ t_modetx.c
$(B)/t_namegen: t_namegen.c ../dm_namegen.c | $(B)
	clang -O1 -Wall -Werror -o $@ t_namegen.c ../dm_namegen.c
$(B)/t_cloudmerge: t_cloudmerge.c ../dm_cloudmerge.c ../dm.h ../hle.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_cloudmerge.c
$(B)/t_cloudsync: t_cloudsync.c ../dm_cloudsync.c ../dm.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_cloudsync.c ../dm_cloudsync.c
$(B)/t_touch: t_touch.c ../hle_touch.c ../front.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_touch.c ../hle_touch.c
$(B)/t_fs: t_fs.c t_fs_stub.c ../hle_fs.c ../hle.h ../dm.h ../dm_i18n.c | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_fs.c t_fs_stub.c ../dm_i18n.c
$(B)/t_audio: t_audio.c ../hle_audio.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_audio.c -framework AudioToolbox -framework CoreAudio -framework CoreFoundation
$(B)/t_audio_spsc: t_audio_spsc.c ../hle_audio.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_audio_spsc.c -framework AudioToolbox -framework CoreAudio -framework CoreFoundation
$(B)/t_audio_spsc_tsan: t_audio_spsc.c ../hle_audio.c ../hle.h ../dm.h | $(B)
	clang -O1 -g -O1 -fsanitize=thread -Wno-unused-function -I.. -o $@ t_audio_spsc.c -framework AudioToolbox -framework CoreAudio -framework CoreFoundation
$(B)/t_swkbd: t_swkbd.c ../hle_swkbd.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_swkbd.c ../hle_swkbd.c
$(B)/t_movie: t_movie.c ../hle_movie.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_movie.c ../hle_movie.c
$(B)/t_movie_real: t_movie_real.c ../hle_movie.c ../dm_movie_decode.c ../dm_movie_decode.h ../hle.h ../dm.h | $(B)
	clang $(MOVIE_TEST_FLAGS) -Wall -Wno-unused-function -DDM_MOVIE_FFMPEG=1 -I.. -o $@ t_movie_real.c ../dm_movie_decode.c $(shell pkg-config --cflags --libs $(MOVIE_PACKAGES))
movie-real: $(B)/t_movie_real
	$(PY) movie_real.py $(B)/t_movie_real
$(B)/t_altstack: t_altstack.c t_altstack_stub.c ../crash.c ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_altstack.c t_altstack_stub.c ../crash.c



$(B)/t_crash_hang: t_crash_hang.c t_altstack_stub.c ../crash.c ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_crash_hang.c t_altstack_stub.c ../crash.c
$(B)/t_splash: t_splash.c ../dm_splash.c ../dm_splash_rule.c ../dm_splash_rule.h ../dm_splash.h ../dm.h ../hle.h ../dm_custom.h ../dm_unit.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_splash.c ../dm_splash_rule.c -lm

$(B)/t_shrine: t_shrine.c ../dm_shrine.c ../dm.h ../hle.h ../dm_custom.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_shrine.c
$(B)/t_splash_rule: t_splash_rule.c ../dm_splash_rule.c ../dm_splash_rule.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_splash_rule.c ../dm_splash_rule.c -lm
$(B)/t_shrine_layout: t_shrine_layout.c ../dm_shrine_layout.c ../dm.h ../dm_custom.h ../dm_build.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_shrine_layout.c -lm

$(B)/t_dmgnum: t_dmgnum.c ../dm_dmgnum.c ../dm_splash.h ../dm_combat_log.inc ../dm_combat_probe.inc ../dm.h ../hle.h ../dm_custom.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_dmgnum.c -lpthread


$(B)/t_recipes_text: t_recipes_text.c ../dm_recipes.c ../dm_recipes.h ../dm.h ../dm_hud.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ t_recipes_text.c ../dm_itemjob.c -lpthread




$(B)/t_itemjob: t_itemjob.c ../dm_itemjob.c ../dm_itemjob.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_itemjob.c ../dm_itemjob.c -lpthread
$(B)/t_item_text: t_item_text.c ../dm_recipes.c ../dm_recipes.h ../dm.h ../dm_hud.h | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ t_item_text.c ../dm_itemjob.c -lpthread
$(B)/%.orig.bin: $(EXEFS)/% | $(B)
	$(PY) ../../tools/nso.py $< $@ > /dev/null
$(B):
	mkdir -p $@
.PHONY: run tsan
tsan: $(B)/t_audio_spsc_tsan
	$(B)/t_audio_spsc_tsan
$(B)/t_condvar: t_condvar.c t_condvar_stub.c ../hle_thread.c ../dm.h | $(B)
	clang -O1 -Wno-unused-function -DDM_TEST_HOOKS -I.. -o $@ t_condvar.c t_condvar_stub.c ../hle_thread.c
$(B)/t_condvar_old: t_condvar.c t_condvar_stub.c ../hle_thread.c ../dm.h | $(B)
	clang -O1 -Wno-unused-function -DDM_TEST_HOOKS -DDM_TEST_CV_OLD_ORDER -I.. -o $@ t_condvar.c t_condvar_stub.c ../hle_thread.c

$(B)/t_fs_case: t_fs_case.c ../hle_fs.c ../hle.h ../dm.h ../dm_i18n.c | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_fs_case.c ../dm_i18n.c


DUMP_FIXTURE_PY ?= python3
DUMP_READER_SRC = ../dump_reader.c ../rd_crypto.c ../rd_crypto_arm.c
DUMP_READER_DEP = t_dump_reader.c $(DUMP_READER_SRC) ../dump_reader.h ../rd_crypto.h ../romfs_validate.h
$(B)/dump_fixture/contract.tsv: ../../tools/mk_dump_fixture.py ../../tools/dump_fixture_vectors.tsv | $(B)
	$(DUMP_FIXTURE_PY) ../../tools/mk_dump_fixture.py --out $(B)/dump_fixture
$(B)/t_dump_reader: $(DUMP_READER_DEP) | $(B)
	$(CC) -std=c11 -O1 -Wall -Wextra -Werror -o $@ t_dump_reader.c $(DUMP_READER_SRC) -lpthread
$(B)/t_dump_reader_san: $(DUMP_READER_DEP) | $(B)
	$(CC) -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -o $@ t_dump_reader.c $(DUMP_READER_SRC) -lpthread
.PHONY: dump-reader
dump-reader: $(B)/t_dump_reader $(B)/t_dump_reader_san $(B)/dump_fixture/contract.tsv
	DM_DUMP_FIXTURE="$(abspath $(B)/dump_fixture)" $(B)/t_dump_reader
	DM_DUMP_FIXTURE="$(abspath $(B)/dump_fixture)" ASAN_OPTIONS=detect_leaks=1 $(B)/t_dump_reader_san



.PHONY: dump-reader-real
dump-reader-real: $(B)/t_dump_reader
	@test -n "$(DM_REAL_DUMP_DIR)" || { echo "DM_REAL_DUMP_DIR not set"; exit 2; }
	DM_REAL_DUMP_DIR="$(DM_REAL_DUMP_DIR)" DM_REAL_OUT="$(abspath $(B)/real_dump)" $(B)/t_dump_reader
$(B)/t_romfs_validate: t_romfs_validate.c ../romfs_validate.c ../romfs_validate.h ../dm_i18n.c ../dm_i18n_tab.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_romfs_validate.c ../romfs_validate.c ../dm_i18n.c

$(B)/t_hid_script: t_hid_script.c t_touch_tick_stub.c ../hle_hid.c ../dm_cursor.c ../dm_hud.c ../dm_hud.h ../touch_mac.c ../../ios/Sources/dm_touch_controls.c ../hle.h ../dm.h ../dm_input.h ../front.h | $(B) ../dm_prof.c
	clang -O1 -Wno-unused-function -I.. -I../../ios/Sources -o $@ t_hid_script.c t_touch_tick_stub.c ../dm_cursor.c ../dm_hud.c ../touch_mac.c ../../ios/Sources/dm_touch_controls.c -lm ../dm_prof.c

$(B)/t_oe: t_oe.c ../hle_oe.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_oe.c

$(B)/t_hid_receipt: t_hid_receipt.c t_touch_tick_stub.c ../hle_hid.c ../dm_cursor.c ../dm_hud.c ../dm_hud.h ../touch_mac.c ../../ios/Sources/dm_touch_controls.c ../hle.h ../dm.h ../dm_input.h ../front.h | $(B) ../dm_prof.c
	clang -O1 -Wno-unused-function -I.. -I../../ios/Sources -o $@ t_hid_receipt.c t_touch_tick_stub.c ../dm_cursor.c ../dm_hud.c ../touch_mac.c ../../ios/Sources/dm_touch_controls.c -lm ../dm_prof.c
$(B)/t_touch_mac: t_touch_mac.c ../touch_mac.c ../dm_cursor.c ../dm_hud.c ../dm_hud.h ../../ios/Sources/dm_touch_controls.c ../../ios/Sources/dm_touch_controls.h ../../host/dm_input.h | $(B)
	clang -std=c11 -Wall -Wextra -Werror -I.. -I../../ios/Sources -o $@ t_touch_mac.c ../dm_cursor.c ../dm_hud.c -lm ../dm_prof.c
$(B)/t_target_rank: t_target_rank.c ../hle_target.c ../dm.h | $(B)
	clang -O1 -Wno-unused-function -I.. -o $@ t_target_rank.c ../hle_target.c
$(B)/t_rift_clock: t_rift_clock.c ../dm_rift.c ../dm_rift.h ../hle_os.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wall -Wno-unused-function -ffunction-sections -fdata-sections -Wl,-dead_strip -I.. -o $@ t_rift_clock.c ../hle_os.c
$(B)/t_rift_next: t_rift_next.c ../dm_rift.c ../dm_rift.h ../hle_os.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wall -Wno-unused-function -ffunction-sections -fdata-sections -Wl,-dead_strip -I.. -o $@ t_rift_next.c ../hle_os.c
$(B)/t_rift_oldrun: t_rift_oldrun.c ../dm_rift.c ../dm_rift.h ../hle_os.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wall -Wno-unused-function -ffunction-sections -fdata-sections -Wl,-dead_strip -I.. -o $@ t_rift_oldrun.c ../hle_os.c
$(B)/t_rift_wave: t_rift_wave.c ../dm_rift.c ../dm_rift.h ../hle_os.c ../hle.h ../dm.h | $(B)
	clang -O1 -Wall -Wno-unused-function -ffunction-sections -fdata-sections -Wl,-dead_strip -I.. -o $@ t_rift_wave.c ../hle_os.c
$(B)/t_thread_leak: t_thread_leak.c ../thread.c ../dm.h | $(B)
	clang -O1 -Wall -Wno-unused-function -I.. -I../../ios/Sources -o $@ t_thread_leak.c

$(B)/t_interaction_diag: t_interaction_diag.c ../dm_world.c ../dm_unit.h ../touch_mac.h | $(B)
	clang -O1 -Wno-unused-function -Wl,-dead_strip -I.. -o $@ t_interaction_diag.c

$(B)/t_loader_weak: t_loader_weak.c ../loader.c ../hle.c ../dm.h ../hle.h | $(B)
	clang -O1 -Wno-deprecated-declarations -I.. -Wl,-dead_strip -o $@ t_loader_weak.c ../hle.c

$(B)/t_map_diag: t_map_diag.c ../dm_cursor.c ../dm_unit.h | $(B)
	clang -O1 -Wno-unused-function -Wl,-dead_strip -I.. -o $@ t_map_diag.c

$(B)/t_warp_diag: t_warp_diag.c ../dm_rift.c ../dm_unit.h | $(B)
	clang -O1 -Wno-unused-function -Wl,-dead_strip -I.. -o $@ t_warp_diag.c

$(B)/t_loader_weak: t_loader_weak.c ../loader.c ../hle.c ../dm.h ../hle.h | $(B)
	clang -O1 -Wno-deprecated-declarations -I.. -Wl,-dead_strip -o $@ t_loader_weak.c ../hle.c

$(B)/t_rift_hud: t_rift_hud.m ../../ios/Sources/dm_rift_hud.m ../../ios/Sources/dm_rift_hud.h ../dm_rift.h | $(B)
	clang -O1 -Wall -Werror -Wno-deprecated-declarations -fobjc-arc -I.. -framework Cocoa -framework QuartzCore -framework CoreText -o $@ t_rift_hud.m ../../ios/Sources/dm_rift_hud.m

$(B)/t_ui_move: t_ui_move.c ../dm_cursor.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wl,-dead_strip -o $@ t_ui_move.c
$(B)/t_legend: t_legend.c ../dm_padmode.c $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wl,-dead_strip -o $@ t_legend.c
$(B)/t_vault_icons: t_vault_icons.c ../dm_vault_ui.c ../dm_vault_tab.h $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wl,-dead_strip -o $@ t_vault_icons.c
$(B)/t_vault_tiers: t_vault_tiers.c ../dm_vault_ui.c ../dm_vault_tab.h $(wildcard ../*.h) | $(B)
	clang -O1 -Wall -Werror -Wno-unused-function -Wl,-dead_strip -o $@ t_vault_tiers.c


../dm_vault_tab.h: ../../ios/tools/gen_vault_table.py
	$(MAKE) -C .. dm_vault_tab.h DM_ROOT="$(DM_ROOT)" ROMFS="$(ROMFS)"

SIM_SRC = ../dm_sim_calc.c ../dm_sim_data.c ../dm_sim_core.c ../dm_sim_skill.c ../dm_sim_timeline.c ../dm_sim_report.c ../dm_splash_rule.c
SIM_CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Werror
$(B)/t_sim: t_sim.c $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim.c $(SIM_SRC) -lm
$(B)/t_sim_fight: t_sim_fight.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_fight.c $(SIM_SRC) -lm
$(B)/t_sim_ref: t_sim_ref.c t_sim_review.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_ref.c t_sim_review.c $(SIM_SRC) -lm
$(B)/sim_cli: sim_cli.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ sim_cli.c $(SIM_SRC) -lm
.PHONY: sim-test sim-accept sim-cli sim-bench
sim-test: $(B)/t_sim $(B)/t_sim_fight $(B)/t_gear_core $(B)/t_gear_compose
	$(B)/t_sim_itemcmp
	$(B)/t_sim
	$(B)/t_gear_core score
	DM_SIM_DATA= $(B)/t_sim_fight
	python3 ../../ios/tools/test_gen_sim_data.py
	python3 ../../ios/tools/test_gen_itemcmp_data.py
	DM_GEAR_DATA= $(B)/t_gear_compose
sim-accept: sim-test $(B)/t_sim_ref $(B)/t_gear_grants $(B)/t_gear_dual $(B)/t_sim_hands
	test -n "$(DM_SIM_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_ref sim_skills.manifest
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_runtime
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_fight
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_std
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_gear_core tables
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_gear_grants
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_gear_dual
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_hands
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/t_sim_level1
	test -n "$(DM_GEAR_DATA)"
	DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_compose
sim-cli: $(B)/sim_cli
sim-bench: $(B)/sim_cli
	DM_SIM_DATA="$(DM_SIM_DATA)" $(B)/sim_cli --bench
$(B)/t_sim_std: t_sim_std.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_std.c $(SIM_SRC) -lm
sim-accept: $(B)/t_sim_std
$(B)/t_sim_runtime: t_sim_runtime.c sim_test_io.h ../dm_sim.c $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_runtime.c ../dm_sim.c $(SIM_SRC) -lm -lpthread
$(B)/t_gear_dual: t_gear_dual.c sim_test_io.h ../dm_gear_core.c ../dm_gear_core.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_dual.c ../dm_gear_core.c $(SIM_SRC) -lm
$(B)/t_sim_hands: t_sim_hands.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_hands.c $(SIM_SRC) -lm
$(B)/t_gear_grants: t_gear_grants.c sim_test_io.h ../dm_gear_core.c ../dm_gear_core.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_grants.c ../dm_gear_core.c $(SIM_SRC) -lm
$(B)/t_gear_compose: t_gear_compose.c ../dm_gear_compose.c ../dm_gear_compose.h $(B)/sim_itemcmp.o ../dm_itemcmp_core.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_compose.c ../dm_gear_compose.c $(B)/sim_itemcmp.o
$(B)/t_gear_core: t_gear_core.c sim_test_io.h ../dm_gear_core.c ../dm_gear_core.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_core.c ../dm_gear_core.c $(SIM_SRC) -lm

.PHONY: gear-order-test gear-equip-tab-test
sim-test: gear-order-test gear-equip-test gear-equip-tab-test
gear-order-test: $(B)/t_gear_order
	$(B)/t_gear_order
$(B)/t_gear_order: t_gear_order.c ../dm_gear_order.c ../dm_gear_order.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -I.. -o $@ t_gear_order.c ../dm_gear_order.c


SAN_RUN ?=
GEAR_THREAD_SRC = t_gear_threads.c ../dm_gear_core.c ../dm_gear_live.c ../dm_itemcmp_core.c ../dm_sim.c $(SIM_SRC)
GEAR_THREAD_DEPS = $(GEAR_THREAD_SRC) sim_test_io.h ../dm_gear_core.h ../dm_gear_live.h ../dm_itemcmp_core.h ../dm_sim_ui.h ../dm_sim_core.h ../dm_sim_internal.h
$(B)/t_gear_threads_tsan: $(GEAR_THREAD_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=thread -Wno-misleading-indentation -I.. -o $@ $(GEAR_THREAD_SRC) -lm -lpthread
$(B)/t_gear_threads_asan: $(GEAR_THREAD_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -Wno-misleading-indentation -I.. -o $@ $(GEAR_THREAD_SRC) -lm -lpthread


GEAR_SEARCH_SRC = ../dm_gear_search.c ../dm_gear_bound.c ../dm_gear_compose.c ../dm_gear_core.c ../dm_gear_dump.c $(SIM_SRC)
GEAR_SEARCH_DEPS = $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o t_gear_stock.h gear_brute.c gear_brute.h sim_test_io.h ../dm_gear_search.h ../dm_gear_bound.h ../dm_gear_compose.h \
	../dm_gear_dump.h ../dm_gear_core.h ../dm_itemcmp_core.h ../dm_sim_core.h ../dm_sim_internal.h
$(B)/t_gear_search: t_gear_search.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_search.c gear_brute.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread
$(B)/t_gear_bound: t_gear_bound.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_bound.c gear_brute.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread

$(B)/t_gear_dump: t_gear_dump.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_dump.c gear_brute.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread

$(B)/t_gear_tab: t_gear_tab.c ../dm_gear_tab.c ../dm_gear_tab.h ../dm_gear_live.c ../dm_gear_live.h $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_tab.c gear_brute.c ../dm_gear_tab.c ../dm_gear_live.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread
$(B)/t_gear_tab_tsan: t_gear_tab.c ../dm_gear_tab.c ../dm_gear_tab.h ../dm_gear_live.c ../dm_gear_live.h $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=thread -Wno-misleading-indentation -I.. -o $@ t_gear_tab.c gear_brute.c ../dm_gear_tab.c ../dm_gear_live.c $(GEAR_SEARCH_SRC) ../dm_itemcmp_core.c -lm -lpthread
$(B)/gear_bench: gear_bench.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ gear_bench.c gear_brute.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread

NDK_CC = $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang
$(B)/gear_bench_arm64: gear_bench.c $(GEAR_SEARCH_DEPS) | $(B)
	test -n "$(NDK)"
	$(NDK_CC) -std=c11 -O2 -Wall -Wextra -I.. -static -o $@ gear_bench.c gear_brute.c $(GEAR_SEARCH_SRC) ../dm_itemcmp_core.c -lm
.PHONY: gear-test gear-bench gear-bench-arm64
gear-bench-arm64: $(B)/gear_bench_arm64
gear-test: $(B)/t_gear_bound $(B)/t_gear_search $(B)/t_gear_tab $(B)/t_gear_dump
	test -n "$(DM_SIM_DATA)" -a -n "$(DM_GEAR_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_bound
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_search
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_tab
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_dump
gear-bench: $(B)/gear_bench
	test -n "$(DM_SIM_DATA)" -a -n "$(DM_GEAR_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/gear_bench

$(B)/t_gear_pages: t_gear_pages.c ../dm_gear_pages_core.c ../dm_gear_pages.h ../dm_gear_tab.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_pages.c ../dm_gear_pages_core.c
$(B)/t_gear_pages_asan: t_gear_pages.c ../dm_gear_pages_core.c ../dm_gear_pages.h ../dm_gear_tab.h | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -I.. -o $@ t_gear_pages.c ../dm_gear_pages_core.c

$(B)/t_itembuild: t_itembuild.c ../dm_itembuild_core.c ../dm_itembuild.h | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -I.. -o $@ t_itembuild.c ../dm_itembuild_core.c
.PHONY: gear-pages-test
gear-equip-test: $(B)/t_gear_equip $(B)/t_gear_equip_asan
	$(B)/t_gear_equip
	$(SAN_RUN) $(B)/t_gear_equip_asan
$(B)/t_gear_equip: t_gear_equip.c ../dm_gear_equip_core.c ../dm_gear_equip_core.h ../dm_gear_order.c ../dm_gear_order.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -I.. -o $@ t_gear_equip.c ../dm_gear_equip_core.c ../dm_gear_order.c
$(B)/t_gear_equip_asan: t_gear_equip.c ../dm_gear_equip_core.c ../dm_gear_equip_core.h ../dm_gear_order.c ../dm_gear_order.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -I.. -o $@ t_gear_equip.c ../dm_gear_equip_core.c ../dm_gear_order.c
gear-equip-tab-test: $(B)/t_gear_equip_tab $(B)/t_gear_equip_tab_asan $(B)/t_gear_equip_tab_text $(B)/t_gear_equip_tab_text_asan
	$(B)/t_gear_equip_tab
	$(SAN_RUN) $(B)/t_gear_equip_tab_asan
	$(B)/t_gear_equip_tab_text
	$(SAN_RUN) $(B)/t_gear_equip_tab_text_asan
$(B)/t_gear_equip_tab_text: t_gear_equip_tab_text.c ../dm_gear_equip_tab.c ../dm_gear_equip_want.c ../dm_feats_core.c ../dm_gear_equip_tab.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -I.. -o $@ t_gear_equip_tab_text.c ../dm_gear_equip_want.c ../dm_feats_core.c -lpthread
$(B)/t_gear_equip_tab_text_asan: t_gear_equip_tab_text.c ../dm_gear_equip_tab.c ../dm_gear_equip_want.c ../dm_feats_core.c ../dm_gear_equip_tab.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -I.. -o $@ t_gear_equip_tab_text.c ../dm_gear_equip_want.c ../dm_feats_core.c -lpthread
$(B)/t_gear_equip_tab: t_gear_equip_tab.c ../dm_gear_equip_want.c ../dm_gear_equip_tab.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -I.. -o $@ t_gear_equip_tab.c ../dm_gear_equip_want.c
$(B)/t_gear_equip_tab_asan: t_gear_equip_tab.c ../dm_gear_equip_want.c ../dm_gear_equip_tab.h | $(B)
	$(CC) -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -I.. -o $@ t_gear_equip_tab.c ../dm_gear_equip_want.c
gear-equip-rules-test: $(B)/t_gear_equip_rules $(B)/t_gear_equip_rules_asan
	test -n "$(DM_SIM_DATA)" -a -n "$(DM_GEAR_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_equip_rules
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(SAN_RUN) $(B)/t_gear_equip_rules_asan
$(B)/t_gear_equip_rules_asan: t_gear_equip_rules.c ../dm_gear_equip_rules.c ../dm_gear_equip_rules.h ../dm_gear_order.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -Wno-misleading-indentation -I.. -o $@ t_gear_equip_rules.c ../dm_gear_equip_rules.c ../dm_gear_order.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread
$(B)/t_gear_equip_rules: t_gear_equip_rules.c ../dm_gear_equip_rules.c ../dm_gear_equip_rules.h ../dm_gear_order.c $(GEAR_SEARCH_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_gear_equip_rules.c ../dm_gear_equip_rules.c ../dm_gear_order.c $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread

GEAR_CAPTURE_SRC = t_gear_equip_capture.c ../dm_gear_equip.c ../dm_gear_equip_want.c ../dm_gear_equip_core.c ../dm_gear_equip_rules.c ../dm_gear_order.c
GEAR_CAPTURE_DEPS = $(GEAR_CAPTURE_SRC) ../dm_gear_equip.h ../dm_gear_equip_tab.h ../dm_gear_equip_core.h ../dm_gear_equip_rules.h ../dm_gear_order.h $(GEAR_SEARCH_DEPS)
$(B)/t_gear_equip_capture: $(GEAR_CAPTURE_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -D_GNU_SOURCE -DDM_PLAYER_BUILD -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ $(GEAR_CAPTURE_SRC) $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread
$(B)/t_gear_equip_capture_asan: $(GEAR_CAPTURE_DEPS) | $(B)
	$(CC) $(SIM_CFLAGS) -O1 -D_GNU_SOURCE -DDM_PLAYER_BUILD -fsanitize=address,undefined -fno-sanitize-recover=all -Wno-unused-function -Wno-misleading-indentation -I.. -o $@ $(GEAR_CAPTURE_SRC) $(GEAR_SEARCH_SRC) $(B)/sim_itemcmp.o -lm -lpthread
.PHONY: gear-equip-capture-test
gear-equip-capture-test: $(B)/t_gear_equip_capture $(B)/t_gear_equip_capture_asan
	test -n "$(DM_SIM_DATA)" -a -n "$(DM_GEAR_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(B)/t_gear_equip_capture
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(SAN_RUN) $(B)/t_gear_equip_capture_asan
gear-test: gear-equip-capture-test
gear-pages-test: $(B)/t_gear_pages $(B)/t_gear_pages_asan $(B)/t_itembuild
	$(B)/t_gear_pages
	$(SAN_RUN) $(B)/t_gear_pages_asan
	$(SAN_RUN) $(B)/t_itembuild
.PHONY: gear-threads
gear-threads: $(B)/t_gear_threads_tsan $(B)/t_gear_threads_asan $(B)/t_gear_tab_tsan
	test -n "$(DM_SIM_DATA)" -a -n "$(DM_GEAR_DATA)"
	DM_SIM_DATA="$(DM_SIM_DATA)" $(SAN_RUN) $(B)/t_gear_threads_tsan
	DM_SIM_DATA="$(DM_SIM_DATA)" $(SAN_RUN) $(B)/t_gear_threads_asan
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_GEAR_DATA="$(DM_GEAR_DATA)" $(SAN_RUN) $(B)/t_gear_tab_tsan
$(B)/t_sim_itemcmp: t_sim_itemcmp.c $(SIM_SRC) $(B)/sim_itemcmp.o ../dm_itemcmp_core.h ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_itemcmp.c $(SIM_SRC) $(B)/sim_itemcmp.o -lm
sim-test: $(B)/t_sim_itemcmp
$(B)/t_sim_validation: t_sim_validation.c t_sim.c $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_validation.c $(SIM_SRC) -lm
.PHONY: sim-validation
sim-validation: $(B)/t_sim_validation
	$(B)/t_sim_validation
$(B)/t_combat_log: t_combat_log.c t_dmgnum.c ../dm_dmgnum.c ../dm_combat_log.inc ../dm_combat_probe.inc ../dm.h ../hle.h | $(B)
	$(CC) -std=c11 -O1 -Wall -Werror -Wno-unused-function -I.. -o $@ t_combat_log.c -lpthread
.PHONY: combat-log-test
combat-log-test: $(B)/t_combat_log
	$(B)/t_combat_log

.PHONY: repo-checks
repo-checks: $(B)/t_combat_log
	python3 native_write_scan.py --selftest ..
	python3 widget_cache_scan.py --selftest ..
	python3 mode_scan.py ../..
	python3 ../../i18n/scan_literals.py
	$(B)/t_combat_log 2>/dev/null
sim-accept: $(B)/t_sim_runtime

$(B)/t_book: t_book.c ../dm_book.c ../dm_book_core.c ../dm_book.h ../dm_fightlog.h ../dm_sim_ui.c ../dm_sim_ui.h ../dm_gear_core.c ../dm_sim.c $(SIM_SRC) ../dm_sim_core.h | $(B)
	$(CC) $(SIM_CFLAGS) -Wno-unused-function -I.. -o $@ t_book.c ../dm_sim_ui.c ../dm_gear_core.c ../dm_sim.c $(SIM_SRC) -lm -lpthread
$(B)/t_sim_level1: t_sim_level1.c sim_test_io.h ../dm_sim_ui.c ../dm_sim_ui.h ../dm_gear_core.c ../dm_gear_core.h ../dm_sim.c $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_level1.c ../dm_sim_ui.c ../dm_gear_core.c ../dm_sim.c $(SIM_SRC) -lm -lpthread
sim-accept: $(B)/t_sim_level1

$(B)/sim_itemcmp.o: ../dm_itemcmp_core.c ../dm_itemcmp_core.h | $(B)
	$(CC) -std=c11 -O0 -Wall -Wextra -c ../dm_itemcmp_core.c -o $@

$(B)/t_sim_ui: t_sim_ui.c sim_test_io.h ../dm_sim_ui.c ../dm_sim_ui.h ../dm_sim_text.h ../dm_gear_core.c ../dm_gear_core.h ../dm_sim_core.h ../dm_sim_internal.h ../dm_sim.c $(SIM_SRC) | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ t_sim_ui.c ../dm_sim_ui.c ../dm_gear_core.c ../dm_sim.c $(SIM_SRC) -lm -lpthread
.PHONY: sim-ui-test
sim-ui-test: $(B)/t_sim_ui $(B)/t_sim_bindings $(B)/t_sim_text
	DM_SIM_DATA="$(DM_SIM_DATA)" DM_SIM_UI_DIR="$(DM_SIM_UI_DIR)" $(B)/t_sim_ui
	$(B)/t_sim_bindings
	$(B)/t_sim_text
$(B)/t_sim_text: t_sim_text.c ../dm_sim_text.h | $(B)
	$(CC) $(SIM_CFLAGS) -o $@ t_sim_text.c -lm

$(B)/t_sim_bindings: t_sim_bindings.c ../../ios/Sources/dm_touch_game_bridge.c ../../ios/Sources/dm_touch_game_identity.c ../../ios/Sources/dm_touch_game_state.c ../../ios/Sources/dm_touch_game_state.h | $(B)
	$(CC) $(SIM_CFLAGS) -DDM_TG_TEST_SYNTHETIC_IDENTITY -o $@ t_sim_bindings.c ../../ios/Sources/dm_touch_game_bridge.c ../../ios/Sources/dm_touch_game_identity.c ../../ios/Sources/dm_touch_game_state.c -lpthread

$(B)/t_tab_text: t_tab_text.c ../dm_tab_text.c ../dm_tab_text.h ../dm_widget.h | $(B)
	clang -O1 -Wall -Werror -I.. -o $@ t_tab_text.c
$(B)/sim_matrix: sim_matrix.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ sim_matrix.c $(SIM_SRC) -lm
$(B)/sim_monte_carlo: sim_monte_carlo.c sim_test_io.h $(SIM_SRC) ../dm_sim_core.h ../dm_sim_internal.h | $(B)
	$(CC) $(SIM_CFLAGS) -I.. -o $@ sim_monte_carlo.c $(SIM_SRC) -lm
.PHONY: sim-matrix sim-monte-carlo
sim-matrix: $(B)/sim_matrix
	test -n "$(DM_SIM_DATA)"
	$(B)/sim_matrix "$(DM_SIM_DATA)"
sim-monte-carlo: $(B)/sim_monte_carlo
	test -n "$(DM_SIM_DATA)"
	$(B)/sim_monte_carlo "$(DM_SIM_DATA)"
