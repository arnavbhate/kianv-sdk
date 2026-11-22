/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_SPI_H
#define _HARDWARE_SPI_H

#include "kianv/platform.h"
#include "hardware/clock.h"

#ifdef __cplusplus
extern "C" {
#endif

// Three SPI masters, 8 bits per transfer, MSB first, one byte at a time:
//   spi0: SD card, clk_sys / (2 * divider), mode 0
//   spi1: network header (SOC_HAS_NETWORK), fixed 7.5 MHz at 60 MHz, mode 0
//   spi2: OLED (SOC_HAS_EXT_OLED), fixed 10 MHz at 60 MHz, mode 3, no MISO
// Chip select is driven by the SPI block: use spi_cs_select() and
// spi_cs_deselect() around a transaction. An SPI missing from the SoC build
// ignores writes and reads 0.

#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_SPI
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_SPI 0
#endif

typedef struct {
    // write: bit 0 = 1 asserts chip select (pin low)
    // read: bit 31 = transfer in progress, bit 0 = chip select pin level
    volatile uint32_t ctrl;
    // write: starts a transfer of bits 7:0; read: last byte received
    volatile uint32_t data;
} spi_hw_t;

#define SPI_CTRL_BUSY_BITS (1u << 31)
#define SPI_CTRL_CS_BITS (1u << 0)

typedef struct spi_inst spi_inst_t;

#define SPI_INSTANCE(num) ((spi_inst_t *)SPI_BASE(num))
#define SPI_NUM(spi) \
    ((unsigned int)(((uintptr_t)(spi) - SPI0_BASE) / SPI_STRIDE))
#define SPI_IS_INSTANCE(spi)                                                   \
    ((uintptr_t)(spi) >= SPI0_BASE && (uintptr_t)(spi) < SPI_BASE(NUM_SPIS) && \
     ((uintptr_t)(spi) - SPI0_BASE) % SPI_STRIDE == 0)

#define spi0 SPI_INSTANCE(0)
#define spi1 SPI_INSTANCE(1)
#define spi2 SPI_INSTANCE(2)

#ifndef KIANV_DEFAULT_SPI
#define KIANV_DEFAULT_SPI 0
#endif
#if KIANV_DEFAULT_SPI < 0 || KIANV_DEFAULT_SPI >= NUM_SPIS
#error KIANV_DEFAULT_SPI must be less than NUM_SPIS
#endif
#ifndef spi_default
#define spi_default SPI_INSTANCE(KIANV_DEFAULT_SPI)
#endif

typedef enum { SPI_CPHA_0 = 0, SPI_CPHA_1 = 1 } spi_cpha_t;
typedef enum { SPI_CPOL_0 = 0, SPI_CPOL_1 = 1 } spi_cpol_t;
typedef enum { SPI_LSB_FIRST = 0, SPI_MSB_FIRST = 1 } spi_order_t;

static inline void check_spi_param(__unused const spi_inst_t *spi)
{
    if (PARAM_ASSERTIONS_ENABLED_HARDWARE_SPI && !SPI_IS_INSTANCE(spi))
        panic("invalid SPI instance %p", (const void *)spi);
}

static inline unsigned int spi_get_index(const spi_inst_t *spi)
{
    check_spi_param(spi);
    return SPI_NUM(spi);
}

// NULL if there is no such SPI
static inline spi_inst_t *spi_get_instance(unsigned int num)
{
    return num < NUM_SPIS ? SPI_INSTANCE(num) : NULL;
}

static inline spi_hw_t *spi_get_hw(spi_inst_t *spi)
{
    check_spi_param(spi);
    return (spi_hw_t *)spi;
}

static inline const spi_hw_t *spi_get_const_hw(const spi_inst_t *spi)
{
    check_spi_param(spi);
    return (const spi_hw_t *)spi;
}

// Deasserts chip select and sets the baud rate; returns the actual baud rate
unsigned int spi_init(spi_inst_t *spi, unsigned int baudrate);
void spi_deinit(spi_inst_t *spi);

// Only spi0 has a programmable clock: sets the fastest rate not above
// baudrate. Returns the actual baud rate (the fixed one for spi1 and spi2).
unsigned int spi_set_baudrate(spi_inst_t *spi, unsigned int baudrate);
unsigned int spi_get_baudrate(const spi_inst_t *spi);

// The format is fixed in hardware: does nothing if the requested format is
// the hardware's, panics otherwise
void spi_set_format(spi_inst_t *spi, unsigned int data_bits, spi_cpol_t cpol,
                    spi_cpha_t cpha, spi_order_t order);

static inline void spi_cs_select(spi_inst_t *spi)
{
    spi_get_hw(spi)->ctrl = SPI_CTRL_CS_BITS;
}

// Call only after the last transfer has finished (the blocking transfer
// functions return once it has)
static inline void spi_cs_deselect(spi_inst_t *spi)
{
    spi_get_hw(spi)->ctrl = 0;
}

static inline bool spi_cs_is_selected(const spi_inst_t *spi)
{
    return !(spi_get_const_hw(spi)->ctrl & SPI_CTRL_CS_BITS);
}

static inline bool spi_is_busy(const spi_inst_t *spi)
{
    return spi_get_const_hw(spi)->ctrl & SPI_CTRL_BUSY_BITS;
}

// No FIFOs: a byte can be written, or the last received byte read, whenever
// no transfer is in progress
static inline bool spi_is_writable(const spi_inst_t *spi)
{
    return !spi_is_busy(spi);
}

static inline bool spi_is_readable(const spi_inst_t *spi)
{
    return !spi_is_busy(spi);
}

// Blocking transfers; each returns len
int spi_write_read_blocking(spi_inst_t *spi, const uint8_t *src, uint8_t *dst,
                            size_t len);
int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len);
int spi_read_blocking(spi_inst_t *spi, uint8_t repeated_tx_data, uint8_t *dst,
                      size_t len);

#ifdef __cplusplus
}
#endif

#endif
