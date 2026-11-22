if (NOT TARGET kianv_stdlib_headers)
    add_library(kianv_stdlib_headers INTERFACE)
    target_include_directories(kianv_stdlib_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()

if (NOT TARGET kianv_stdlib)
    kianv_add_impl_library(kianv_stdlib)

    set(KIANV_STDLIB_LIBRARIES
            hardware_uart
            kianv_platform
            kianv_crt0
            kianv_standard_link
            kianv_clib_interface
            kianv_cxx_options
            kianv_atomic
            kianv_time
            kianv_stdio
    )
    foreach(LIB IN LISTS KIANV_STDLIB_LIBRARIES)
        if (TARGET ${LIB})
            kianv_mirrored_target_link_libraries(kianv_stdlib INTERFACE ${LIB})
        endif()
    endforeach()
endif()
