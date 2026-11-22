/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

// Space kept for the stack at the top of RAM, and the minimum heap size (the heap is in the SDRAM below the
// program, see memmap_default.ld)
#ifndef KIANV_STACK_SIZE
#define KIANV_STACK_SIZE 0x4000
#endif
#ifndef KIANV_HEAP_SIZE
#define KIANV_HEAP_SIZE 0x800
#endif

.section .reset, "ax"

.global _entry_point
_entry_point:

// Reset handler:
// - sets up gp, sp and tp
// - installs the trap handler
// - copies the rest of the image from flash into RAM (only the first __boot_size__ bytes are loaded at boot)
// - clears .bss
// - calls runtime_init
// - calls main
// - calls exit (which should eventually hang the processor via _exit)

_reset_handler:
.option push
.option norelax
    la gp, __global_pointer$
.option pop
    la sp, __StackTop
    // a single hart, so a single thread-local storage block, which the linker script puts in place
    la tp, __tls_base
    // Only the first __boot_size__ bytes of the image (which include this code) were loaded into RAM at boot.
    // Copy the rest from flash. Until this is done, nothing outside .reset may be used (no calls, no .rodata
    // or .data), and the copy itself must not use the stack.
    la a1, __boot_copy_dst__
    la a2, __image_end__
    la a3, __boot_copy_src__
    j image_copy_test
image_copy_loop:
    lw t0, (a3)
    sw t0, (a1)
    addi a3, a3, 4
    addi a1, a1, 4
image_copy_test:
    bltu a1, a2, image_copy_loop
    // the rest of .text was just written with stores: make it visible to instruction fetch
    fence.i

    // Leave interrupts globally disabled until .bss is clear; each one also has to be enabled in mie
    // RTL-WORKAROUND (csr_unit.v): direct mode (mtvec bits 1:0 = 0), as vectored mode also offsets exceptions
    la a0, isr_riscv_machine_trap
    csrw mtvec, a0

    // .bss (and .tbss, which comes just before it)
    la a1, __bss_start__
    la a2, __bss_end__
    j bss_fill_test
bss_fill_loop:
    sw zero, (a1)
    addi a1, a1, 4
bss_fill_test:
    bne a1, a2, bss_fill_loop

    csrw mie, zero
    csrsi mstatus, 0x8 // RVCSR_MSTATUS_MIE_BITS

platform_entry: // symbol for stack traces
    // Use `call` pseudo-instruction instead of a bare `jal` so that the
    // linker can use longer sequences if these are out of `jal` range. Will
    // still get relaxed to a `jal` if possible.
    call runtime_init
    li a0, 0 // argc
    li a1, 0 // argv
    call main
    call exit
    // exit should not return.  If it does, hang the core.
    // (fall thru into our hang exit impl
halt: // separate label because exit can be moved out of branch range
    wfi
    j halt

// ----------------------------------------------------------------------------
// Trap handler: saves the caller-saved registers, then calls isr_riscv_machine_timer for the machine timer
// interrupt, isr_riscv_machine_exception for an exception, or __unhandled_user_irq for any other interrupt (which
// is never enabled). These are ordinary C functions, called with mcause and a pointer to the saved registers
// (ra first), and run with interrupts disabled. An exception handler can read mcause/mepc/mtval, and must advance
// mepc to skip the faulting instruction. The defaults report the trap through panic().
// RTL-WORKAROUND (main_fsm.v): misaligned loads and stores (mcause 4 and 6) first go to kianv_emulate_misaligned (kianv_platform's
// misaligned.c), which performs them a byte at a time; only those it cannot emulate reach the exception handler.

.p2align 2
.global isr_riscv_machine_trap
isr_riscv_machine_trap:
    addi sp, sp, -64
    sw ra,  0(sp)
    sw t0,  4(sp)
    sw t1,  8(sp)
    sw t2, 12(sp)
    sw a0, 16(sp)
    sw a1, 20(sp)
    sw a2, 24(sp)
    sw a3, 28(sp)
    sw a4, 32(sp)
    sw a5, 36(sp)
    sw a6, 40(sp)
    sw a7, 44(sp)
    sw t3, 48(sp)
    sw t4, 52(sp)
    sw t5, 56(sp)
    sw t6, 60(sp)

    csrr a0, mcause
    mv a1, sp               // saved registers
    bltz a0, not_exception
    andi a2, a0, ~2
    li a3, 4
    beq a2, a3, misaligned  // mcause 4 or 6
exception:
    call isr_riscv_machine_exception
    j trap_return
misaligned:
    // The emulation may read or write any register, so also save the callee-saved ones, each at 4 * its
    // register number in a 32-word block (the others stay in the trap frame)
    addi sp, sp, -128
    sw gp,  12(sp)
    sw tp,  16(sp)
    sw s0,  32(sp)
    sw s1,  36(sp)
    sw s2,  72(sp)
    sw s3,  76(sp)
    sw s4,  80(sp)
    sw s5,  84(sp)
    sw s6,  88(sp)
    sw s7,  92(sp)
    sw s8,  96(sp)
    sw s9, 100(sp)
    sw s10, 104(sp)
    sw s11, 108(sp)
    mv a2, sp               // callee-saved registers
    addi a1, sp, 128        // trap frame
    call kianv_emulate_misaligned
    lw gp,  12(sp)
    lw tp,  16(sp)
    lw s0,  32(sp)
    lw s1,  36(sp)
    lw s2,  72(sp)
    lw s3,  76(sp)
    lw s4,  80(sp)
    lw s5,  84(sp)
    lw s6,  88(sp)
    lw s7,  92(sp)
    lw s8,  96(sp)
    lw s9, 100(sp)
    lw s10, 104(sp)
    lw s11, 108(sp)
    addi sp, sp, 128
    bnez a0, trap_return
    csrr a0, mcause         // not emulated: report it as usual
    mv a1, sp
    j exception
not_exception:
    slli a2, a0, 1
    li a3, 7 << 1           // machine timer interrupt
    bne a2, a3, not_timer
    call isr_riscv_machine_timer
    j trap_return
not_timer:
    call __unhandled_user_irq

trap_return:

    lw ra,  0(sp)
    lw t0,  4(sp)
    lw t1,  8(sp)
    lw t2, 12(sp)
    lw a0, 16(sp)
    lw a1, 20(sp)
    lw a2, 24(sp)
    lw a3, 28(sp)
    lw a4, 32(sp)
    lw a5, 36(sp)
    lw a6, 40(sp)
    lw a7, 44(sp)
    lw t3, 48(sp)
    lw t4, 52(sp)
    lw t5, 56(sp)
    lw t6, 60(sp)
    addi sp, sp, 64
    mret

// Default handlers: an unexpected exception or interrupt is reported through panic() (kianv_platform's panic.c)

.weak kianv_emulate_misaligned
kianv_emulate_misaligned:
    li a0, 0
    ret

.weak isr_riscv_machine_exception
isr_riscv_machine_exception:
    tail kianv_unhandled_exception

.weak isr_riscv_machine_timer
isr_riscv_machine_timer:
.weak __unhandled_user_irq
__unhandled_user_irq:
    tail kianv_unhandled_interrupt

// ----------------------------------------------------------------------------
// Provide safe default for exit
// Full implementation is provided by kianv_clib_interface

.weak exit
exit:
    wfi
    j exit

// ----------------------------------------------------------------------------
// Stack and heap spacers, which the linker script uses to size the stack and check that everything fits

.macro spacer_section name
.section \name, "aw", @nobits
.endm

spacer_section .stack
.p2align 4
    .equ StackSize, KIANV_STACK_SIZE
.space StackSize

spacer_section .heap
.p2align 2
    .equ HeapSize, KIANV_HEAP_SIZE
.space HeapSize
