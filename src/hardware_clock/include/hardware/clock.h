/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_CLOCK_H
#define _HARDWARE_CLOCK_H

#include "kianv/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t clock_get_hz()
{
    return *CPU_FREQ_REG;
}

#ifdef __cplusplus
}
#endif

#endif
