/*
 * Copyright (c) 2026 Arnav Bhate
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "kianv/platform.h"

// RTL-WORKAROUND (main_fsm.v): the core traps on misaligned loads and stores.
// crt0 calls this for those traps, before the exception handler: it performs
// the access a byte at a time and skips the instruction. AMOs, LR and SC are
// not emulated (they would no longer be atomic), nor are loads into sp; those
// reach the exception handler.
// Disable with KIANV_EMULATE_MISALIGNED=0.
#ifndef KIANV_EMULATE_MISALIGNED
#define KIANV_EMULATE_MISALIGNED 1
#endif

#if KIANV_EMULATE_MISALIGNED
// Where crt0 saved each register: the index in the trap frame for the
// caller-saved registers, -1 for those in saved[] (indexed by register number)
static const int8_t frame_slot[32] = {
    -1, 0,  -1, -1, -1, 1,  2,  3,  -1, -1, 4,  5,  6,  7,  8,  9,
    10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15,
};

static uint32_t *reg_ptr(unsigned r, uint32_t *frame, uint32_t *saved)
{
    return frame_slot[r] >= 0 ? &frame[frame_slot[r]] : &saved[r];
}

static uint32_t reg_get(unsigned r, uint32_t *frame, uint32_t *saved)
{
    if (r == 0)
        return 0;
    if (r == 2)
        return (uint32_t)(uintptr_t)(frame + 16); // sp before the trap
    return *reg_ptr(r, frame, saved);
}
#endif

bool kianv_emulate_misaligned(__unused uint32_t mcause,
                              __unused uint32_t *frame,
                              __unused uint32_t *saved)
{
#if KIANV_EMULATE_MISALIGNED
    uint32_t mepc;
    __asm__ volatile("csrr %0, mepc" : "=r"(mepc));
    uint32_t insn = *(const volatile uint32_t *)(uintptr_t)mepc;
    unsigned opcode = insn & 0x7f;
    unsigned funct3 = (insn >> 12) & 7;
    unsigned rd = (insn >> 7) & 31;
    unsigned rs1 = (insn >> 15) & 31;
    unsigned rs2 = (insn >> 20) & 31;
    unsigned size = 1u << (funct3 & 3);

    if (opcode == 0x03 && mcause == 4 &&
        (funct3 == 1 || funct3 == 2 || funct3 == 5) && rd != 2) {
        // lh, lw, lhu
        uint32_t addr = reg_get(rs1, frame, saved) + ((int32_t)insn >> 20);
        const volatile uint8_t *p = (const volatile uint8_t *)(uintptr_t)addr;
        uint32_t value = 0;
        for (unsigned i = size; i-- > 0;)
            value = value << 8 | p[i];
        if (funct3 == 1)
            value = (uint32_t)(int32_t)(int16_t)value;
        if (rd != 0)
            *reg_ptr(rd, frame, saved) = value;
    } else if (opcode == 0x23 && mcause == 6 && (funct3 == 1 || funct3 == 2)) {
        // sh, sw
        int32_t imm = ((int32_t)(insn & 0xfe000000u) >> 20) |
                      (int32_t)((insn >> 7) & 31);
        uint32_t addr = reg_get(rs1, frame, saved) + imm;
        volatile uint8_t *p = (volatile uint8_t *)(uintptr_t)addr;
        uint32_t value = reg_get(rs2, frame, saved);
        for (unsigned i = 0; i < size; i++)
            p[i] = (uint8_t)(value >> (8 * i));
    } else {
        return false;
    }

    __asm__ volatile("csrw mepc, %0" : : "r"(mepc + 4));
    return true;
#else
    return false;
#endif
}
