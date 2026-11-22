if (NOT TARGET kianv_time_headers)
    add_library(kianv_time_headers INTERFACE)
    target_include_directories(kianv_time_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_link_libraries(kianv_time_headers INTERFACE hardware_timer_headers)
endif()

if (NOT TARGET kianv_time)
    kianv_add_impl_library(kianv_time)

    target_sources(kianv_time INTERFACE ${CMAKE_CURRENT_LIST_DIR}/time.c)

    target_link_libraries(kianv_time INTERFACE hardware_timer)
endif()
