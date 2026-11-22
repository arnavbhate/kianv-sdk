if (NOT TARGET kianv_cxx_options)
    kianv_add_library(kianv_cxx_options)

    target_sources(kianv_cxx_options INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/new_delete.cpp
    )

    # KIANV_CMAKE_CONFIG: KIANV_CXX_ENABLE_EXCEPTIONS, Enable CXX exception handling, type=bool, default=0, group=kianv_cxx_options
    # KIANV_BUILD_DEFINE: KIANV_CXX_ENABLE_EXCEPTIONS, value of CMake var KIANV_CXX_ENABLE_EXCEPTIONS, type=string, default=0, group=kianv_cxx_options
    if (NOT KIANV_CXX_ENABLE_EXCEPTIONS)
        target_compile_definitions( kianv_cxx_options INTERFACE KIANV_CXX_ENABLE_EXCEPTIONS=0)
        target_compile_options( kianv_cxx_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
        target_compile_options( kianv_cxx_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables>)
    else()
        target_compile_definitions( kianv_cxx_options INTERFACE KIANV_CXX_ENABLE_EXCEPTIONS=1)
    endif()

    # KIANV_CMAKE_CONFIG: KIANV_CXX_ENABLE_RTTI, Enable CXX rtti, type=bool, default=0, group=kianv_cxx_options
    if (NOT KIANV_CXX_ENABLE_RTTI)
        target_compile_options( kianv_cxx_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
    endif()

    # KIANV_CMAKE_CONFIG: KIANV_CXX_ENABLE_CXA_ATEXIT, Enable cxa-atexit, type=bool, default=0, group=kianv_cxx_options
    if (NOT KIANV_CXX_ENABLE_CXA_ATEXIT)
        target_compile_options( kianv_cxx_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>)
    endif()
endif()
