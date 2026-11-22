/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "kianv/time.h"

static bool init = false;

typedef struct {
    uint64_t target;
    alarm_callback_t callback;
    void *user_data;
    alarm_id_t id; // 0 when the slot is free
} alarm_t;

static alarm_t alarms[KIANV_TIME_MAX_ALARMS];
static alarm_id_t next_id = 1;

// The CLINT timer interrupt is level triggered (mtime >= mtimecmp), so an alarm
// that is already due is simply left to the interrupt, which is taken as soon
// as interrupts are enabled. Never run callbacks from here: this is called from
// inside alarm callbacks (via add_alarm_at/cancel_alarm), and the alarm whose
// callback is running is still due.
static void update_hardware_alarm(void)
{
    uint64_t earliest = UINT64_MAX;
    for (int i = 0; i < KIANV_TIME_MAX_ALARMS; i++) {
        if (alarms[i].id && alarms[i].target < earliest)
            earliest = alarms[i].target;
    }
    if (earliest == UINT64_MAX)
        hardware_alarm_cancel();
    else
        set_mtimecmp(earliest);
}

static void run_due_alarms(void)
{
    for (int i = 0; i < KIANV_TIME_MAX_ALARMS; i++) {
        alarm_t *a = &alarms[i];
        if (!a->id || !time_reached(a->target))
            continue;
        alarm_id_t id = a->id;
        int64_t rc = a->callback(id, a->user_data);
        // the callback may have cancelled the alarm, or its slot may now
        // hold another alarm
        if (a->id != id)
            continue;
        if (rc < 0) {
            a->target = delayed_by_us(a->target, (uint64_t)-rc);
        } else if (rc > 0) {
            a->target = make_timeout_time_us(rc);
        } else {
            a->id = 0;
        }
    }
    // an alarm rescheduled into the past makes the interrupt fire again right
    // after this returns
    update_hardware_alarm();
}

static void alarm_irq_callback()
{
    run_due_alarms();
}

alarm_id_t add_alarm_at(uint64_t time, alarm_callback_t callback,
                        void *user_data, bool fire_if_past)
{
    if (time_reached(time)) {
        if (!fire_if_past)
            return 0;
        int64_t rc = callback(0, user_data);
        if (rc == 0)
            return 0;
        time = rc < 0 ? delayed_by_us(time, (uint64_t)-rc) :
                        make_timeout_time_us(rc);
    }
    uint32_t save = save_and_disable_interrupts();
    if (!init) {
        init = hardware_alarm_claim_unused(false);
        if (!init) {
            restore_interrupts(save);
            return -1;
        }
        hardware_alarm_set_callback(alarm_irq_callback);
    }
    alarm_id_t id = -1;
    for (int i = 0; i < KIANV_TIME_MAX_ALARMS; i++) {
        if (!alarms[i].id) {
            id = next_id;
            next_id = next_id == INT32_MAX ? 1 : next_id + 1;
            alarms[i] = (alarm_t){ time, callback, user_data, id };
            break;
        }
    }
    if (id > 0)
        update_hardware_alarm();
    restore_interrupts(save);
    return id;
}

static alarm_t *find_alarm(alarm_id_t alarm_id)
{
    for (int i = 0; alarm_id > 0 && i < KIANV_TIME_MAX_ALARMS; i++) {
        if (alarms[i].id == alarm_id)
            return &alarms[i];
    }
    return NULL;
}

bool cancel_alarm(alarm_id_t alarm_id)
{
    uint32_t save = save_and_disable_interrupts();
    alarm_t *a = find_alarm(alarm_id);
    if (a) {
        a->id = 0;
        update_hardware_alarm();
    }
    restore_interrupts(save);
    return a != NULL;
}

int64_t remaining_alarm_time_us(alarm_id_t alarm_id)
{
    uint32_t save = save_and_disable_interrupts();
    alarm_t *a = find_alarm(alarm_id);
    int64_t rc = a ? (int64_t)(a->target - time_us_64()) : -1;
    restore_interrupts(save);
    return a && rc < 0 ? 0 : rc;
}

int32_t remaining_alarm_time_ms(alarm_id_t alarm_id)
{
    int64_t us = remaining_alarm_time_us(alarm_id);
    return us < 0 ? -1 : (int32_t)(us / 1000);
}

static int64_t repeating_timer_callback(alarm_id_t id, void *user_data)
{
    repeating_timer_t *rt = (repeating_timer_t *)user_data;
    // the first call can come from add_alarm_at(), before alarm_id is set
    if (id > 0)
        rt->alarm_id = id;
    if (rt->callback(rt)) {
        return rt->delay_us;
    }
    rt->alarm_id = 0;
    return 0;
}

bool add_repeating_timer_us(int64_t delay_us,
                            repeating_timer_callback_t callback,
                            void *user_data, repeating_timer_t *out)
{
    if (!delay_us)
        delay_us = 1;
    out->callback = callback;
    out->delay_us = delay_us;
    out->user_data = user_data;
    uint64_t first = (uint64_t)(delay_us < 0 ? -delay_us : delay_us);
    out->alarm_id = add_alarm_at(make_timeout_time_us(first),
                                 repeating_timer_callback, out, true);
    return out->alarm_id > 0;
}

bool cancel_repeating_timer(repeating_timer_t *timer)
{
    bool rc = false;
    if (timer->alarm_id) {
        rc = cancel_alarm(timer->alarm_id);
        timer->alarm_id = 0;
    }
    return rc;
}

// ---- sleep ----

// RTL-WORKAROUND (csr_unit.v): the core treats wfi as a no-op, so there is
// nothing to gain from waiting for an alarm interrupt: sleeping just polls the
// timer. This also works with interrupts disabled and uses no alarm slot.
void sleep_until(uint64_t t)
{
    while (!time_reached(t))
        ;
}

void sleep_us(uint64_t us)
{
    sleep_until(make_timeout_time_us(us));
}

void sleep_ms(uint32_t ms)
{
    sleep_us((uint64_t)ms * 1000);
}

// May return before the timeout (here: immediately); returns whether it has
// been reached
bool best_effort_wfe_or_timeout(uint64_t timeout_timestamp)
{
    return time_reached(timeout_timestamp);
}
