/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "kianv/platform.h"

#define COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))

// The handler panic() calls. A program can supply its own by defining
// KIANV_PANIC_HANDLER, e.g.
//   target_compile_definitions(app PRIVATE KIANV_PANIC_HANDLER=my_handler)
// with my_handler matching panic_handler_t.
#ifndef KIANV_PANIC_HANDLER
#if LIB_KIANV_STDIO
#define KIANV_PANIC_HANDLER kianv_stdio_panic_handler
#else
#define KIANV_PANIC_HANDLER kianv_panic_handler_silent
#endif
#endif

void KIANV_PANIC_HANDLER(const char *fmt, va_list args);

void kianv_panic_handler_silent(__unused const char *fmt, __unused va_list args)
{
}

__weak void panic(const char *fmt, ...)
{
    disable_interrupts();
    va_list args;
    va_start(args, fmt);
    KIANV_PANIC_HANDLER(fmt, args);
    va_end(args);
    for (;;)
        __wfi();
}

// ----------------------------------------------------------------------------
// Crash reports: crt0 calls these for exceptions and interrupts that have no
// handler. frame is the trap frame crt0 saved (ra is its first word).

static const char *const exception_names[] = {
    "instruction address misaligned",
    "instruction access fault",
    "illegal instruction",
    "breakpoint (ebreak, e.g. from abort())",
    "load address misaligned",
    "load access fault",
    "store address misaligned",
    "store access fault",
    "ecall from U-mode",
    "ecall from S-mode",
    "reserved",
    "ecall from M-mode",
    "instruction page fault",
    "load page fault",
    "reserved",
    "store page fault",
};

static const char *const interrupt_names[] = {
    "reserved", "supervisor software", "reserved", "machine software",
    "reserved", "supervisor timer",    "reserved", "machine timer",
    "reserved", "supervisor external", "reserved", "machine external",
};

void __attribute__((noreturn)) kianv_unhandled_exception(uint32_t mcause,
                                                         const uint32_t *frame)
{
    uint32_t mepc, mtval;
    __asm__ volatile("csrr %0, mepc" : "=r"(mepc));
    __asm__ volatile("csrr %0, mtval" : "=r"(mtval));
    const char *name = mcause < COUNT_OF(exception_names) ?
                           exception_names[mcause] :
                           "unknown exception";
    panic("%s (mcause %lu) at pc 0x%08lx, mtval 0x%08lx, ra 0x%08lx, "
          "sp 0x%08lx",
          name, (unsigned long)mcause, (unsigned long)mepc,
          (unsigned long)mtval, (unsigned long)frame[0],
          (unsigned long)(uintptr_t)(frame + 16));
}

void __attribute__((noreturn))
kianv_unhandled_interrupt(uint32_t mcause, __unused const uint32_t *frame)
{
    uint32_t mepc;
    __asm__ volatile("csrr %0, mepc" : "=r"(mepc));
    uint32_t code = mcause & 0x7fffffffu;
    const char *name =
        code < COUNT_OF(interrupt_names) ? interrupt_names[code] : "unknown";
    panic("unhandled %s interrupt (mcause 0x%08lx) at pc 0x%08lx", name,
          (unsigned long)mcause, (unsigned long)mepc);
}
