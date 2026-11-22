/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _KIANV_COMPILER_H
#define _KIANV_COMPILER_H

#ifndef __ASSEMBLER__

#if defined __GNUC__
#include <sys/cdefs.h>
#if !defined(__printflike) && defined(__picolibc_format)
#define __printflike(a, b) __picolibc_format(printf, a, b)
#endif
// note LLVM defines __GNUC__
#ifdef __clang__
#define KIANV_C_COMPILER_IS_CLANG 1
#else
#define KIANV_C_COMPILER_IS_GNU 1
#endif
#else
#error Unsupported toolchain
#endif

// GCC_Like_Pragma(x) is a pragma on GNUC compatible compilers
#ifdef __GNUC__
#define GCC_Like_Pragma _Pragma
#else
#define GCC_Like_Pragma(x)
#endif

// Clang_Pragma(x) is a pragma on Clang only
#ifdef __clang__
#define Clang_Pragma _Pragma
#else
#define Clang_Pragma(x)
#endif

// GCC_Pragma(x) is a pragma on GCC only
#if KIANV_C_COMPILER_IS_GNU
#define GCC_Pragma _Pragma
#else
#define GCC_Pragma(x)
#endif

#ifndef __unused
#define __unused __attribute__((unused))
#endif
#ifndef __weak
#define __weak __attribute__((weak))
#endif

static void __compiler_memory_barrier(void)
{
    __asm__ volatile("" : : : "memory");
}

#ifndef __breakpoint
#define __breakpoint() __asm volatile("ebreak")
#endif

#else // __ASSEMBLER__

#if defined __GNUC__
// note LLVM defines __GNUC__
#ifdef __clang__
#define KIANV_ASSEMBLER_IS_CLANG 1
#else
#define KIANV_ASSEMBLER_IS_GNU 1
#endif
#else
#error Unsupported toolchain
#endif

#endif // !__ASSEMBLER__

#endif
