# Based on pico-sdk

if (NOT TARGET kianv_clib_interface)
    kianv_add_library(kianv_clib_interface)

    # ---- newlib ----
    kianv_add_library(kianv_newlib_interface)

    target_sources(kianv_newlib_interface INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/newlib_interface.c
    )
    kianv_mirrored_target_link_libraries(kianv_newlib_interface INTERFACE kianv_time kianv_atomic kianv_cxx_options)

    # ---- picolibc ----

    kianv_add_library(kianv_picolibc_interface)

    target_sources(kianv_picolibc_interface INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/picolibc_interface.c
    )

    kianv_mirrored_target_link_libraries(kianv_picolibc_interface INTERFACE kianv_time kianv_atomic kianv_cxx_options)
    # replacing the functions is fine it seems
    #target_compile_definitions(kianv_picolibc_interface INTERFACE
    #            KIANV_STDIO_SHORT_CIRCUIT_CLIB_FUNCS=0
    #)

    if (NOT KIANV_CLIB)
        # KIANV_CMAKE_CONFIG: KIANV_CLIB, The C library to use e.g. newlib/picolibc, type=string, default=based on KIANV_COMPILER, group=build, docref=cmake-toolchain-config
        set(CLIB_DETECT_CODE "
            #include <stdio.h>
            #ifdef __PICOLIBC__
            #define USING_PICOLIBC 1
            #elif defined(__NEWLIB__)
            #define USING_NEWLIB 1
            #endif
            
            #if (picolibc && !USING_PICOLIBC) || (newlib && !USING_NEWLIB)
            #error not using wanted c library
            #endif
        ")

        # Create a temporary file for the source
        set(CLIB_DETECT_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/clib_check.c")
        file(WRITE "${CLIB_DETECT_SOURCE}" "${CLIB_DETECT_CODE}")

        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
        foreach(CLIB newlib picolibc)
            # Attempt to compile the source file
            try_compile(CLIB_MATCHED
                    "${CMAKE_CURRENT_BINARY_DIR}"              # Directory for temporary build files
                    "${CLIB_DETECT_SOURCE}"                    # Source file to compile
                    COMPILE_DEFINITIONS "-D${CLIB}=1"
            )
            if (CLIB_MATCHED)
                set(KIANV_CLIB ${CLIB})
                break()
            endif()
        endforeach ()
        if (NOT KIANV_CLIB)
            message("C library not detected; defaulting to newlib")
            set(KIANV_CLIB newlib)
        endif()
        file(REMOVE "${CLIB_DETECT_SOURCE}")
        set(KIANV_CLIB "${KIANV_CLIB}" CACHE INTERNAL "")
    endif()
    message("C library type is ${KIANV_CLIB}")
    target_link_libraries(kianv_clib_interface INTERFACE kianv_${KIANV_CLIB}_interface)

    # Linking this library is what links the C library, and in projects with C++ the C++ standard library; programs
    # without it are built without them
    get_property(_kianv_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if (CXX IN_LIST _kianv_languages)
        target_link_libraries(kianv_clib_interface INTERFACE "-Wl,--start-group" stdc++ c m gcc "-Wl,--end-group")
    else()
        target_link_libraries(kianv_clib_interface INTERFACE "-Wl,--start-group" c m gcc "-Wl,--end-group")
    endif()
endif()
