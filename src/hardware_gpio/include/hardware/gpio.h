/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_GPIO_H
#define _HARDWARE_GPIO_H

#include "kianv/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// Every pin is a plain input or output: no pin functions, pulls (set in the
// FPGA constraints file) or interrupts.
// RTL-WORKAROUND (gpio.v): the hardware has no atomic set/clear registers and
// ignores byte enables, so functions changing only some pins read-modify-write
// whole registers with interrupts disabled.

#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_GPIO
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_GPIO 0
#endif

#if NUM_GPIOS == 32
#define GPIO_ALL_PINS_MASK 0xFFFFFFFFu
#else
#define GPIO_ALL_PINS_MASK ((1u << NUM_GPIOS) - 1u)
#endif

#define GPIO_OUT 1
#define GPIO_IN 0

typedef struct {
    volatile uint32_t dir; // 1 = output, 0 = input (all inputs after reset)
    volatile uint32_t out; // levels driven on output pins
    volatile uint32_t in; // levels on the pins (read only)
} gpio_hw_t;

#define gpio_hw ((gpio_hw_t *)GPIO_BASE)

static inline void check_gpio_param(__unused unsigned int gpio)
{
    if (PARAM_ASSERTIONS_ENABLED_HARDWARE_GPIO && gpio >= NUM_GPIOS)
        panic("invalid GPIO %u (this SoC has %d)", gpio, NUM_GPIOS);
}

// Makes the pins inputs, with output level low
void gpio_init(unsigned int gpio);
void gpio_deinit(unsigned int gpio);
void gpio_init_mask(uint32_t gpio_mask);

static inline bool gpio_get(unsigned int gpio)
{
    check_gpio_param(gpio);
    return (gpio_hw->in >> gpio) & 1u;
}

static inline uint32_t gpio_get_all(void)
{
    return gpio_hw->in & GPIO_ALL_PINS_MASK;
}

static inline void gpio_set_mask(uint32_t mask)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->out |= mask & GPIO_ALL_PINS_MASK;
    restore_interrupts(save);
}

static inline void gpio_clr_mask(uint32_t mask)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->out &= ~mask;
    restore_interrupts(save);
}

static inline void gpio_xor_mask(uint32_t mask)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->out ^= mask & GPIO_ALL_PINS_MASK;
    restore_interrupts(save);
}

// Sets the pins in mask to the corresponding bits of value
static inline void gpio_put_masked(uint32_t mask, uint32_t value)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->out = (gpio_hw->out & ~mask) | (value & mask & GPIO_ALL_PINS_MASK);
    restore_interrupts(save);
}

static inline void gpio_put_all(uint32_t value)
{
    gpio_hw->out = value & GPIO_ALL_PINS_MASK;
}

static inline void gpio_put(unsigned int gpio, bool value)
{
    check_gpio_param(gpio);
    uint32_t mask = 1u << gpio;
    if (value)
        gpio_set_mask(mask);
    else
        gpio_clr_mask(mask);
}

// The level last written to the pin, not the level on it (see gpio_get())
static inline bool gpio_get_out_level(unsigned int gpio)
{
    check_gpio_param(gpio);
    return (gpio_hw->out >> gpio) & 1u;
}

static inline void gpio_set_dir_out_masked(uint32_t mask)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->dir |= mask & GPIO_ALL_PINS_MASK;
    restore_interrupts(save);
}

static inline void gpio_set_dir_in_masked(uint32_t mask)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->dir &= ~mask;
    restore_interrupts(save);
}

// Sets the direction of the pins in mask from the corresponding bits of value
// (1 = output)
static inline void gpio_set_dir_masked(uint32_t mask, uint32_t value)
{
    uint32_t save = save_and_disable_interrupts();
    gpio_hw->dir = (gpio_hw->dir & ~mask) | (value & mask & GPIO_ALL_PINS_MASK);
    restore_interrupts(save);
}

static inline void gpio_set_dir_all_bits(uint32_t values)
{
    gpio_hw->dir = values & GPIO_ALL_PINS_MASK;
}

static inline void gpio_set_dir(unsigned int gpio, bool out)
{
    check_gpio_param(gpio);
    uint32_t mask = 1u << gpio;
    if (out)
        gpio_set_dir_out_masked(mask);
    else
        gpio_set_dir_in_masked(mask);
}

static inline bool gpio_is_dir_out(unsigned int gpio)
{
    check_gpio_param(gpio);
    return (gpio_hw->dir >> gpio) & 1u;
}

static inline unsigned int gpio_get_dir(unsigned int gpio)
{
    return gpio_is_dir_out(gpio);
}

#ifdef __cplusplus
}
#endif

#endif
