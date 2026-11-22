/*
 * Copyright (c) 2026 Arnav Bhate
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* Set memory locations either to the already defined value, or to the default value */

RAM_ORIGIN_DEFAULT = 0x81C00000;
RAM_LENGTH_DEFAULT = 0x81FFFFFF + 1 - 0x81C00000;

/* The program lives in flash and is linked to run from RAM. Only the first BOOT_SIZE bytes of the flash image are
   loaded into RAM by whatever starts the program; crt0 copies the rest of the image from flash into RAM. */
FLASH_ORIGIN_DEFAULT = 0x20100000;
/* The flash is 0x20000000-0x20FFFFFF; the program starts 1M into it, and may use the rest */
FLASH_LENGTH_DEFAULT = 0x20FFFFFF + 1 - 0x20100000;
BOOT_SIZE_DEFAULT = 8192;
/* The heap uses the SDRAM below the program: 0x80000000 up to RAM_ORIGIN (28 MiB) */
HEAP_ORIGIN_DEFAULT = 0x80000000;

PROVIDE( RAM_ORIGIN = RAM_ORIGIN_DEFAULT);
PROVIDE( RAM_LENGTH = RAM_LENGTH_DEFAULT);

RAM_ORIGIN = DEFINED(RAM_ORIGIN) ? RAM_ORIGIN : RAM_ORIGIN_DEFAULT;
RAM_LENGTH = DEFINED(RAM_LENGTH) ? RAM_LENGTH : RAM_LENGTH_DEFAULT;

PROVIDE( FLASH_ORIGIN = FLASH_ORIGIN_DEFAULT);
PROVIDE( FLASH_LENGTH = FLASH_LENGTH_DEFAULT);
PROVIDE( BOOT_SIZE = BOOT_SIZE_DEFAULT);
PROVIDE( HEAP_ORIGIN = HEAP_ORIGIN_DEFAULT);

FLASH_ORIGIN = DEFINED(FLASH_ORIGIN) ? FLASH_ORIGIN : FLASH_ORIGIN_DEFAULT;
FLASH_LENGTH = DEFINED(FLASH_LENGTH) ? FLASH_LENGTH : FLASH_LENGTH_DEFAULT;
BOOT_SIZE = DEFINED(BOOT_SIZE) ? BOOT_SIZE : BOOT_SIZE_DEFAULT;
HEAP_ORIGIN = DEFINED(HEAP_ORIGIN) ? HEAP_ORIGIN : HEAP_ORIGIN_DEFAULT;
PROVIDE( HEAP_LENGTH = RAM_ORIGIN - HEAP_ORIGIN);

MEMORY
{
    FLASH(rx) : ORIGIN = FLASH_ORIGIN, LENGTH = FLASH_LENGTH
    RAM(rwx) : ORIGIN =  RAM_ORIGIN, LENGTH = RAM_LENGTH
    HEAP(rw) : ORIGIN = HEAP_ORIGIN, LENGTH = HEAP_LENGTH
}

/* Separate read-only and writable segments, rather than one RWX segment, which GNU ld warns about */
PHDRS
{
    text PT_LOAD FLAGS(5);  /* Read and Execute */
    data PT_LOAD FLAGS(6);  /* Read and Write */
    heap PT_LOAD FLAGS(6);  /* Read and Write, nothing loaded */
    tls PT_TLS;             /* Thread-local storage (needed by ld.lld) */
}

/* Define entry point symbol */
ENTRY(_entry_point)

/* Defines the following symbols for use by code:
    __logical_binary_start
    __reset_start, __reset_end
    __eh_frame_start
*/

SECTIONS
{
    .text : AT(FLASH_ORIGIN + (ADDR(.text) - ORIGIN(RAM))) {
        __logical_binary_start = .;
        __reset_start = .;
        KEEP (*(.reset))
        __reset_end = .;
        *(.text*)
        . = ALIGN(4);
        *(.init)
        *(.fini)
        /* Pull all c'tors into .text */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)
        /* Followed by destructors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        /* C++ exception unwind tables, registered by runtime_init() in kianv_clib_interface */
        . = ALIGN(4);
        __eh_frame_start = .;
        KEEP (*(.eh_frame*))
        LONG(0) /* terminator for the unwinder */
    } > RAM : text
}

SECTIONS
{
    .rodata : AT(FLASH_ORIGIN + (ADDR(.rodata) - ORIGIN(RAM))) {
        . = ALIGN(4);
        *(.rodata*)
        *(.srodata*)
        *(.gcc_except_table*)
        . = ALIGN(4);
    } > RAM : text
}

/* Defines the following symbols for use by code:
    __data_start__, __data_end__
    __preinit_array_start, __preinit_array_end
    __init_array_start, __init_array_end
    __bothinit_array_start, __bothinit_array_end
    __fini_array_start, __fini_array_end
    __tdata_end
    __etext
    __bss_start__, __bss_end__
    __tls_base, __tls_end
    __tbss_end
    __global_pointer$
    __dso_handle
*/

SECTIONS
{
    .data : AT(FLASH_ORIGIN + (ADDR(.data) - ORIGIN(RAM))) {
        __data_start__ = .;
        *(vtable)

        *(.time_critical*)

        *(.data*)
        *(.sdata*)

        . = ALIGN(4);
        PROVIDE_HIDDEN (__emutls_array_start = .);
        *(.*.__emutls_v.*)
        PROVIDE_HIDDEN (__emutls_array_end = .);

        . = ALIGN(4);
        *(.after_data.*)
        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__bothinit_array_start = .);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(SORT(.preinit_array.*)))
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        PROVIDE_HIDDEN (__bothinit_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        *(SORT(.fini_array.*))
        *(.fini_array)
        PROVIDE_HIDDEN (__fini_array_end = .);
        PROVIDE(__dso_handle = 0);

        *(.jcr)
        . = ALIGN(4);
   } > RAM : data

    .tdata : AT(FLASH_ORIGIN + (ADDR(.tdata) - ORIGIN(RAM))) {
        . = ALIGN(4);
        *(.tdata .tdata.* .gnu.linkonce.td.*)
        /* All data end */
        __tdata_end = .;
    } > RAM : data : tls
    PROVIDE(__data_end__ = .);

    /* End of everything that has to be copied from flash to RAM (.text, .rodata, .data, .tdata) */
    __image_end__ = ALIGN(4);

    /* __etext is (for backwards compatibility) the name of the .data init source pointer (...) */
    __etext = LOADADDR(.data);

    . = ALIGN(4);
    /* note that .tbss is expected to directly follow .tdata */
    .tbss (NOLOAD) : {
        __bss_start__ = .;
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)

        . = ALIGN(4);
        __tls_end = .;
    } > RAM : data : tls

    /* workaround RISC-V GNU linker bug with spacer section (otherwise it places .bss over .tbss) */
    .tbss_space (NOLOAD) : {
        . = __bss_start__ + SIZEOF(.tbss);
    } > RAM : data

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __tbss_end = .;

        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
        *(COMMON)
        PROVIDE(__global_pointer$ = . + 2K);
        *(.sbss*)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM : data
}

SECTIONS
{
    .uninitialized_data (NOLOAD): {
        . = ALIGN(4);
        *(.uninitialized_data*)
    } > RAM : data
}

/* Sections for proper TLS

    Defines the following symbols for use by code:
    __tls_start, __tls_size, __tls_align
    __tls_base (== __tls_start: there is one hart, so one TLS block, used in place; crt0 points tp at it)
*/

SECTIONS
{
    PROVIDE( __tls_start = SIZEOF(.tdata) ? ADDR(.tdata) : ADDR(.tbss) );
    PROVIDE( __tls_size = __tls_end - __tls_start );
    PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
    __tls_base = __tls_start;
}

/* Defines the following symbols for use by code (used by crt0 to load the part of the image that was not
   loaded at boot):
    __boot_size__       number of bytes of the image that are already in RAM when the program starts
    __boot_copy_dst__   RAM address of the first byte that crt0 has to copy
    __boot_copy_src__   flash address of the first byte that crt0 has to copy
    __image_end__       RAM address of the end of the image (.text, .rodata, .data, .tdata)
*/

SECTIONS
{
    __boot_size__ = BOOT_SIZE;
    __boot_copy_dst__ = ORIGIN(RAM) + BOOT_SIZE;
    __boot_copy_src__ = LOADADDR(.text) + BOOT_SIZE;

    /* The reset code (which does the copying) has to be inside the part that is loaded at boot */
    ASSERT(__reset_start == ORIGIN(RAM), ".reset must be at the start of RAM")
    ASSERT(__reset_end <= ORIGIN(RAM) + BOOT_SIZE, ".reset does not fit in the part of the image loaded at boot")
    /* crt0 copies by offset, so the flash image must be laid out exactly like the RAM image */
    ASSERT(LOADADDR(.text) == FLASH_ORIGIN, ".text must be at the start of flash")
    ASSERT(LOADADDR(.rodata) - ADDR(.rodata) == FLASH_ORIGIN - ORIGIN(RAM), ".rodata flash layout differs from RAM layout")
    ASSERT(LOADADDR(.data) - ADDR(.data) == FLASH_ORIGIN - ORIGIN(RAM), ".data flash layout differs from RAM layout")
    ASSERT(LOADADDR(.tdata) - ADDR(.tdata) == FLASH_ORIGIN - ORIGIN(RAM), ".tdata flash layout differs from RAM layout")
}

/* Defines the following symbols for use by code:
    __end__, end
    __HeapLimit
*/

SECTIONS
{
    /* .heap is the start of the HEAP region (the SDRAM below the program), and contains crt0's minimum heap
       allocation (KIANV_HEAP_SIZE); the heap may grow to the end of the region */
    .heap (NOLOAD):
    {
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        . = ALIGN(16);
    } > HEAP : heap
    __HeapLimit = ORIGIN(HEAP) + LENGTH(HEAP);
}

SECTIONS
{
    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later
     *
     * it contains crt0's stack allocation (KIANV_STACK_SIZE), and is placed after .bss so that the linker
     * checks that it fits; the stack itself is at the top of RAM, and may grow down to the end of .bss
     */
    .stack_dummy (NOLOAD):
    {
        KEEP(*(.stack*))
    } > RAM : data
}

/* Defines the following symbols for use by code:
    __StackLimit
    __StackTop, __StackBottom
    __stack (== __StackTop)

    Also defines additional symbols for other compilers/libcs
*/

SECTIONS
{
    /* stack limit is poorly named, but historically is maximum heap ptr (used by _sbrk) */
    __StackLimit = __HeapLimit;
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackBottom = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* picolibc and LLVM */
    PROVIDE (__heap_start = __end__);
    PROVIDE (__heap_end = __HeapLimit);

    /* llvm-libc */
    PROVIDE (_end = __end__);

    /* Check that the minimum heap fits, and that the program and stack fit in RAM */
    ASSERT(__end__ + SIZEOF(.heap) <= __HeapLimit, "region HEAP overflowed")
    ASSERT(__bss_end__ + SIZEOF(.stack_dummy) <= __StackTop, "region RAM overflowed")
}
