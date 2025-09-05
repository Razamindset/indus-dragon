cmake_minimum_required(VERSION 3.15)

project(IndusDragon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_NAME indus-dragon)

# Add source files
set(SOURCES
    src/main.cpp
    src/engine.cpp
    src/search.cpp
    src/time_manager.cpp
    src/tt.cpp
    src/nnue/misc.cpp
    src/nnue/nnue.cpp
)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Include directories
target_include_directories(${EXECUTABLE_NAME} PUBLIC src)

# Find and link threads
find_package(Threads REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Threads::Threads)

# Set compiler flags for release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Architecture-specific optimizations
include(CheckCXXCompilerFlag)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    check_cxx_compiler_flag("-mavx2" HAVE_AVX2)
    check_cxx_compiler_flag("-msse4.1" HAVE_SSE41)
    check_cxx_compiler_flag("-mssse3" HAVE_SSSE3)
    check_cxx_compiler_flag("-msse2" HAVE_SSE2)

    if(HAVE_AVX2)
        message(STATUS "AVX2 support detected, enabling AVX2 optimizations.")
        target_compile_options(${EXECUTABLE_NAME} PRIVATE "-mavx2")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_AVX2)
    elseif(HAVE_SSE41)
        message(STATUS "SSE4.1 support detected, enabling SSE4.1 optimizations.")
        target_compile_options(${EXECUTABLE_NAME} PRIVATE "-msse4.1")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_SSE41)
    elseif(HAVE_SSSE3)
        message(STATUS "SSSE3 support detected, enabling SSSE3 optimizations.")
        target_compile_options(${EXECUTABLE_NAME} PRIVATE "-mssse3")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_SSSE3)
    elseif(HAVE_SSE2)
        message(STATUS "SSE2 support detected, enabling SSE2 optimizations.")
        target_compile_options(${EXECUTABLE_NAME} PRIVATE "-msse2")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_SSE2)
    endif()
elseif(MSVC)
    # The source code enables USE_AVX2 and others via _MSC_VER
    # We can use /arch:AVX2 to ensure the compiler can generate AVX2 instructions
    message(STATUS "MSVC detected, enabling AVX2 optimizations.")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /arch:AVX2)
endif()

# 64-bit check
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IS_64BIT)
endif()

install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
)
