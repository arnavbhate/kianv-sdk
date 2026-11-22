if (NOT KIANV_SDK_INIT_DONE)
    # The toolchain settings (compiler, -march/-mabi, flags) only take effect if they are set before the first
    # project() call enables the languages; otherwise CMake detects the host compiler and ignores the *_INIT flags.
    if (CMAKE_C_COMPILER_LOADED OR CMAKE_CXX_COMPILER_LOADED OR CMAKE_ASM_COMPILER_LOADED)
        message(FATAL_ERROR "include kianv_sdk_init.cmake before project()")
    endif()
    set(KIANV_SDK_INIT_DONE 1)
    set(KIANV_SDK_PATH ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "Path to the KianV SDK" FORCE)

    include(${CMAKE_CURRENT_LIST_DIR}/cmake/kianv_toolchain.cmake)

    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()

    macro(kianv_sdk_init)
        # the SDK libraries need the compiler, which project() sets up
        if (NOT CMAKE_C_COMPILER_LOADED)
            message(FATAL_ERROR "call kianv_sdk_init() after project(test C ASM) (include kianv_sdk_init.cmake before it)")
        endif()
        add_subdirectory(${KIANV_SDK_PATH} kianv-sdk)
    endmacro()

    function(kianv_is_top_level_project VAR)
        string(TOLOWER ${CMAKE_CURRENT_LIST_DIR} __list_dir)
        string(TOLOWER ${CMAKE_SOURCE_DIR} __source_dir)
        if (__source_dir STREQUAL __list_dir)
            set(${VAR} 1 PARENT_SCOPE)
        else()
            set(${VAR} 0 PARENT_SCOPE)
        endif()
    endfunction()
endif()
