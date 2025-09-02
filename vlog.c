#include "vlog.h"
#include <sys/time.h>

#define MAX_CALLBACKS 32
#define MAX_RATE_ENTRIES 256

typedef struct {
    vlog_LogFn fn;
    void* udata;
    int level;
} Callback;

typedef struct {
    unsigned long hash;
    struct timeval last_log_time;
} RateEntry;

static struct {
    void* udata;
    vlog_LockFn lock;
    int level;
    bool quiet;
    Callback callbacks[MAX_CALLBACKS];
    double rate_limit_interval;
    RateEntry rate_entries[MAX_RATE_ENTRIES];
} L;

static const char* level_strings[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

#ifdef VLOG_USE_COLOR
static const char* level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m",
    "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#endif

static void stdout_callback(vlog_Event* ev) {
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
    level_colors[ev->level], level_strings[ev->level], ev->file, ev->line);
#else
    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level],
    ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void file_callback(vlog_Event* ev) {
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level],
    ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void lock(void) {
    if(L.lock) {
        L.lock(true, L.udata);
    }
}

static void unlock(void) {
    if(L.lock) {
        L.lock(false, L.udata);
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

void vlog_set_rate_limit(double interval_seconds) {
    L.rate_limit_interval = interval_seconds;
}

static unsigned long hash_location(const char* file, int line) {
    unsigned long hash = 5381;
    
    // Hash the filename
    while (*file) {
        hash = ((hash << 5) + hash) + (unsigned char)*file++;
    }
    
    // Include line number in hash
    hash = ((hash << 5) + hash) + line;
    
    return hash;
}

static bool should_rate_limit(const char* file, int line) {
    if (L.rate_limit_interval <= 0.0) {
        return false; // Rate limiting disabled
    }
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    unsigned long hash = hash_location(file, line);
    int index = hash % MAX_RATE_ENTRIES;
    
    RateEntry* entry = &L.rate_entries[index];
    
    // Calculate time difference in seconds
    double time_diff = (current_time.tv_sec - entry->last_log_time.tv_sec) +
                      (current_time.tv_usec - entry->last_log_time.tv_usec) / 1000000.0;
    
    // Check if this is a new entry or enough time has passed
    if (entry->hash != hash || time_diff >= L.rate_limit_interval) {
        entry->hash = hash;
        entry->last_log_time = current_time;
        return false; // Don't rate limit
    }
    
    return true; // Rate limit this log
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
}

void vlog_log(int level, const char* file, int line, const char* fmt, ...) {
    vlog_Event ev = {
        .fmt = fmt,
        .file = file,
        .line = line,
        .level = level,
    };

    lock();

    // Check rate limiting
    if (should_rate_limit(file, line)) {
        unlock();
        return;
    }

    if(!L.quiet && level >= L.level) {
        init_event(&ev, stderr);
        va_start(ev.ap, fmt);
        stdout_callback(&ev);
        va_end(ev.ap);
    }

    for(int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Callback* cb = &L.callbacks[i];
        if(level >= cb->level) {
            init_event(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    unlock();
}