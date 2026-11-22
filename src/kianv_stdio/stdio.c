/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/uart.h"
#include "kianv/stdio.h"

#include <stdio.h>

#ifndef KIANV_STDIO_DEFAULT_CRLF
#define KIANV_STDIO_DEFAULT_CRLF 1
#endif

// strlen() that also works without the C library (kianv_clib_interface).
// Counted through a volatile pointer: GCC would otherwise turn the loop back
// into a strlen() call.
static int stdio_strlen(const char *s)
{
    int len = 0;
    for (const volatile char *q = s; *q; q++)
        len++;
    return len;
}

static bool translate_crlf = KIANV_STDIO_DEFAULT_CRLF;

void stdio_set_translate_crlf(bool translate)
{
    translate_crlf = translate;
}

static void stdio_out_chars(const char *s, int len)
{
    if (!translate_crlf) {
        uart_write_blocking(uart_default, (const uint8_t *)s, (size_t)len);
        return;
    }
    for (int i = 0; i < len; i++) {
        if (s[i] == '\n')
            uart_putc(uart_default, '\r');
        uart_putc(uart_default, s[i]);
    }
}

static int stdio_in_chars(char *buf, int len)
{
    int n = 0;
    while (n < len && uart_is_readable(uart_default))
        buf[n++] = (char)uart_default->data;
    return n;
}

int stdio_put_string(const char *s, int len, bool newline)
{
    if (len == -1)
        len = stdio_strlen(s);
    stdio_out_chars(s, len);
    if (newline) {
        const char c = '\n';
        stdio_out_chars(&c, 1);
    }
    return len;
}

int stdio_get_until(char *buf, int len, uint64_t until)
{
    do {
        int read = stdio_in_chars(buf, len);
        if (read > 0) {
            return read;
        }
        if (time_reached(until)) {
            return -1;
        }
    } while (true);
}

void stdio_flush(void)
{
// picolibc also defines __NEWLIB__, and its fflush calls this function
// (picolibc_flush), so it must not be called from here with picolibc
#if LIB_KIANV_CLIB_INTERFACE && defined(__NEWLIB__) && !defined(__PICOLIBC__)
    // newlib buffers stdout (picolibc's stdout here is unbuffered, and its
    // flush calls this function)
    fflush(stdout);
#endif
    uart_tx_wait_blocking(uart_default);
}

bool stdio_init_all(void)
{
    uart_init(uart_default, KIANV_DEFAULT_UART_BAUD_RATE);
    return true;
}

bool stdio_deinit_all(void)
{
    stdio_flush();
    return true;
}

int stdio_getchar_timeout_us(uint32_t timeout_us)
{
    char buf[1];
    int rc =
        stdio_get_until(buf, sizeof(buf), make_timeout_time_us(timeout_us));
    if (rc < 0)
        return rc;
    return (uint8_t)buf[0];
}

int stdio_getchar(void)
{
    char buf[1];
    int len = stdio_get_until(buf, 1, UINT64_MAX);
    if (len < 0)
        return len;
    return (uint8_t)buf[0];
}

int stdio_putchar(int c)
{
    char cc = (char)c;
    stdio_put_string(&cc, 1, false);
    return c;
}

int stdio_puts(const char *s)
{
    int len = stdio_strlen(s);
    stdio_put_string(s, len, true);
    stdio_flush();
    return len;
}

// ----------------------------------------------------------------------------
// Panic handler used when kianv_stdio is linked (see panic.c). It writes
// straight to the UART with its own small formatter instead of printf: it may
// run in an interrupt handler or inside printf, and works without a C library.
// Supports %c %s %d %i %u %x %X %p %%, the 0 and - flags, a width and the l
// modifier.

static void panic_putc(char c)
{
    if (c == '\n' && translate_crlf)
        uart_putc(uart_default, '\r');
    uart_putc(uart_default, c);
}

static void panic_puts(const char *s)
{
    while (*s)
        panic_putc(*s++);
}

// Prints s (len characters), padded to width: on the right if left is set,
// otherwise on the left with pad
static void panic_put_padded(const char *s, int len, int width, bool left,
                             char pad)
{
    if (!left)
        for (int i = len; i < width; i++)
            panic_putc(pad);
    for (int i = 0; i < len; i++)
        panic_putc(s[i]);
    if (left)
        for (int i = len; i < width; i++)
            panic_putc(' ');
}

static void panic_put_number(uint32_t v, unsigned int base, bool upper,
                             bool negative, int width, bool left, char pad)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char buf[12];
    int n = sizeof buf;
    do {
        buf[--n] = digits[v % base];
        v /= base;
    } while (v);
    if (negative) {
        if (pad == '0' && !left) {
            panic_putc('-'); // sign before the zeros
            width--;
        } else {
            buf[--n] = '-';
        }
    }
    panic_put_padded(buf + n, (int)sizeof buf - n, width, left, pad);
}

static void panic_vprintf(const char *fmt, va_list args)
{
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            panic_putc(*fmt);
            continue;
        }
        bool left = false;
        char pad = ' ';
        int width = 0;
        for (fmt++; *fmt == '-' || *fmt == '0'; fmt++) {
            if (*fmt == '-')
                left = true;
            else
                pad = '0';
        }
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');
        while (*fmt == 'l')
            fmt++;
        switch (*fmt) {
        case 'c': {
            char c = (char)va_arg(args, int);
            panic_put_padded(&c, 1, width, left, ' ');
            break;
        }
        case 's': {
            const char *str = va_arg(args, const char *);
            if (!str)
                str = "(null)";
            // counted through a volatile pointer: GCC would otherwise turn
            // the loop into a strlen() call, which does not exist without a
            // C library
            int len = 0;
            for (const volatile char *q = str; *q; q++)
                len++;
            panic_put_padded(str, len, width, left, ' ');
            break;
        }
        case 'd':
        case 'i': {
            int32_t v = va_arg(args, int32_t);
            panic_put_number(v < 0 ? -(uint32_t)v : (uint32_t)v, 10, false,
                             v < 0, width, left, pad);
            break;
        }
        case 'u':
            panic_put_number(va_arg(args, uint32_t), 10, false, false, width,
                             left, pad);
            break;
        case 'x':
        case 'X':
            panic_put_number(va_arg(args, uint32_t), 16, *fmt == 'X', false,
                             width, left, pad);
            break;
        case 'p':
            panic_puts("0x");
            panic_put_number((uint32_t)(uintptr_t)va_arg(args, void *), 16,
                             false, false, 8, false, '0');
            break;
        case '\0':
            return;
        default: // %% and anything unsupported
            panic_putc(*fmt);
        }
    }
}

void kianv_stdio_panic_handler(const char *fmt, va_list args)
{
    // the UART may not have been set up yet
    if (!uart_get_baudrate(uart_default))
        uart_init(uart_default, KIANV_DEFAULT_UART_BAUD_RATE);
    panic_puts("\n*** PANIC *** ");
    panic_vprintf(fmt, args);
    panic_putc('\n');
    uart_tx_wait_blocking(uart_default);
}
