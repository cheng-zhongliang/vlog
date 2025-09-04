#ifndef __VLOG_H__
#define __VLOG_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define VLOG_VERSION "0.1.0"

typedef struct {
    va_list ap;
    const char* fmt;
    const char* file;
    const char* module;
    struct tm* time;
    void* udata;
    int line;
    int level;
    int max_size;
} vlog_Event;

typedef void (*vlog_LogFn)(vlog_Event* ev);
typedef void (*vlog_LockFn)(bool lock, void* udata);

enum { VLOG_TRACE, VLOG_DEBUG, VLOG_INFO, VLOG_WARN, VLOG_ERROR, VLOG_FATAL };

#define vlog_trace(...) \
    vlog_log(VLOG_TRACE, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_debug(...) \
    vlog_log(VLOG_DEBUG, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_info(...) \
    vlog_log(VLOG_INFO, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_warn(...) \
    vlog_log(VLOG_WARN, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_error(...) \
    vlog_log(VLOG_ERROR, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)
#define vlog_fatal(...) \
    vlog_log(VLOG_FATAL, __MODULE__, __FILE__, __LINE__, __VA_ARGS__)

const char* vlog_level_string(int level);
void vlog_set_level(int level);
void vlog_set_quiet(bool enable);
void vlog_set_module(const char* module);
int vlog_add_callback(vlog_LogFn fn, void* udata, int level);

void vlog_log(int level, const char* module, const char* file, int line, const char* fmt, ...);

int vlog_init(const char* path, int level, int size);
void vlog_deinit();

#endif