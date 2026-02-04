#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

/* =======================
   AST Types
   ======================= */

typedef enum {
    EXPR_INT,
    EXPR_BINARY,
    EXPR_VAR,
    EXPR_ASSIGN
} ExprKind;

typedef struct Expr Expr;

typedef struct {
    char *name;
    int offset;
} Local;

struct Expr {
    ExprKind kind;
    union {
        int int_value;
        struct {
            Expr *left;
            TokenKind op;
            Expr *right;
        } binary;
        Local *var;
    };
};

typedef enum {
    STMT_RETURN,
    STMT_DECL
} StmtKind;

typedef struct Stmt Stmt;

struct Stmt {
    StmtKind kind;
    Stmt *next;

    union {
        Expr *expr;
        struct {
            Local *var;
            Expr *init;
        } decl;
    };
};

typedef struct {
    char *name;
    Stmt *body;

    Local locals[64];
    int local_count;
    int stack_size;
} Function;

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;


Local *add_local(Function *fn, char *name)
{
    Local *l = &fn->locals[fn->local_count++];
    l->name = name;
    return l;
}

Local *find_local(Function *fn, char *name)
{
    for (int i = 0; i < fn->local_count; i++) {
        if (strcmp(fn->locals[i].name, name) == 0)
            return &fn->locals[i];
    }
    return NULL;
}

void assign_stack(Function *fn)
{
    int offset = 0;
    for (int i = 0; i < fn->local_count; ++i) {
        offset += 4;
        fn->locals[i].offset = offset;
    }
    fn->stack_size = (offset + 15) & ~15;
}

void parser_init(Parser *p, Lexer *lexer)
{
    p->lexer = lexer;
    p->current = next_token(lexer);
}

Token parser_advance(Parser *p)
{
    Token prev = p->current;
    p->current = next_token(p->lexer);
    return prev;
}

Token parser_peek(Parser *p)
{
    return p->current;
}

int parser_match(Parser *p, TokenKind kind)
{
    if (p->current.kind == kind) {
        parser_advance(p);
        return 1;
    }
    return 0;
}

void parser_expect(Parser *p, TokenKind kind)
{
    if (p->current.kind != kind) {
        fprintf(stderr,
            "Expected token %d, got %d at %d:%d\n",
            kind,
            p->current.kind,
            p->current.line,
            p->current.column);
        exit(1);
    }
    parser_advance(p);
}

Expr *parse_primary(Parser *p, Function *fn)
{
    if (p->current.kind == TOK_INT_LIT) {
        Expr *e = malloc(sizeof(Expr));
        e->kind = EXPR_INT;
        e->int_value = p->current.int_value;
        parser_advance(p);
        return e;
    }
    
    if (p->current.kind == TOK_IDENT) {
        Local *var = find_local(fn, p->current.text);
        if (!var) {
            fprintf(stderr,
                "Undefined variable '%s' at %d:%d\n",
                p->current.text,
                p->current.line,
                p->current.column);
            exit(1);
        }

        Expr *e = malloc(sizeof(Expr));
        e->kind = EXPR_VAR;
        e->var = var;
        parser_advance(p);
        return e;
    }

    fprintf(stderr,
        "Expected expression at %d:%d\n",
        p->current.line,
        p->current.column);
    exit(1);
}


int precedence(TokenKind kind)
{
    switch (kind) {
    case TOK_PLUS:
    case TOK_MINUS: return 1;
    case TOK_MUL:
    case TOK_DIV:   return 2;
    default:        return 0;
    }
}

Expr *parse_expression(Parser *p, Function *fn, int min_prec)
{
    Expr *left = parse_primary(p, fn);

    while (1) {
        int prec = precedence(p->current.kind);
        if (prec < min_prec) break;

        TokenKind op = p->current.kind;
        parser_advance(p);

        Expr *right = parse_expression(p, fn, prec + 1);

        Expr *bin = malloc(sizeof(Expr));
        bin->kind = EXPR_BINARY;
        bin->binary.left = left;
        bin->binary.op = op;
        bin->binary.right = right;

        left = bin;
    }

    return left;
}

Stmt *parse_return(Parser *p, Function *fn)
{
    parser_expect(p, TOK_RETURN);

    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_RETURN;
    s->expr = parse_expression(p, fn, 1);
    s->next = NULL;

    parser_expect(p, TOK_SEMICOLON);
    return s;
}

Stmt *parse_decl(Parser *p, Function *fn)
{
    parser_expect(p, TOK_INT);

    if (p->current.kind != TOK_IDENT) {
        fprintf(stderr, "Expected identifier\n");
        exit(1);
    }

    char *name = p->current.text;
    parser_advance(p);

    Local *var = add_local(fn, name);

    parser_expect(p, TOK_ASSIGN);
    Expr *init = parse_expression(p, fn, 1);
    parser_expect(p, TOK_SEMICOLON);

    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_DECL;
    s->decl.var = var;
    s->decl.init = init;
    s->next = NULL;

    return s;
}

Stmt *parse_stmt(Parser *p, Function *fn)
{
    if (p->current.kind == TOK_RETURN)
        return parse_return(p, fn);

    if (p->current.kind == TOK_INT)
        return parse_decl(p, fn);

    fprintf(stderr,
        "Unexpected token %d at %d:%d\n",
        p->current.kind,
        p->current.line,
        p->current.column);
    exit(1);
}

Stmt *parse_block(Parser *p, Function *fn)
{
    Stmt head = {0};
    Stmt *cur = &head;

    while (p->current.kind != TOK_RBRACE) {
        cur->next = parse_stmt(p, fn);
        cur = cur->next;
    }

    return head.next;
}

Function *parse_function(Parser *p)
{
    parser_expect(p, TOK_INT);

    if (p->current.kind != TOK_IDENT) {
        fprintf(stderr, "Expected function name\n");
        exit(1);
    }

    Function *fn = calloc(1, sizeof(Function));
    fn->name = p->current.text;
    parser_advance(p);

    parser_expect(p, TOK_LPAREN);
    parser_expect(p, TOK_RPAREN);

    parser_expect(p, TOK_LBRACE);
    fn->body = parse_block(p, fn);
    parser_expect(p, TOK_RBRACE);

    return fn;
}

#endif
