cmake_minimum_required(VERSION 3.10)

project(firmware_flash C)

set(CMAKE_C_STANDARD 11)

# INCLUDE
file(GLOB_RECURSE HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

set(INCLUDE_DIRS "")
foreach(_header ${HEADER_FILES})
    get_filename_component(_dir ${_header} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
include_directories(${INCLUDE_DIRS})
# SOURCE
file(GLOB_RECURSE ALL_C_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

set(SRC_FILES "")
foreach(c_file ${ALL_C_FILES})
    if(NOT c_file MATCHES "CMakeCCompilerId\\.c$" AND
       NOT c_file MATCHES "CMakeFiles/" AND
       NOT c_file MATCHES "build/")
        list(APPEND SRC_FILES ${c_file})
    endif()
endforeach()

add_executable(firmware_flash ${SRC_FILES})

if (WIN32)
    target_link_libraries(firmware_flash
        setupapi
        ws2_32
    )
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE ALL_C_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

