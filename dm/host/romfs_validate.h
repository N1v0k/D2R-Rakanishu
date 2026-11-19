#ifndef DM_ROMFS_VALIDATE_H
#define DM_ROMFS_VALIDATE_H
#include <stddef.h>
int dm_romfs_validate(const char *dir, char *why, size_t why_n);
int dm_romfs_report(const char *dir, char *out, size_t out_n);
#endif
