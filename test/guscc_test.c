#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "ut.h"

/* Run guscc on a source file. Prints the command. Returns guscc exit status. */
static int compile_file(char *file)
{
    char str[1024];
    snprintf(str, sizeof(str), "./guscc %s > /dev/null 2>&1", file);
    return system(str);
}

/* Run guscc, assemble the output, execute and assert exit code == expected.
 * asm_out and bin_out are derived from src: the last path component minus ".c". */
static int compile_and_run(const char *src, int expected_exit)
{
    const char *slash = strrchr(src, '/');
    const char *filename = slash ? slash + 1 : src;
    const char *dot = strrchr(filename, '.');
    int stem_len = dot ? (int)(dot - filename) : (int)strlen(filename);

    char asm_out[256], bin_out[256];
    snprintf(asm_out, sizeof(asm_out), "./%.*s.s", stem_len, filename);
    snprintf(bin_out, sizeof(bin_out), "./%.*s_out", stem_len, filename);

    char cmd[1024];
    int ret;

    ret = compile_file(cmd);

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
    return compile_and_run("../test/files/return_literal.c", 42);
}

static int guscc_test_func_params_unused(void)
{
    return compile_and_run("../test/files/func_params_unused.c", 42);
}

/* unsupported_features uses unsupported features (printf, char**) — just verify no crash */
static int guscc_test_unsupported_features(void)
{
    compile_file("../test/files/unsupported_features.c");
    return 0;
}

static int guscc_test_binary_ops(void)
{
    return compile_and_run("../test/files/binary_ops.c", 42);
}

static int guscc_test_unary_ops(void)
{
    return compile_and_run("../test/files/unary_ops.c", 42);
}

static int guscc_test_ternary(void)
{
    return compile_and_run("../test/files/ternary.c", 42);
}

static int guscc_test_cast_and_sizeof(void)
{
    return compile_and_run("../test/files/cast_and_sizeof.c", 42);
}

/* ---- Identifier / symbol table / locals ---- */

static int guscc_test_local_var_decl(void)
{
    return compile_and_run("../test/files/local_var_decl.c", 42);
}

static int guscc_test_local_var_initializer(void)
{
    return compile_and_run("../test/files/local_var_initializer.c", 42);
}

static int guscc_test_local_var_shadowing(void)
{
    return compile_and_run("../test/files/local_var_shadowing.c", 10);
}

static int guscc_test_multiple_locals(void)
{
    return compile_and_run("../test/files/multiple_locals.c", 42);
}

/* ---- Function calls ---- */

static int guscc_test_func_call_int_params(void)
{
    return compile_and_run("../test/files/func_call_int_params.c", 42);
}

static int guscc_test_nested_func_calls(void)
{
    return compile_and_run("../test/files/nested_func_calls.c", 42);
}

/* ---- Compound assignment, postfix ops ---- */

static int guscc_test_compound_assign(void)
{
    return compile_and_run("../test/files/compound_assign.c", 42);
}

static int guscc_test_postfix_inc_dec(void)
{
    return compile_and_run("../test/files/postfix_inc_dec.c", 42);
}

/* ---- Pointers ---- */

static int guscc_test_pointer_ops(void)
{
    return compile_and_run("../test/files/pointer_ops.c", 42);
}

/* ---- if/else ---- */

static int guscc_test_if_no_else(void)
{
    return compile_and_run("../test/files/if_no_else.c", 7);
}

static int guscc_test_if_else(void)
{
    return compile_and_run("../test/files/if_else.c", 2);
}

static int guscc_test_if_else_func_call(void)
{
    return compile_and_run("../test/files/if_else_func_call.c", 5);
}

/* ---- while ---- */

static int guscc_test_while_basic(void)
{
    return compile_and_run("../test/files/while_basic.c", 5);
}

static int guscc_test_while_func_call(void)
{
    return compile_and_run("../test/files/while_func_call.c", 32);
}

/* ---- break / continue ---- */

static int guscc_test_while_break(void)
{
    return compile_and_run("../test/files/while_break.c", 5);
}

static int guscc_test_while_continue(void)
{
    return compile_and_run("../test/files/while_continue.c", 5);
}

static int guscc_test_while_nested_break(void)
{
    return compile_and_run("../test/files/while_nested_break.c", 3);
}

/* ---- do-while ---- */

static int guscc_test_do_while_basic(void)
{
    return compile_and_run("../test/files/do_while_basic.c", 5);
}

static int guscc_test_do_while_false_cond(void)
{
    return compile_and_run("../test/files/do_while_false_cond.c", 1);
}

static int guscc_test_do_while_continue(void)
{
    return compile_and_run("../test/files/do_while_continue.c", 4);
}

/* ---- for ---- */

static int guscc_test_for_expr_init(void)
{
    return compile_and_run("../test/files/for_expr_init.c", 0);
}

static int guscc_test_for_basic(void)
{
    return compile_and_run("../test/files/for_basic.c", 15);
}

static int guscc_test_for_continue(void)
{
    return compile_and_run("../test/files/for_continue.c", 4);
}

static int guscc_test_for_break(void)
{
    return compile_and_run("../test/files/for_break.c", 2);
}

static int guscc_test_for_empty_clauses(void)
{
    return compile_and_run("../test/files/for_empty_clauses.c", 5);
}

/* ---- Failure paths: guscc must exit non-zero ---- */

static int guscc_test_fail_syntax_error(void)
{
    int ret = compile_file("../test/files/fail_syntax_error.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_undeclared_var(void)
{
    int ret = compile_file("../test/files/fail_undeclared_var.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_break_outside_loop(void)
{
    int ret = compile_file("../test/files/fail_break_outside_loop.c");
    ASSERT(ret != 0);
    return 0;
}

static int guscc_test_fail_continue_outside_loop(void)
{
    int ret = compile_file("../test/files/fail_continue_outside_loop.c");
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
