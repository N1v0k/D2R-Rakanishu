

#ifndef GAME_ADDRS_H
#define GAME_ADDRS_H
#define GB(module, version, build_id)
#define GL(module, name, value) GL_##name = value,
#define GA(module, name, addr, kind, note) GA_##name = addr,
#define GF(module, name, off, witness, note) GF_##name = off,
enum {
#include "game_addrs.def"
};
#undef GB
#undef GL
#undef GA
#undef GF

#define GB(module, version, build_id) static const char GAME_VERSION_##module[] __attribute__((unused)) = version, \
    GAME_BUILD_ID_##module[] __attribute__((unused)) = build_id;
#define GL(module, name, value)
#define GA(module, name, addr, kind, note)
#define GF(module, name, off, witness, note)
#include "game_addrs.def"
#undef GB
#undef GL
#undef GA
#undef GF
#endif
