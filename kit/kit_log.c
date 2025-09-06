#include "kit.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// from https://github.com/rxi/log.c

#define MAX_CALLBACKS 32

typedef struct {
    kit_log_fn fn;
    void *udata;
    kit_log_level level;
} _kit_log_callback;

static struct {
    void* udata;
    kit_log_lock_fn lock;
    kit_log_level level;
    bool quiet;
    _kit_log_callback callbacks[MAX_CALLBACKS];
} _kit_logger;

static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef KIT_LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void _stdout_callback(kit_log_event *ev) {
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
    #ifdef KIT_LOG_USE_COLOR
    fprintf(
        ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
        buf, level_colors[ev->level], level_strings[ev->level],
        ev->file, ev->line);
    #else
    fprintf(
        ev->udata, "%s %-5s %s:%d: ",
        buf, level_strings[ev->level], ev->file, ev->line);
    #endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void _file_callback(kit_log_event *ev) {
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(
        ev->udata, "%s %-5s %s:%d: ",
        buf, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void _lock(void)   {
    if (_kit_logger.lock) { _kit_logger.lock(true, _kit_logger.udata); }
}


static void _unlock(void) {
    if (_kit_logger.lock) { _kit_logger.lock(false, _kit_logger.udata); }
}


const char* _log_level_string(int level) {
    return level_strings[level];
}


void kit_log_set_lock(kit_log_lock_fn fn, void *udata) {
    _kit_logger.lock = fn;
    _kit_logger.udata = udata;
}


void kit_log_set_level(kit_log_level level) {
  _kit_logger.level = level;
}


void kit_log_set_quiet(bool enable) {
  _kit_logger.quiet = enable;
}


bool kit_log_add_callback(kit_log_fn fn, void *udata, kit_log_level level) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!_kit_logger.callbacks[i].fn) {
        _kit_logger.callbacks[i] = (_kit_log_callback) { fn, udata, level };
        return true;
        }
    }
    return false;
}

bool kit_log_add_file(const char *path, kit_log_level level) {
    FILE *file = fopen(path, "a");
    if (!file) return false;

    return kit_log_add_callback(_file_callback, file, level);
}

static void _init_event(kit_log_event* ev, void *udata) {
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}


void kit_log(kit_log_level level, const char* file, int line, const char *fmt, ...) {
    kit_log_event ev = {
        .fmt   = fmt,
        .file  = file,
        .line  = line,
        .level = level,
    };

    _lock();

    if (!_kit_logger.quiet && level >= _kit_logger.level) {
        _init_event(&ev, stderr);
        va_start(ev.ap, fmt);
        _stdout_callback(&ev);
        va_end(ev.ap);
    }

    for (int i = 0; i < MAX_CALLBACKS && _kit_logger.callbacks[i].fn; i++) {
        _kit_log_callback* cb = &_kit_logger.callbacks[i];
        if (level >= cb->level) {
            _init_event(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    _unlock();
}