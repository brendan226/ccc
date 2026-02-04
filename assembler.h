#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "parser.h"


void gen_expr(Expr *e)
{
    switch (e->kind) {

    case EXPR_INT:
        printf("    mov eax, %d\n", e->int_value);
        return;

    case EXPR_VAR:
        printf("    mov eax, DWORD PTR [rbp-%d]\n", e->var->offset);
        return;

    case EXPR_ASSIGN:
        gen_expr(e->binary.right);
        printf("    mov DWORD PTR [rbp-%d], eax\n", e->var->offset);
        return;

    case EXPR_BINARY:
        gen_expr(e->binary.left);
        printf("    push rax\n");

        gen_expr(e->binary.right);
        printf("    mov ebx, eax\n");
        printf("    pop rax\n");

        switch (e->binary.op) {
        case TOK_PLUS:
            printf("    add eax, ebx\n");
            return;
        case TOK_MINUS:
            printf("    sub eax, ebx\n");
            return;
        case TOK_MUL:
            printf("    imul eax, ebx\n");
            return;
        case TOK_DIV:
            printf("    cdq\n");
            printf("    idiv ebx\n");
            return;
        }
    }
}

void gen_stmt(Stmt *s)
{
    switch (s->kind) {

    case STMT_DECL:
        gen_expr(s->decl.init);
        printf("  mov DWORD PTR [rbp-%d], eax\n", s->decl.var->offset);
        return;

    case STMT_RETURN:
        gen_expr(s->expr);
        return;
    }
}

void gen_function(Function *fn)
{
    assign_stack(fn);

    printf(".globl %s\n", fn->name);
    printf("%s:\n", fn->name);
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    for (Stmt *s = fn->body; s; s = s->next)
        gen_stmt(s);

    printf("  leave\n");
    printf("  ret\n");
}


#endif
