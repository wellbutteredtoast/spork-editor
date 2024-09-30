cmake_minimum_required(VERSION 3.10)
project(Spork C)

# Set the output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Recursively gather all C source files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

# Include directories
include_directories("${CMAKE_SOURCE_DIR}/include/")

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Add the executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Link against SDL libraries
target_link_libraries(${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/libsdl/SDL2main.lib"
    "${CMAKE_SOURCE_DIR}/libsdl/SDL2.lib"
    "${CMAKE_SOURCE_DIR}/libsdl/SDL2_ttf.lib"
    "${CMAKE_SOURCE_DIR}/libsdl/SDL2_image.lib"
)

# Copy DLLs to the output directory
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/libsdl/dynamic/*.dll")
foreach(DLL ${DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endforeach()