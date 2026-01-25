#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "assembler.h"

int main(int argc, char *argv[])
{
    char *filename;
    if (argv) {
        filename = argv[1]; 
    }
    
    FILE *fp = fopen("input.c", "r");
    if (!fp) { perror("fopen"); return 1; }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    char *src = malloc(size + 1);
    fread(src, 1, size, fp);
    src[size] = '\0';
    fclose(fp);

    Lexer lex = { .src = src, .pos = 0, .line = 1, .col = 1 };
    Parser p;
    parser_init(&p, &lex);

    Function *fn = parse_function(&p);

    FILE *out = freopen("out.s", "w", stdout);  // Just use freopen directly
    if (!out) { perror("freopen"); return 1; }

    printf(".intel_syntax noprefix\n");
    gen_function(fn);

    fclose(out); 
    free(src);

    return 0;
}
