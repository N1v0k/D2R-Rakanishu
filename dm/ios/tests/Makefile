DM_ROOT ?= $(HOME)/dm-work
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = t_touch_controls
METAL_CACHE_TARGET = t_metal_cache_guard
BINDING_TARGETS = t_native_binding t_game_bridge t_binding_freshness t_portal_sequence t_binding_group t_native_hud_identity t_native_hud_bridge
BINDING_FIXTURES = fixtures/native_binding/mapping.bin fixtures/native_binding/bindings.bin


HAVE_BINDING_FIXTURES := $(and $(wildcard fixtures/native_binding/mapping.bin),$(wildcard fixtures/native_binding/bindings.bin))
fixture_test = $(if $(HAVE_BINDING_FIXTURES),./$(1) $(BINDING_FIXTURES),@echo "跳过 $(1)：没有夹具 $(BINDING_FIXTURES)（公开仓库不带，要自己从运行中的游戏抓，见 fixtures/native_binding/README）")

.PHONY: all run test clean
all: $(TARGET) $(BINDING_TARGETS) t_native_art t_skill_aim t_joy_travel $(METAL_CACHE_TARGET)
run: test
$(TARGET): t_touch_controls.c $(wildcard ../Sources/dm_touch_controls.[ch]) ../../host/dm_input.h
	$(CC) $(CFLAGS) -I../Sources t_touch_controls.c ../Sources/dm_touch_controls.c -lm -o $@
t_native_binding: t_native_binding.c ../Sources/dm_touch_game_state.c ../Sources/dm_touch_game_state.h ../Sources/dm_touch_controls.c ../Sources/dm_touch_controls.h
	$(CC) $(CFLAGS) -I../Sources t_native_binding.c ../Sources/dm_touch_game_state.c ../Sources/dm_touch_controls.c -lm -o $@
t_game_bridge: t_game_bridge.c ../Sources/dm_touch_game_bridge.c ../Sources/dm_touch_game_identity.c ../Sources/dm_touch_game_identity.h ../Sources/dm_touch_game_state.c ../Sources/dm_touch_game_state.h ../../host/dm.h
	$(CC) $(CFLAGS) -DDM_TG_TEST_SYNTHETIC_IDENTITY -I../Sources t_game_bridge.c ../Sources/dm_touch_game_bridge.c ../Sources/dm_touch_game_identity.c ../Sources/dm_touch_game_state.c -lpthread -o $@
t_binding_freshness: t_binding_freshness.c ../Sources/dm_touch_game_state.c ../Sources/dm_touch_game_state.h ../Sources/dm_touch_controls.c ../Sources/dm_touch_controls.h
	$(CC) $(CFLAGS) -I../Sources t_binding_freshness.c ../Sources/dm_touch_game_state.c ../Sources/dm_touch_controls.c -lm -o $@
t_portal_sequence: t_portal_sequence.c ../Sources/dm_touch_controls.c ../Sources/dm_touch_controls.h
	$(CC) $(CFLAGS) -I../Sources t_portal_sequence.c ../Sources/dm_touch_controls.c -lm -o $@
t_binding_group: t_binding_group.c ../Sources/dm_touch_game_state.c ../Sources/dm_touch_game_state.h
	$(CC) $(CFLAGS) -I../Sources t_binding_group.c ../Sources/dm_touch_game_state.c -o $@
t_native_hud_identity: t_native_hud_identity.c ../Sources/dm_touch_game_identity.c ../Sources/dm_touch_game_identity.h ../Sources/dm_touch_game_state.h
	$(CC) $(CFLAGS) -I../Sources t_native_hud_identity.c ../Sources/dm_touch_game_identity.c -o $@
t_native_hud_bridge: t_native_hud_bridge.c ../Sources/dm_touch_game_bridge.c ../Sources/dm_touch_game_identity.c ../Sources/dm_touch_game_identity.h ../Sources/dm_touch_game_state.c ../Sources/dm_touch_game_state.h ../../host/dm.h
	$(CC) $(CFLAGS) -I../Sources t_native_hud_bridge.c ../Sources/dm_touch_game_bridge.c ../Sources/dm_touch_game_identity.c ../Sources/dm_touch_game_state.c -lpthread -o $@



GAME_VERSION:=$(firstword $(subst -, ,$(shell head -1 $(dir $(lastword $(MAKEFILE_LIST)))../../PRODUCT)))
ROMFS?=$(or $(DM_ROMFS_SRC),$(DM_ROOT)/data/switch/$(GAME_VERSION)/Update/RomFS)
../Sources/dm_touch_native_art.c: ../tools/native_art/generate.py ../tools/native_art/implementation.c.inc ../Sources/TouchArt/native_art_manifest.json
	python3 ../tools/native_art/generate.py "$(ROMFS)"
t_native_art: t_native_art.c ../Sources/dm_touch_native_art.c ../Sources/dm_touch_native_art.h
	$(CC) $(CFLAGS) -I../Sources t_native_art.c ../Sources/dm_touch_native_art.c -o $@
t_skill_aim: t_skill_aim.c ../Sources/dm_touch_controls.c ../Sources/dm_touch_controls.h ../../host/dm_input.h
	$(CC) $(CFLAGS) -I../Sources t_skill_aim.c ../Sources/dm_touch_controls.c -lm -o $@
t_joy_travel: t_joy_travel.c ../Sources/dm_touch_controls.c ../Sources/dm_touch_controls.h ../../host/dm_input.h
	$(CC) $(CFLAGS) -I../Sources t_joy_travel.c ../Sources/dm_touch_controls.c -lm -o $@
t_metal_cache_guard: t_metal_cache_guard.c ../Sources/dm_metal_cache_guard.c ../Sources/dm_metal_cache_guard.h
	$(CC) $(CFLAGS) -I../Sources t_metal_cache_guard.c ../Sources/dm_metal_cache_guard.c -o $@
test: $(TARGET) $(BINDING_TARGETS) t_native_art t_skill_aim t_joy_travel $(METAL_CACHE_TARGET)
	./$(METAL_CACHE_TARGET)
	./$(TARGET)
	./t_skill_aim
	./t_joy_travel
	./t_native_art
	python3 test_slowframe.py
	python3 test_native_art_capture.py
	python3 test_touch_layout_options.py
	python3 test_touch_layout_language.py
	python3 test_vault_tabs.py
	$(call fixture_test,t_native_binding)
	$(call fixture_test,t_game_bridge)
	./t_binding_freshness
	./t_portal_sequence
	./t_binding_group
	./t_native_hud_identity
	$(call fixture_test,t_native_hud_bridge)
	python3 test_skill_live.py
	python3 test_skill_bank_recheck.py
	python3 test_portal_receipt.py
	python3 test_native_binding_seam.py
	python3 test_skill_aim_front.py
	python3 test_touch_receipt.py --source ../Sources/front_ios.m
clean:
	rm -f $(TARGET) $(BINDING_TARGETS) t_native_art t_skill_aim t_joy_travel $(METAL_CACHE_TARGET)
