/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <machine/time.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#if LIB_KIANV_STDIO
#include "kianv/stdio.h"
#endif

#include "kianv/time.h"

#define weak_raw_printf printf
#define weak_raw_vprintf vprintf

static int picolibc_putc(char c, __unused FILE *file) {
#if LIB_KIANV_STDIO
    stdio_putchar(c);
#endif
    return c;
}

static int picolibc_getc(__unused FILE *file) {
#if LIB_KIANV_STDIO
    return stdio_getchar();
#else
    return -1;
#endif
}

static int picolibc_flush(__unused FILE *file) {
#if LIB_KIANV_STDIO
    stdio_flush();
#endif
    return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(picolibc_putc,
                                        picolibc_getc,
                                        picolibc_flush,
                                        _FDEV_SETUP_RW);

#ifdef __GNUCLIKE_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

FILE *const stdin = &__stdio; __strong_reference(stdin, stdout); __strong_reference(stdin, stderr);

void __weak __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
    panic("assertion \"%s\" failed: file \"%s\", line %d%s%s",
          failedexpr, file, line, func ? ", function: " : "",
          func ? func : "");
}


// Halts: there is no debugger to stop at, and an ebreak here would be reported
// as a crash by the default exception handler
void __attribute__((noreturn)) __weak _exit(__unused int status) {
    disable_interrupts();
    while (1) {
        __wfi();
    }
}

int __weak getpid(void) {
    return 0;
}

// picolibc's raise() calls _exit() directly for a signal without a handler, so
// abort() is replaced to report through panic() (a SIGABRT handler installed
// with signal() is not called)
void __weak abort(void) {
    panic("abort() called");
}

// Reached by raise() when the signal has no handler, e.g. from abort()
int __weak kill(__unused int pid, int sig) {
    panic("%s (signal %d)", sig == SIGABRT ? "abort() called" : "raise()", sig);
}

static int64_t epoch_time_us_since_boot;

__weak int gettimeofday (struct timeval *__restrict tv, __unused void *__restrict tz) {
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

__weak clock_t times(struct tms *tms) {
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

void runtime_init(void) {
    // piolibc __libc_init_array does __preint_array and __init_array
    extern void __libc_init_array(void);
    __libc_init_array();
}