/*
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "kianv/platform.h"

#define CXA_GUARD_UNINITIALIZED 0
#define CXA_GUARD_INITIALIZED 1
#define CXA_GUARD_INITIALIZING 2

int __cxa_guard_acquire(volatile int32_t *guard_object) {
    int32_t guard_val = *guard_object;
    if (guard_val == CXA_GUARD_INITIALIZED) {
        return 0;
    }
    if (guard_val & CXA_GUARD_INITIALIZING) {
        panic("recursive CXX initializer");
    }
    *guard_object = CXA_GUARD_INITIALIZING;
    return 1;
}

void __cxa_guard_release(volatile int32_t *guard_object) {
    *guard_object = CXA_GUARD_INITIALIZED;
}

void __cxa_guard_abort(volatile int32_t *guard_object) {
    *guard_object = CXA_GUARD_UNINITIALIZED;
}