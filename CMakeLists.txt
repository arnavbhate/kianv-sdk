cmake_minimum_required(VERSION 3.13...3.27)

# Note: this CMakeLists.txt can be used as a top-level CMakeLists.txt for the SDK itself. For all other uses
# it is included as a subdirectory via the pico_sdk_init() method provided by pico_sdk_init.cmake
if (NOT TARGET _kianv_sdk_inclusion_marker)
    add_library(_kianv_sdk_inclusion_marker INTERFACE)
    # This is a no-op unless we are the top-level CMakeLists.txt
    include(kianv_sdk_init.cmake)

    kianv_is_top_level_project(KIANV_SDK_TOP_LEVEL_PROJECT)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 11)

    if (NOT KIANV_SDK_TOP_LEVEL_PROJECT)
        set(KIANV_SDK 1 PARENT_SCOPE)
    endif()

    add_subdirectory(src)
endif()
