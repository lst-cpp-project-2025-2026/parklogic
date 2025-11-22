cmake_minimum_required(VERSION 3.20)

project(parklogic VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)
set(RAYLIB_VERSION 5.5)
message(STATUS "Fetching Raylib ${RAYLIB_VERSION}...")

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# This tells CMake that the include directories for the 'raylib' target
# should be treated as SYSTEM headers (suppressing warnings) when used by other targets.
target_include_directories(raylib SYSTEM INTERFACE ${raylib_SOURCE_DIR}/src)

# --- Sources ---
file(GLOB_RECURSE SOURCES "src/*.cpp")

# --- Target ---
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# --- Compiler Flags ---
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
