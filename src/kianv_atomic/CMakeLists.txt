if (NOT TARGET kianv_atomic)
    kianv_add_library(kianv_atomic)

    target_sources(kianv_atomic INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/atomic.c
            )

    target_include_directories(kianv_atomic_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    kianv_mirrored_target_link_libraries(kianv_atomic INTERFACE kianv_platform)
endif()
