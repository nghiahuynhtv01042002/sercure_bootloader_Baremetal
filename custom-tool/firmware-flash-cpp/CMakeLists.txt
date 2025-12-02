cmake_minimum_required(VERSION 3.10)

project(firmware_flash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include folder
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Source files
set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial.cpp
)

# Output folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware-flash-build
)

add_executable(firmware_sender ${SRC_FILES})

# Windows libraries
if (WIN32)
    target_link_libraries(firmware_sender
        setupapi
        ws2_32
    )
endif()
