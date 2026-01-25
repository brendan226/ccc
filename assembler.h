#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "parser.h"


void gen_expr(Expr *e)
{
    if (e->kind == EXPR_INT) {
        printf("    mov eax, %d\n", e->int_value);
        return;
    }

    if (e->kind == EXPR_BINARY) {
        gen_expr(e->binary.left);
        printf("    push rax\n");

        gen_expr(e->binary.right);
        printf("    mov ebx, eax\n");
        printf("    pop rax\n");

        switch (e->binary.op) {
        case TOK_PLUS:
            printf("    add eax, ebx\n");
            break;
        case TOK_MINUS:
            printf("    sub eax, ebx\n");
            break;
        case TOK_MUL:
            printf("    imul eax, ebx\n");
            break;
        case TOK_DIV:
            printf("    cdq\n");
            printf("    idiv ebx\n");
            break;
        }
    }
}

void gen_stmt(Stmt *s)
{
    if (s->kind == STMT_RETURN) {
        gen_expr(s->expr);
    }
}

void gen_function(Function *fn)
{
    printf(".globl %s\n", fn->name);
    printf("%s:\n", fn->name);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");

    gen_stmt(fn->body);

    printf("    pop rbp\n");
    printf("    ret\n");
}

#endif
