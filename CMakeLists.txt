cmake_minimum_required(VERSION 3.18)
project(8085_simulator_tests LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(8085_core
    Core_engine/compiler/CodeGen.cpp
    Core_engine/compiler/Lexer.cpp
    Core_engine/compiler/Parser.cpp
    Core_engine/compiler/SymbolTable.cpp
    Core_engine/compiler/main.cpp
    Core_engine/cpu/Cpu8085.cpp
    Core_engine/memory/Memory.cpp
)
target_include_directories(8085_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Core_engine")

if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(cpu_tests
        test/test_alu.cpp
        test/test_flags.cpp
        test/test_branching.cpp
        test/test_cpu_coverage.cpp
        test/test_assembler.cpp
        test/test_memory.cpp
    )
    target_link_libraries(cpu_tests PRIVATE 8085_core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(cpu_tests)
endif()
