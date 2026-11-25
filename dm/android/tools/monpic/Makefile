




B ?= build
SAN ?=
LDFLAGS ?=
CC ?= cc
CXX ?= c++
DM := ../../..

SOURCES := $(foreach s,$(filter-out \#%,$(shell cat sources.txt)),$(wildcard $(s)))
INCLUDES := -I. -I$(DM)/gfx/tex -I$(DM)/gfx/nvn -I$(DM)/third_party/libdeflate -I$(DM)/third_party/libwebp -I$(DM)/third_party/libwebp/src

COMMON := -O2 -g -fPIC -fvisibility=hidden -ffp-contract=off -pthread $(SAN) $(INCLUDES)
CFLAGS_ALL := $(COMMON) -std=c11
CXXFLAGS_ALL := $(COMMON) -std=c++17 -Wall -Wextra -Wshadow
OBJS := $(patsubst %,$(B)/obj/%.o,$(subst ../,up/,$(SOURCES)))

.PHONY: all test clean
.SECONDARY:
all: $(B)/libmonpic.so $(B)/monpic

$(B)/obj/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_ALL) -c $< -o $@
$(B)/obj/up/up/up/%.c.o: $(DM)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_ALL) -c $< -o $@
$(B)/obj/tests/%.cpp.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_ALL) -c $< -o $@

$(B)/libmonpic.so: $(OBJS)
	$(CXX) -shared -pthread $(SAN) $(LDFLAGS) -o $@ $^
$(B)/monpic: $(B)/obj/cli.cpp.o $(OBJS)
	$(CXX) -pthread $(SAN) $(LDFLAGS) -o $@ $^

TESTS := t_bitknit t_archive t_granny t_model t_texture t_render t_webp
$(B)/%: $(B)/obj/tests/%.cpp.o $(B)/obj/tests/support.cpp.o $(OBJS)
	$(CXX) -pthread $(SAN) -o $@ $^

test:
	$(MAKE) B=build/san SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" $(addprefix build/san/,$(TESTS))
	@for t in $(TESTS); do echo "== $$t"; ./build/san/$$t || exit 1; done

clean:
	rm -rf build
