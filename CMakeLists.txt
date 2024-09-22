cmake_minimum_required(VERSION 3.27)
project(pigdb)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable clang-tidy if the environment variable DISABLE_STATIC_ANALYSIS is set
if(DEFINED ENV{DISABLE_STATIC_ANALYSIS} AND ENV{DISABLE_STATIC_ANALYSIS})
    message(STATUS "Static analysis disabled")
    set(CMAKE_CXX_CLANG_TIDY "")
else()
    message(STATUS "Running with clang-tidy")
endif()

# Custom target to run Clang Static Analyzer using scan-build
add_custom_target(static_analyzer
    COMMAND scan-build make
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Clang Static Analyzer..."
)

include(FetchContent)

# Find and link spdlog
find_package(spdlog CONFIG REQUIRED)

# Find and link fmt
find_package(fmt CONFIG REQUIRED)

# Find jemalloc using pkg-config as it doesn't build with cmake target
find_package(PkgConfig REQUIRED)
pkg_check_modules(JEMALLOC REQUIRED jemalloc)

# Include jemalloc directories and link libraries
include_directories(${JEMALLOC_INCLUDE_DIRS})
link_directories(${JEMALLOC_LIBRARY_DIRS})

# Add your source directory to the include path
include_directories(${CMAKE_SOURCE_DIR}/src)

# Define the pigdb target (executable) and add all the source files from src/
file(GLOB_RECURSE PIGDB_SRC "src/*.cpp")
add_executable(pigdb ${PIGDB_SRC})

# Now link libraries to the pigdb target
target_link_libraries(pigdb PRIVATE spdlog::spdlog fmt::fmt ${JEMALLOC_LIBRARIES})

# Add your source code directories (optional if you have subdirectories)
add_subdirectory(src)
