# Run after linking TARGET.elf: writes TARGET.bin (the raw image to load), renames the linker's TARGET.elf.map to
# TARGET.map, and writes a disassembly with the section headers to TARGET.asm

string(REGEX REPLACE "\\.elf$" "" BASE "${ELF}")

execute_process(COMMAND ${OBJCOPY} -O binary ${ELF} ${BASE}.bin RESULT_VARIABLE RC)
if (NOT RC EQUAL 0)
    message(FATAL_ERROR "${OBJCOPY} -O binary ${ELF} ${BASE}.bin failed")
endif()

if (EXISTS ${ELF}.map)
    file(RENAME ${ELF}.map ${BASE}.map)
endif()

if (OBJDUMP)
    execute_process(COMMAND ${OBJDUMP} -h ${ELF} OUTPUT_FILE ${BASE}.asm RESULT_VARIABLE RC)
    execute_process(COMMAND ${OBJDUMP} -d ${ELF} OUTPUT_VARIABLE DISASSEMBLY RESULT_VARIABLE RC2)
    if (NOT RC EQUAL 0 OR NOT RC2 EQUAL 0)
        message(FATAL_ERROR "${OBJDUMP} failed on ${ELF}")
    endif()
    file(APPEND ${BASE}.asm "${DISASSEMBLY}")
endif()
