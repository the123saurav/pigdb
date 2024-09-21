cmake_minimum_required(VERSION 3.10)
project(pigdb)

set(CMAKE_CXX_STANDARD 17)

# Set Clang-Tidy for static code analysis during the build
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

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

# Define the pigdb target (executable)
add_executable(pigdb src/main.cpp)  # Adjust this to match your source files

# Now link libraries to the pigdb target
target_link_libraries(pigdb PRIVATE spdlog::spdlog fmt::fmt ${JEMALLOC_LIBRARIES})

# Add your source code directories
add_subdirectory(src)
