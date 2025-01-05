#ifndef NULL
#define NULL ((void *)0)
#endif /*NULL*/

/*types*/
enum blok_token_id {
    blok_token_number_lit,
    blok_token_string_lit,
    blok_token_symbol,
    blok_token_open_bracket,
    blok_token_close_bracket,
};

/*error*/
enum blok_lexer_err {
    blok_lexer_err_none,   
    blok_lexer_err_out_of_memory,   
    blok_lexer_err_invalid_input,
};


/*parse data types*/
struct blok_lexer {

    /*memory*/
    char * mem;
    long mem_len;
    long mem_i;

    /*input*/
    char * input; 
    long input_len;
    long input_i;

    /*token*/ 
    enum blok_token_id token_id;

    char * token_string_lit; 
    long token_string_lit_len; 

    char * token_symbol_lit;
    long token_symbol_lit_len; 

    long token_integer_lit;
    long token_decimal_lit;
};

/*lexer*/
int
blok_ch_is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n';
}


int
blok_ch_is_alpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}


int
blok_ch_is_num(char ch) {
    return (ch >= '0' && ch <= '9');
}


int blok_ch_is_printable(char ch) {
    return ch >= 32 && ch <= 126;
}


long
blok_ch_to_num(char ch) {
    return ch - '0';
}

/*init*/
enum blok_lexer_err
blok_lexer_init(
        struct blok_lexer * lex,
        char * mem, long mem_len,
        char * input, long input_len
        ) {
    lex->mem = mem;
    lex->mem_len = mem_len;
    lex->mem_i = 0;

    lex->input = input;
    lex->input_len = input_len;
    lex->input_i = 0;

    return blok_lexer_err_none;
}

char  
blok_lexer_get_ch(struct blok_lexer * lex) {
    if(lex->input_i >= lex->input_len) {
        return 0;
    }
    if(lex->input == NULL) {
        return 0;
    }
    return lex->input[lex->input_i];
}

char blok_lexer_advance_ch(struct blok_lexer * lex) {
    lex->input_i += 1;
    return blok_lexer_get_ch(lex);
}

enum blok_lexer_err
blok_lexer_skip_whitespace(struct blok_lexer * lex) {
    char ch = blok_lexer_get_ch(lex);
    while(ch != 0 && blok_ch_is_whitespace(ch)) {
        lex->input_i += 1;
        ch = blok_lexer_get_ch(lex); 
    }
    if(ch == 0) {
        return blok_lexer_err_invalid_input;
    }
    return blok_lexer_err_none;
}

enum blok_lexer_number_parse {
    blok_lexer_number_start,
    blok_lexer_number_before_decimal,
    blok_lexer_number_after_decimal,
};

enum blok_lexer_err 
blok_lexer_get_number(struct blok_lexer * lex) {
    enum blok_lexer_number_parse state = blok_lexer_number_start;
    char ch = blok_lexer_get_ch(lex);
    int is_negative = 0;
    long integer_part = 0;
    long decimal_part = 0;
    int finished = 0;

    while(!finished){
        if(state == blok_lexer_number_start) {
            if(ch == '-') {
                is_negative = 1;
                ch = blok_lexer_advance_ch(lex);
                state = blok_lexer_number_before_decimal;
            } else if(blok_ch_is_num(ch)) {
                is_negative = 0;
                state = blok_lexer_number_before_decimal;
            } else {
                return blok_lexer_err_invalid_input;
            }
        } else if(state == blok_lexer_number_before_decimal) {
            if(blok_ch_is_num(ch)) {
                integer_part *= 10;
                integer_part = blok_ch_to_num(ch);
                ch = blok_lexer_advance_ch(lex);
            } else if (ch == '.') {
                ch = blok_lexer_advance_ch(lex);
                state = blok_lexer_number_after_decimal;
            } else {
                finished = 1;
            }
        } else if (state == blok_lexer_number_after_decimal ) {
            if(blok_ch_is_num(ch)) {
                decimal_part *= 10;
                decimal_part = blok_ch_to_num(ch);
                ch = blok_lexer_advance_ch(lex);
            } else {
                finished = 1;  
            }
        } else {
            return blok_lexer_err_invalid_input;
        }
    }

    lex->token_integer_lit = integer_part;
    lex->token_decimal_lit = decimal_part;
    lex->token_id = blok_token_number_lit;

    return blok_lexer_err_none;
}

enum blok_lexer_err
blok_lexer_get_string(struct blok_lexer * lex) {
    char ch = blok_lexer_get_ch(lex);
    int escape = 0;
    char * str = &lex->mem[lex->mem_i];
    long str_i = 0;
    long str_len = lex->mem_len - lex->mem_i; 

    if(ch == '\"') {
        ch = blok_lexer_advance_ch(lex); 
    } else {
        return blok_lexer_err_invalid_input;
    }

    while(1) {
        if(escape) {
            if(ch == '\\') {
                str[str_i] = '\\';     
            } else if (ch == '\"') {
                str[str_i] = '\"';     
            } else if (ch == 'n') {
                str[str_i] = '\n';
            } else {
                return blok_lexer_err_out_of_memory;
            }
            /*update string index*/
            str_i += 1;
            if(str_i >= str_len) {
                return blok_lexer_err_out_of_memory;
            }
            ch = blok_lexer_advance_ch(lex);
            escape = 0;
        } else {
            if(ch == '\\') {
                escape = 1;
                ch = blok_lexer_advance_ch(lex);
            } else if (ch == '\"') {
                ch = blok_lexer_advance_ch(lex);
                /*string has ended*/
                return blok_lexer_err_none;
            } else if(blok_ch_is_printable(ch)) {
                str[str_i] = ch; 
                str_i += 1;
                if(str_i >= str_len) {
                    return blok_lexer_err_out_of_memory;
                }
                ch = blok_lexer_advance_ch(lex);
            } else {
                return blok_lexer_err_invalid_input;
            }
        }
    }
    return blok_lexer_err_none;
}


enum blok_lexer_err
blok_lexer_get_symbol(struct blok_lexer * lex) {

    return blok_lexer_err_none;
}

enum blok_lexer_err
blok_lexer_get_tok(struct blok_lexer * lex) {
    enum blok_lexer_err err = blok_lexer_err_none;
    char ch = 0;

    if(lex->input_len < 1 || lex->input == NULL) {
        return blok_lexer_err_invalid_input;
    }

    err = blok_lexer_skip_whitespace(lex);
    if(err != blok_lexer_err_none) {
        return err;
    }

    ch = blok_lexer_get_ch(lex);
    if(ch == 0) {
        return blok_lexer_err_invalid_input;
    }

    if(ch == '[') {
        lex->token_id = blok_token_open_bracket;
    } else if (ch == ']') {
        lex->token_id = blok_token_close_bracket;
    } else if (blok_ch_is_num(ch) || ch == '-') {
        err = blok_lexer_get_number(lex);
        if(err != blok_lexer_err_none) {
            return err;
        }
    } else if (ch == '\"') {
        err = blok_lexer_get_string(lex);
        if(err != blok_lexer_err_none) {
            return err;
        }
    } else if (blok_ch_is_alpha(ch)) {
        err = blok_lexer_get_symbol(lex);
        if(err != blok_lexer_err_none) {
            return err;
        }
    } else {
        return blok_lexer_err_invalid_input;

    }

    return blok_lexer_err_none;
}
