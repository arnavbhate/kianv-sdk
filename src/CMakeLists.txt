# Based on pico-sdk

cmake_policy(SET CMP0079 NEW) # allow inserting of dependencies into our INTERFACE libraries

# takes dependencies after the target
function(kianv_mirrored_target_link_libraries TARGET SCOPE)
    if (${ARGC} LESS 3)
        message(FATAL_ERROR "expected a target, scope and at least one dependency")
    endif()
    if (NOT TARGET ${TARGET}_headers)
        message(FATAL_ERROR "${TARGET} does not have headers")
    endif()
    # library should depend on its own header
    target_link_libraries(${TARGET} ${SCOPE} ${TARGET}_headers)
    foreach(DEPENDENCY IN LISTS ARGN)
        if (DEPENDENCY MATCHES ".*_headers")
            message(FATAL_ERROR "should not use headers with kianv_mirrored_target_link_libraries")
        endif()
        # note, it would be nice to only add the dependency if it exists, but we do
        # not necessarily add libraries in reverse dependency order, so we do this
        # unconditionally, so this function should only be passed dependencies that
        # have headers, or link will fail with a missing library -lfoo_headers
        target_link_libraries(${TARGET}_headers ${SCOPE} ${DEPENDENCY}_headers)
        target_link_libraries(${TARGET} ${SCOPE} ${DEPENDENCY})
    endforeach()
endfunction()

# create a hardware_NAME_headers target (see kianv_simple_hardware_headers_target)
# create a hardware_NAME target (see kianv_simple_hardware_target)
macro(kianv_simple_hardware_target NAME)
    kianv_simple_hardware_headers_target(${NAME})
    kianv_simple_hardware_impl_target(${NAME})
endmacro()

# create an INTERFACE library named target, and define LIB_TARGET=1 (upper case) as a compile option
# and make it dependent on a pre-existing corresponding _headers library
# optional arg NOFLAG will skip the LIB_TARGET definition
function(kianv_add_impl_library target)
    add_library(${target} INTERFACE)
    string(TOUPPER ${target} TARGET_UPPER)
    if (${ARGC} GREATER 1)
        if (NOT "${ARGV1}" STREQUAL "NOFLAG")
            message(FATAL_ERROR "Unknown parameter ${ARGV1}")
        endif()
    else()
        target_compile_definitions(${target} INTERFACE LIB_${TARGET_UPPER}=1)
    endif()
    target_link_libraries(${target} INTERFACE ${target}_headers)
endfunction()

# create an INTERFACE library named target along with associated header, and define LIB_TARGET=1 (upper case) as a compile option
# optional arg NOFLAG will skip the LIB_TARGET definition
function(kianv_add_library target)
    add_library(${target}_headers INTERFACE)
    kianv_add_impl_library(${target} ${ARGN})
endfunction()

# create an INTERFACE library named hardware_NAME_headers INTERFACE library if it doesn't already exist,
#        and add include/ relative to the calling directory to the includes.
#        and kianv_platform as dependencies of the library
macro(kianv_simple_hardware_headers_target NAME)
    if (NOT TARGET hardware_${NAME}_headers)
        add_library(hardware_${NAME}_headers INTERFACE)

        target_include_directories(hardware_${NAME}_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
        target_link_libraries(hardware_${NAME}_headers INTERFACE kianv_platform)
    endif()
endmacro()

# create an INTERFACE library named hardware_NAME if it doesn't exist, along with a hardware_NAME_headers
#        INTERFACE library that it depends on. The hardware_NAME_headers library add include/ relative to
#        and kianv_platform
macro(kianv_simple_hardware_headers_only_target NAME)
    if (NOT TARGET hardware_${NAME})
        # Choosing not to add LIB_HARDWARE_ defines to avoid command line bloat pending a need (they aren't
        #   super interesting except to determine functionality as they are mostly passive accessors, however
        #   they could be useful to determine if the header is available.
        # kianv_add_impl_library(hardware_${NAME})
        add_library(hardware_${NAME}_headers INTERFACE)

        # a headers only target should still have an explicit _headers library for consistency
        target_include_directories(hardware_${NAME}_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
        target_link_libraries(hardware_${NAME}_headers INTERFACE kianv_platform)

        add_library(hardware_${NAME} INTERFACE)
        target_link_libraries(hardware_${NAME} INTERFACE hardware_${NAME}_headers)
    endif()
endmacro()

# create an INTERFACE library named hardware_NAME if it doesn't exist, dependent on a pre-existing  hardware_NAME_headers
#        INTERFACE library and kianv_platform. The file NAME.c relative to the caller is added to the C sources for the hardware_NAME
macro(kianv_simple_hardware_impl_target NAME)
    if (NOT TARGET hardware_${NAME})
        # Choosing not to add LIB_HARDWARE_ defines to avoid command line bloat pending a need (they aren't
        #   super interesting except to determine functionality as they are mostly passive accessors, however
        #   they could be useful to determine if the header is available.
        # kianv_add_impl_library(hardware_${NAME})
        add_library(hardware_${NAME} INTERFACE)

        target_sources(hardware_${NAME} INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/${NAME}.c
                )

        kianv_mirrored_target_link_libraries(hardware_${NAME} INTERFACE kianv_platform)
    endif()
endmacro()

add_subdirectory(hardware_clock)
add_subdirectory(hardware_gpio)
add_subdirectory(hardware_spi)
add_subdirectory(hardware_timer)
add_subdirectory(hardware_uart)
add_subdirectory(kianv_atomic)
add_subdirectory(kianv_crt0)
add_subdirectory(kianv_cxx_options)
add_subdirectory(kianv_platform)
add_subdirectory(kianv_standard_link)
add_subdirectory(kianv_stdio)
add_subdirectory(kianv_time)
add_subdirectory(kianv_clib_interface)
add_subdirectory(kianv_stdlib)
