/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "hardware/timer.h"

static hardware_alarm_callback_t alarm_callback = NULL;
static bool alarm_claimed = false;

bool hardware_alarm_claim_unused(bool required)
{
    uint32_t save = save_and_disable_interrupts();
    bool was_claimed =
        __atomic_exchange_n(&alarm_claimed, true, __ATOMIC_ACQUIRE);
    restore_interrupts(save);
    if (!was_claimed)
        return true;
    if (required)
        panic(
            "hardware_alarm_claim_unused: the hardware alarm is already claimed");
    return false;
}

void hardware_alarm_unclaim(void)
{
    uint32_t save = save_and_disable_interrupts();
    if (alarm_claimed)
        __atomic_store_n(&alarm_claimed, false, __ATOMIC_RELEASE);
    else
        panic("hardware_alarm_unclaim: the hardware alarm is not claimed");
    restore_interrupts(save);
}

bool hardware_alarm_is_claimed(void)
{
    return alarm_claimed;
}

void isr_riscv_machine_timer(void)
{
    // disarm first: the interrupt stays pending until mtimecmp is above mtime
    set_mtimecmp(UINT64_MAX);
    hardware_alarm_callback_t callback = alarm_callback;
    if (callback)
        callback();
}

void hardware_alarm_set_callback(hardware_alarm_callback_t callback)
{
    uint32_t save = save_and_disable_interrupts();
    if (callback) {
        if (!alarm_callback) {
            // RTL-WORKAROUND (clint.v): mtimecmp resets to 0, so it is set
            // before the interrupt is enabled
            set_mtimecmp(UINT64_MAX);
            riscv_set_csr(mie, 1 << 7);
        }
    } else if (alarm_callback) {
        riscv_clear_csr(mie, 1 << 7);
        set_mtimecmp(UINT64_MAX);
    }
    alarm_callback = callback;
    restore_interrupts(save);
}

bool hardware_alarm_set_target(uint64_t t)
{
    uint32_t save = save_and_disable_interrupts();
    uint64_t now = time_us_64();
    bool missed = (t <= now);
    if (!missed) {
        set_mtimecmp(t);
    }
    restore_interrupts(save);
    return missed;
}
