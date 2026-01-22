#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum {
    TOK_INT,
    TOK_RETURN,
    TOK_IDENT,
    TOK_INT_LIT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_ASSIGN,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_EOF
} TokenKind;

typedef struct {
    TokenKind kind;
    char *text;
    int int_value;
    int line;
    int column;
} Token;

typedef struct {
    const char *src;
    size_t pos;
    int line;
    int col;
} Lexer;

static char *strndup_c(const char *s, size_t n)
{
    char *p = malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

const char *token_kind_name(TokenKind k)
{
    switch (k) {
        case TOK_INT: return "TOK_INT";
        case TOK_RETURN: return "TOK_RETURN";
        case TOK_IDENT: return "TOK_IDENT";
        case TOK_INT_LIT: return "TOK_INT_LIT";
        case TOK_PLUS: return "TOK_PLUS";
        case TOK_MINUS: return "TOK_MINUS";
        case TOK_MUL: return "TOK_MUL";
        case TOK_DIV: return "TOK_DIV";
        case TOK_ASSIGN: return "TOK_ASSIGN";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_EOF: return "TOK_EOF";
    }
    return "UNKNOWN";
}

char peek(Lexer *l)
{
    return l->src[l->pos];
}

char advance(Lexer *l)
{
    char c = l->src[l->pos++];
    
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else l->col++;
    
    return c;
}

int is_at_end(Lexer *l)
{
    return l->src[l->pos] == '\0';
}

void skip_whitespace(Lexer *l)
{
    while (!is_at_end(l)) {
        char c = peek(l);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(l);
        } else if (c == '\n') {
            advance(l);
        } else {
            break;
        }
    }
}

Token next_token(Lexer *l)
{
    skip_whitespace(l);
    Token tok = {0};
    tok.line = l->line;
    tok.column = l->col;

    if (is_at_end(l)) {
        tok.kind = TOK_EOF;
        return tok;
    }

    char c = advance(l);

    switch (c) {
        case '+': tok.kind = TOK_PLUS; return tok;
        case '-': tok.kind = TOK_MINUS; return tok;
        case '*': tok.kind = TOK_MUL; return tok;
        case '/': tok.kind = TOK_DIV; return tok;
        case '=': tok.kind = TOK_ASSIGN; return tok;
        case '(': tok.kind = TOK_LPAREN; return tok;
        case ')': tok.kind = TOK_RPAREN; return tok;
        case '{': tok.kind = TOK_LBRACE; return tok;
        case '}': tok.kind = TOK_RBRACE; return tok;
        case ';': tok.kind = TOK_SEMICOLON; return tok;
    }

     if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_') {

        size_t start = l->pos - 1;
        while (!is_at_end(l)) {
            char p = peek(l);
            if ((p >= 'a' && p <= 'z') ||
                (p >= 'A' && p <= 'Z') ||
                (p >= '0' && p <= '9') ||
                p == '_') {
                advance(l);
            } else {
                break;
            }
        }

        size_t len = l->pos - start;
        tok.text = strndup_c(l->src + start, len);

        if (strcmp(tok.text, "int") == 0)
            tok.kind = TOK_INT;
        else if (strcmp(tok.text, "return") == 0)
            tok.kind = TOK_RETURN;
        else
            tok.kind = TOK_IDENT;

        return tok;
    }

    if (c >= '0' && c <= '9') {
        int value = c - '0';
        while (!is_at_end(l)) {
            char p = peek(l);
            if (p >= '0' && p <= '9') {
                value = value * 10 + (advance(l) - '0');
            } else {
                break;
            }
        }
        tok.kind = TOK_INT_LIT;
        tok.int_value = value;
        return tok;
    }

    fprintf(stderr, "Unexpected character '%c' at %d:%d\n",
            c, tok.line, tok.column);
    exit(1);
}

int main(void)
{
    FILE *fp = fopen("input.c", "r");

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    rewind(fp);

    char *src = malloc(size + 1);
    fread(src, 1, size, fp);
    src[size] = '\0';

    Lexer lex = {
        .src = src,
        .pos = 0,
        .line = 1,
        .col = 1
    };

    for (;;) {
        Token t = next_token(&lex);
        printf("%s at %d:%d\n", token_kind_name(t.kind), t.line, t.column);
        if (t.kind == TOK_EOF)
            break;
    }

    return 0;
}

