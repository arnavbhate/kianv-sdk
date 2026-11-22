/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _KIANV_CSR_H
#define _KIANV_CSR_H

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

#define _riscv_read_csr(csrname)                                  \
    ({                                                            \
        uint32_t __csr_tmp_u32;                                   \
        asm volatile("csrr %0, " #csrname : "=r"(__csr_tmp_u32)); \
        __csr_tmp_u32;                                            \
    })

#define _riscv_write_csr(csrname, data)                           \
    ({                                                            \
        if (__builtin_constant_p(data) && !((data) & -32u)) {     \
            asm volatile("csrwi " #csrname ", %0" : : "i"(data)); \
        } else {                                                  \
            asm volatile("csrw " #csrname ", %0" : : "r"(data));  \
        }                                                         \
    })

#define _riscv_set_csr(csrname, data)                             \
    ({                                                            \
        if (__builtin_constant_p(data) && !((data) & -32u)) {     \
            asm volatile("csrsi " #csrname ", %0" : : "i"(data)); \
        } else {                                                  \
            asm volatile("csrs " #csrname ", %0" : : "r"(data));  \
        }                                                         \
    })

#define _riscv_clear_csr(csrname, data)                           \
    ({                                                            \
        if (__builtin_constant_p(data) && !((data) & -32u)) {     \
            asm volatile("csrci " #csrname ", %0" : : "i"(data)); \
        } else {                                                  \
            asm volatile("csrc " #csrname ", %0" : : "r"(data));  \
        }                                                         \
    })

#define _riscv_read_write_csr(csrname, data)                  \
    ({                                                        \
        uint32_t __csr_tmp_u32;                               \
        if (__builtin_constant_p(data) && !((data) & -32u)) { \
            asm volatile("csrrwi %0, " #csrname ", %1"        \
                         : "=r"(__csr_tmp_u32)                \
                         : "i"(data));                        \
        } else {                                              \
            asm volatile("csrrw %0, " #csrname ", %1"         \
                         : "=r"(__csr_tmp_u32)                \
                         : "r"(data));                        \
        }                                                     \
        __csr_tmp_u32;                                        \
    })

#define _riscv_read_set_csr(csrname, data)                    \
    ({                                                        \
        uint32_t __csr_tmp_u32;                               \
        if (__builtin_constant_p(data) && !((data) & -32u)) { \
            asm volatile("csrrsi %0, " #csrname ", %1"        \
                         : "=r"(__csr_tmp_u32)                \
                         : "i"(data));                        \
        } else {                                              \
            asm volatile("csrrs %0, " #csrname ", %1"         \
                         : "=r"(__csr_tmp_u32)                \
                         : "r"(data));                        \
        }                                                     \
        __csr_tmp_u32;                                        \
    })

#define _riscv_read_clear_csr(csrname, data)                  \
    ({                                                        \
        uint32_t __csr_tmp_u32;                               \
        if (__builtin_constant_p(data) && !((data) & -32u)) { \
            asm volatile("csrrci %0, " #csrname ", %1"        \
                         : "=r"(__csr_tmp_u32)                \
                         : "i"(data));                        \
        } else {                                              \
            asm volatile("csrrc %0, " #csrname ", %1"         \
                         : "=r"(__csr_tmp_u32)                \
                         : "r"(data));                        \
        }                                                     \
        __csr_tmp_u32;                                        \
    })

// Argument macro expansion layer (CSR name may be a macro that expands to a
// CSR number, or it may be a bare name that the assembler knows about.)
#define riscv_read_csr(csrname) _riscv_read_csr(csrname)
#define riscv_write_csr(csrname, data) _riscv_write_csr(csrname, data)
#define riscv_set_csr(csrname, data) _riscv_set_csr(csrname, data)
#define riscv_clear_csr(csrname, data) _riscv_clear_csr(csrname, data)
#define riscv_read_write_csr(csrname, data) _riscv_read_write_csr(csrname, data)
#define riscv_read_set_csr(csrname, data) _riscv_read_set_csr(csrname, data)
#define riscv_read_clear_csr(csrname, data) _riscv_read_clear_csr(csrname, data)

#ifdef __cplusplus
}
#endif

#endif
#endif
