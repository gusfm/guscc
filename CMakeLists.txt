cmake_minimum_required(VERSION 2.8)
project(guscc C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(guscc_obj OBJECT
    src/token.c
    src/lex.c
    src/ast.c
    src/parser.c
    src/codegen.c
)

add_executable(guscc
    $<TARGET_OBJECTS:guscc_obj>
    src/guscc.c
    )

add_executable(guscc_test
    $<TARGET_OBJECTS:guscc_obj>
    test/lex_test.c
    test/guscc_test.c
    test/main.c
    )

add_test(test guscc_test)
