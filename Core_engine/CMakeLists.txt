cmake_minimum_required(VERSION 3.18)
project(8085_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORE_SOURCES
    compiler/Lexer.cpp
    compiler/Parser.cpp
    compiler/SymbolTable.cpp
    compiler/CodeGen.cpp
    compiler/main.cpp
    memory/Memory.cpp
    io/IoBuffer.cpp
    cpu/Cpu8085.cpp
)

if(CMAKE_JS_INC)
    find_path(NODE_ADDON_API_INCLUDE_DIR napi.h
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-addon-api"
            "${CMAKE_JS_INC}/../node-addon-api"
        NO_DEFAULT_PATH
        REQUIRED
    )

    add_library(${PROJECT_NAME} SHARED
        index.cpp
        ${CORE_SOURCES}
        ${CMAKE_JS_SRC}
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_JS_INC}
        ${NODE_ADDON_API_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE NAPI_CPP_EXCEPTIONS)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".node"
    )
else()
    message(STATUS "CMAKE_JS_INC not set; building the standalone 8085 core library")
    add_library(${PROJECT_NAME}_core STATIC ${CORE_SOURCES})
    target_include_directories(${PROJECT_NAME}_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
