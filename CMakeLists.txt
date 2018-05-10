cmake_minimum_required(VERSION 2.8)
project(guscc C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c89")
include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(guscc
    src/lex.c
    src/main.c
    )

enable_testing()
add_executable(guscc_test
    src/lex.c
    test/lex_test.c
    test/main.c
    )
add_test(test guscc_test)
