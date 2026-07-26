cmake_minimum_required(VERSION 3.15)

project(IndusDragon LANGUAGES CXX)

# Add this right after the project() line:
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "No build type specified, defaulting to Release")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_NAME indus-dragon)
option(INDUS_ENABLE_AVX2 "Build with AVX2 NNUE intrinsics" OFF)

# Add source files
set(SOURCES
    src/main.cpp
    src/engine.cpp
    src/search.cpp
    src/time_manager.cpp
    src/tt.cpp
    src/nnue.cpp
)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Include directories
target_include_directories(${EXECUTABLE_NAME} PUBLIC src)



# Set compiler flags for release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Architecture-specific optimizations
include(CheckCXXCompilerFlag)

if(INDUS_ENABLE_AVX2 AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    check_cxx_compiler_flag("-mavx2" HAVE_AVX2)

    if(HAVE_AVX2)
        message(STATUS "AVX2 support detected, enabling AVX2 only for NNUE.")
        set_source_files_properties(src/nnue.cpp PROPERTIES
            COMPILE_OPTIONS "-mavx2"
            COMPILE_DEFINITIONS USE_AVX2
        )
    endif()
elseif(INDUS_ENABLE_AVX2 AND MSVC)
    message(STATUS "MSVC detected, enabling AVX2 only for NNUE.")
    set_source_files_properties(src/nnue.cpp PROPERTIES
        COMPILE_OPTIONS "/arch:AVX2"
        COMPILE_DEFINITIONS USE_AVX2
    )
endif()

# 64-bit check
# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#     target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IS_64BIT)
# endif()

install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
)
