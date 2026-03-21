#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    return run_test("../test/files/test_1.c");
}

static int guscc_test_2(void)
{
    return run_test("../test/files/test_2.c");
}

static int guscc_test_3(void)
{
    run_test("../test/files/test_3.c");
    return 0;
}

void guscc_test(void)
{
    ut_run(guscc_test_1);
    ut_run(guscc_test_2);
    ut_run(guscc_test_3);
}
