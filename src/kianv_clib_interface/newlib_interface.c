/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "kianv/time.h"

#define weak_raw_printf printf
#define weak_raw_vprintf vprintf
#if LIB_KIANV_STDIO
#include "kianv/stdio.h"
#endif

extern char __StackLimit; /* Set by linker.  */

#define STDIO_HANDLE_STDIN  0
#define STDIO_HANDLE_STDOUT 1
#define STDIO_HANDLE_STDERR 2

// Halts: there is no debugger to stop at, and an ebreak here would be reported
// as a crash by the default exception handler
void __attribute__((noreturn)) __weak _exit(__unused int status) {
    disable_interrupts();
    while (1) {
        __wfi();
    }
}

void __weak _fini(void) {
}

__weak void *_sbrk(int incr) {
    extern char end; /* Set by linker.  */
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0)
        heap_end = &end;

    prev_heap_end = heap_end;
    char *next_heap_end = heap_end + incr;

    if (__builtin_expect(next_heap_end > (&__StackLimit), false)) {
#if KIANV_USE_OPTIMISTIC_SBRK
        if (heap_end == &__StackLimit) {
//        errno = ENOMEM;
            return (char *) -1;
        }
        next_heap_end = &__StackLimit;
#else
        return (char *) -1;
#endif
    }

    heap_end = next_heap_end;
    return (void *) prev_heap_end;
}

static int64_t epoch_time_us_since_boot;

__weak int _gettimeofday (struct timeval *__restrict tv, __unused void *__restrict tz) {
    if (tv) {
        int64_t us_since_epoch = ((int64_t)time_us_64()) - epoch_time_us_since_boot;
        tv->tv_sec = (time_t)(us_since_epoch / 1000000);
        tv->tv_usec = (suseconds_t)(us_since_epoch % 1000000);
    }
    return 0;
}

__weak int settimeofday(__unused const struct timeval *tv, __unused const struct timezone *tz) {
    if (tv) {
        int64_t us_since_epoch = tv->tv_sec * 1000000 + tv->tv_usec;
        epoch_time_us_since_boot = (int64_t)time_us_64() - us_since_epoch;
    }
    return 0;
}

__weak int _times(struct tms *tms) {
#if CLOCKS_PER_SEC >= 1000000
    tms->tms_utime = (clock_t)(time_us_64() * (CLOCKS_PER_SEC / 1000000));
#else
    tms->tms_utime = (clock_t)(time_us_64() / (1000000 / CLOCKS_PER_SEC));
#endif
    tms->tms_stime = 0;
    tms->tms_cutime = 0;
    tms->tms_cstime = 0;
    return 0;
}

__weak pid_t _getpid(void) {
    return 0;
}

// Reached by raise() when the signal has no handler, e.g. from abort()
__weak int _kill(__unused pid_t pid, int sig) {
    panic("%s (signal %d)", sig == SIGABRT ? "abort() called" : "raise()", sig);
}

int __attribute__((weak)) _read(int handle, char *buffer, int length) {
#if LIB_KIANV_STDIO
    if (handle == STDIO_HANDLE_STDIN) {
        return stdio_get_until(buffer, length, UINT64_MAX);
    }
#endif
    return -1;
}

int __attribute__((weak)) _write(int handle, char *buffer, int length) {
#if LIB_KIANV_STDIO
    if (handle == STDIO_HANDLE_STDOUT || handle == STDIO_HANDLE_STDERR) {
        stdio_put_string(buffer, length, false);
        return length;
    }
#endif
    return -1;
}

int __attribute__((weak)) _open(__unused const char *fn, __unused int oflag, ...) {
    return -1;
}

int __attribute__((weak)) _close(__unused int fd) {
    return -1;
}

off_t __attribute__((weak)) _lseek(__unused int fd, __unused off_t pos, __unused int whence) {
    return -1;
}

// newlib makes a stream line buffered only if fstat succeeds and reports a character device; otherwise stdout is
// fully buffered (BUFSIZ bytes), and printf output only appears once a buffer fills
int __attribute__((weak)) _fstat(int fd, struct stat *buf) {
    if (fd == STDIO_HANDLE_STDIN || fd == STDIO_HANDLE_STDOUT || fd == STDIO_HANDLE_STDERR) {
        buf->st_mode = S_IFCHR;
        return 0;
    }
    return -1;
}

int __attribute__((weak)) _isatty(int fd) {
    return fd == STDIO_HANDLE_STDIN || fd == STDIO_HANDLE_STDOUT || fd == STDIO_HANDLE_STDERR;
}

int __attribute__((weak)) _getentropy (__unused void *buffer, __unused size_t length) {
    // note we don't hook this up as it isn't clear if/where it is used, and we don't have
    // kianv_rand. the user can supply their own strong implementation if they need it!
    return -1;
}

// exit is not useful... no desire to pull in __call_exitprocs
void exit(int status) {
    // atexit handlers are not run, but buffered output must not be lost
#if LIB_KIANV_STDIO
    stdio_flush();
#endif
    _exit(status);
}

// incorrect warning from GCC 6
GCC_Pragma("GCC diagnostic push")
GCC_Pragma("GCC diagnostic ignored \"-Wsuggest-attribute=format\"")
void __weak __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
    panic("assertion \"%s\" failed: file \"%s\", line %d%s%s",
          failedexpr, file, line, func ? ", function: " : "",
          func ? func : "");
}
GCC_Pragma("GCC diagnostic pop")

void runtime_init(void) {
    // todo maybe we want to do this in the future, but it does stuff like register_tm_clones
    //      which we didn't do in previous SDKs
    //extern void __libc_init_array(void);
    //__libc_init_array();

    // ... so instead just do the __preinit_array
    extern void (*__preinit_array_start)(void);
    extern void (*__preinit_array_end)(void);
    for (void (**p)(void) = &__preinit_array_start; p < &__preinit_array_end; ++p) {
        (*p)();
    }
    // ... and the __init_array
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);
    for (void (**p)(void) = &__init_array_start; p < &__init_array_end; ++p) {
        (*p)();
    }
}