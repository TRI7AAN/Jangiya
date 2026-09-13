# JOCKY — C++20 compiler/runtime for the .jky forensic DSL (Phase 7).
cmake_minimum_required(VERSION 3.20)

project(jocky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(jocky src/cli/main.cpp src/stdlib/metadata_scanner.cpp)
target_include_directories(jocky PRIVATE include)

add_executable(scan_registry tools/scan_registry.cpp src/stdlib/metadata_scanner.cpp)
target_include_directories(scan_registry PRIVATE include)

add_executable(jockyc src/compiler/main.cpp src/stdlib/metadata_scanner.cpp)
target_include_directories(jockyc PRIVATE include)
target_compile_definitions(
    jockyc PRIVATE JOCKY_INCLUDE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/include")

enable_testing()
add_executable(phase6_embedder_test tests/phase6/embedder_test.cpp)
target_include_directories(phase6_embedder_test PRIVATE include)
add_test(NAME phase6_embedder_stale_hash COMMAND phase6_embedder_test)

find_program(BASH_PROGRAM bash REQUIRED)
add_test(
    NAME phase6_jockyc_integration
    COMMAND ${BASH_PROGRAM}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/phase6/run_phase6.sh
            $<TARGET_FILE:jockyc>
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/phase6-integration)

add_executable(phase7_runtime_test tests/phase7/runtime_test.cpp)
target_include_directories(phase7_runtime_test PRIVATE include)
add_test(NAME phase7_runtime_safety COMMAND phase7_runtime_test)

add_test(
    NAME phase7_embedded_runtime
    COMMAND ${BASH_PROGRAM}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/phase7/run_phase7.sh
            $<TARGET_FILE:jockyc>
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/phase7-integration)
