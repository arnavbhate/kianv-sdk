/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _KIANV_PLATFORM_H
#define _KIANV_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "kianv/compiler.h"
#include "kianv/csr.h"
#include "kianv/sync.h"

#define IO_BASE 0x10000000

#define DIV_REG (volatile uint32_t *)(IO_BASE + 0x0010)
#define UART_DIV_REG (const volatile uint16_t *)(IO_BASE + 0x0010)
// RTL-WORKAROUND (soc.v): the SoC decodes only the exact address 0x10000010
// for the divider register (other addresses read as 0), so the SPI0 divider
// is (*DIV_REG >> 16), not a halfword at 0x10000012
#define CPU_FREQ_REG (volatile uint32_t *)(IO_BASE + 0x0014)
// SDRAM size in bytes
#define SDRAM_SIZE_REG (volatile uint32_t *)(IO_BASE + 0x0018)
#define SDRAM_BASE 0x80000000u

// Writing REBOOT_MAGIC to REBOOT_ADDR resets the whole SoC, which then boots
// from flash again
#define REBOOT_ADDR 0x11100000u
#define REBOOT_MAGIC 0x7777u

// Number of UARTs the SoC was built with (its NUM_UARTS parameter); set with
// the KIANV_NUM_UARTS CMake variable. UART n is at IO_BASE + 0x100 * n; the SoC
// defines addresses for UART0-UART4.
#ifndef NUM_UARTS
#define NUM_UARTS 2
#endif
#if NUM_UARTS < 1 || NUM_UARTS > 5
#error NUM_UARTS must be between 1 and 5
#endif
#define UART_STRIDE 0x100u
#define UART_BASE(n) (IO_BASE + UART_STRIDE * (n))
#define UART0_BASE UART_BASE(0)
#if NUM_UARTS > 1
#define UART1_BASE UART_BASE(1)
#endif
#if NUM_UARTS > 2
#define UART2_BASE UART_BASE(2)
#endif
#if NUM_UARTS > 3
#define UART3_BASE UART_BASE(3)
#endif
#if NUM_UARTS > 4
#define UART4_BASE UART_BASE(4)
#endif

// GPIO block (SOC_HAS_GPIO): direction, output and input registers, one bit per
// pin. NUM_GPIOS is the SoC's GPIO_NR (10 on the ULX3S build); set it with the
// KIANV_NUM_GPIOS CMake variable, 0 if the SoC has no GPIO block.
#ifndef NUM_GPIOS
#define NUM_GPIOS 10
#endif
#if NUM_GPIOS < 0 || NUM_GPIOS > 32
#error NUM_GPIOS must be between 0 and 32
#endif
#define GPIO_BASE (IO_BASE + 0x0700)

// SPI masters: SPI0 (SD card, divider in DIV_REG bits 31:16), SPI1
// (SOC_HAS_NETWORK, fixed clock) and SPI2 (SOC_HAS_EXT_OLED, fixed clock,
// transmit only). SPI n is at SPI_BASE(n).
#define NUM_SPIS 3
#define SPI_STRIDE 0x100u
#define SPI_BASE(n) (0x10500000u + SPI_STRIDE * (n))
#define SPI0_BASE SPI_BASE(0)
#define SPI1_BASE SPI_BASE(1)
#define SPI2_BASE SPI_BASE(2)

#ifdef __cplusplus
extern "C" {
#endif

// Prints a message (printf-style) through the panic handler, then halts with
// interrupts disabled. The handler is chosen when the program is built:
// KIANV_PANIC_HANDLER if the program defines it, otherwise the stdio one
// (prints to the UART) if kianv_stdio is linked, otherwise one that does
// nothing. Unhandled exceptions and interrupts also end up here.
void __attribute__((noreturn, format(printf, 1, 2))) panic(const char *fmt,
                                                           ...);

// Type of a panic handler: called with interrupts disabled; panic() halts
// when it returns
typedef void (*panic_handler_t)(const char *fmt, va_list args);

void runtime_init(void);

// Resets the SoC. Anything still being sent on the UART is cut off, so call
// stdio_flush() first if needed.
static inline void __attribute__((noreturn)) kianv_reboot(void)
{
    *(volatile uint32_t *)REBOOT_ADDR = REBOOT_MAGIC;
    for (;;)
        ;
}

#ifdef __cplusplus
}
#endif

#endif
