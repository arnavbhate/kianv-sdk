if (NOT TARGET kianv_crt0)
    kianv_add_library(kianv_crt0)

    target_sources(kianv_crt0 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/crt0.S)

    # crt0 calls runtime_init() and the trap handlers in kianv_platform
    kianv_mirrored_target_link_libraries(kianv_crt0 INTERFACE kianv_platform)
endif()
