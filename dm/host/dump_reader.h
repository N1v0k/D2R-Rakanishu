





#ifndef DM_DUMP_READER_H
#define DM_DUMP_READER_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char id[16];
    char detail[96];
} pb_error;

enum { PB_TARGET_EXEFS = 0, PB_TARGET_DATA = 1 };


typedef struct {
    const char *unit;
    int target;
    const char *name;
    uint64_t size;
    const char *sha256;
} pb_file;


typedef struct {
    uint64_t title;
    size_t n;
    const pb_file *files;
    pb_file *owned_files;
    char *owned_text;
} pb_contract;


const pb_contract *pb_contract_game(void);

int pb_contract_parse(const char *text, size_t len, pb_contract *c, pb_error *e);
void pb_contract_free(pb_contract *c);

enum { PB_KIND_KEYS = 0, PB_KIND_PACKAGE = 1, PB_KIND_BASE = 2, PB_KIND_UPDATE = 3, PB_KIND_ADDON = 4 };





int pb_probe(int fd, uint64_t length, const uint8_t *keys, size_t keys_len, const pb_contract *c, int *kind, char identity[65], pb_error *e);

typedef struct pb_reader pb_reader;



pb_reader *pb_open(int base_fd, int update_fd, const uint8_t *keys, size_t keys_len, const pb_contract *c, pb_error *e);


typedef void (*pb_progress)(void *ctx, uint64_t unit_bytes, const char *file);



int pb_extract(pb_reader *r, const char *unit, const char *exefs_dir, const char *data_dir, pb_progress cb, void *ctx, pb_error *e);


void pb_cancel(pb_reader *r);

void pb_close(pb_reader *r);

#endif
