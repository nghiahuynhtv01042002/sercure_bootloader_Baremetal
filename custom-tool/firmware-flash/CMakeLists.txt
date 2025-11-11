cmake_minimum_required(VERSION 3.10)

project(firmware_flash C)

set(CMAKE_C_STANDARD 11)

# INCLUDE
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# SOURCE FILES 
set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/firmware_sender.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial_com.c
)

# OUTPUT FOLDER
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/firmware-flash-build)
add_executable(firmware_sender ${SRC_FILES})

# LINK WINDOWS LIBS
if (WIN32)
    target_link_libraries(firmware_sender
        setupapi
        ws2_32
    )
endif()
