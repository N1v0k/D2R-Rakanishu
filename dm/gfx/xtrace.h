

#pragma once
#ifdef __OHOS__
#include <hitrace/trace.h>
#define XTRACE_BEGIN(name) OH_HiTrace_StartTrace(name)
#define XTRACE_END() OH_HiTrace_FinishTrace()
#define XTRACE_COUNT(name, v) OH_HiTrace_CountTrace(name, (int64_t)(v))
#else
#define XTRACE_BEGIN(name) ((void)0)
#define XTRACE_END() ((void)0)
#define XTRACE_COUNT(name, v) ((void)0)
#endif
