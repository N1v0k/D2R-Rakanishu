





#pragma once
#include <stdint.h>
int dm_ipf_on(void);
uint64_t dm_ipf_instr(void);
void dm_ipf_add(int table, const char *name, uint64_t instr);
void dm_ipf_frame(int table);
#define DM_PROF(table, name, stmt) do { if (dm_ipf_on()) { uint64_t _p0 = dm_ipf_instr(); stmt; dm_ipf_add(table, name, dm_ipf_instr() - _p0); } else { stmt; } } while (0)
