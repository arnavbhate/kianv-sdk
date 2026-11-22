/*
 * Copyright (c) Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "kianv/platform.h"

#if !KIANV_CXX_ENABLE_EXCEPTIONS 
// Override the standard allocators to use regular malloc/free

#if !KIANV_CXX_DISABLE_ALLOCATION_OVERRIDES // Let user override
#include <stdlib.h> // weird looking, but it is required if dropping picolibc on top of GCC+newlib (reent issue)
#include <cstdlib>

void *operator new(std::size_t n) {
    return std::malloc(n);
}

void *operator new[](std::size_t n) {
    return std::malloc(n);
}

void operator delete(void *p) noexcept { std::free(p); }

void operator delete[](void *p) noexcept { std::free(p); }

#if __cpp_sized_deallocation

void operator delete(void *p, __unused std::size_t n) noexcept { std::free(p); }

void operator delete[](void *p, __unused std::size_t n) noexcept { std::free(p); }

#endif

#endif

#endif