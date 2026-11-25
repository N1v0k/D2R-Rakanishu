

#include "../loader.c"
#include <assert.h>

struct dm_flags g_flag;
uint64_t unimpl_stubs[4096];
void *hlecount_wrap(const char *kind, const char *name, void *function) {
    (void)kind; (void)name; return function;
}
HLE("host_provided", int, host_provided, void) { return 7; }

int main(void) {
    _Alignas(16) uint8_t importer[4096] = {0}, exporter[4096] = {0};
    const char names[] = "\0optional\0provided\0host_provided\0required";
    Elf64_Sym imports[5] = {
        {0}, {.st_name=1, .st_info=0x20}, {.st_name=10, .st_info=0x20},
        {.st_name=19, .st_info=0x20}, {.st_name=33, .st_info=0x10}
    };
    Elf64_Sym exports[2] = {{0}, {.st_name=10, .st_info=0x10, .st_shndx=1, .st_value=0x100}};
    Elf64_Dyn dynamic[] = {
        {.d_tag=DT_RELA, .d_un.d_ptr=0x300},
        {.d_tag=DT_RELASZ, .d_un.d_val=4*sizeof(Elf64_Rela)}, {0}
    };
    Elf64_Rela *rel = (Elf64_Rela *)(importer + 0x300);
    for (int i=0; i<4; i++) rel[i] = (Elf64_Rela){.r_offset=0x200+i*8,
        .r_info=((uint64_t)(i+1)<<32)|R_AARCH64_GLOB_DAT};
    g_mod[0]=(module_t){.base=importer, .size=sizeof importer, .symtab=imports,
        .strtab=names, .nsym=5, .dyn=dynamic};
    g_mod[1]=(module_t){.base=exporter, .size=sizeof exporter, .symtab=exports,
        .strtab=names, .nsym=2};
    g_nmod=2;
    relocate(&g_mod[0]);
    uint64_t *values=(uint64_t *)(importer+0x200);
    assert(values[0]==0);
    assert(values[1]==(uintptr_t)(exporter+0x100));
    assert(values[2]==(uintptr_t)host_provided);
    assert(values[3]!=0);
    assert(resolve_sym(&g_mod[0], "required", 1)==0);
    puts("Host ELF linking: absent weak, guest-defined weak, host-defined weak, strong named stub passed.");
    return 0;
}
