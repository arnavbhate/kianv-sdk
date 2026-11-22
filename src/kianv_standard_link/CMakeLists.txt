if (NOT TARGET kianv_standard_link)
    kianv_add_library(kianv_standard_link)

    target_link_options(kianv_standard_link INTERFACE "-T${CMAKE_CURRENT_LIST_DIR}/memmap_default.ld")
    target_link_options(kianv_standard_link INTERFACE -nostartfiles)
endif()
