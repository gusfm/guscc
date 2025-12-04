cmake_minimum_required(VERSION 2.8)
project(guscc C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")
include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(guscc
    src/token.c
    src/lex.c
    src/ast.c
    src/parser.c
    src/guscc.c
)
add_test(test guscc)
