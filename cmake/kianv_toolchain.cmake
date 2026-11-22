#   KIANV_ARCH            -march string (default: the SoC's ISA, rv32ima_zicntr_zicsr_zifencei_zaamo_zalrsc_sstc);
#                         older toolchains that do not know zaamo/zalrsc/sstc can use rv32ima_zicntr_zicsr_zifencei
#   KIANV_TOOLCHAIN_PATH  install prefix of the toolchain (the directory containing bin/); PATH is searched otherwise
#   KIANV_TOOLCHAIN_PREFIX  prefix of the toolchain's programs (default: riscv32-unknown-elf-)
#
# Each is a CMake variable (e.g. -DKIANV_TOOLCHAIN_PATH=/opt/riscv32); environment variables are not used.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)
# compiler checks only compile, since nothing links without the SDK's startup code and linker script
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if (NOT KIANV_ARCH)
    set(KIANV_ARCH rv32ima_zicntr_zicsr_zifencei_zaamo_zalrsc_sstc)
endif()
set(KIANV_ARCH ${KIANV_ARCH} CACHE STRING "-march value for the KianV SoC")
if (NOT DEFINED KIANV_TOOLCHAIN_PREFIX)
    set(KIANV_TOOLCHAIN_PREFIX riscv32-unknown-elf-)
endif()
set(KIANV_TOOLCHAIN_PREFIX ${KIANV_TOOLCHAIN_PREFIX} CACHE STRING "Prefix of the toolchain's programs")

function(kianv_find_program VAR)
    find_program(${VAR} NAMES ${ARGN} PATHS ${KIANV_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)
    find_program(${VAR} NAMES ${ARGN})
    if (NOT ${VAR})
        message(FATAL_ERROR "Could not find ${ARGN}; set KIANV_TOOLCHAIN_PATH to the toolchain's install directory (and KIANV_TOOLCHAIN_PREFIX if its programs are not named riscv32-unknown-elf-*)")
    endif()
endfunction()

set(KIANV_ARCH_FLAGS "-march=${KIANV_ARCH} -mabi=ilp32")

set(_cxx_flags "")

set(CMAKE_C_COMPILER)
set(CMAKE_CXX_COMPILER)
set(CMAKE_OBJCOPY)
set(CMAKE_OBJDUMP)
kianv_find_program(C_COMPILER ${KIANV_TOOLCHAIN_PREFIX}gcc)
kianv_find_program(CXX_COMPILER ${KIANV_TOOLCHAIN_PREFIX}g++)
kianv_find_program(OBJCOPY ${KIANV_TOOLCHAIN_PREFIX}objcopy)
# optional: without it, no TARGET.asm is written
find_program(OBJDUMP ${KIANV_TOOLCHAIN_PREFIX}objdump PATHS ${KIANV_TOOLCHAIN_PATH} PATH_SUFFIXES bin NO_DEFAULT_PATH)
find_program(OBJDUMP ${KIANV_TOOLCHAIN_PREFIX}objdump)
if (NOT OBJDUMP)
    set(OBJDUMP "")
endif()
set(CMAKE_C_COMPILER ${C_COMPILER})
set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_OBJCOPY ${OBJCOPY})
set(CMAKE_OBJDUMP ${OBJDUMP})

set(_flags "${KIANV_ARCH_FLAGS}")
# crt0 comes from the SDK, and the C library and C++ standard library are only linked by programs that link
# kianv_clib_interface (directly or through kianv_stdlib); every executable gets libgcc (64-bit division, ...)
set(_link_flags "-nostdlib")
set(CMAKE_C_STANDARD_LIBRARIES_INIT "-lgcc")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "-lgcc")

# thread-local variables all live in the executable, so never need the GOT
set(KIANV_COMPILE_FLAGS "${_flags} -ffunction-sections -fdata-sections -ftls-model=local-exec")
set(KIANV_LINK_FLAGS "${_flags} ${_link_flags} -Wl,--gc-sections")

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
foreach (LANG IN ITEMS C CXX ASM)
    # Also added to every SDK program by kianv_platform, so that they are kept if CMAKE_<LANG>_FLAGS is set on
    # the command line (which replaces these defaults)
    set(CMAKE_${LANG}_FLAGS_INIT "${KIANV_COMPILE_FLAGS}")
    set(CMAKE_${LANG}_FLAGS_RELEASE_INIT "-O2 -g")
    set(CMAKE_EXECUTABLE_SUFFIX_${LANG} .elf)
endforeach()
set(CMAKE_CXX_FLAGS_INIT "${_cxx_flags} ${CMAKE_CXX_FLAGS_INIT}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${KIANV_LINK_FLAGS}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# The program to load is a raw binary, so every executable is also written as one (TARGET.bin, next to TARGET.elf),
# along with a link map (TARGET.map) and a disassembly (TARGET.asm)
foreach (LANG IN ITEMS C CXX)
    set(CMAKE_${LANG}_LINK_EXECUTABLE
            "<CMAKE_${LANG}_COMPILER> <FLAGS> <CMAKE_${LANG}_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> -Wl,-Map=<TARGET>.map <LINK_LIBRARIES>"
            "\"${CMAKE_COMMAND}\" \"-DOBJCOPY=${CMAKE_OBJCOPY}\" \"-DOBJDUMP=${CMAKE_OBJDUMP}\" -DELF=<TARGET> -P \"${CMAKE_CURRENT_LIST_DIR}/kianv_elf_to_bin.cmake\"")
endforeach()
