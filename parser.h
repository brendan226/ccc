#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    EXPR_INT,
    EXPR_BINARY
} ExprKind;

typedef struct Expr Expr;

struct Expr {
    ExprKind kind;
    union {
        int int_value;
        struct {
            Expr *left;
            TokenKind op;
            Expr *right;
        } binary;
    };
};

typedef enum {
    STMT_RETURN
} StmtKind;

typedef struct {
    StmtKind kind;
    Expr *expr;
} Stmt;

typedef struct {
    char *name;
    Stmt *body;
} Function;

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;

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
                p->current.column
                );
        exit(1);
    }
    parser_advance(p);
}

Expr *parse_primary(Parser *p)
{
    if (p->current.kind == TOK_INT_LIT) {
        Expr *e = malloc(sizeof(Expr));
        e->kind = EXPR_INT;
        e->int_value = p->current.int_value;
        parser_advance(p);
        return e;
    }

    fprintf(stderr,
            "Expected expression at %d:%d\n",
            p->current.line,
            p->current.column
            );
    exit(1);
}

int precedence(TokenKind kind)
{
    switch (kind) {
    case TOK_PLUS:
    case TOK_MINUS:
        return 1;
    case TOK_MUL:
    case TOK_DIV:
        return 2;
    default:
        return 0;
    }
}

Expr *parse_expression(Parser *p, int min_prec)
{
    Expr *left = parse_primary(p);

    while (1) {
        int prec = precedence(p->current.kind);
        if (prec < min_prec) break;

        TokenKind op = p->current.kind;
        parser_advance(p);

        Expr *right = parse_expression(p, prec + 1);
        Expr *bin = malloc(sizeof(Expr));        
        bin->kind = EXPR_BINARY;
        bin->binary.left = left;
        bin->binary.op = op;
        bin->binary.right = right;
        
        left = bin;
    }
    return left;
}

Stmt *parse_return(Parser *p)
{
    parser_expect(p, TOK_RETURN);

    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_RETURN;
    s->expr = parse_expression(p, 1);

    parser_expect(p, TOK_SEMICOLON);
    return s;
}

Function *parse_function(Parser *p)
{
    parser_expect(p, TOK_INT);

    if (p->current.kind != TOK_IDENT) {
        fprintf(stderr,
                "Expected function name at %d:%d\n",
                p->current.line,
                p->current.column
                );
        exit(1);
    }

    Function *fn = malloc(sizeof(Function));
    fn->name = p->current.text;
    parser_advance(p);

    parser_expect(p, TOK_LPAREN);
    parser_expect(p, TOK_RPAREN);

    parser_expect(p, TOK_LBRACE);

    fn->body = parse_return(p);
    parser_expect(p, TOK_RBRACE);
    return fn;
}

void print_expr(Expr *e, int indent)
{
    for (int i = 0; i < indent; ++i) printf("  ");
    if (e->kind == EXPR_INT) {
        printf("Int(%d)\n", e->int_value);
    }

    if (e->kind == EXPR_BINARY) {
        printf("Binary(");
        switch (e->binary.op) {
        case TOK_PLUS:  printf("+"); break;
        case TOK_MINUS: printf("-"); break;
        case TOK_MUL:   printf("*"); break;
        case TOK_DIV:   printf("/"); break;
        default:        printf("?"); break;            
        }
        printf(")\n");

        print_expr(e->binary.left, indent + 1);
        print_expr(e->binary.right, indent + 1);
    }
}

void print_stmt(Stmt *s, int indent)
{
    for (int i = 0; i < indent; ++i) printf("  ");
    switch (s->kind) {
    case STMT_RETURN:
        printf("Return\n");
        print_expr(s->expr, indent + 1);
    }
}

void print_function(Function *fn)
{
    printf("Function %s\n", fn->name);
    print_stmt(fn->body, 1);
}

#endif
