#ifndef _KIANV_SYNC_H
#define _KIANV_SYNC_H

#include "kianv/compiler.h"
#include "kianv/csr.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void __wfi(void)
{
    __asm__ volatile("wfi");
}

static inline void __nop(void)
{
    __asm__ volatile("nop");
}

static inline bool interrupts_disabled(void)
{
    uint32_t mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    return !(mstatus & 8);
}

static inline void disable_interrupts(void)
{
    __compiler_memory_barrier();
    riscv_clear_csr(mstatus, 8);
    __compiler_memory_barrier();
}

static inline void enable_interrupts(void)
{
    __compiler_memory_barrier();
    riscv_set_csr(mstatus, 8);
    __compiler_memory_barrier();
}

static inline uint32_t save_and_disable_interrupts(void)
{
    uint32_t status;
    __asm volatile("csrrci %0, mstatus, 0x8\n" : "=r"(status)::"memory");
    return status;
}

static inline void restore_interrupts(uint32_t status)
{
    if (status & 8) {
        __asm__ volatile("csrsi mstatus, 8" ::: "memory");
    }
}

// no longer use __mem_fence_acquire here, as it is overkill on cortex M0+
#if KIANV_C_COMPILER_IS_GNU
#define remove_volatile_cast(t, x) (t)(x)
#define remove_volatile_cast_no_barrier(t, x) (t)(x)
#else
#define remove_volatile_cast(t, x)                                    \
    (__compiler_memory_barrier(),                                     \
     Clang_Pragma("clang diagnostic push")                            \
         Clang_Pragma("clang diagnostic ignored \"-Wcast-qual\"")(t)( \
             x)Clang_Pragma("clang diagnostic pop"))
#define remove_volatile_cast_no_barrier(t, x)                        \
    Clang_Pragma("clang diagnostic push")                            \
        Clang_Pragma("clang diagnostic ignored \"-Wcast-qual\"")(t)( \
            x)Clang_Pragma("clang diagnostic pop")
#endif

#ifdef __cplusplus
}
#endif

#endif
