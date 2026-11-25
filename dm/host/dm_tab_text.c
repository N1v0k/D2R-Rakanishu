#include "dm_tab_text.h"
#include "dm_widget.h"
#include "hle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAB_LABEL_MAX 128
#define TAB_METRIC_MAX 64
static struct { char text[TAB_LABEL_MAX]; int delta; } metrics[TAB_METRIC_MAX];
static int metric_count, loaded;

static int read_metrics(FILE *file) {
    char line[160]; int count = 0;
    metric_count = 0;
    if (!fgets(line, sizeof line, file) || strcmp(line, "TAB_INK_V1\n")) return 0;
    while (fgets(line, sizeof line, file)) {
        if (!strcmp(line, "END\n")) {
            if (!count || fgetc(file) != EOF || ferror(file)) return 0;
            metric_count = count;
            return 1;
        }
        char *end; long delta = strtol(line, &end, 10);
        if (end == line || *end != '\t' || delta < -1000 || delta > 1000 || count == TAB_METRIC_MAX) return 0;
        char *text = end + 1; size_t length = strlen(text);
        if (length < 2 || length > TAB_LABEL_MAX || text[length - 1] != '\n') return 0;
        text[--length] = 0;
        for (size_t i = 0; i < length; i++) if ((unsigned char)text[i] < 32) return 0;
        for (int i = 0; i < count; i++) if (!strcmp(metrics[i].text, text)) return 0;
        memcpy(metrics[count].text, text, length + 1);
        metrics[count++].delta = (int)delta;
    }
    return 0;
}

void dm_tab_text_center(uint8_t *base, uint64_t tabs, int language) {
    if (language != 0 && language != 1 && language != 12) return;
    if (!dm_widget_is(base, tabs, DM_VT_TABBAR)) return;
    int count = *(int32_t *)(uintptr_t)(tabs + 0x558);

    if (count < 1 || count > 8) return;
    if (!loaded) {
        char path[1200];
        FILE *file = dm_overlay_file("tab_text.tsv", path, sizeof path) ? fopen(path, "rb") : NULL;
        loaded = 1;
        int valid = file && read_metrics(file);
        if (file) fclose(file);
        if (!valid) { metric_count = 0; fprintf(stderr, "[dm] TAB_TEXT_DATA: missing or invalid tab_text.tsv\n"); }
    }
    if (!metric_count) return;
    for (int index = 0; index < count; index++) {
        uint64_t image = *(uint64_t *)(uintptr_t)(tabs + 0xdc8 + 8 * index);
        uint64_t text = *(uint64_t *)(uintptr_t)(tabs + 0xe08 + 8 * index);
        if (!dm_widget_is(base, image, DM_VT_IMAGE) || !dm_widget_is(base, text, DM_VT_TEXT)) continue;
        const char *label = *(const char **)(uintptr_t)(text + 0x88);
        uint64_t length = *(uint64_t *)(uintptr_t)(text + 0x90);
        int size = *(int32_t *)(uintptr_t)(text + 0xb8);
        if (!label || !length || length >= TAB_LABEL_MAX || size < 1 || size > 100) continue;
        for (int row = 0; row < metric_count; row++) {
            if (strlen(metrics[row].text) != length || memcmp(label, metrics[row].text, length)) continue;
            int scaled = metrics[row].delta * size;
            int delta = scaled < 0 ? -((-scaled + 500) / 1000) : (scaled + 500) / 1000;

            int y = *(int32_t *)(uintptr_t)(image + 0x74) + *(int32_t *)(uintptr_t)(tabs + 0xb2c) + delta;
            *(int32_t *)(uintptr_t)(text + 0x74) = y;
            break;
        }
    }
}
