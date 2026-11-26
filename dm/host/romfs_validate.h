#ifndef DM_ROMFS_VALIDATE_H
#define DM_ROMFS_VALIDATE_H
#include <stddef.h>
int dm_romfs_validate(const char *dir, char *why, size_t why_n);
int dm_romfs_report(const char *dir, char *out, size_t out_n);








static const char *const dm_romfs_need[] = {
    "00-00000001.data", "00-00000002.data", "00-00000003.data", "00-00000004.data", "00-00000005.data", "00-00000006.data",
    "01-00000001.data", "01-00000002.data", "02-00000001.data", "02-00000002.data", "03-00000001.data", "03-00000002.data",
    "04-00000001.data", "04-00000002.data", "05-00000001.data", "05-00000002.data", "17-00000001.data", "17-00000002.data"};




static const long long dm_romfs_size[] = {
    1073741824, 1073741821, 1026342790, 3664822, 392518366, 3794559,
    344377972, 37205060, 368762692, 23213783, 251620752, 55071907,
    118831405, 8915612, 478931036, 92085048, 204495617, 4496514};
#define DM_ROMFS_NEED_N (sizeof dm_romfs_need / sizeof *dm_romfs_need)



static const char *const dm_romfs_voice[] = {
    "06-00000001.data", "06-00000002.data", "07-00000001.data", "07-00000002.data", "08-00000001.data", "08-00000002.data", "09-00000001.data", "09-00000002.data", "10-00000001.data", "10-00000002.data", "11-00000001.data", "11-00000002.data", "12-00000001.data", "12-00000002.data", "13-00000001.data", "13-00000002.data", "14-00000001.data", "14-00000002.data", "15-00000001.data", "15-00000002.data", "16-00000001.data", "16-00000002.data"};
static const long long dm_romfs_voice_size[] = {
    221422762, 4573157, 227505280, 7036462, 215073435, 4568579, 235735634, 4666589, 258055895, 4567587, 207554659, 4404247, 541207627, 4720770, 206331662, 4579633, 723891179, 3725235, 206172517, 4378926, 227467276, 4119666};
#define DM_ROMFS_VOICE_N (sizeof dm_romfs_voice / sizeof *dm_romfs_voice)
#endif
