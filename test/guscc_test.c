#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "ut.h"

static int run_test(char *file)
{
    char str[128];
    snprintf(str, sizeof(str), "./guscc %s > /dev/null", file);
    printf("%s\n", str);
    int ret = system(str);
    return ret;
}

static int guscc_test_1(void)
{
    int ret = run_test("../test/files/test_1.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_1.s -o /tmp/guscc_test_1_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_1_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

static int guscc_test_2(void)
{
    int ret = run_test("../test/files/test_2.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_2.s -o /tmp/guscc_test_2_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_2_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

static int guscc_test_3(void)
{
    run_test("../test/files/test_3.c");
    return 0;
}

static int guscc_test_4(void)
{
    int ret = run_test("../test/files/test_4.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_4.s -o /tmp/guscc_test_4_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_4_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

static int guscc_test_5(void)
{
    int ret = run_test("../test/files/test_5.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_5.s -o /tmp/guscc_test_5_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_5_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

static int guscc_test_6(void)
{
    int ret = run_test("../test/files/test_6.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_6.s -o /tmp/guscc_test_6_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_6_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

static int guscc_test_7(void)
{
    int ret = run_test("../test/files/test_7.c");
    if (ret != 0)
        return ret;
    ret = system("gcc ./test_7.s -o /tmp/guscc_test_7_out > /dev/null 2>&1");
    ASSERT(ret == 0);
    ret = system("/tmp/guscc_test_7_out");
    ASSERT(WEXITSTATUS(ret) == 42);
    return 0;
}

void guscc_test(void)
{
    ut_run(guscc_test_1);
    ut_run(guscc_test_2);
    ut_run(guscc_test_3);
    ut_run(guscc_test_4);
    ut_run(guscc_test_5);
    ut_run(guscc_test_6);
    ut_run(guscc_test_7);
}
