















#pragma once
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
typedef int kern_return_t;
typedef unsigned mach_port_t;
typedef mach_port_t thread_act_t;
typedef thread_act_t *thread_act_array_t;
typedef unsigned mach_msg_type_number_t;
typedef uintptr_t vm_address_t;
typedef size_t vm_size_t;
typedef int *task_info_t;
typedef int *thread_info_t;
typedef unsigned *thread_state_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 5

static inline mach_port_t mach_task_self(void) { return 0; }
static inline mach_port_t mach_thread_self(void) { return (mach_port_t)gettid(); }
static inline mach_port_t pthread_mach_thread_np(pthread_t t) { return (mach_port_t)pthread_gettid_np(t); }
static inline kern_return_t mach_port_deallocate(mach_port_t task, mach_port_t name) { (void)task; (void)name; return KERN_SUCCESS; }
static inline kern_return_t vm_deallocate(mach_port_t task, vm_address_t a, vm_size_t n) { (void)task; (void)n; free((void *)a); return KERN_SUCCESS; }

static inline kern_return_t vm_read_overwrite(mach_port_t task, vm_address_t addr, vm_size_t n, vm_address_t to, vm_size_t *got) {
    (void)task;
    struct iovec l = {(void *)to, n}, r = {(void *)addr, n};
    ssize_t k = syscall(__NR_process_vm_readv, getpid(), &l, 1ul, &r, 1ul, 0ul);
    if (got) *got = k > 0 ? (vm_size_t)k : 0;
    return k == (ssize_t)n ? KERN_SUCCESS : KERN_FAILURE;
}

#define TASK_VM_INFO 22
typedef struct { uint64_t phys_footprint, resident_size, virtual_size; } task_vm_info_data_t;
#define TASK_VM_INFO_COUNT ((mach_msg_type_number_t)(sizeof(task_vm_info_data_t) / sizeof(int)))
static inline kern_return_t task_info(mach_port_t task, int flavor, task_info_t out, mach_msg_type_number_t *count) {
    (void)task; (void)count;
    if (flavor != TASK_VM_INFO) return KERN_FAILURE;
    task_vm_info_data_t *vi = (task_vm_info_data_t *)out;
    FILE *f = fopen("/proc/self/statm", "r"); unsigned long long vs = 0, rs = 0;
    if (!f) return KERN_FAILURE;
    int ok = fscanf(f, "%llu %llu", &vs, &rs) == 2; fclose(f);
    if (!ok) return KERN_FAILURE;
    long pg = sysconf(_SC_PAGESIZE);
    vi->virtual_size = vs * (uint64_t)pg; vi->resident_size = vi->phys_footprint = rs * (uint64_t)pg;
    return KERN_SUCCESS;
}



typedef struct { uint64_t __x[29], __fp, __lr, __sp, __pc; uint32_t __cpsr, __pad; } arm_thread_state64_t;
#define ARM_THREAD_STATE64 6
#define ARM_THREAD_STATE64_COUNT ((mach_msg_type_number_t)(sizeof(arm_thread_state64_t) / sizeof(uint32_t)))
#define __darwin_arm_thread_state64_get_pc(s) ((s).__pc)
#define __darwin_arm_thread_state64_get_lr(s) ((s).__lr)
#define __darwin_arm_thread_state64_get_sp(s) ((s).__sp)
#define __darwin_arm_thread_state64_get_fp(s) ((s).__fp)
kern_return_t thread_suspend(thread_act_t t);
kern_return_t thread_resume(thread_act_t t);
kern_return_t thread_get_state(thread_act_t t, int flavor, thread_state_t out, mach_msg_type_number_t *count);
kern_return_t task_threads(mach_port_t task, thread_act_array_t *out, mach_msg_type_number_t *n);

typedef struct { int seconds, microseconds; } time_value_t;
typedef struct { time_value_t user_time, system_time; int cpu_usage, policy, run_state, flags, suspend_count, sleep_time; } thread_basic_info_data_t;
#define THREAD_BASIC_INFO 3
#define THREAD_BASIC_INFO_COUNT ((mach_msg_type_number_t)(sizeof(thread_basic_info_data_t) / sizeof(int)))
kern_return_t thread_info(thread_act_t t, int flavor, thread_info_t out, mach_msg_type_number_t *count);
