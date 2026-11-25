

#include "../vulkan/vk_fps.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define MS 1000000ull
static vk_fps_window w;
static uint64_t word, now = 5000 * MS;
static uint32_t tag;

static int run(int n, uint64_t step_ns) { for (int i = 0; i < n; i++) { now += step_ns; word = vk_fps_present(&w, now, tag); } return vk_fps_read(word, tag, now); }
int main(void) {


    CHECK(vk_fps_read(0, 0, now) == -1);
    CHECK(run(1, 16666667) == -1);
    CHECK(run(29, 16666667) == -1);
    CHECK(run(1, 16666667) == 60);


    CHECK(run(29, 33333334) == 60);
    CHECK(run(1, 33333334) == 30);
    CHECK(run(30, 33333334) == 30);

    { int worst = 0; for (int i = 0; i < 600; i++) { now += i & 1 ? 17333333 : 16000000; word = vk_fps_present(&w, now, tag); int v = vk_fps_read(word, tag, now); if (v > worst) worst = v; } CHECK(worst <= 61 && worst >= 59); }


    CHECK(vk_fps_read(word, tag, now + 400 * MS) > 0);
    CHECK(vk_fps_read(word, tag, now + 501 * MS) == -1);


    CHECK(vk_fps_read(word, tag, now) > 0);
    tag++;
    CHECK(vk_fps_read(word, tag, now) == -1);


    CHECK(run(1, 8333334) == -1);
    CHECK(run(59, 8333334) == -1);
    CHECK(run(1, 8333334) == 120);


    { uint64_t late = vk_fps_present(&w, now + 8333334, tag); now += 8333334; tag++; word = late; CHECK(vk_fps_read(word, tag, now) == -1); }

    CHECK(run(1, 8333334) == -1 && run(60, 8333334) == 120);
    CHECK(vk_fps_read(word, tag + 0x10000u, now) == 120 && vk_fps_read(word, tag + 1, now) == -1);


    CHECK(run(1, 2000 * MS) == -1);
    CHECK(run(15, 33333334) == 30);

    CHECK(run(1, 500 * MS) == 30);

    { vk_fps_window z = w; uint64_t back = vk_fps_present(&z, now - MS, tag); CHECK(vk_fps_read(back, tag, now) == -1); }
    { vk_fps_window z = {0}; uint64_t t0 = now; vk_fps_present(&z, t0, tag); uint64_t big = 0; for (int i = 1; i <= 600000; i++) big = vk_fps_present(&z, t0 + i * 1000ull, tag);
      CHECK(((big >> 32) & 0xffffu) == VK_FPS_UNKNOWN - 1); }
    if (fails) { printf("t_vk_fps: %d 处失败\n", fails); return 1; }
    printf("t_vk_fps OK\n"); return 0;
}
