#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "ut.h"

/* Run guscc on a source file. Prints the command. Returns guscc exit status. */
static int run_test(char *file)
{
    char str[256];
    snprintf(str, sizeof(str), "./guscc %s > /dev/null 2>&1", file);
    printf("%s\n", str);
    return system(str);
}

/* Run guscc, assemble the output, execute and assert exit code == expected. */
static int compile_and_run(const char *src, const char *asm_out,
                           const char *bin_out, int expected_exit)
{
    char cmd[512];
    int ret;

    snprintf(cmd, sizeof(cmd), "./guscc %s > /dev/null 2>&1", src);
    ret = system(cmd);
    if (ret != 0)
        return ret;

    snprintf(cmd, sizeof(cmd), "gcc %s -o %s > /dev/null 2>&1", asm_out,
             bin_out);
    ret = system(cmd);
    ASSERT(ret == 0);

    ret = system(bin_out);
    ASSERT(WEXITSTATUS(ret) == expected_exit);
    return 0;
}

/* ---- Original tests ---- */

static int guscc_test_1(void)
{
    return compile_and_run("../test/files/test_1.c", "./test_1.s",
                           "/tmp/guscc_test_1_out", 42);
}

static int guscc_test_2(void)
{
    return compile_and_run("../test/files/test_2.c", "./test_2.s",
                           "/tmp/guscc_test_2_out", 42);
}

/* test_3 uses unsupported features (printf, char**) — just verify no crash */
static int guscc_test_3(void)
{
    run_test("../test/files/test_3.c");
    return 0;
}

static int guscc_test_4(void)
{
    return compile_and_run("../test/files/test_4.c", "./test_4.s",
                           "/tmp/guscc_test_4_out", 42);
}

static int guscc_test_5(void)
{
    return compile_and_run("../test/files/test_5.c", "./test_5.s",
                           "/tmp/guscc_test_5_out", 42);
}

static int guscc_test_6(void)
{
    return compile_and_run("../test/files/test_6.c", "./test_6.s",
                           "/tmp/guscc_test_6_out", 42);
}

static int guscc_test_7(void)
{
    return compile_and_run("../test/files/test_7.c", "./test_7.s",
                           "/tmp/guscc_test_7_out", 42);
}

/* ---- New success-path tests ---- */

/* test_8: local variable declaration and assignment */
static int guscc_test_8(void)
{
    return compile_and_run("../test/files/test_8.c", "./test_8.s",
                           "/tmp/guscc_test_8_out", 42);
}

/* test_9: local variable with initializer */
static int guscc_test_9(void)
{
    return compile_and_run("../test/files/test_9.c", "./test_9.s",
                           "/tmp/guscc_test_9_out", 42);
}

/* test_10: multiple local variables */
static int guscc_test_10(void)
{
    return compile_and_run("../test/files/test_10.c", "./test_10.s",
                           "/tmp/guscc_test_10_out", 42);
}

/* test_11: function call with parameters */
static int guscc_test_11(void)
{
    return compile_and_run("../test/files/test_11.c", "./test_11.s",
                           "/tmp/guscc_test_11_out", 42);
}

/* test_12: compound assignment operators (+= -=) */
static int guscc_test_12(void)
{
    return compile_and_run("../test/files/test_12.c", "./test_12.s",
                           "/tmp/guscc_test_12_out", 42);
}

/* test_13: postfix ++ and -- */
static int guscc_test_13(void)
{
    return compile_and_run("../test/files/test_13.c", "./test_13.s",
                           "/tmp/guscc_test_13_out", 42);
}

/* test_14: nested function calls */
static int guscc_test_14(void)
{
    return compile_and_run("../test/files/test_14.c", "./test_14.s",
                           "/tmp/guscc_test_14_out", 42);
}

/* test_15: pointer parameter and address-of / dereference */
static int guscc_test_15(void)
{
    return compile_and_run("../test/files/test_15.c", "./test_15.s",
                           "/tmp/guscc_test_15_out", 42);
}

/* ---- Failure-path tests: guscc must exit non-zero ---- */

/* test_fail_1: syntax error (missing semicolon) */
static int guscc_test_fail_1(void)
{
    int ret = run_test("../test/files/test_fail_1.c");
    ASSERT(ret != 0);
    return 0;
}

/* test_fail_2: unsupported 'while' statement — parser returns NULL */
static int guscc_test_fail_2(void)
{
    int ret = run_test("../test/files/test_fail_2.c");
    ASSERT(ret != 0);
    return 0;
}

/* test_16: if without else (no braces) */
static int guscc_test_16(void)
{
    return compile_and_run("../test/files/test_16.c", "./test_16.s",
                           "/tmp/guscc_test_16_out", 7);
}

/* test_17: if/else */
static int guscc_test_17(void)
{
    return compile_and_run("../test/files/test_17.c", "./test_17.s",
                           "/tmp/guscc_test_17_out", 2);
}

/* test_18: if/else with braces and function call in condition */
static int guscc_test_18(void)
{
    return compile_and_run("../test/files/test_18.c", "./test_18.s",
                           "/tmp/guscc_test_18_out", 5);
}

/* test_fail_3: undefined variable */
static int guscc_test_fail_3(void)
{
    int ret = run_test("../test/files/test_fail_3.c");
    ASSERT(ret != 0);
    return 0;
}

void guscc_test(void)
{
    /* Original tests */
    ut_run(guscc_test_1);
    ut_run(guscc_test_2);
    ut_run(guscc_test_3);
    ut_run(guscc_test_4);
    ut_run(guscc_test_5);
    ut_run(guscc_test_6);
    ut_run(guscc_test_7);

    /* Identifier / symbol table codegen */
    ut_run(guscc_test_8);
    ut_run(guscc_test_9);
    ut_run(guscc_test_10);

    /* Function calls */
    ut_run(guscc_test_11);
    ut_run(guscc_test_14);

    /* Compound assignment, postfix ops */
    ut_run(guscc_test_12);
    ut_run(guscc_test_13);

    /* Pointers */
    ut_run(guscc_test_15);

    /* if/else */
    ut_run(guscc_test_16);
    ut_run(guscc_test_17);
    ut_run(guscc_test_18);

    /* Failure paths */
    ut_run(guscc_test_fail_1);
    ut_run(guscc_test_fail_2);
    ut_run(guscc_test_fail_3);
}
