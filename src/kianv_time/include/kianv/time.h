/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _KIANV_TIME_H
#define _KIANV_TIME_H

#include "hardware/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t delayed_by_us(uint64_t t, uint64_t us)
{
    return (us >= (UINT64_MAX - t)) ? UINT64_MAX : (t + us);
}

static inline uint64_t delayed_by_ms(uint64_t t, uint32_t ms)
{
    return delayed_by_us(t, (uint64_t)ms * 1000);
}

static inline uint64_t make_timeout_time_us(uint64_t us)
{
    return delayed_by_us(time_us_64(), us);
}

static inline uint64_t make_timeout_time_ms(uint32_t ms)
{
    return delayed_by_ms(time_us_64(), ms);
}

static inline bool time_reached(uint64_t t)
{
    return (time_us_64() >= t);
}

void sleep_until(uint64_t t);
void sleep_us(uint64_t us);
void sleep_ms(uint32_t ms);
bool best_effort_wfe_or_timeout(uint64_t timeout_timestamp);

#ifndef KIANV_TIME_MAX_ALARMS
#define KIANV_TIME_MAX_ALARMS 16
#endif

typedef int32_t alarm_id_t;
typedef int64_t (*alarm_callback_t)(alarm_id_t id, void *user_data);

alarm_id_t add_alarm_at(uint64_t time, alarm_callback_t callback,
                        void *user_data, bool fire_if_past);

static inline alarm_id_t add_alarm_in_us(uint64_t us, alarm_callback_t callback,
                                         void *user_data, bool fire_if_past)
{
    return add_alarm_at(time_us_64() + us, callback, user_data, fire_if_past);
}

static inline alarm_id_t add_alarm_in_ms(uint32_t ms, alarm_callback_t callback,
                                         void *user_data, bool fire_if_past)
{
    return add_alarm_at(time_us_64() + ms * (uint64_t)1000, callback, user_data,
                        fire_if_past);
}

bool cancel_alarm(alarm_id_t alarm_id);

int64_t remaining_alarm_time_us(alarm_id_t alarm_id);
int32_t remaining_alarm_time_ms(alarm_id_t alarm_id);

typedef struct repeating_timer repeating_timer_t;

typedef bool (*repeating_timer_callback_t)(repeating_timer_t *rt);

struct repeating_timer {
    int64_t delay_us;
    alarm_id_t alarm_id;
    repeating_timer_callback_t callback;
    void *user_data;
};

bool add_repeating_timer_us(int64_t delay_us,
                            repeating_timer_callback_t callback,
                            void *user_data, repeating_timer_t *out);

static inline bool add_repeating_timer_ms(int32_t delay_ms,
                                          repeating_timer_callback_t callback,
                                          void *user_data,
                                          repeating_timer_t *out)
{
    return add_repeating_timer_us(delay_ms * (int64_t)1000, callback, user_data,
                                  out);
}

bool cancel_repeating_timer(repeating_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif
