




#include <adrenotools/driver.h>
#include <dlfcn.h>

__attribute__((visibility("default"))) void *dm_driverload_open(const char *hook_dir, const char *driver_dir, const char *library) {
    return adrenotools_open_libvulkan(RTLD_NOW, ADRENOTOOLS_DRIVER_CUSTOM, NULL, hook_dir, driver_dir, library, NULL, NULL);
}
