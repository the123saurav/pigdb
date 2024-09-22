cmake_minimum_required(VERSION 3.27)
project(pigdb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the source files for the executable (only .cpp files)
file(GLOB_RECURSE PIGDB_SRC "src/*.cpp")

# Define the header files (only .h files)
file(GLOB_RECURSE PIGDB_HEADER "src/*.h" "include/*.h")

# Disable clang-tidy if the environment variable DISABLE_STATIC_ANALYSIS is set
if(DEFINED ENV{DISABLE_STATIC_ANALYSIS} AND ENV{DISABLE_STATIC_ANALYSIS})
    message(STATUS "Static analysis disabled")
    set(CMAKE_CXX_CLANG_TIDY "")
else()
    message(STATUS "Running with clang-tidy")
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")  # Ensure clang-tidy is set

    # Find the clang-format executable
    find_program(CLANG_FORMAT "clang-format")

    if(NOT CLANG_FORMAT)
        message(FATAL_ERROR "clang-format not found. Please install it or ensure it's in your PATH.")
    endif()

    # Create the clang-format target, using both source and header files
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i ${PIGDB_SRC} ${PIGDB_HEADER}
        COMMENT "Running clang-format on source and header files"
    )
endif()

# Custom target to run Clang Static Analyzer using scan-build
add_custom_target(analyze
    COMMAND scan-build cmake --build .
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

# Define the pigdb target
add_executable(pigdb ${PIGDB_SRC})

# Now link libraries to the pigdb target
target_link_libraries(pigdb PRIVATE spdlog::spdlog fmt::fmt ${JEMALLOC_LIBRARIES})

# Add your source code directories (optional if you have subdirectories)
add_subdirectory(src)
