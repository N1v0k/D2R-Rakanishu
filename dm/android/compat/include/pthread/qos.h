



#pragma once
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>
typedef enum {
    QOS_CLASS_USER_INTERACTIVE = 0x21, QOS_CLASS_USER_INITIATED = 0x19, QOS_CLASS_DEFAULT = 0x15,
    QOS_CLASS_UTILITY = 0x11, QOS_CLASS_BACKGROUND = 0x09, QOS_CLASS_UNSPECIFIED = 0x00,
} qos_class_t;
static __thread qos_class_t dm_qos_self_ = QOS_CLASS_UNSPECIFIED;
static inline int dm_qos_nice_(qos_class_t c) {
    switch (c) {
    case QOS_CLASS_USER_INTERACTIVE: return -10;
    case QOS_CLASS_USER_INITIATED: return -4;
    case QOS_CLASS_UTILITY: return 5;
    case QOS_CLASS_BACKGROUND: return 10;
    default: return 0;
    }
}
static inline int pthread_set_qos_class_self_np(qos_class_t c, int rel) {
    (void)rel; dm_qos_self_ = c;
    return setpriority(PRIO_PROCESS, (id_t)gettid(), dm_qos_nice_(c)) ? -1 : 0;
}
static inline int pthread_get_qos_class_np(pthread_t t, qos_class_t *c, int *rel) {
    (void)t; if (c) *c = dm_qos_self_; if (rel) *rel = 0; return 0;
}
static inline qos_class_t qos_class_self(void) { return dm_qos_self_; }
