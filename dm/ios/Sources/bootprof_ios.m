





#import <Foundation/Foundation.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <mach/mach.h>
#include <pthread.h>
#include <unistd.h>

int proc_pid_rusage(int pid, int flavor, rusage_info_t *buffer);

uint64_t now_ns(void);
static uint64_t g_t0;
static double g_pre_s;

static void *sampler(void *p) {
    FILE *f = fopen((const char *)p, "w"); if (!f) return NULL;
    fprintf(f, "t_since_spawn\tpageins\tdisk_read_MiB\tfootprint_MiB\n");
    for (int i = 0; i < 300; i++) {
        struct rusage_info_v4 ri; task_vm_info_data_t vi; mach_msg_type_number_t n = TASK_VM_INFO_COUNT;
        proc_pid_rusage(getpid(), RUSAGE_INFO_V4, (rusage_info_t *)&ri);
        task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &n);
        fprintf(f, "%.2f\t%llu\t%.1f\t%llu\n", g_pre_s + (now_ns() - g_t0) / 1e9, ri.ri_pageins, ri.ri_diskio_bytesread / 1048576.0, vi.phys_footprint >> 20);
        fflush(f);
        usleep(100000);
    }
    fclose(f); return NULL;
}

__attribute__((constructor)) static void bootprof_ios(void) {
    g_t0 = now_ns();
    if (!getenv("DM_BOOTPROF")) return;
    struct kinfo_proc kp; size_t len = sizeof kp; int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    struct timeval now; gettimeofday(&now, NULL);
    if (!sysctl(mib, 4, &kp, &len, NULL, 0))
        g_pre_s = (now.tv_sec - kp.kp_proc.p_starttime.tv_sec) + (now.tv_usec - kp.kp_proc.p_starttime.tv_usec) / 1e6;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    char *path = strdup([doc stringByAppendingPathComponent:@"bootprof.tsv"].fileSystemRepresentation);
    pthread_t th; pthread_create(&th, NULL, sampler, path); pthread_detach(th);
}
