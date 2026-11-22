/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_TIMER_H
#define _HARDWARE_TIMER_H

#include "kianv/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t time_us_64(void)
{
    uint32_t hi, lo, hi2;
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime %0" : "=r"(lo));
        __asm__ volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

static inline uint32_t time_us_32(void)
{
    uint32_t lo;
    __asm__ volatile("rdtime %0" : "=r"(lo));
    return lo;
}

static inline void busy_wait_until(uint64_t t)
{
    uint32_t hi_target = (uint32_t)(t >> 32u);
    uint32_t hi;
    __asm__ volatile("rdtimeh %0" : "=r"(hi));
    while (hi < hi_target)
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
    while (hi == hi_target && time_us_32() < (uint32_t)t)
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
}

static inline void busy_wait_us(uint64_t delay_us)
{
    uint64_t base = time_us_64();
    uint64_t target = base + delay_us;
    if (target < base) {
        target = (uint64_t)-1;
    }
    busy_wait_until(target);
}

static inline void busy_wait_us_32(uint32_t delay_us)
{
    if (0 <= (int32_t)delay_us) {
        // we only allow 31 bits, otherwise we could have a race in the loop
        // below with values very close to 2^32
        uint32_t start = time_us_32();
        while (time_us_32() - start < delay_us)
            ;
    } else {
        busy_wait_us(delay_us);
    }
}

static inline void busy_wait_ms(uint32_t delay_ms)
{
    if (delay_ms <= 0x7fffffffu / 1000) {
        busy_wait_us_32(delay_ms * 1000);
    } else {
        busy_wait_us(delay_ms * 1000ull);
    }
}

typedef void (*hardware_alarm_callback_t)(void);

/*
 * RTL-WORKAROUND (csr_unit.v, clint.v): the machine timer interrupt (MTIP)
 * comes from the memory-mapped CLINT (IRQ7 = mtime >= mtimecmp), NOT from the
 * custom CSRs 0x7C0/0x7C1 (those are only stored/read back by the core and
 * never compared against time), so the SDK uses the CLINT. CLINT mtimecmp
 * resets to 0, so MTIP is asserted from reset until this register is written:
 * it is set before the interrupt is enabled.
 */
#define CLINT_BASE 0x02000000u
#define CLINT_MTIMECMP_LO (*(volatile uint32_t *)(CLINT_BASE + 0x4000u))
#define CLINT_MTIMECMP_HI (*(volatile uint32_t *)(CLINT_BASE + 0x4004u))

static inline uint64_t get_mtimecmp(void)
{
    uint32_t lo, hi, hi2;
    do {
        hi = CLINT_MTIMECMP_HI;
        lo = CLINT_MTIMECMP_LO;
        hi2 = CLINT_MTIMECMP_HI;
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

static inline void set_mtimecmp(uint64_t val)
{
    CLINT_MTIMECMP_LO = 0xFFFFFFFFu;
    CLINT_MTIMECMP_HI = (uint32_t)(val >> 32);
    CLINT_MTIMECMP_LO = (uint32_t)val;
}

bool hardware_alarm_claim_unused(bool required);

void hardware_alarm_unclaim(void);

bool hardware_alarm_is_claimed(void);

void isr_riscv_machine_timer(void);

void hardware_alarm_set_callback(hardware_alarm_callback_t callback);

bool hardware_alarm_set_target(uint64_t t);

static inline void hardware_alarm_cancel(void)
{
    set_mtimecmp(UINT64_MAX);
}

static inline void hardware_alarm_force_irq(void)
{
    set_mtimecmp(0);
}

#ifdef __cplusplus
}
#endif

#endif
