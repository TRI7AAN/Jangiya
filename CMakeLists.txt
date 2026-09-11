# JOCKY — C++20 compiler/runtime for the .jky forensic DSL (Phase 1 skeleton).
cmake_minimum_required(VERSION 3.20)

project(jocky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(jocky src/cli/main.cpp)
target_include_directories(jocky PRIVATE include)

add_executable(scan_registry tools/scan_registry.cpp src/stdlib/metadata_scanner.cpp)
target_include_directories(scan_registry PRIVATE include)
