





#include "../dm_hud.c"
#include <stdio.h>
#include <string.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)



static uint64_t last_seq; static int game_players = 1, applied_count;
static void tick(int allowed) {
    int want = 0; uint64_t seq = dm_hud_players_take(&want), ack = 0;
    if (seq && seq != last_seq) { last_seq = ack = seq; if (allowed) { game_players = want; applied_count++; } }
    dm_hud_info h; memset(&h, 0, sizeof h); h.valid = 1; h.players = game_players; dm_hud_publish(&h);
    if (ack) dm_hud_players_done(ack);
}

int main(void) {
    CHECK(dm_hud_players_wanted() == 0, "还没有拍：0 = 不知道");
    tick(1);
    CHECK(dm_hud_players_wanted() == 1 && applied_count == 0, "没请求：显示本拍的 1");
    dm_hud_request_players(3);
    CHECK(dm_hud_players_wanted() == 3, "请求 3、暂停中没拍：显示 3");
    dm_hud_request_players(4); dm_hud_request_players(5);
    CHECK(dm_hud_players_wanted() == 5 && dm_hud_requested_players() == 5, "最后一个请求算数：5");
    tick(1);
    CHECK(game_players == 5 && applied_count == 1 && dm_hud_players_wanted() == 5, "一拍办掉最后那个（5），回执后显示本拍的 5");
    dm_hud_request_players(2); tick(0);
    CHECK(game_players == 5 && dm_hud_players_wanted() == 5, "被原版拒了（模式不允许）：也回执，显示回到真值 5（不挂在 2）");
    dm_hud_request_players(5); tick(1);
    CHECK(applied_count == 2, "同一个数（5）再请求一次：新序号，再办一次（比如聊天里打过 /players 之后）");
    game_players = 3;
    tick(1);
    CHECK(dm_hud_players_wanted() == 3 && applied_count == 2, "游戏自己改了：显示 3，不重办旧请求");
    dm_hud_request_players(5); tick(1);
    CHECK(game_players == 5 && applied_count == 3, "又请求 5（与上次请求同值）：照办");

    { dm_hud_request_players(7); int want = 0; uint64_t seq = dm_hud_players_take(&want);
      dm_hud_request_players(8);
      dm_hud_players_done(seq);
      CHECK(want == 7 && dm_hud_players_wanted() == 8, "回执 7 那个序号时 8 已请求：显示 8（%d）", dm_hud_players_wanted());
      dm_hud_players_done(seq - 1);
      CHECK(dm_hud_players_wanted() == 8, "回执更旧的序号不倒退"); }
    dm_hud_request_players(0); CHECK(dm_hud_requested_players() == 1, "夹到 1");
    dm_hud_request_players(12); CHECK(dm_hud_requested_players() == 8, "夹到 8");
    printf("t_hud_players：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
