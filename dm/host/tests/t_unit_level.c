





#include "dm_unit.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static int old_level(uint64_t u) { uint64_t r = rd(rd(rd(rd(u + 0x38) + 0x20) + 0x18) + 0x90); return r ? *(int32_t *)(uintptr_t)(r + 0x1f8) : 0; }


static int old_dies(uint64_t u) {
    fflush(stdout); pid_t p = fork(); if (p == 0) { signal(SIGSEGV, SIG_DFL); signal(SIGBUS, SIG_DFL); volatile int v = old_level(u); (void)v; _exit(0); }
    int st = 0; waitpid(p, &st, 0); return WIFSIGNALED(st) ? WTERMSIG(st) : 0;
}

int main(void) {
    static uint8_t unit[0x200], path[0x100], room[0x100], ex[0x100], level[0x300];


    *(uint64_t *)(unit + 0x38) = (uint64_t)(uintptr_t)path; *(uint64_t *)(path + 0x20) = (uint64_t)(uintptr_t)room;
    *(uint64_t *)(room + 0x18) = (uint64_t)(uintptr_t)ex; *(uint64_t *)(ex + 0x90) = (uint64_t)(uintptr_t)level; *(int32_t *)(level + 0x1f8) = 59;
    uint64_t u = (uint64_t)(uintptr_t)unit;
    CHECK(dm_unit_level(u) == 59, "完整链应读到 59，读到 %d", dm_unit_level(u));
    CHECK(dm_unit_level_ptr(u) == (uint64_t)(uintptr_t)level, "完整链应返回关卡结构指针");
    CHECK(old_level(u) == 59, "旧写法在完整链上也应读到 59（对照）");



    *(uint64_t *)(path + 0x20) = 0;
    int sg = old_dies(u); CHECK(sg == SIGSEGV || sg == SIGBUS, "旧写法在「有路径、房间为空」上应崩（红），实际信号 %d", sg);
    CHECK(dm_unit_level(u) == 0 && dm_unit_level_ptr(u) == 0, "新写法在「有路径、房间为空」上应返回 0");


    *(uint64_t *)(path + 0x20) = (uint64_t)(uintptr_t)room; *(uint64_t *)(unit + 0x38) = 0;
    sg = old_dies(u); CHECK(sg == SIGSEGV || sg == SIGBUS, "旧写法在「路径为空」上应崩（红），实际信号 %d", sg);
    CHECK(dm_unit_level(u) == 0 && dm_unit_level_ptr(u) == 0, "新写法在「路径为空」上应返回 0");


    sg = old_dies(0); CHECK(sg == SIGSEGV || sg == SIGBUS, "旧写法在「单位为空」上应崩（红），实际信号 %d", sg);
    CHECK(dm_unit_level(0) == 0 && dm_unit_level_ptr(0) == 0, "新写法在「单位为空」上应返回 0");


    *(uint64_t *)(unit + 0x38) = (uint64_t)(uintptr_t)path; *(uint64_t *)(room + 0x18) = 0;
    CHECK(dm_unit_level(u) == 0, "房间 +0x18 为空应返回 0");
    *(uint64_t *)(room + 0x18) = (uint64_t)(uintptr_t)ex; *(uint64_t *)(ex + 0x90) = 0;
    CHECK(dm_unit_level(u) == 0, "关卡指针为空应返回 0");
    *(uint64_t *)(ex + 0x90) = (uint64_t)(uintptr_t)level;
    CHECK(dm_unit_level(u) == 59, "链接回去后应再读到 59");


    CHECK(dm_level_rooms(0) == 0, "关卡指针为空时房间链头应为 0");
    *(uint64_t *)(level + 0x10) = (uint64_t)(uintptr_t)room; CHECK(dm_level_rooms((uint64_t)(uintptr_t)level) == (uint64_t)(uintptr_t)room, "关卡非空时应读到 +0x10 的房间");
    printf("t_unit_level: %d 项，%d 失败\n", checks, fails); return fails != 0;
}
