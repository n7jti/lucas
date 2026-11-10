cmake_minimum_required(VERSION 3.20)

project(lucas VERSION 0.1.0 LANGUAGES CXX)

option(LUCAS_REGENERATE_PARSER "Regenerate parser sources with Bison and Flex" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LUCAS_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/generated")
set(LUCAS_PARSER_SOURCE "${LUCAS_GENERATED_DIR}/parser.cpp")
set(LUCAS_LEXER_SOURCE "${LUCAS_GENERATED_DIR}/lexer.cpp")

if(LUCAS_REGENERATE_PARSER)
    find_package(BISON 3.8 REQUIRED)
    find_package(FLEX 2.6 REQUIRED)

    BISON_TARGET(LucasParser
        "${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/parser.y"
        "${LUCAS_PARSER_SOURCE}"
        DEFINES_FILE "${LUCAS_GENERATED_DIR}/parser.hpp")
    FLEX_TARGET(LucasLexer
        "${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/lexer.l"
        "${LUCAS_LEXER_SOURCE}")
    ADD_FLEX_BISON_DEPENDENCY(LucasLexer LucasParser)
endif()

foreach(generated_file IN ITEMS "${LUCAS_PARSER_SOURCE}" "${LUCAS_LEXER_SOURCE}" "${LUCAS_GENERATED_DIR}/parser.hpp")
    if(NOT EXISTS "${generated_file}")
        message(FATAL_ERROR
            "Missing generated parser file: ${generated_file}. "
            "Install Bison/Flex and configure with -DLUCAS_REGENERATE_PARSER=ON.")
    endif()
endforeach()

function(lucas_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(lucas_frontend
    src/frontend/ast.cpp
    src/frontend/ast_renderer.cpp
    src/frontend/parser.cpp
    "${LUCAS_PARSER_SOURCE}"
    "${LUCAS_LEXER_SOURCE}")
if(MSVC)
    set_source_files_properties("${LUCAS_LEXER_SOURCE}" PROPERTIES COMPILE_OPTIONS "/wd4005;/wd4996")
    set_source_files_properties("${LUCAS_PARSER_SOURCE}" PROPERTIES COMPILE_OPTIONS "/wd4127;/wd4702")
endif()
target_include_directories(lucas_frontend
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${LUCAS_GENERATED_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/frontend")
lucas_enable_warnings(lucas_frontend)

add_library(lucas_backend src/backend/evaluator.cpp)
target_include_directories(lucas_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(lucas_backend PUBLIC lucas_frontend)
lucas_enable_warnings(lucas_backend)

add_library(lucas_engine src/engine/engine.cpp)
target_include_directories(lucas_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(lucas_engine PUBLIC lucas_frontend lucas_backend)
lucas_enable_warnings(lucas_engine)

add_executable(lucas src/cli/main.cpp)
target_link_libraries(lucas PRIVATE lucas_engine)
lucas_enable_warnings(lucas)

include(CTest)
if(BUILD_TESTING)
    add_executable(lucas_tests tests/engine_tests.cpp)
    target_link_libraries(lucas_tests PRIVATE lucas_engine)
    lucas_enable_warnings(lucas_tests)
    add_test(NAME lucas_tests COMMAND lucas_tests)
    add_test(NAME cli_expression COMMAND lucas "2 + 3 * 4")
    set_tests_properties(cli_expression PROPERTIES PASS_REGULAR_EXPRESSION "14")
    add_test(NAME cli_ans COMMAND lucas "ans + 1")
    set_tests_properties(cli_ans PROPERTIES PASS_REGULAR_EXPRESSION "1")
    add_test(NAME cli_verbose COMMAND lucas --verbose "2 + 3")
    set_tests_properties(cli_verbose PROPERTIES PASS_REGULAR_EXPRESSION "Binary\\(\\+\\)")
    add_test(NAME cli_precision COMMAND lucas "1 / 3")
    set_tests_properties(cli_precision PROPERTIES PASS_REGULAR_EXPRESSION "0\\.333333333333333")
    add_test(NAME cli_trailing_zeros COMMAND lucas "10.0 / 4.0")
    set_tests_properties(cli_trailing_zeros PROPERTIES PASS_REGULAR_EXPRESSION "2\\.5")
endif()
