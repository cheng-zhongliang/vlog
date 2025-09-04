#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "vlog.h"

#ifdef UNIT_TESTING
#define malloc test_malloc
#define realloc test_realloc
#define calloc test_calloc
#define free test_free
#endif

#define MAX_CALLBACKS 32
#define MAX_MODULE_NAME_LEN 16

typedef struct {
    vlog_LogFn fn;
    void* udata;
    int level;
} Callback;

static struct {
    void* udata;
    vlog_LockFn lock;
    int level;
    bool quiet;
    Callback callbacks[MAX_CALLBACKS];
    pthread_mutex_t mutex;
    FILE* fp;
    int max_size;
    char module[MAX_MODULE_NAME_LEN];
} L;

static const char* level_strings[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

#ifdef VLOG_USE_COLOR
static const char* level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m",
    "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#endif

static void stdout_callback(vlog_Event* ev) {
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef VLOG_USE_COLOR
    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%-5s %s:%d:\x1b[0m ", buf,
    level_colors[ev->level], level_strings[ev->level], ev->module, ev->file, ev->line);
#else
    fprintf(ev->udata, "%s %-5s %-5s %s:%d: ", buf, level_strings[ev->level],
    ev->module, ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void file_callback(vlog_Event* ev) {
    char buf[64];
    int file_size;
    int fd;

    fseek(ev->udata, 0L, SEEK_END);
    file_size = ftell(ev->udata);

    if(file_size > ev->max_size) {
        fd = fileno(ev->udata);
        ftruncate(fd, 0);
        rewind(ev->udata);
    }

    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(ev->udata, "%s %-5s %-5s %s:%d: ", buf, level_strings[ev->level],
    ev->module, ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void lock(void) {
    if(L.lock) {
        L.lock(true, L.udata);
    } else {
        pthread_mutex_lock(&L.mutex);
    }
}

static void unlock(void) {
    if(L.lock) {
        L.lock(false, L.udata);
    } else {
        pthread_mutex_unlock(&L.mutex);
    }
}

const char* vlog_level_string(int level) {
    return level_strings[level];
}

void vlog_set_lock(vlog_LockFn fn, void* udata) {
    L.lock = fn;
    L.udata = udata;
}

void vlog_set_level(int level) {
    L.level = level;
}

void vlog_set_quiet(bool enable) {
    L.quiet = enable;
}

int vlog_add_callback(vlog_LogFn fn, void* udata, int level) {
    for(int i = 0; i < MAX_CALLBACKS; i++) {
        if(!L.callbacks[i].fn) {
            L.callbacks[i] = (Callback){ fn, udata, level };
            return 0;
        }
    }
    return -1;
}

int vlog_add_fp(FILE* fp, int level) {
    return vlog_add_callback(file_callback, fp, level);
}

static void init_event(vlog_Event* ev, void* udata) {
    if(!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
    ev->max_size = L.max_size;
}

void vlog_log(int level, const char* module, const char* file, int line, const char* fmt, ...) {
    vlog_Event ev = {
        .fmt = fmt,
        .file = file,
        .line = line,
        .level = level,
        .module = module,
    };

    lock();

    for(int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Callback* cb = &L.callbacks[i];
        if(level >= cb->level) {
            init_event(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    if(L.module[0] != '\0' && strcmp(L.module, module)) {
        unlock();
        return;
    }

    if(!L.quiet && level >= L.level) {
        init_event(&ev, stderr);
        va_start(ev.ap, fmt);
        stdout_callback(&ev);
        va_end(ev.ap);
    }

    if(level >= L.level) {
        init_event(&ev, L.fp);
        va_start(ev.ap, fmt);
        file_callback(&ev);
        va_end(ev.ap);
    }

    unlock();
}

int vlog_init(const char* path, int level, int size) {
    if(pthread_mutex_init(&L.mutex, NULL)) {
        return -1;
    }

    L.fp = fopen(path, "a");
    if(!L.fp) {
        return -1;
    }

    L.level = level;
    L.max_size = size;

    return 0;
}

void vlog_deinit() {
    pthread_mutex_destroy(&L.mutex);
    fclose(L.fp);
}

void vlog_set_module(const char* module) {
    if(module) {
        strncpy(L.module, module, MAX_MODULE_NAME_LEN - 1);
        L.module[MAX_MODULE_NAME_LEN - 1] = '\0';
    } else {
        L.module[0] = '\0';
    }
}