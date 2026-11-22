if (NOT TARGET kianv_stdio_headers)
    add_library(kianv_stdio_headers INTERFACE)
    target_include_directories(kianv_stdio_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_link_libraries(kianv_stdio_headers INTERFACE hardware_uart_headers kianv_time_headers)
endif()

if (NOT TARGET kianv_stdio)
    kianv_add_impl_library(kianv_stdio)

    target_sources(kianv_stdio INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stdio.c)

    target_link_libraries(kianv_stdio INTERFACE hardware_uart kianv_time)
endif()
