#include "dm_hud.h"
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <stdint.h>







static _Atomic uint64_t g_req;
static _Atomic uint64_t g_done;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_hud_info g_info;
void dm_hud_request_players(int n) {
    n = n < 1 ? 1 : n > 8 ? 8 : n;
    uint64_t old = atomic_load(&g_req), want;
    do want = ((old >> 8) + 1) << 8 | (uint64_t)n; while (!atomic_compare_exchange_weak(&g_req, &old, want));
}
int dm_hud_requested_players(void) { return (int)(atomic_load(&g_req) & 0xff); }
uint64_t dm_hud_players_take(int *n) { uint64_t r = atomic_load(&g_req); *n = (int)(r & 0xff); return r >> 8; }
void dm_hud_players_done(uint64_t seq) { uint64_t d = atomic_load(&g_done); while (d < seq && !atomic_compare_exchange_weak(&g_done, &d, seq)) {} }
int dm_hud_players_wanted(void) {
    uint64_t r = atomic_load(&g_req);
    if ((r >> 8) > atomic_load(&g_done)) return (int)(r & 0xff);
    dm_hud_info h; dm_hud_get(&h); return h.players;
}




static _Atomic uint32_t g_held;
void dm_hud_set_held(uint32_t id) { atomic_store(&g_held, id); }
uint32_t dm_hud_held(void) { return atomic_load(&g_held); }
void dm_hud_get(dm_hud_info *out) { pthread_mutex_lock(&g_mu); *out = g_info; pthread_mutex_unlock(&g_mu); out->holding = (int)atomic_load(&g_held); }



static uint64_t g_pub_ns;
void dm_hud_publish(const dm_hud_info *in) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); uint64_t t = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    pthread_mutex_lock(&g_mu); g_info = *in; g_pub_ns = t; pthread_mutex_unlock(&g_mu);
}
void dm_hud_get_stamped(dm_hud_info *out, uint64_t *pub_ns) { pthread_mutex_lock(&g_mu); *out = g_info; *pub_ns = g_pub_ns; pthread_mutex_unlock(&g_mu); out->holding = (int)atomic_load(&g_held); }
