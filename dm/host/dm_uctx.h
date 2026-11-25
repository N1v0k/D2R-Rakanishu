



#pragma once
#include <stdint.h>
#include <sys/ucontext.h>
#ifdef __APPLE__
#define UC_PC(uc) ((uc)->uc_mcontext->__ss.__pc)
#define UC_LR(uc) ((uc)->uc_mcontext->__ss.__lr)
#define UC_SP(uc) ((uc)->uc_mcontext->__ss.__sp)
#define UC_FP(uc) ((uc)->uc_mcontext->__ss.__fp)
#define UC_X(uc, i) ((uc)->uc_mcontext->__ss.__x[i])
static inline uint64_t UC_ESR(const ucontext_t *uc) { return uc->uc_mcontext->__es.__esr; }
#else
#define UC_PC(uc) ((uc)->uc_mcontext.pc)
#define UC_LR(uc) ((uc)->uc_mcontext.regs[30])
#define UC_SP(uc) ((uc)->uc_mcontext.sp)
#define UC_FP(uc) ((uc)->uc_mcontext.regs[29])
#define UC_X(uc, i) ((uc)->uc_mcontext.regs[i])
static inline uint64_t UC_ESR(const ucontext_t *uc) {
    const uint8_t *p = (const uint8_t *)uc->uc_mcontext.__reserved, *end = p + sizeof uc->uc_mcontext.__reserved;
    while (p + 8 <= end) {
        uint32_t magic = *(const uint32_t *)p, size = *(const uint32_t *)(p + 4);
        if (!magic || size < 8) break;
        if (magic == 0x45535201u && size >= 16) return *(const uint64_t *)(p + 8);
        p += size;
    }
    return 0;
}
#endif
