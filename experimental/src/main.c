#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "lexer.c"

void blok_lexer_print_token(struct blok_lexer * lex) {
    if(lex->token_id == blok_token_close_bracket) {
        puts("Bracket: ]");
    } else if (lex->token_id == blok_token_open_bracket) {
        puts("Bracket: [");
    } else if (lex->token_id == blok_token_symbol) {
        printf("Symbol: %s\n", lex->token_symbol_lit);
    } else if (lex->token_id == blok_token_string_lit) {
        printf("String: %s\n", lex->token_string_lit);
    } else if (lex->token_id == blok_token_number_lit) {
        printf("Number: %ld.%ld\n", lex->token_integer_lit, lex->token_decimal_lit);
    }
}

enum blok_builtin {
    /*keywords*/
    blok_builtin_def,
    blok_builtin_when,

    /*io*/
    blok_builtin_print,

    /*math*/
    blok_builtin_minus,
    blok_builtin_plus,
    blok_builtin_times,
    blok_builtin_divide,
    blok_builtin_modulus,
};



int main(int argc, char ** argv) {
    char * code = "["
        "[def foo = 10]"
        "[def dotimes n var block = ["
           "[def var n]"
           "[block]"
           "[when [gte i 0]"
               "[dotimes [minus n 1] var block]]]]"
        "[dotimes foo i [print i]]"
        ;


    struct blok_lexer lex;
    enum blok_lexer_err err; 
    char mem[100000];

    err = blok_lexer_init(&lex, mem, 100000, code, strlen(code));
    err = blok_lexer_get_token(&lex);
    while(err == blok_lexer_err_none) {
        blok_lexer_print_token(&lex);
        err = blok_lexer_get_token(&lex);
    } 
    if(err != blok_lex

    return 0;
}
