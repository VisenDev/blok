/*types*/
enum blok_token_id {
    blok_token_number_lit,
    blok_token_string_lit,
    blok_token_symbol,
    blok_token_open_bracket,
    blok_token_close_bracket,
};

/*
struct blok_string {
    long len;
    char * ptr; 
};

struct blok_number {
    long integer_part; 
    long decimal_part;
};

struct blok_token_value {
    struct blok_string symbol;
    struct blok_string string_lit;
    struct blok_number number_lit;
};

struct blok_token {
    enum blok_token_id id;
    struct blok_token_value value;
};*/

/*parse data types*/
struct blok_lexer {
    /*memory*/
    char * mem;
    long mem_cap;
    long mem_i;
    /*input*/
    char * input; 
    long input_i;
    long input_cap;
    /*token*/ 
    enum blok_token_id token_id;
    char * token_string_lit; 
    char * token_symbol_lit;
    long token_integer_lit;
    long token_decimal_lit;
};

/*lexer*/
int
blok_is_whitespace(int * result, char ch) {
    *result = ch == ' ' || ch == '\t' || ch == '\n';
    return 0;
}



