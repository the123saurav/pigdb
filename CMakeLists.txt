cmake_minimum_required(VERSION 3.27)
project(pigdb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Integrate vcpkg toolchain file if not already done
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Assume built-in pthreads on MacOS, to keep cmake happy
IF(APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

# Compiler and linker flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# Define the source files for the executable (only .cpp files)
file(GLOB_RECURSE PIGDB_SRC "src/*.cpp")

# Define the header files (only .h files)
file(GLOB_RECURSE PIGDB_HEADER "src/*.h" "include/*.h")

# Add your source code directories (optional if you have subdirectories)
add_subdirectory(src)

# Disable clang-tidy if the environment variable DISABLE_STATIC_ANALYSIS is set
if(DEFINED ENV{DISABLE_STATIC_ANALYSIS} AND ENV{DISABLE_STATIC_ANALYSIS})
    message(STATUS "Static analysis disabled")
    set(CMAKE_CXX_CLANG_TIDY "")
else()
    message(STATUS "Running with clang-tidy")
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

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

# FetchContent module for external dependencies
include(FetchContent)

# Find and include xxhash using vcpkg
find_package(xxhash CONFIG REQUIRED)

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

# Link libraries to the pigdb target
target_link_libraries(pigdb PRIVATE spdlog::spdlog fmt::fmt xxHash::xxhash ${JEMALLOC_LIBRARIES})

# Include jemalloc directories for the target
target_include_directories(pigdb PRIVATE ${JEMALLOC_INCLUDE_DIRS})

# Include directories for the pigdb target
target_include_directories(pigdb PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

# Make sure we link against jemalloc library directories
link_directories(${JEMALLOC_LIBRARY_DIRS})

# Google Test Integration
find_package(GTest CONFIG REQUIRED)

# Create the test executable
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")
add_executable(pigdb_tests ${TEST_SOURCES})

# Link Google Test libraries to the test executable
target_link_libraries(pigdb_tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main spdlog::spdlog fmt::fmt ${JEMALLOC_LIBRARIES})

# Add test directories to include path
target_include_directories(pigdb_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

# Enable CTest for running tests
include(CTest)
enable_testing()

# Add test to CTest
add_test(NAME pigdb_tests COMMAND pigdb_tests --gtest_output=xml:test_results.xml)