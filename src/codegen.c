#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sym.h"

// Forward declarations
static void cg_node(codegen_t *cg, node_t *n);
static void cg_expr(codegen_t *cg, node_t *n);

// x86-64 System V ABI integer argument registers (64-bit / 32-bit / 8-bit)
static const char *arg_regs64[6] = {"%rdi", "%rsi", "%rdx",
                                    "%rcx", "%r8",  "%r9"};
static const char *arg_regs32[6] = {"%edi", "%esi", "%edx",
                                    "%ecx", "%r8d", "%r9d"};
static const char *arg_regs8[6] = {"%dil", "%sil", "%dl",
                                   "%cl",  "%r8b", "%r9b"};

// Return the byte size of a sym (1 / 4 / 8)
static int sym_get_size(sym_t *sym)
{
    if (sym->pointer_level > 0)
        return 8;
    if (sym->decl_spec == NULL || sym->decl_spec->decl_spec.type_spec == NULL)
        return 4;
    switch (sym->decl_spec->decl_spec.type_spec->type_spec) {
        case ND_TYPE_CHAR:
            return 1;
        case ND_TYPE_INT:
            return 4;
        case ND_TYPE_VOID:
            return 1;
    }
    return 4;
}

void codegen_init(codegen_t *cg, FILE *out)
{
    cg->out = out;
    cg->func_name = NULL;
    cg->func_name_len = 0;
    cg->label_count = 0;
    cg->errors = 0;
}

void codegen_finish(codegen_t *cg)
{
    cg->out = NULL;
}

static void cg_num(codegen_t *cg, node_t *n)
{
    char tmp[64];
    int len = n->num.val.len < 63 ? n->num.val.len : 63;
    memcpy(tmp, n->num.val.str, len);
    tmp[len] = '\0';
    long val = strtol(tmp, NULL, 10);
    fprintf(cg->out, "\tmovl\t$%ld, %%eax\n", val);
}

// Emit the binary arithmetic operation given left in %ecx and right in %eax.
// Result is left in %eax.  Does NOT handle &&, ||, or short-circuit ops.
static void cg_arith(codegen_t *cg, int op)
{
    switch (op) {
        case '+':
            fprintf(cg->out, "\taddl\t%%ecx, %%eax\n");
            break;
        case '-':
            // left - right = %ecx - %eax
            fprintf(cg->out, "\tsubl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            break;
        case '*':
            fprintf(cg->out, "\timull\t%%ecx, %%eax\n");
            break;
        case '/':
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tcdq\n");
            fprintf(cg->out, "\tidivl\t%%r8d\n");
            break;
        case '%':
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tcdq\n");
            fprintf(cg->out, "\tidivl\t%%r8d\n");
            fprintf(cg->out, "\tmovl\t%%edx, %%eax\n");
            break;
        case TOKEN_EQ_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsete\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_NE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetne\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '<':
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetl\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '>':
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetg\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_LE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetle\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case TOKEN_GE_OP:
            fprintf(cg->out, "\tcmpl\t%%eax, %%ecx\n");
            fprintf(cg->out, "\tsetge\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '&':
            fprintf(cg->out, "\tandl\t%%ecx, %%eax\n");
            break;
        case '|':
            fprintf(cg->out, "\torl\t%%ecx, %%eax\n");
            break;
        case '^':
            fprintf(cg->out, "\txorl\t%%ecx, %%eax\n");
            break;
        case TOKEN_LEFT_OP:
            // left (ecx) << right (eax) — count must be in %cl
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tmovl\t%%r8d, %%ecx\n");
            fprintf(cg->out, "\tsall\t%%cl, %%eax\n");
            break;
        case TOKEN_RIGHT_OP:
            fprintf(cg->out, "\tmovl\t%%eax, %%r8d\n");
            fprintf(cg->out, "\tmovl\t%%ecx, %%eax\n");
            fprintf(cg->out, "\tmovl\t%%r8d, %%ecx\n");
            fprintf(cg->out, "\tsarl\t%%cl, %%eax\n");
            break;
        default:
            fprintf(stderr, "codegen: unsupported binary op %d\n", op);
            break;
    }
}

static void cg_binop(codegen_t *cg, node_t *n)
{
    int op = n->binop.op;

    // Short-circuit &&
    if (op == TOKEN_AND_OP) {
        int lbl = cg->label_count++;
        cg_expr(cg, n->binop.left);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tje\t.L%d\n", lbl);
        cg_expr(cg, n->binop.right);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tsetne\t%%al\n");
        fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
        fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
        fprintf(cg->out, ".L%d:\n", lbl);
        fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
        fprintf(cg->out, ".L%d_end:\n", lbl);
        return;
    }

    // Short-circuit ||
    if (op == TOKEN_OR_OP) {
        int lbl = cg->label_count++;
        cg_expr(cg, n->binop.left);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tjne\t.L%d\n", lbl);
        cg_expr(cg, n->binop.right);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tsetne\t%%al\n");
        fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
        fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
        fprintf(cg->out, ".L%d:\n", lbl);
        fprintf(cg->out, "\tmovl\t$1, %%eax\n");
        fprintf(cg->out, ".L%d_end:\n", lbl);
        return;
    }

    // General case: left → %rax, push; right → %rax; pop left into %rcx
    cg_expr(cg, n->binop.left);
    fprintf(cg->out, "\tpushq\t%%rax\n");
    cg_expr(cg, n->binop.right);
    fprintf(cg->out, "\tpopq\t%%rcx\n");
    cg_arith(cg, op);
}

static void cg_unop(codegen_t *cg, node_t *n)
{
    int op = n->unop.op;

    // Address-of: &x → lea x, %rax
    if (op == '&') {
        node_t *operand = n->unop.operand;
        if (operand->kind == ND_IDENT && operand->ident.sym) {
            fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n",
                    operand->ident.sym->offset);
        } else {
            fprintf(stderr, "codegen: & applied to non-addressable operand\n");
        }
        return;
    }

    // Dereference: *p → load from pointer
    if (op == '*') {
        cg_expr(cg, n->unop.operand); // pointer → %rax
        fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
        return;
    }

    cg_expr(cg, n->unop.operand);
    switch (op) {
        case '-':
            fprintf(cg->out, "\tnegl\t%%eax\n");
            break;
        case '+':
            break;
        case '!':
            fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
            fprintf(cg->out, "\tsete\t%%al\n");
            fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
            break;
        case '~':
            fprintf(cg->out, "\tnotl\t%%eax\n");
            break;
        default:
            fprintf(stderr, "codegen: unsupported unary op %d\n", op);
            break;
    }
}

static void cg_ternary(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    cg_expr(cg, n->ternary.cond);
    fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
    fprintf(cg->out, "\tje\t.L%d\n", lbl);
    cg_expr(cg, n->ternary.then_expr);
    fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
    fprintf(cg->out, ".L%d:\n", lbl);
    cg_expr(cg, n->ternary.else_expr);
    fprintf(cg->out, ".L%d_end:\n", lbl);
}

static void cg_cast(codegen_t *cg, node_t *n)
{
    cg_expr(cg, n->cast.expr);
    node_t *ds = n->cast.type_node;
    if (ds && ds->kind == ND_DECL_SPEC && ds->decl_spec.pointer_level == 0 &&
        ds->decl_spec.type_spec &&
        ds->decl_spec.type_spec->type_spec == ND_TYPE_CHAR) {
        fprintf(cg->out, "\tmovsbl\t%%al, %%eax\n");
    }
}

static void cg_sizeof_type(codegen_t *cg, node_t *n)
{
    int size = 4;
    node_t *ds = n->sizeof_type.type_node;
    if (ds && ds->kind == ND_DECL_SPEC) {
        if (ds->decl_spec.pointer_level > 0) {
            size = 8;
        } else if (ds->decl_spec.type_spec) {
            switch (ds->decl_spec.type_spec->type_spec) {
                case ND_TYPE_CHAR:
                    size = 1;
                    break;
                case ND_TYPE_INT:
                    size = 4;
                    break;
                case ND_TYPE_VOID:
                    size = 1;
                    break;
            }
        }
    }
    fprintf(cg->out, "\tmovl\t$%d, %%eax\n", size);
}

static void cg_comma(codegen_t *cg, node_t *n)
{
    cg_expr(cg, n->comma.left);
    cg_expr(cg, n->comma.right);
}

// Load a sym_t's value into %rax/%eax
static void cg_load_sym(codegen_t *cg, sym_t *sym)
{
    int size = sym_get_size(sym);
    if (size == 8)
        fprintf(cg->out, "\tmovq\t%d(%%rbp), %%rax\n", sym->offset);
    else if (size == 1)
        fprintf(cg->out, "\tmovsbl\t%d(%%rbp), %%eax\n", sym->offset);
    else
        fprintf(cg->out, "\tmovl\t%d(%%rbp), %%eax\n", sym->offset);
}

// Store %rax/%eax to a sym_t's stack slot
static void cg_store_sym(codegen_t *cg, sym_t *sym)
{
    int size = sym_get_size(sym);
    if (size == 8)
        fprintf(cg->out, "\tmovq\t%%rax, %d(%%rbp)\n", sym->offset);
    else if (size == 1)
        fprintf(cg->out, "\tmovb\t%%al, %d(%%rbp)\n", sym->offset);
    else
        fprintf(cg->out, "\tmovl\t%%eax, %d(%%rbp)\n", sym->offset);
}

// Evaluate the address of an lvalue into %rax
static void cg_lvalue_addr(codegen_t *cg, node_t *n)
{
    if (n->kind == ND_IDENT) {
        if (n->ident.sym == NULL) {
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n",
                    n->ident.name.len, n->ident.name.str);
            cg->errors++;
            return;
        }
        fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n", n->ident.sym->offset);
    } else if (n->kind == ND_UNOP && n->unop.op == '*') {
        // *ptr — the address IS the pointer value
        cg_expr(cg, n->unop.operand);
    } else {
        fprintf(stderr, "codegen: unsupported lvalue kind %d\n", n->kind);
    }
}

// Evaluate an lvalue into %rax/%eax (rvalue read of a memory location)
static void cg_load_lvalue(codegen_t *cg, node_t *n)
{
    if (n->kind == ND_IDENT) {
        if (n->ident.sym == NULL) {
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n",
                    n->ident.name.len, n->ident.name.str);
            cg->errors++;
            fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
            return;
        }
        cg_load_sym(cg, n->ident.sym);
    } else if (n->kind == ND_UNOP && n->unop.op == '*') {
        cg_expr(cg, n->unop.operand); // pointer in %rax
        fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
    } else {
        cg_expr(cg, n); // fallback: evaluate as expression
    }
}

// Store %rax/%eax to an lvalue.  Address must NOT be in %rax already.
static void cg_store_to_lvalue(codegen_t *cg, node_t *lhs)
{
    if (lhs->kind == ND_IDENT) {
        if (lhs->ident.sym == NULL) {
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n",
                    lhs->ident.name.len, lhs->ident.name.str);
            cg->errors++;
            return;
        }
        cg_store_sym(cg, lhs->ident.sym);
    } else {
        // General lvalue: save value, compute address, restore, store
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_lvalue_addr(cg, lhs); // address in %rax
        fprintf(cg->out, "\tmovq\t%%rax, %%rcx\n");
        fprintf(cg->out, "\tpopq\t%%rax\n");
        fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
    }
}

// Map compound assignment operator to its base binary op token
static int compound_base_op(int op)
{
    switch (op) {
        case TOKEN_ADD_ASSIGN:
            return '+';
        case TOKEN_SUB_ASSIGN:
            return '-';
        case TOKEN_MUL_ASSIGN:
            return '*';
        case TOKEN_DIV_ASSIGN:
            return '/';
        case TOKEN_MOD_ASSIGN:
            return '%';
        case TOKEN_LEFT_ASSIGN:
            return TOKEN_LEFT_OP;
        case TOKEN_RIGHT_ASSIGN:
            return TOKEN_RIGHT_OP;
        case TOKEN_AND_ASSIGN:
            return '&';
        case TOKEN_XOR_ASSIGN:
            return '^';
        case TOKEN_OR_ASSIGN:
            return '|';
        default:
            return op;
    }
}

static void cg_ident(codegen_t *cg, node_t *n)
{
    if (n->ident.sym == NULL) {
        fprintf(stderr, "error: use of undeclared identifier '%.*s'\n",
                n->ident.name.len, n->ident.name.str);
        cg->errors++;
        fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
        return;
    }
    cg_load_sym(cg, n->ident.sym);
}

static void cg_assign(codegen_t *cg, node_t *n)
{
    int op = n->assign.op;

    if (op == '=') {
        cg_expr(cg, n->assign.rhs);
        cg_store_to_lvalue(cg, n->assign.lhs);
        // Value of expression stays in %eax/%rax (correct for assignment)
    } else {
        // Compound assignment: load current lhs, push; eval rhs; pop; arith;
        // store
        cg_load_lvalue(cg, n->assign.lhs);
        fprintf(cg->out, "\tpushq\t%%rax\n"); // save lhs value (left operand)
        cg_expr(cg, n->assign.rhs);           // rhs in %eax (right operand)
        fprintf(cg->out, "\tpopq\t%%rcx\n");  // left back in %rcx
        cg_arith(cg, compound_base_op(op));   // result in %eax
        cg_store_to_lvalue(cg, n->assign.lhs);
    }
}

static void cg_local_decl(codegen_t *cg, node_t *n)
{
    if (n->local_decl.init == NULL)
        return; // uninitialized — nothing to emit
    cg_expr(cg, n->local_decl.init);
    if (n->local_decl.sym)
        cg_store_sym(cg, n->local_decl.sym);
}

static void cg_call(codegen_t *cg, node_t *n)
{
    int nargs = n->call.nargs;

    // Evaluate arguments in reverse order and push onto stack
    for (int i = nargs - 1; i >= 0; i--) {
        cg_expr(cg, n->call.args[i]);
        fprintf(cg->out, "\tpushq\t%%rax\n");
    }

    // Pop arguments into the correct registers (forward order)
    int reg_args = nargs < 6 ? nargs : 6;
    for (int i = 0; i < reg_args; i++)
        fprintf(cg->out, "\tpopq\t%s\n", arg_regs64[i]);

    // Emit the call
    node_t *func = n->call.func;
    if (func->kind == ND_IDENT) {
        fprintf(cg->out, "\tcall\t%.*s\n", func->ident.name.len,
                func->ident.name.str);
    } else {
        cg_expr(cg, func); // indirect call (function pointer)
        fprintf(cg->out, "\tcall\t*%%rax\n");
    }
    // Return value is in %rax/%eax
}

static void cg_postop(codegen_t *cg, node_t *n)
{
    node_t *operand = n->postop.operand;
    if (operand->kind != ND_IDENT || operand->ident.sym == NULL) {
        fprintf(stderr, "codegen: postfix op on unsupported lvalue\n");
        return;
    }
    sym_t *sym = operand->ident.sym;
    int size = sym_get_size(sym);

    // Load current value (this is the expression result)
    cg_load_sym(cg, sym);

    // Increment or decrement the variable in place
    if (n->postop.op == TOKEN_INC_OP) {
        if (size == 8)
            fprintf(cg->out, "\taddq\t$1, %d(%%rbp)\n", sym->offset);
        else
            fprintf(cg->out, "\taddl\t$1, %d(%%rbp)\n", sym->offset);
    } else {
        if (size == 8)
            fprintf(cg->out, "\tsubq\t$1, %d(%%rbp)\n", sym->offset);
        else
            fprintf(cg->out, "\tsubl\t$1, %d(%%rbp)\n", sym->offset);
    }
}

static void cg_expr(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_NUM:
            cg_num(cg, n);
            break;
        case ND_IDENT:
            cg_ident(cg, n);
            break;
        case ND_BINOP:
            cg_binop(cg, n);
            break;
        case ND_UNOP:
            cg_unop(cg, n);
            break;
        case ND_POSTOP:
            cg_postop(cg, n);
            break;
        case ND_ASSIGN:
            cg_assign(cg, n);
            break;
        case ND_CALL:
            cg_call(cg, n);
            break;
        case ND_TERNARY:
            cg_ternary(cg, n);
            break;
        case ND_CAST:
            cg_cast(cg, n);
            break;
        case ND_SIZEOF_TYPE:
            cg_sizeof_type(cg, n);
            break;
        case ND_COMMA:
            cg_comma(cg, n);
            break;
        default:
            fprintf(stderr, "codegen: unsupported expression kind %d\n",
                    n->kind);
            break;
    }
}

static void cg_return_stmt(codegen_t *cg, node_t *n)
{
    if (n->return_stmt.expr != NULL)
        cg_expr(cg, n->return_stmt.expr);
    fprintf(cg->out, "\tjmp\t.L%.*s_end\n", cg->func_name_len, cg->func_name);
}

static void cg_expr_stmt(codegen_t *cg, node_t *n)
{
    if (n->expr_stmt.expr != NULL)
        cg_expr(cg, n->expr_stmt.expr);
}

static void cg_comp_stmt(codegen_t *cg, node_t *n)
{
    for (int i = 0; i < n->comp_stmt.nstmts; ++i)
        cg_node(cg, n->comp_stmt.stmts[i]);
}

static void cg_func(codegen_t *cg, node_t *n)
{
    node_str_t name = n->func.declarator->direct_decl.ident;

    cg->func_name = name.str;
    cg->func_name_len = name.len;

    fprintf(cg->out, "\t.text\n");
    fprintf(cg->out, "\t.globl\t%.*s\n", name.len, name.str);
    fprintf(cg->out, "\t.type\t%.*s, @function\n", name.len, name.str);
    fprintf(cg->out, "%.*s:\n", name.len, name.str);
    fprintf(cg->out, "\tpushq\t%%rbp\n");
    fprintf(cg->out, "\tmovq\t%%rsp, %%rbp\n");

    // Reserve stack space for locals and parameters
    if (n->func.frame_size > 0)
        fprintf(cg->out, "\tsubq\t$%d, %%rsp\n", n->func.frame_size);

    // Spill integer parameters from registers to their stack slots.
    // params_sym_list is in reverse order (last param first); collect and
    // reverse.
    node_t *param_list = n->func.declarator->direct_decl.param_list;
    if (param_list != NULL) {
        int nparams = param_list->param_list.nparams;
        // Build an array of sym_t pointers in param declaration order (0..n-1)
        sym_t *ordered[8] = {NULL};
        // Walk params_sym_list (reverse) and match by name
        for (int i = 0; i < nparams && i < 8; i++) {
            node_t *pd = param_list->param_list.params[i];
            node_str_t pname = pd->param_decl.declarator->direct_decl.ident;
            for (sym_t *s = n->func.params_sym_list; s != NULL; s = s->next) {
                if (s->name_len == pname.len &&
                    memcmp(s->name, pname.str, (size_t)pname.len) == 0) {
                    ordered[i] = s;
                    break;
                }
            }
        }
        for (int i = 0; i < nparams && i < 6; i++) {
            sym_t *sym = ordered[i];
            if (sym == NULL)
                continue;
            int size = sym_get_size(sym);
            if (size == 8)
                fprintf(cg->out, "\tmovq\t%s, %d(%%rbp)\n", arg_regs64[i],
                        sym->offset);
            else if (size == 1)
                fprintf(cg->out, "\tmovb\t%s, %d(%%rbp)\n", arg_regs8[i],
                        sym->offset);
            else
                fprintf(cg->out, "\tmovl\t%s, %d(%%rbp)\n", arg_regs32[i],
                        sym->offset);
        }
    }

    cg_node(cg, n->func.comp_stmt);

    fprintf(cg->out, ".L%.*s_end:\n", name.len, name.str);
    if (n->func.frame_size > 0)
        fprintf(cg->out, "\taddq\t$%d, %%rsp\n", n->func.frame_size);
    fprintf(cg->out, "\tpopq\t%%rbp\n");
    fprintf(cg->out, "\tret\n");
}

static void cg_translation_unit(codegen_t *cg, node_t *n)
{
    for (int i = 0; i < n->translation_unit.nfuncs; i++)
        cg_node(cg, n->translation_unit.funcs[i]);
    // Mark stack as non-executable (required by modern Linux toolchains)
    fprintf(cg->out, "\t.section\t.note.GNU-stack,\"\",@progbits\n");
}

static void cg_node(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_TRANSLATION_UNIT:
            cg_translation_unit(cg, n);
            break;
        case ND_FUNC:
            cg_func(cg, n);
            break;
        case ND_COMP_STMT:
            cg_comp_stmt(cg, n);
            break;
        case ND_RETURN_STMT:
            cg_return_stmt(cg, n);
            break;
        case ND_EXPR_STMT:
            cg_expr_stmt(cg, n);
            break;
        case ND_LOCAL_DECL:
            cg_local_decl(cg, n);
            break;
        case ND_DECL_SPEC:
        case ND_TYPE_SPEC:
        case ND_PARAM_DECL:
        case ND_PARAM_LIST:
        case ND_DIRECT_DECL:
            break;
        default:
            // expression nodes that appear as statements
            cg_expr(cg, n);
            break;
    }
}

int codegen_exec(codegen_t *cg, node_t *root)
{
    if (root == NULL)
        return -1;
    cg_node(cg, root);
    return cg->errors > 0 ? -1 : 0;
}
