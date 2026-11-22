/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/spi.h"

// RTL-WORKAROUND (defines_soc.vh, spi.v): KIANV_SPI_CTRL1_FREQ and
// KIANV_SPI_CTRL2_FREQ are not the SPI1/SPI2 clocks but targets the fixed
// dividers are derived from: divider = SYSTEM_CLK / target, and SCLK =
// SYSTEM_CLK / (2 * divider), half the named frequency
#ifndef KIANV_SPI1_DIV_TARGET_HZ
#define KIANV_SPI1_DIV_TARGET_HZ 13000000u
#endif
#ifndef KIANV_SPI2_DIV_TARGET_HZ
#define KIANV_SPI2_DIV_TARGET_HZ 20000000u
#endif

static unsigned int spi_get_divider(const spi_inst_t *spi)
{
    unsigned int div;
    switch (spi_get_index(spi)) {
    case 0:
        div = *DIV_REG >> 16;
        break;
    case 1:
        div = clock_get_hz() / KIANV_SPI1_DIV_TARGET_HZ;
        break;
    default:
        div = clock_get_hz() / KIANV_SPI2_DIV_TARGET_HZ;
        break;
    }
    // a divider of 0 behaves like 1 (SCLK toggles every clk_sys cycle)
    return div ? div : 1;
}

static inline void spi_wait_idle(const spi_hw_t *hw)
{
    while (hw->ctrl & SPI_CTRL_BUSY_BITS)
        ;
}

// RTL-WORKAROUND (spi.v): always wait for the SPI to be idle before writing the
// data register: a write while a transfer is in progress is held off by the
// bus until it ends, and if it lands in the cycle the transfer completes the
// SPI block locks up with its busy flag stuck (the S_XFER completion branch
// overrides the new transfer's state change). Also wait before reading: during
// a transfer the data register still holds the previous byte.
static inline uint8_t spi_transfer_byte(spi_hw_t *hw, uint8_t tx)
{
    spi_wait_idle(hw);
    hw->data = tx;
    spi_wait_idle(hw);
    return (uint8_t)hw->data;
}

unsigned int spi_init(spi_inst_t *spi, unsigned int baudrate)
{
    spi_hw_t *hw = spi_get_hw(spi);
    spi_wait_idle(hw);
    spi_cs_deselect(spi);
    return spi_set_baudrate(spi, baudrate);
}

void spi_deinit(spi_inst_t *spi)
{
    spi_wait_idle(spi_get_hw(spi));
    spi_cs_deselect(spi);
}

unsigned int spi_set_baudrate(spi_inst_t *spi, unsigned int baudrate)
{
    if (spi_get_index(spi) == 0 && baudrate) {
        uint32_t clk = clock_get_hz();
        // fastest rate not above the requested one: smallest divider with clk /
        // (2 * div) <= baudrate
        uint64_t div =
            ((uint64_t)clk + 2ull * baudrate - 1) / (2ull * baudrate);
        if (div < 1)
            div = 1;
        if (div > 0xFFFF)
            div = 0xFFFF;
        // RTL-WORKAROUND (soc.v): DIV_REG bits 15:0 are the UART divider, and
        // the register is always written as a whole (byte enables ignored)
        spi_wait_idle(spi_get_hw(spi));
        uint32_t save = save_and_disable_interrupts();
        *DIV_REG = (*DIV_REG & 0x0000FFFFu) | ((uint32_t)div << 16);
        restore_interrupts(save);
    }
    return spi_get_baudrate(spi);
}

unsigned int spi_get_baudrate(const spi_inst_t *spi)
{
    return clock_get_hz() / (2 * spi_get_divider(spi));
}

void spi_set_format(spi_inst_t *spi, unsigned int data_bits, spi_cpol_t cpol,
                    spi_cpha_t cpha, spi_order_t order)
{
    // spi0 and spi1 are mode 0, spi2 (OLED) is mode 3
    unsigned int mode = spi_get_index(spi) == 2 ? 1 : 0;
    if (data_bits != 8 || order != SPI_MSB_FIRST ||
        (unsigned int)cpol != mode || (unsigned int)cpha != mode)
        panic("spi_set_format(spi%u, %u bits, CPOL %d, CPHA %d, %s first): "
              "spi%u only supports 8 bits, MSB first, mode %u",
              spi_get_index(spi), data_bits, cpol, cpha,
              order == SPI_MSB_FIRST ? "MSB" : "LSB", spi_get_index(spi),
              mode ? 3u : 0u);
}

int spi_write_read_blocking(spi_inst_t *spi, const uint8_t *src, uint8_t *dst,
                            size_t len)
{
    spi_hw_t *hw = spi_get_hw(spi);
    for (size_t i = 0; i < len; i++)
        dst[i] = spi_transfer_byte(hw, src[i]);
    return (int)len;
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len)
{
    spi_hw_t *hw = spi_get_hw(spi);
    for (size_t i = 0; i < len; i++)
        (void)spi_transfer_byte(hw, src[i]);
    return (int)len;
}

int spi_read_blocking(spi_inst_t *spi, uint8_t repeated_tx_data, uint8_t *dst,
                      size_t len)
{
    spi_hw_t *hw = spi_get_hw(spi);
    for (size_t i = 0; i < len; i++)
        dst[i] = spi_transfer_byte(hw, repeated_tx_data);
    return (int)len;
}
