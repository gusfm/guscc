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
static int compile_and_run(const char *src, const char *asm_out, const char *bin_out,
                           int expected_exit)
{
    char cmd[512];
    int ret;

    snprintf(cmd, sizeof(cmd), "./guscc %s > /dev/null 2>&1", src);
    ret = system(cmd);
    if (ret != 0)
        return ret;

    snprintf(cmd, sizeof(cmd), "gcc %s -o %s > /dev/null 2>&1", asm_out, bin_out);
    ret = system(cmd);
    ASSERT(ret == 0);

    ret = system(bin_out);
    ASSERT(WEXITSTATUS(ret) == expected_exit);
    return 0;
}

/* ---- Original tests ---- */

static int guscc_test_return_literal(void)
{
    return compile_and_run("../test/files/return_literal.c", "./return_literal.s",
                           "/tmp/guscc_return_literal_out", 42);
}

static int guscc_test_func_params_unused(void)
{
    return compile_and_run("../test/files/func_params_unused.c", "./func_params_unused.s",
                           "/tmp/guscc_func_params_unused_out", 42);
}

/* unsupported_features uses unsupported features (printf, char**) — just verify no crash */
static int guscc_test_unsupported_features(void)
{
    run_test("../test/files/unsupported_features.c");
    return 0;
}

static int guscc_test_binary_ops(void)
{
    return compile_and_run("../test/files/binary_ops.c", "./binary_ops.s",
                           "/tmp/guscc_binary_ops_out", 42);
}

static int guscc_test_unary_ops(void)
{
    return compile_and_run("../test/files/unary_ops.c", "./unary_ops.s", "/tmp/guscc_unary_ops_out",
                           42);
}

static int guscc_test_ternary(void)
{
    return compile_and_run("../test/files/ternary.c", "./ternary.s", "/tmp/guscc_ternary_out", 42);
}

static int guscc_test_cast_and_sizeof(void)
{
    return compile_and_run("../test/files/cast_and_sizeof.c", "./cast_and_sizeof.s",
                           "/tmp/guscc_cast_and_sizeof_out", 42);
}

/* ---- Identifier / symbol table / locals ---- */

static int guscc_test_local_var_decl(void)
{
    return compile_and_run("../test/files/local_var_decl.c", "./local_var_decl.s",
                           "/tmp/guscc_local_var_decl_out", 42);
}

static int guscc_test_local_var_initializer(void)
{
    return compile_and_run("../test/files/local_var_initializer.c", "./local_var_initializer.s",
                           "/tmp/guscc_local_var_initializer_out", 42);
}

static int guscc_test_local_var_shadowing(void)
{
    return compile_and_run("../test/files/local_var_shadowing.c", "./local_var_shadowing.s",
                           "/tmp/guscc_local_var_shadowing_out", 10);
}

static int guscc_test_multiple_locals(void)
{
    return compile_and_run("../test/files/multiple_locals.c", "./multiple_locals.s",
                           "/tmp/guscc_multiple_locals_out", 42);
}

/* ---- Function calls ---- */

static int guscc_test_func_call_int_params(void)
{
    return compile_and_run("../test/files/func_call_int_params.c", "./func_call_int_params.s",
                           "/tmp/guscc_func_call_int_params_out", 42);
}

static int guscc_test_nested_func_calls(void)
{
    return compile_and_run("../test/files/nested_func_calls.c", "./nested_func_calls.s",
                           "/tmp/guscc_nested_func_calls_out", 42);
}

/* ---- Compound assignment, postfix ops ---- */

static int guscc_test_compound_assign(void)
{
    return compile_and_run("../test/files/compound_assign.c", "./compound_assign.s",
                           "/tmp/guscc_compound_assign_out", 42);
}

static int guscc_test_postfix_inc_dec(void)
{
    return compile_and_run("../test/files/postfix_inc_dec.c", "./postfix_inc_dec.s",
                           "/tmp/guscc_postfix_inc_dec_out", 42);
}

/* ---- Pointers ---- */

static int guscc_test_pointer_ops(void)
{
    return compile_and_run("../test/files/pointer_ops.c", "./pointer_ops.s",
                           "/tmp/guscc_pointer_ops_out", 42);
}

/* ---- if/else ---- */

static int guscc_test_if_no_else(void)
{
    return compile_and_run("../test/files/if_no_else.c", "./if_no_else.s",
                           "/tmp/guscc_if_no_else_out", 7);
}

static int guscc_test_if_else(void)
{
    return compile_and_run("../test/files/if_else.c", "./if_else.s", "/tmp/guscc_if_else_out", 2);
}

static int guscc_test_if_else_func_call(void)
{
    return compile_and_run("../test/files/if_else_func_call.c", "./if_else_func_call.s",
                           "/tmp/guscc_if_else_func_call_out", 5);
}

/* ---- while ---- */

static int guscc_test_while_basic(void)
{
    return compile_and_run("../test/files/while_basic.c", "./while_basic.s",
                           "/tmp/guscc_while_basic_out", 5);
}

static int guscc_test_while_func_call(void)
{
    return compile_and_run("../test/files/while_func_call.c", "./while_func_call.s",
                           "/tmp/guscc_while_func_call_out", 32);
}

/* ---- break / continue ---- */

static int guscc_test_while_break(void)
{
    return compile_and_run("../test/files/while_break.c", "./while_break.s",
                           "/tmp/guscc_while_break_out", 5);
}

static int guscc_test_while_continue(void)
{
    return compile_and_run("../test/files/while_continue.c", "./while_continue.s",
                           "/tmp/guscc_while_continue_out", 5);
}

static int guscc_test_while_nested_break(void)
{
    return compile_and_run("../test/files/while_nested_break.c", "./while_nested_break.s",
                           "/tmp/guscc_while_nested_break_out", 3);
}

/* ---- do-while ---- */

static int guscc_test_do_while_basic(void)
{
    return compile_and_run("../test/files/do_while_basic.c", "./do_while_basic.s",
                           "/tmp/guscc_do_while_basic_out", 5);
}

static int guscc_test_do_while_false_cond(void)
{
    return compile_and_run("../test/files/do_while_false_cond.c", "./do_while_false_cond.s",
                           "/tmp/guscc_do_while_false_cond_out", 1);
}

static int guscc_test_do_while_continue(void)
{
    return compile_and_run("../test/files/do_while_continue.c", "./do_while_continue.s",
                           "/tmp/guscc_do_while_continue_out", 4);
}

/* ---- for ---- */

static int guscc_test_for_expr_init(void)
{
    return compile_and_run("../test/files/for_expr_init.c", "./for_expr_init.s",
                           "/tmp/guscc_for_expr_init_out", 0);
}

static int guscc_test_for_basic(void)
{
    return compile_and_run("../test/files/for_basic.c", "./for_basic.s", "/tmp/guscc_for_basic_out",
                           15);
}

static int guscc_test_for_continue(void)
{
    return compile_and_run("../test/files/for_continue.c", "./for_continue.s",
                           "/tmp/guscc_for_continue_out", 4);
}

static int guscc_test_for_break(void)
{
    return compile_and_run("../test/files/for_break.c", "./for_break.s", "/tmp/guscc_for_break_out",
                           2);
}

static int guscc_test_for_empty_clauses(void)
{
    return compile_and_run("../test/files/for_empty_clauses.c", "./for_empty_clauses.s",
                           "/tmp/guscc_for_empty_clauses_out", 5);
}

/* ---- Failure paths: guscc must exit non-zero ---- */

static int guscc_test_fail_syntax_error(void)
{
    int ret = run_test("../test/files/fail_syntax_error.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_undeclared_var(void)
{
    int ret = run_test("../test/files/fail_undeclared_var.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_break_outside_loop(void)
{
    int ret = run_test("../test/files/fail_break_outside_loop.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_continue_outside_loop(void)
{
    int ret = run_test("../test/files/fail_continue_outside_loop.c");
    ASSERT(ret != 0);
    return 0;
}

void guscc_test(void)
{
    /* Original tests */
    ut_run(guscc_test_return_literal);
    ut_run(guscc_test_func_params_unused);
    ut_run(guscc_test_unsupported_features);
    ut_run(guscc_test_binary_ops);
    ut_run(guscc_test_unary_ops);
    ut_run(guscc_test_ternary);
    ut_run(guscc_test_cast_and_sizeof);

    /* Identifier / symbol table / locals */
    ut_run(guscc_test_local_var_decl);
    ut_run(guscc_test_local_var_initializer);
    ut_run(guscc_test_local_var_shadowing);
    ut_run(guscc_test_multiple_locals);

    /* Function calls */
    ut_run(guscc_test_func_call_int_params);
    ut_run(guscc_test_nested_func_calls);

    /* Compound assignment, postfix ops */
    ut_run(guscc_test_compound_assign);
    ut_run(guscc_test_postfix_inc_dec);

    /* Pointers */
    ut_run(guscc_test_pointer_ops);

    /* if/else */
    ut_run(guscc_test_if_no_else);
    ut_run(guscc_test_if_else);
    ut_run(guscc_test_if_else_func_call);

    /* while */
    ut_run(guscc_test_while_basic);
    ut_run(guscc_test_while_func_call);

    /* break / continue */
    ut_run(guscc_test_while_break);
    ut_run(guscc_test_while_continue);
    ut_run(guscc_test_while_nested_break);

    /* do-while */
    ut_run(guscc_test_do_while_basic);
    ut_run(guscc_test_do_while_false_cond);
    ut_run(guscc_test_do_while_continue);

    /* for */
    ut_run(guscc_test_for_expr_init);
    ut_run(guscc_test_for_basic);
    ut_run(guscc_test_for_continue);
    ut_run(guscc_test_for_break);
    ut_run(guscc_test_for_empty_clauses);

    /* Failure paths */
    ut_run(guscc_test_fail_syntax_error);
    ut_run(guscc_test_fail_undeclared_var);
    ut_run(guscc_test_fail_break_outside_loop);
    ut_run(guscc_test_fail_continue_outside_loop);
}
