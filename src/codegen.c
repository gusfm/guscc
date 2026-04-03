#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sym.h"

// Forward declarations
static void cg_node(codegen_t *cg, node_t *n);
static void cg_expr(codegen_t *cg, node_t *n);
static void cg_load_sym(codegen_t *cg, sym_t *sym);
static void cg_member_addr(codegen_t *cg, node_t *n);
static void cg_member(codegen_t *cg, node_t *n);
static void cg_subscript_addr(codegen_t *cg, node_t *n);
static void cg_subscript(codegen_t *cg, node_t *n);
static void cg_str(codegen_t *cg, node_t *n);
static void cg_zero_array(codegen_t *cg, int base, int total);
static void cg_array_init_str(codegen_t *cg, sym_t *sym, node_t *n);
static void cg_array_init_list(codegen_t *cg, sym_t *sym, node_t *n);

// x86-64 System V ABI integer argument registers (64-bit / 32-bit / 8-bit)
static const char *arg_regs64[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static const char *arg_regs32[6] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static const char *arg_regs8[6] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};

// Return the byte size of a sym (1 / 4 / 8 or struct size)
static int sym_get_size(sym_t *sym)
{
    if (sym->pointer_level > 0)
        return 8;
    if (sym->decl_spec == NULL || sym->decl_spec->decl_spec.type_spec == NULL)
        return 4;
    if (sym->decl_spec->decl_spec.type_spec->kind == ND_STRUCT_SPEC) {
        struct_def_t *def = sym->decl_spec->decl_spec.type_spec->struct_spec.def;
        return def ? def->size : 4;
    }
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

// Return the byte size of one subscript element for a sym.
// For arrays (pointer_level==0): returns the base element size via sym_get_size.
// For pointers (pointer_level>0): returns the pointee size (not the pointer size itself).
static int sym_elem_size(sym_t *sym)
{
    if (sym->pointer_level > 1)
        return 8; // pointer-to-pointer: element is a pointer (8 bytes)
    if (sym->pointer_level == 1) {
        // pointer to base type: element size is the base type size
        if (sym->decl_spec == NULL || sym->decl_spec->decl_spec.type_spec == NULL)
            return 4;
        node_t *ts = sym->decl_spec->decl_spec.type_spec;
        if (ts->kind == ND_STRUCT_SPEC) {
            struct_def_t *def = ts->struct_spec.def;
            return def ? def->size : 4;
        }
        switch (ts->type_spec) {
            case ND_TYPE_CHAR: return 1;
            case ND_TYPE_INT:  return 4;
            default:           return 1;
        }
    }
    // array (pointer_level==0): sym_get_size gives the element size
    return sym_get_size(sym);
}

// Return pointee element size if the expression evaluates to a pointer, else 0.
// Handles ND_IDENT (pointer or decayed array) and ND_UNOP '&'.
static int cg_expr_elem_size(node_t *n)
{
    if (n == NULL)
        return 0;
    switch (n->kind) {
        case ND_IDENT:
            if (n->ident.sym == NULL)
                return 0;
            if (n->ident.sym->pointer_level > 0 || n->ident.sym->array_size > 0)
                return sym_elem_size(n->ident.sym);
            return 0;
        case ND_UNOP:
            if (n->unop.op == '&') {
                // &x has type T* — pointee size is the size of x
                node_t *operand = n->unop.operand;
                if (operand->kind == ND_IDENT && operand->ident.sym)
                    return sym_get_size(operand->ident.sym);
                return 4;
            }
            return 0;
        case ND_ASSIGN:
            return cg_expr_elem_size(n->assign.lhs);
        case ND_COMMA:
            return cg_expr_elem_size(n->comma.right);
        default:
            return 0;
    }
}

void codegen_init(codegen_t *cg, FILE *out)
{
    cg->out = out;
    cg->func_name = NULL;
    cg->func_name_len = 0;
    cg->label_count = 0;
    cg->errors = 0;
    cg->loop_label = -1;
    cg->loop_cont_kind = 0;
    cg->str_count = 0;
}

void codegen_finish(codegen_t *cg)
{
    cg->out = NULL;
}

static long cg_node_int_val(node_t *n)
{
    char tmp[64];
    int len = n->num.val.len < 63 ? n->num.val.len : 63;
    memcpy(tmp, n->num.val.str, len);
    tmp[len] = '\0';
    return strtol(tmp, NULL, 10);
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

    // Determine if either operand is a pointer (for pointer arithmetic / comparisons)
    int left_elem = cg_expr_elem_size(n->binop.left);
    int right_elem = cg_expr_elem_size(n->binop.right);

    // Pointer arithmetic: pointer +/- integer, integer + pointer, pointer - pointer
    if (op == '+' || op == '-') {
        if (left_elem > 0 && right_elem == 0) {
            // pointer +/- integer: scale integer by element size
            cg_expr(cg, n->binop.left);                          // pointer → %rax
            fprintf(cg->out, "\tpushq\t%%rax\n");
            cg_expr(cg, n->binop.right);                         // integer → %eax
            fprintf(cg->out, "\tmovslq\t%%eax, %%rax\n");        // sign-extend
            if (left_elem != 1)
                fprintf(cg->out, "\timulq\t$%d, %%rax\n", left_elem);
            fprintf(cg->out, "\tpopq\t%%rcx\n");
            if (op == '+')
                fprintf(cg->out, "\taddq\t%%rcx, %%rax\n");
            else {
                fprintf(cg->out, "\tsubq\t%%rax, %%rcx\n");
                fprintf(cg->out, "\tmovq\t%%rcx, %%rax\n");
            }
            return;
        }
        if (left_elem == 0 && right_elem > 0 && op == '+') {
            // integer + pointer (commutative)
            cg_expr(cg, n->binop.right);                         // pointer → %rax
            fprintf(cg->out, "\tpushq\t%%rax\n");
            cg_expr(cg, n->binop.left);                          // integer → %eax
            fprintf(cg->out, "\tmovslq\t%%eax, %%rax\n");
            if (right_elem != 1)
                fprintf(cg->out, "\timulq\t$%d, %%rax\n", right_elem);
            fprintf(cg->out, "\tpopq\t%%rcx\n");
            fprintf(cg->out, "\taddq\t%%rcx, %%rax\n");
            return;
        }
        if (left_elem > 0 && right_elem > 0 && op == '-') {
            // pointer - pointer: byte difference divided by element size
            cg_expr(cg, n->binop.left);
            fprintf(cg->out, "\tpushq\t%%rax\n");
            cg_expr(cg, n->binop.right);
            fprintf(cg->out, "\tpopq\t%%rcx\n");
            fprintf(cg->out, "\tsubq\t%%rax, %%rcx\n");          // byte diff in %rcx
            fprintf(cg->out, "\tmovq\t%%rcx, %%rax\n");
            if (left_elem == 2)
                fprintf(cg->out, "\tsarq\t$1, %%rax\n");
            else if (left_elem == 4)
                fprintf(cg->out, "\tsarq\t$2, %%rax\n");
            else if (left_elem == 8)
                fprintf(cg->out, "\tsarq\t$3, %%rax\n");
            return;
        }
    }

    // Pointer comparisons: use 64-bit cmpq when either operand is a pointer
    if ((left_elem > 0 || right_elem > 0) &&
        (op == TOKEN_EQ_OP || op == TOKEN_NE_OP || op == '<' || op == '>' ||
         op == TOKEN_LE_OP || op == TOKEN_GE_OP)) {
        cg_expr(cg, n->binop.left);
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_expr(cg, n->binop.right);
        fprintf(cg->out, "\tpopq\t%%rcx\n");
        fprintf(cg->out, "\tcmpq\t%%rax, %%rcx\n");
        switch (op) {
            case TOKEN_EQ_OP: fprintf(cg->out, "\tsete\t%%al\n");  break;
            case TOKEN_NE_OP: fprintf(cg->out, "\tsetne\t%%al\n"); break;
            case '<':         fprintf(cg->out, "\tsetl\t%%al\n");  break;
            case '>':         fprintf(cg->out, "\tsetg\t%%al\n");  break;
            case TOKEN_LE_OP: fprintf(cg->out, "\tsetle\t%%al\n"); break;
            case TOKEN_GE_OP: fprintf(cg->out, "\tsetge\t%%al\n"); break;
        }
        fprintf(cg->out, "\tmovzbl\t%%al, %%eax\n");
        return;
    }

    // General integer arithmetic: left → %rax, push; right → %rax; pop left into %rcx
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
            if (operand->ident.sym->is_global)
                fprintf(cg->out, "\tleaq\t%.*s(%%rip), %%rax\n", operand->ident.sym->name_len,
                        operand->ident.sym->name);
            else
                fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n", operand->ident.sym->offset);
        } else if (operand->kind == ND_MEMBER) {
            cg_member_addr(cg, operand);
        } else if (operand->kind == ND_SUBSCRIPT) {
            cg_subscript_addr(cg, operand);
        } else {
            fprintf(stderr, "codegen: & applied to non-addressable operand\n");
        }
        return;
    }

    // Dereference: *p → load from pointer (size depends on pointee type)
    if (op == '*') {
        int elem = cg_expr_elem_size(n->unop.operand);
        // Diagnose dereference of a plain non-pointer identifier
        if (elem == 0 && n->unop.operand->kind == ND_IDENT &&
            n->unop.operand->ident.sym != NULL &&
            n->unop.operand->ident.sym->pointer_level == 0 &&
            n->unop.operand->ident.sym->array_size == 0) {
            fprintf(stderr, "%d:%d: error: dereference of non-pointer type '%.*s'\n", n->line,
                    n->col, n->unop.operand->ident.name.len, n->unop.operand->ident.name.str);
            cg->errors++;
            fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
            return;
        }
        cg_expr(cg, n->unop.operand); // pointer → %rax
        if (elem == 8)
            fprintf(cg->out, "\tmovq\t(%%rax), %%rax\n");
        else if (elem == 1)
            fprintf(cg->out, "\tmovsbl\t(%%rax), %%eax\n");
        else
            fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
        return;
    }

    // Prefix ++/--: increment/decrement lvalue and return updated value
    if (op == TOKEN_INC_OP || op == TOKEN_DEC_OP) {
        node_t *operand = n->unop.operand;
        if (operand->kind != ND_IDENT || operand->ident.sym == NULL) {
            fprintf(stderr, "codegen: prefix ++/-- on unsupported lvalue\n");
            cg->errors++;
            return;
        }
        sym_t *sym = operand->ident.sym;
        int size = sym_get_size(sym);
        int delta = (sym->pointer_level > 0) ? sym_elem_size(sym) : 1;
        const char *add_op = (size == 8) ? "addq" : "addl";
        const char *sub_op = (size == 8) ? "subq" : "subl";
        const char *instr = (op == TOKEN_INC_OP) ? add_op : sub_op;
        if (sym->is_global)
            fprintf(cg->out, "\t%s\t$%d, %.*s(%%rip)\n", instr, delta, sym->name_len, sym->name);
        else
            fprintf(cg->out, "\t%s\t$%d, %d(%%rbp)\n", instr, delta, sym->offset);
        cg_load_sym(cg, sym); // return updated value
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
        ds->decl_spec.type_spec && ds->decl_spec.type_spec->kind == ND_TYPE_SPEC &&
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
        } else if (ds->decl_spec.type_spec && ds->decl_spec.type_spec->kind == ND_STRUCT_SPEC) {
            struct_def_t *def = ds->decl_spec.type_spec->struct_spec.def;
            if (def)
                size = def->size;
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
    if (sym->is_global) {
        if (size == 8)
            fprintf(cg->out, "\tmovq\t%.*s(%%rip), %%rax\n", sym->name_len, sym->name);
        else if (size == 1)
            fprintf(cg->out, "\tmovsbl\t%.*s(%%rip), %%eax\n", sym->name_len, sym->name);
        else
            fprintf(cg->out, "\tmovl\t%.*s(%%rip), %%eax\n", sym->name_len, sym->name);
        return;
    }
    if (size == 8)
        fprintf(cg->out, "\tmovq\t%d(%%rbp), %%rax\n", sym->offset);
    else if (size == 1)
        fprintf(cg->out, "\tmovsbl\t%d(%%rbp), %%eax\n", sym->offset);
    else
        fprintf(cg->out, "\tmovl\t%d(%%rbp), %%eax\n", sym->offset);
}

// Store %rax/%eax to a sym_t's stack slot (or global via %rip)
static void cg_store_sym(codegen_t *cg, sym_t *sym)
{
    int size = sym_get_size(sym);
    if (sym->is_global) {
        if (size == 8)
            fprintf(cg->out, "\tmovq\t%%rax, %.*s(%%rip)\n", sym->name_len, sym->name);
        else if (size == 1)
            fprintf(cg->out, "\tmovb\t%%al, %.*s(%%rip)\n", sym->name_len, sym->name);
        else
            fprintf(cg->out, "\tmovl\t%%eax, %.*s(%%rip)\n", sym->name_len, sym->name);
        return;
    }
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
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n", n->ident.name.len,
                    n->ident.name.str);
            cg->errors++;
            return;
        }
        if (n->ident.sym->is_global)
            fprintf(cg->out, "\tleaq\t%.*s(%%rip), %%rax\n", n->ident.sym->name_len,
                    n->ident.sym->name);
        else
            fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n", n->ident.sym->offset);
    } else if (n->kind == ND_UNOP && n->unop.op == '*') {
        // *ptr — the address IS the pointer value
        cg_expr(cg, n->unop.operand);
    } else if (n->kind == ND_MEMBER) {
        cg_member_addr(cg, n);
    } else if (n->kind == ND_SUBSCRIPT) {
        cg_subscript_addr(cg, n);
    } else {
        fprintf(stderr, "codegen: unsupported lvalue kind %d\n", n->kind);
    }
}

// Evaluate an lvalue into %rax/%eax (rvalue read of a memory location)
static void cg_load_lvalue(codegen_t *cg, node_t *n)
{
    if (n->kind == ND_IDENT) {
        if (n->ident.sym == NULL) {
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n", n->ident.name.len,
                    n->ident.name.str);
            cg->errors++;
            fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
            return;
        }
        cg_load_sym(cg, n->ident.sym);
    } else if (n->kind == ND_UNOP && n->unop.op == '*') {
        int elem = cg_expr_elem_size(n->unop.operand);
        cg_expr(cg, n->unop.operand); // pointer in %rax
        if (elem == 8)
            fprintf(cg->out, "\tmovq\t(%%rax), %%rax\n");
        else if (elem == 1)
            fprintf(cg->out, "\tmovsbl\t(%%rax), %%eax\n");
        else
            fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
    } else if (n->kind == ND_MEMBER) {
        cg_member(cg, n);
    } else if (n->kind == ND_SUBSCRIPT) {
        cg_subscript(cg, n);
    } else {
        cg_expr(cg, n); // fallback: evaluate as expression
    }
}

// Store %rax/%eax to an lvalue.  Address must NOT be in %rax already.
static void cg_store_to_lvalue(codegen_t *cg, node_t *lhs)
{
    if (lhs->kind == ND_IDENT) {
        if (lhs->ident.sym == NULL) {
            fprintf(stderr, "error: use of undeclared identifier '%.*s'\n", lhs->ident.name.len,
                    lhs->ident.name.str);
            cg->errors++;
            return;
        }
        cg_store_sym(cg, lhs->ident.sym);
    } else if (lhs->kind == ND_MEMBER) {
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_member_addr(cg, lhs);
        fprintf(cg->out, "\tmovq\t%%rax, %%rcx\n");
        fprintf(cg->out, "\tpopq\t%%rax\n");
        if (lhs->member.resolved) {
            int size = lhs->member.resolved->size;
            if (size == 8)
                fprintf(cg->out, "\tmovq\t%%rax, (%%rcx)\n");
            else if (size == 1)
                fprintf(cg->out, "\tmovb\t%%al, (%%rcx)\n");
            else
                fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
        } else {
            fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
        }
    } else if (lhs->kind == ND_SUBSCRIPT) {
        int elem_size = 4;
        if (lhs->subscript.array->kind == ND_IDENT && lhs->subscript.array->ident.sym)
            elem_size = sym_elem_size(lhs->subscript.array->ident.sym);
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_subscript_addr(cg, lhs); // address → %rax
        fprintf(cg->out, "\tmovq\t%%rax, %%rcx\n");
        fprintf(cg->out, "\tpopq\t%%rax\n");
        if (elem_size == 8)
            fprintf(cg->out, "\tmovq\t%%rax, (%%rcx)\n");
        else if (elem_size == 1)
            fprintf(cg->out, "\tmovb\t%%al, (%%rcx)\n");
        else
            fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
    } else if (lhs->kind == ND_UNOP && lhs->unop.op == '*') {
        // *ptr = value: evaluate pointer, store value at its address
        int elem = cg_expr_elem_size(lhs->unop.operand);
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_expr(cg, lhs->unop.operand); // pointer → %rax
        fprintf(cg->out, "\tmovq\t%%rax, %%rcx\n");
        fprintf(cg->out, "\tpopq\t%%rax\n");
        if (elem == 8)
            fprintf(cg->out, "\tmovq\t%%rax, (%%rcx)\n");
        else if (elem == 1)
            fprintf(cg->out, "\tmovb\t%%al, (%%rcx)\n");
        else
            fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
    } else {
        // General lvalue: save value, compute address, restore, store
        fprintf(cg->out, "\tpushq\t%%rax\n");
        cg_lvalue_addr(cg, lhs); // address in %rax
        fprintf(cg->out, "\tmovq\t%%rax, %%rcx\n");
        fprintf(cg->out, "\tpopq\t%%rax\n");
        fprintf(cg->out, "\tmovl\t%%eax, (%%rcx)\n");
    }
}

// Compute address of a struct member into %rax
static void cg_member_addr(codegen_t *cg, node_t *n)
{
    if (n->member.resolved == NULL) {
        fprintf(stderr, "codegen: unresolved struct member '%.*s'\n", n->member.field.len,
                n->member.field.str);
        cg->errors++;
        return;
    }
    int offset = n->member.resolved->offset;
    if (n->member.is_ptr)
        cg_expr(cg, n->member.object); // pointer value → %rax
    else
        cg_lvalue_addr(cg, n->member.object); // struct base address → %rax
    if (offset != 0)
        fprintf(cg->out, "\taddq\t$%d, %%rax\n", offset);
}

// Load struct member value into %rax/%eax
static void cg_member(codegen_t *cg, node_t *n)
{
    cg_member_addr(cg, n);
    if (n->member.resolved == NULL)
        return;
    int size = n->member.resolved->size;
    if (size == 8)
        fprintf(cg->out, "\tmovq\t(%%rax), %%rax\n");
    else if (size == 1)
        fprintf(cg->out, "\tmovsbl\t(%%rax), %%eax\n");
    else
        fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
}

// Compute address of array[index] into %rax
static void cg_subscript_addr(codegen_t *cg, node_t *n)
{
    node_t *arr = n->subscript.array;
    node_t *idx = n->subscript.index;

    // Determine element size
    int elem_size = 4;
    if (arr->kind == ND_IDENT && arr->ident.sym)
        elem_size = sym_elem_size(arr->ident.sym);

    // Step 1: compute scaled index → %rcx
    cg_expr(cg, idx);                                         // index → %eax
    fprintf(cg->out, "\tmovslq\t%%eax, %%rcx\n");            // sign-extend to 64-bit
    if (elem_size != 1)
        fprintf(cg->out, "\timulq\t$%d, %%rcx\n", elem_size); // scale

    // Step 2: compute base address → %rax
    if (arr->kind == ND_IDENT && arr->ident.sym && arr->ident.sym->array_size > 0) {
        // Array: base is stack slot (local) or global label
        if (arr->ident.sym->is_global)
            fprintf(cg->out, "\tleaq\t%.*s(%%rip), %%rax\n", arr->ident.sym->name_len,
                    arr->ident.sym->name);
        else
            fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n", arr->ident.sym->offset);
    } else {
        // Pointer: evaluate to get pointer value (may clobber %rcx, so save it first)
        fprintf(cg->out, "\tpushq\t%%rcx\n");
        cg_expr(cg, arr);  // pointer value → %rax
        fprintf(cg->out, "\tpopq\t%%rcx\n");
    }

    // Step 3: address = base + scaled index
    fprintf(cg->out, "\taddq\t%%rcx, %%rax\n");
}

// Load value at array[index] into %rax/%eax
static void cg_subscript(codegen_t *cg, node_t *n)
{
    int elem_size = 4;
    if (n->subscript.array->kind == ND_IDENT && n->subscript.array->ident.sym)
        elem_size = sym_elem_size(n->subscript.array->ident.sym);

    cg_subscript_addr(cg, n); // address → %rax
    if (elem_size == 8)
        fprintf(cg->out, "\tmovq\t(%%rax), %%rax\n");
    else if (elem_size == 1)
        fprintf(cg->out, "\tmovsbl\t(%%rax), %%eax\n");
    else
        fprintf(cg->out, "\tmovl\t(%%rax), %%eax\n");
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
        fprintf(stderr, "error: use of undeclared identifier '%.*s'\n", n->ident.name.len,
                n->ident.name.str);
        cg->errors++;
        fprintf(cg->out, "\txorl\t%%eax, %%eax\n");
        return;
    }
    if (n->ident.sym->array_size > 0) {
        // Array decays to pointer: yield base address of first element
        if (n->ident.sym->is_global)
            fprintf(cg->out, "\tleaq\t%.*s(%%rip), %%rax\n", n->ident.sym->name_len,
                    n->ident.sym->name);
        else
            fprintf(cg->out, "\tleaq\t%d(%%rbp), %%rax\n", n->ident.sym->offset);
    } else {
        cg_load_sym(cg, n->ident.sym);
    }
}

static void cg_assign(codegen_t *cg, node_t *n)
{
    int op = n->assign.op;

    if (op == '=') {
        cg_expr(cg, n->assign.rhs);
        cg_store_to_lvalue(cg, n->assign.lhs);
        // Value of expression stays in %eax/%rax (correct for assignment)
    } else {
        // Compound assignment: load current lhs, push; eval rhs; pop; arith; store
        if ((op == TOKEN_ADD_ASSIGN || op == TOKEN_SUB_ASSIGN) &&
            cg_expr_elem_size(n->assign.lhs) > 0) {
            // pointer += integer  or  pointer -= integer
            int lhs_elem = cg_expr_elem_size(n->assign.lhs);
            cg_load_lvalue(cg, n->assign.lhs);            // pointer → %rax
            fprintf(cg->out, "\tpushq\t%%rax\n");
            cg_expr(cg, n->assign.rhs);                   // integer → %eax
            fprintf(cg->out, "\tmovslq\t%%eax, %%rax\n"); // sign-extend
            if (lhs_elem != 1)
                fprintf(cg->out, "\timulq\t$%d, %%rax\n", lhs_elem);
            fprintf(cg->out, "\tpopq\t%%rcx\n");
            if (op == TOKEN_ADD_ASSIGN)
                fprintf(cg->out, "\taddq\t%%rcx, %%rax\n");
            else {
                fprintf(cg->out, "\tsubq\t%%rax, %%rcx\n");
                fprintf(cg->out, "\tmovq\t%%rcx, %%rax\n");
            }
            cg_store_to_lvalue(cg, n->assign.lhs);
        } else {
            cg_load_lvalue(cg, n->assign.lhs);
            fprintf(cg->out, "\tpushq\t%%rax\n"); // save lhs value (left operand)
            cg_expr(cg, n->assign.rhs);           // rhs in %eax (right operand)
            fprintf(cg->out, "\tpopq\t%%rcx\n");  // left back in %rcx
            cg_arith(cg, compound_base_op(op));   // result in %eax
            cg_store_to_lvalue(cg, n->assign.lhs);
        }
    }
}

static void cg_str(codegen_t *cg, node_t *n)
{
    int id = cg->str_count++;
    fprintf(cg->out, "\t.pushsection\t.rodata\n");
    fprintf(cg->out, ".LC%d:\n", id);
    // n->str.val.str points to opening '"'; n->str.val.len includes both quotes
    fprintf(cg->out, "\t.string\t%.*s\n", n->str.val.len, n->str.val.str);
    fprintf(cg->out, "\t.popsection\n");
    fprintf(cg->out, "\tleaq\t.LC%d(%%rip), %%rax\n", id);
}

static void cg_zero_array(codegen_t *cg, int base, int total)
{
    int pos = 0;
    while (pos + 8 <= total) {
        fprintf(cg->out, "\tmovq\t$0, %d(%%rbp)\n", base + pos);
        pos += 8;
    }
    if (pos + 4 <= total) {
        fprintf(cg->out, "\tmovl\t$0, %d(%%rbp)\n", base + pos);
        pos += 4;
    }
    while (pos < total) {
        fprintf(cg->out, "\tmovb\t$0, %d(%%rbp)\n", base + pos);
        pos++;
    }
}

static void cg_array_init_str(codegen_t *cg, sym_t *sym, node_t *n)
{
    int base = sym->offset;
    int capacity = sym->array_size; // char array: 1 byte per element

    // 1. Zero the entire array
    cg_zero_array(cg, base, capacity);

    // 2. Emit movb for each string byte, parsing escape sequences
    const char *src = n->str.val.str + 1; // skip opening '"'
    int src_len = n->str.val.len - 2;      // exclude both quotes
    int dest = 0;
    for (int i = 0; i < src_len && dest < capacity - 1;) {
        int byte_val;
        if (src[i] == '\\' && i + 1 < src_len) {
            switch (src[i + 1]) {
                case 'n':  byte_val = '\n'; break;
                case 't':  byte_val = '\t'; break;
                case 'r':  byte_val = '\r'; break;
                case '\\': byte_val = '\\'; break;
                case '"':  byte_val = '"';  break;
                case '0':  byte_val = '\0'; break;
                default:   byte_val = (unsigned char)src[i + 1]; break;
            }
            i += 2;
        } else {
            byte_val = (unsigned char)src[i];
            i++;
        }
        fprintf(cg->out, "\tmovb\t$%d, %d(%%rbp)\n", byte_val, base + dest);
        dest++;
    }
    // Null terminator at dest is already zero from cg_zero_array
}

static void cg_array_init_list(codegen_t *cg, sym_t *sym, node_t *n)
{
    int base = sym->offset;
    int elem_count = sym->array_size;
    int elem_size = sym_get_size(sym); // per-element size (sym_get_size ignores array_size)
    int total = elem_count * elem_size;

    // 1. Zero the entire array
    cg_zero_array(cg, base, total);

    // 2. Apply each provided initializer
    int count = n->initializer_list.count;
    if (count > elem_count)
        count = elem_count;
    for (int i = 0; i < count; i++) {
        cg_expr(cg, n->initializer_list.items[i]); // result in %rax / %eax
        int offset = base + i * elem_size;
        if (elem_size == 8)
            fprintf(cg->out, "\tmovq\t%%rax, %d(%%rbp)\n", offset);
        else if (elem_size == 1)
            fprintf(cg->out, "\tmovb\t%%al, %d(%%rbp)\n", offset);
        else
            fprintf(cg->out, "\tmovl\t%%eax, %d(%%rbp)\n", offset);
    }
}

static void cg_local_decl(codegen_t *cg, node_t *n)
{
    sym_t *sym = n->local_decl.sym;
    node_t *init = n->local_decl.init;

    if (init == NULL)
        return; // uninitialized — nothing to emit

    if (sym && sym->array_size > 0) {
        // Array initialization
        if (init->kind == ND_STR) {
            cg_array_init_str(cg, sym, init);
        } else if (init->kind == ND_INITIALIZER_LIST) {
            cg_array_init_list(cg, sym, init);
        } else {
            fprintf(stderr, "%d:%d: error: cannot initialize array with scalar expression\n",
                    init->line, init->col);
            cg->errors++;
        }
        return;
    }

    // Scalar initialization (existing path)
    cg_expr(cg, init);
    if (sym)
        cg_store_sym(cg, sym);
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
        fprintf(cg->out, "\tcall\t%.*s\n", func->ident.name.len, func->ident.name.str);
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
    int delta = (sym->pointer_level > 0) ? sym_elem_size(sym) : 1;
    const char *add_op = (size == 8) ? "addq" : "addl";
    const char *sub_op = (size == 8) ? "subq" : "subl";
    const char *instr = (n->postop.op == TOKEN_INC_OP) ? add_op : sub_op;
    if (sym->is_global)
        fprintf(cg->out, "\t%s\t$%d, %.*s(%%rip)\n", instr, delta, sym->name_len, sym->name);
    else
        fprintf(cg->out, "\t%s\t$%d, %d(%%rbp)\n", instr, delta, sym->offset);
}

static void cg_expr(codegen_t *cg, node_t *n)
{
    if (n == NULL)
        return;
    switch (n->kind) {
        case ND_NUM:
            cg_num(cg, n);
            break;
        case ND_STR:
            cg_str(cg, n);
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
        case ND_MEMBER:
            cg_member(cg, n);
            break;
        case ND_SUBSCRIPT:
            cg_subscript(cg, n);
            break;
        case ND_SIZEOF_EXPR: {
            int size = 4;
            node_t *expr = n->sizeof_expr.expr;
            if (expr->kind == ND_IDENT && expr->ident.sym) {
                sym_t *sym = expr->ident.sym;
                int elem = sym_get_size(sym);
                size = (sym->array_size > 0) ? elem * sym->array_size : elem;
            }
            fprintf(cg->out, "\tmovl\t$%d, %%eax\n", size);
            break;
        }
        case ND_INITIALIZER_LIST:
            fprintf(stderr, "%d:%d: error: initializer list in expression context\n",
                    n->line, n->col);
            cg->errors++;
            break;
        default:
            fprintf(stderr, "codegen: unsupported expression kind %d\n", n->kind);
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

static void cg_break_stmt(codegen_t *cg, node_t *n)
{
    if (cg->loop_label < 0) {
        fprintf(stderr, "%d:%d: error: 'break' outside loop\n", n->line, n->col);
        cg->errors++;
        return;
    }
    fprintf(cg->out, "\tjmp\t.L%d_end\n", cg->loop_label);
}

static void cg_continue_stmt(codegen_t *cg, node_t *n)
{
    if (cg->loop_label < 0) {
        fprintf(stderr, "%d:%d: error: 'continue' outside loop\n", n->line, n->col);
        cg->errors++;
        return;
    }
    if (cg->loop_cont_kind == 1)
        fprintf(cg->out, "\tjmp\t.L%d_cond\n", cg->loop_label);
    else if (cg->loop_cont_kind == 2)
        fprintf(cg->out, "\tjmp\t.L%d_post\n", cg->loop_label);
    else
        fprintf(cg->out, "\tjmp\t.L%d_start\n", cg->loop_label);
}

static void cg_while_stmt(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    int prev_loop = cg->loop_label;
    int prev_kind = cg->loop_cont_kind;
    cg->loop_label = lbl;
    cg->loop_cont_kind = 0;
    fprintf(cg->out, ".L%d_start:\n", lbl);
    cg_expr(cg, n->while_stmt.cond);
    fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
    fprintf(cg->out, "\tje\t.L%d_end\n", lbl);
    cg_node(cg, n->while_stmt.body);
    fprintf(cg->out, "\tjmp\t.L%d_start\n", lbl);
    fprintf(cg->out, ".L%d_end:\n", lbl);
    cg->loop_label = prev_loop;
    cg->loop_cont_kind = prev_kind;
}

static void cg_do_while_stmt(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    int prev_loop = cg->loop_label;
    int prev_kind = cg->loop_cont_kind;
    cg->loop_label = lbl;
    cg->loop_cont_kind = 1;
    fprintf(cg->out, ".L%d_start:\n", lbl);
    cg_node(cg, n->while_stmt.body);
    fprintf(cg->out, ".L%d_cond:\n", lbl);
    cg_expr(cg, n->while_stmt.cond);
    fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
    fprintf(cg->out, "\tjne\t.L%d_start\n", lbl);
    fprintf(cg->out, ".L%d_end:\n", lbl);
    cg->loop_label = prev_loop;
    cg->loop_cont_kind = prev_kind;
}

static void cg_for_stmt(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    int prev_loop = cg->loop_label;
    int prev_kind = cg->loop_cont_kind;
    cg->loop_label = lbl;
    cg->loop_cont_kind = 2;

    if (n->for_stmt.init)
        cg_node(cg, n->for_stmt.init);

    fprintf(cg->out, ".L%d_start:\n", lbl);

    if (n->for_stmt.cond) {
        cg_expr(cg, n->for_stmt.cond);
        fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
        fprintf(cg->out, "\tje\t.L%d_end\n", lbl);
    }

    cg_node(cg, n->for_stmt.body);

    fprintf(cg->out, ".L%d_post:\n", lbl);
    if (n->for_stmt.post)
        cg_expr(cg, n->for_stmt.post);

    fprintf(cg->out, "\tjmp\t.L%d_start\n", lbl);
    fprintf(cg->out, ".L%d_end:\n", lbl);

    cg->loop_label = prev_loop;
    cg->loop_cont_kind = prev_kind;
}

static void cg_if_stmt(codegen_t *cg, node_t *n)
{
    int lbl = cg->label_count++;
    cg_expr(cg, n->if_stmt.cond);
    fprintf(cg->out, "\ttestl\t%%eax, %%eax\n");
    if (n->if_stmt.else_) {
        fprintf(cg->out, "\tje\t.L%d_else\n", lbl);
        cg_node(cg, n->if_stmt.then);
        fprintf(cg->out, "\tjmp\t.L%d_end\n", lbl);
        fprintf(cg->out, ".L%d_else:\n", lbl);
        cg_node(cg, n->if_stmt.else_);
    } else {
        fprintf(cg->out, "\tje\t.L%d_end\n", lbl);
        cg_node(cg, n->if_stmt.then);
    }
    fprintf(cg->out, ".L%d_end:\n", lbl);
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
            if (pd->param_decl.declarator == NULL)
                continue;
            node_str_t pname = pd->param_decl.declarator->direct_decl.ident;
            if (pname.str == NULL)
                continue;
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
                fprintf(cg->out, "\tmovq\t%s, %d(%%rbp)\n", arg_regs64[i], sym->offset);
            else if (size == 1)
                fprintf(cg->out, "\tmovb\t%s, %d(%%rbp)\n", arg_regs8[i], sym->offset);
            else
                fprintf(cg->out, "\tmovl\t%s, %d(%%rbp)\n", arg_regs32[i], sym->offset);
        }
    }

    cg_node(cg, n->func.comp_stmt);

    fprintf(cg->out, ".L%.*s_end:\n", name.len, name.str);
    if (n->func.frame_size > 0)
        fprintf(cg->out, "\taddq\t$%d, %%rsp\n", n->func.frame_size);
    fprintf(cg->out, "\tpopq\t%%rbp\n");
    fprintf(cg->out, "\tret\n");
}

static void cg_global_decl(codegen_t *cg, node_t *n)
{
    sym_t *sym = n->global_decl.sym;
    if (sym == NULL)
        return; // bare type decl (e.g. struct definition)

    int size = sym_get_size(sym);
    int total = (sym->array_size > 0) ? sym->array_size * size : size;
    int align = size < 8 ? size : 8;
    // Use struct alignment for struct globals
    if (sym->pointer_level == 0 && sym->decl_spec && sym->decl_spec->decl_spec.type_spec &&
        sym->decl_spec->decl_spec.type_spec->kind == ND_STRUCT_SPEC) {
        struct_def_t *def = sym->decl_spec->decl_spec.type_spec->struct_spec.def;
        if (def) {
            total = def->size;
            align = def->align;
        }
    }
    node_t *init = n->global_decl.init;

    fprintf(cg->out, "\t.globl\t%.*s\n", sym->name_len, sym->name);

    if (init == NULL) {
        // Uninitialized — .comm (zero-initialized by linker)
        fprintf(cg->out, "\t.comm\t%.*s, %d, %d\n", sym->name_len, sym->name, total, align);
        return;
    }

    // Initialized — .data section
    fprintf(cg->out, "\t.data\n");
    fprintf(cg->out, "\t.align\t%d\n", align);
    fprintf(cg->out, "%.*s:\n", sym->name_len, sym->name);

    if (sym->array_size > 0 && init->kind == ND_STR) {
        // char array from string literal: e.g. char s[20] = "hello"
        const char *src = init->str.val.str; // includes quotes
        int slen = init->str.val.len;
        fprintf(cg->out, "\t.string\t%.*s\n", slen, src);
        // .string adds null terminator; pad remaining bytes
        int used = slen - 1; // approximate: chars between quotes + null
        if (total > used)
            fprintf(cg->out, "\t.zero\t%d\n", total - used);
    } else if (sym->array_size > 0 && init->kind == ND_INITIALIZER_LIST) {
        // Array with initializer list: e.g. int arr[5] = {1,2,3,4,5}
        int count = init->initializer_list.count;
        if (count > sym->array_size)
            count = sym->array_size;
        for (int i = 0; i < count; i++) {
            node_t *elem = init->initializer_list.items[i];
            if (elem->kind != ND_NUM) {
                fprintf(stderr, "%d:%d: error: global array initializer must be a constant\n",
                        elem->line, elem->col);
                cg->errors++;
                continue;
            }
            long val = cg_node_int_val(elem);
            if (size == 8)
                fprintf(cg->out, "\t.quad\t%ld\n", val);
            else if (size == 1)
                fprintf(cg->out, "\t.byte\t%ld\n", val);
            else
                fprintf(cg->out, "\t.long\t%ld\n", val);
        }
        int remaining = (sym->array_size - count) * size;
        if (remaining > 0)
            fprintf(cg->out, "\t.zero\t%d\n", remaining);
    } else if (init->kind == ND_NUM) {
        // Scalar integer: e.g. int x = 42
        long val = cg_node_int_val(init);
        if (size == 8)
            fprintf(cg->out, "\t.quad\t%ld\n", val);
        else if (size == 1)
            fprintf(cg->out, "\t.byte\t%ld\n", val);
        else
            fprintf(cg->out, "\t.long\t%ld\n", val);
    } else if (init->kind == ND_STR && sym->pointer_level > 0) {
        // Pointer to string literal: e.g. char *p = "hello"
        int id = cg->str_count++;
        fprintf(cg->out, "\t.quad\t.LC%d\n", id);
        fprintf(cg->out, "\t.section\t.rodata\n");
        fprintf(cg->out, ".LC%d:\n", id);
        fprintf(cg->out, "\t.string\t%.*s\n", init->str.val.len, init->str.val.str);
        fprintf(cg->out, "\t.data\n");
    } else {
        fprintf(stderr, "%d:%d: error: unsupported global initializer\n", init->line, init->col);
        cg->errors++;
    }
}

static void cg_translation_unit(codegen_t *cg, node_t *n)
{
    // Emit global variable definitions first
    for (int i = 0; i < n->translation_unit.ndecls; i++)
        cg_global_decl(cg, n->translation_unit.decls[i]);

    // Switch to .text for functions
    fprintf(cg->out, "\t.text\n");
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
        case ND_IF_STMT:
            cg_if_stmt(cg, n);
            break;
        case ND_WHILE_STMT:
            cg_while_stmt(cg, n);
            break;
        case ND_DO_WHILE_STMT:
            cg_do_while_stmt(cg, n);
            break;
        case ND_FOR_STMT:
            cg_for_stmt(cg, n);
            break;
        case ND_RETURN_STMT:
            cg_return_stmt(cg, n);
            break;
        case ND_BREAK_STMT:
            cg_break_stmt(cg, n);
            break;
        case ND_CONTINUE_STMT:
            cg_continue_stmt(cg, n);
            break;
        case ND_EXPR_STMT:
            cg_expr_stmt(cg, n);
            break;
        case ND_LOCAL_DECL:
            cg_local_decl(cg, n);
            break;
        case ND_GLOBAL_DECL:
            cg_global_decl(cg, n);
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
