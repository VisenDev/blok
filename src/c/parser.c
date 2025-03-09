#include "lexer.c"

enum blok_parser_err {
    blok_parser_err_none,
    blok_parser_err_invalid_input,
    blok_parser_err_out_of_memory,
};

struct blok_parser {
    char * mem;
    long mem_i;
    long mem_len;

    struct blok_lexer * lex;
};

enum blok_parser_err
blok_parser_init(
        struct blok_parser * parse,
        char * mem,
        long mem_len,
        char * input,
        long input_len
        ) {
    if(mem_len < 2 * sizeof(struct blok_lexer)) {
        return blok_parser_err_out_of_memory;
    }

    /*TODO initialize lexer and memory*/

    return blok_parser_err_none;
}


enum blok_parser_err
blok_parser_parse(struct blok_parser * parse) {


    return blok_parser_err_none;
}
