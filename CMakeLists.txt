cmake_minimum_required(VERSION 2.8)
project(guscc C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(guscc_obj OBJECT
    src/token.c
    src/lex.c
    src/ast.c
    src/sym.c
    src/parser.c
    src/codegen.c
    src/guscc.c
)

add_executable(guscc
    $<TARGET_OBJECTS:guscc_obj>
    src/main.c
    )

add_executable(guscc_test
    $<TARGET_OBJECTS:guscc_obj>
    test/lex_test.c
    test/guscc_test.c
    test/main.c
    )

add_test(test guscc_test)

find_program(VALGRIND valgrind)
if(VALGRIND)
    add_custom_target(valgrind
        COMMAND ${VALGRIND}
            --error-exitcode=1
            --leak-check=full
            ./guscc_test
        DEPENDS guscc_test guscc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
