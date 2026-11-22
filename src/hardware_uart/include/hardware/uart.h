/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_UART_H
#define _HARDWARE_UART_H

#include "kianv/platform.h"
#include "hardware/clock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile uint8_t data;
    uint8_t pad[4];
    volatile uint8_t lsr;
} uart_inst_t;

#define UART_INSTANCE(num) ((uart_inst_t *)UART_BASE(num))

#define uart0 UART_INSTANCE(0)
#if NUM_UARTS > 1
#define uart1 UART_INSTANCE(1)
#endif
#if NUM_UARTS > 2
#define uart2 UART_INSTANCE(2)
#endif
#if NUM_UARTS > 3
#define uart3 UART_INSTANCE(3)
#endif
#if NUM_UARTS > 4
#define uart4 UART_INSTANCE(4)
#endif

// UART used by stdio; KIANV_DEFAULT_UART selects it by number (or define
// uart_default directly)
#ifndef KIANV_DEFAULT_UART
#define KIANV_DEFAULT_UART 0
#endif
#if KIANV_DEFAULT_UART < 0 || KIANV_DEFAULT_UART >= NUM_UARTS
#error KIANV_DEFAULT_UART must be less than NUM_UARTS
#endif
#ifndef uart_default
#define uart_default UART_INSTANCE(KIANV_DEFAULT_UART)
#endif

#ifndef KIANV_DEFAULT_UART_BAUD_RATE
#define KIANV_DEFAULT_UART_BAUD_RATE 115200
#endif

#define UART_NUM(uart) \
    ((unsigned int)(((uintptr_t)(uart) - UART0_BASE) / UART_STRIDE))
#define UART_IS_INSTANCE(uart)                   \
    ((uintptr_t)(uart) >= UART0_BASE &&          \
     (uintptr_t)(uart) < UART_BASE(NUM_UARTS) && \
     ((uintptr_t)(uart) - UART0_BASE) % UART_STRIDE == 0)

// The UART with the given number, or NULL if the SoC does not have it
static inline uart_inst_t *uart_get_instance(unsigned int num)
{
    return num < NUM_UARTS ? UART_INSTANCE(num) : NULL;
}

// All UARTs share one baud rate divider (DIV_REG bits 15:0): setting the baud
// rate of one UART sets it for all of them.

// RTL-WORKAROUND (rx_uart.v, tx_uart.v): each bit takes div + 1 clocks, not
// div as tx_uart.v documents (the bit state takes a clock on top of the
// div-clock wait), so the baud rate is clk / (div + 1) and the divider is
// written as the number of clocks per bit minus 1.

// Sets the closest baud rate the divider allows and returns it (0 if uart is
// not a UART or baudrate is 0).
// RTL-WORKAROUND (soc.v): DIV_REG ignores byte enables (all 32 bits are always
// written) and bits 31:16 are the SPI0 divider, so it is read-modify-written
// with interrupts disabled.
static inline unsigned int uart_set_baudrate(uart_inst_t *uart,
                                             unsigned int baudrate)
{
    if (!UART_IS_INSTANCE(uart) || !baudrate)
        return 0;
    uint32_t clocks_per_bit = (clock_get_hz() + baudrate / 2) / baudrate;
    if (clocks_per_bit < 2)
        clocks_per_bit = 2;
    if (clocks_per_bit > 0x10000)
        clocks_per_bit = 0x10000;
    uint32_t div = clocks_per_bit - 1;
    uint32_t save = save_and_disable_interrupts();
    *DIV_REG = (*DIV_REG & 0xFFFF0000u) | div;
    restore_interrupts(save);
    return clock_get_hz() / (div + 1);
}

// The current baud rate, or 0 if the UART has not been set up yet
static inline unsigned int uart_get_baudrate(const uart_inst_t *uart)
{
    unsigned int div = *UART_DIV_REG;
    if (!UART_IS_INSTANCE(uart) || !div)
        return 0;
    return clock_get_hz() / (div + 1); // RTL-WORKAROUND: see above
}

// Sets the baud rate and returns the actual one. There is nothing else to set
// up: the format is fixed (8 data bits, no parity, 1 stop bit).
static inline unsigned int uart_init(uart_inst_t *uart, unsigned int baudrate)
{
    return uart_set_baudrate(uart, baudrate);
}

static inline bool uart_is_writable(uart_inst_t *uart)
{
    if (UART_IS_INSTANCE(uart))
        return (uart->lsr & 0x20);
    else
        return false;
}

// Waits until everything written has been sent: LSR bit 5 (THRE) only means
// there is room for another byte, bit 6 (TEMT) that the transmitter is idle
static inline void uart_tx_wait_blocking(uart_inst_t *uart)
{
    if (UART_IS_INSTANCE(uart))
        while ((uart->lsr & 0x60) != 0x60)
            ;
}

static inline bool uart_is_readable(uart_inst_t *uart)
{
    if (UART_IS_INSTANCE(uart))
        return (uart->lsr & 0x01);
    else
        return false;
}

static inline void uart_write_blocking(uart_inst_t *uart, const uint8_t *src,
                                       size_t len)
{
    if (UART_IS_INSTANCE(uart))
        for (size_t i = 0; i < len; ++i) {
            while (!(uart->lsr & 0x20))
                ;
            uart->data = *src++;
        }
}

static inline void uart_read_blocking(uart_inst_t *uart, uint8_t *dst,
                                      size_t len)
{
    if (UART_IS_INSTANCE(uart))
        for (size_t i = 0; i < len; ++i) {
            while (!(uart->lsr & 0x01))
                ;
            *dst++ = uart->data;
        }
}

static inline void uart_putc(uart_inst_t *uart, const char c)
{
    if (UART_IS_INSTANCE(uart)) {
        while (!(uart->lsr & 0x20))
            ;
        uart->data = c;
    }
}

static inline void uart_puts(uart_inst_t *uart, const char *s)
{
    if (UART_IS_INSTANCE(uart))
        while (*s) {
            while (!(uart->lsr & 0x20))
                ;
            uart->data = *s++;
        }
}

static inline char uart_getc(uart_inst_t *uart)
{
    if (UART_IS_INSTANCE(uart)) {
        while (!(uart->lsr & 0x01))
            ;
        return uart->data;
    } else {
        return 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif
