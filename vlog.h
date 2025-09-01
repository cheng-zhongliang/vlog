#ifndef _VLOG_H_
#define _VLOG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define VLOG_VERSION "0.1.0"

typedef struct {
    va_list ap;
    const char* fmt;
    const char* file;
    struct tm* time;
    void* udata;
    int line;
    int level;
} vlog_Event;

typedef void (*vlog_LogFn)(vlog_Event* ev);
typedef void (*vlog_LockFn)(bool lock, void* udata);

enum { VLOG_TRACE, VLOG_DEBUG, VLOG_INFO, VLOG_WARN, VLOG_ERROR, VLOG_FATAL };

#define vlog_trace(...) vlog_log(VLOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_debug(...) vlog_log(VLOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_info(...) vlog_log(VLOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_warn(...) vlog_log(VLOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_error(...) vlog_log(VLOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_fatal(...) vlog_log(VLOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

const char* vlog_level_string(int level);
void vlog_set_lock(vlog_LockFn fn, void* udata);
void vlog_set_level(int level);
void vlog_set_quiet(bool enable);
int vlog_add_callback(vlog_LogFn fn, void* udata, int level);
int vlog_add_fp(FILE* fp, int level);

void vlog_log(int level, const char* file, int line, const char* fmt, ...);

#endif