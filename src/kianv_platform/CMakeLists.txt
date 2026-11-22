if (NOT TARGET kianv_platform)
    kianv_add_library(kianv_platform)

    target_sources(kianv_platform INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/cxa_guard.c
            ${CMAKE_CURRENT_LIST_DIR}/misaligned.c
            ${CMAKE_CURRENT_LIST_DIR}/panic.c
            ${CMAKE_CURRENT_LIST_DIR}/runtime_init.c
    )

    target_include_directories(kianv_platform_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    # The toolchain's essential compile and link flags (-march, -mabi, ...), also applied here so that they are
    # kept when CMAKE_C_FLAGS etc. are set on the command line
    separate_arguments(_kianv_compile_flags UNIX_COMMAND "${KIANV_COMPILE_FLAGS}")
    separate_arguments(_kianv_link_flags UNIX_COMMAND "${KIANV_LINK_FLAGS}")
    target_compile_options(kianv_platform_headers INTERFACE ${_kianv_compile_flags})
    target_link_options(kianv_platform_headers INTERFACE ${_kianv_link_flags})

    # KIANV_CMAKE_CONFIG: KIANV_NUM_UARTS, number of UARTs the SoC was built with (its NUM_UARTS parameter), 1-5
    set(KIANV_NUM_UARTS 2 CACHE STRING "Number of UARTs in the SoC (1-5)")
    if (NOT KIANV_NUM_UARTS MATCHES "^[1-5]$")
        message(FATAL_ERROR "KIANV_NUM_UARTS must be between 1 and 5 (the SoC has addresses for UART0-UART4)")
    endif()
    target_compile_definitions(kianv_platform_headers INTERFACE NUM_UARTS=${KIANV_NUM_UARTS})

    # KIANV_CMAKE_CONFIG: KIANV_NUM_GPIOS, number of GPIO pins the SoC was built with (gpio_if GPIO_NR), 0-32
    set(KIANV_NUM_GPIOS 10 CACHE STRING "Number of GPIO pins in the SoC (0-32, 0 = no GPIO block)")
    if (NOT KIANV_NUM_GPIOS MATCHES "^([0-9]|[12][0-9]|3[0-2])$")
        message(FATAL_ERROR "KIANV_NUM_GPIOS must be between 0 and 32")
    endif()
    target_compile_definitions(kianv_platform_headers INTERFACE NUM_GPIOS=${KIANV_NUM_GPIOS})
endif()
