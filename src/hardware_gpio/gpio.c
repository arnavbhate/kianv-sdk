/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"

void gpio_init(unsigned int gpio)
{
    check_gpio_param(gpio);
    gpio_init_mask(1u << gpio);
}

void gpio_deinit(unsigned int gpio)
{
    check_gpio_param(gpio);
    gpio_set_dir_in_masked(1u << gpio);
}

void gpio_init_mask(uint32_t gpio_mask)
{
    // input first, so a pin that was an output driving high does not glitch low
    // before becoming an input
    gpio_set_dir_in_masked(gpio_mask);
    gpio_clr_mask(gpio_mask);
}
