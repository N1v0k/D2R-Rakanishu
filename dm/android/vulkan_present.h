#pragma once
#include <stddef.h>
#include <android/native_window.h>
#ifdef __cplusplus
extern "C" {
#endif



void dm_android_vk_set_window(ANativeWindow *window);




int dm_android_vk_initialize(const char *shader_directory,char *error,size_t bytes);
int dm_android_vk_has_window(void);
#ifdef __cplusplus
}
#endif
