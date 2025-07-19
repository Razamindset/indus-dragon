# Claude generated shitty cmake file. I will learn this oneday. 

# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(indus-dragon CXX)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define source files
set(SOURCES
    src/main.cpp
    src/uci.cpp
    src/engine.cpp
    src/evaluation.cpp
    src/search.cpp
    src/tt.cpp
    src/time_manager.cpp
)

# Define header files
set(HEADERS
    src/uci.hpp
    src/engine.hpp
    src/evaluation.hpp
    src/search.hpp
    src/tt.hpp
    src/heuristics.hpp
    src/time_manager.hpp
    src/chess.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

# Optional: Set compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()