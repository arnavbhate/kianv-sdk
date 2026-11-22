/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _KIANV_STDIO_H
#define _KIANV_STDIO_H

#include <stdarg.h>
#include "kianv/time.h"

#ifdef __cplusplus
extern "C" {
#endif

bool stdio_init_all(void);

bool stdio_deinit_all(void);

void stdio_flush(void);

// Whether "\n" is sent as "\r\n" (default: KIANV_STDIO_DEFAULT_CRLF, 1)
void stdio_set_translate_crlf(bool translate);

int stdio_get_until(char *buf, int len, uint64_t until);

int stdio_put_string(const char *s, int len, bool newline);

int stdio_getchar(void);
// Waits up to timeout_us for a character; returns it (0-255), or -1 on timeout
int stdio_getchar_timeout_us(uint32_t timeout_us);

// The panic handler used when kianv_stdio is linked: prints "*** PANIC ***"
// and the message to the default UART. Can be called from a custom handler.
void kianv_stdio_panic_handler(const char *fmt, va_list args);
int stdio_putchar(int c);
int stdio_puts(const char *s);

#ifdef __cplusplus
}
#endif

#endif
