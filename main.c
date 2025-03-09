#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

void fatal_error(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

typedef enum {
    TAG_NIL = 0,
    TAG_INTEGER,
    TAG_REAL,
    TAG_BOOLEAN,
    TAG_CONS,
    TAG_SYMBOL,
    TAG_STRING,
} Tag;
typedef struct Object {
    Tag tag;
    union {
        long integer;
        double real;
        int boolean;
        char * string;
        /*int symbol;*/
        char * symbols;
        struct {
            struct Object * car;
            struct Object * cdr;
        } cons;
    } value;
} Object;

#define HEAP_SIZE 2048
typedef struct {
    Object heap[HEAP_SIZE]; 
    int heap_i;
} Lisp;

Object * new_object(Lisp * l) {
    if(l->heap_i > HEAP_SIZE) {
        fatal_error("Out of memory");
    }
    l->heap[l->heap_i].tag = TAG_NIL;
    ++l->heap_i;
    return &l->heap[l->heap_i - 1];
}

Object * read_file(Lisp * l, FILE * fp) {
    Object * root = new_object(l);
    char tok[1024] = {0};
    int tok_i = 0;
    char ch = fgetc(fp);
    while(!feof(fp)) {
        switch(ch) {
        case '('



        }
    }
}



/*
int find_string(char haystack[][], long len, char * tok) {
    long i = 0;
    for(;i < len; ++i) {
        if(strncmp(haystack[i], tok, maxtoklen) == 0) {
            return i;
        }
    }
    return -1;
}
*/

#if 0

typedef struct {
    char tokens[maxunique][maxtoklen];
    int tokens_i;
} Compiler;

int intern(Compiler* t, char * tok) {
    if(tok == NULL) {
        return -1;
    } else {
        int index = find_string(t->tokens, sizeof(t->tokens) / sizeof(t->tokens[0]), tok);
        if(index != -1) {
            return index;
        } else {
            if(t->tokens_i > maxtoklen) {
                fatal_error("Too many unique tokens found");
            }
            memmove(t->tokens[t->tokens_i], tok, strnlen(tok, maxtoklen));
            ++t->tokens_i;
            return t->tokens_i - 1;
        }
    }

    return 0;
}

char * raw_get_token(FILE * fp) {
    static char buf[1024] = {0};
    int i = 0;
    char ch = 0;

    if(feof(fp)) return NULL;
    
    /*skip leading whitespace*/
    /*while(!feof(fp) && isspace(ch = fgetc(fp)));*/

    /*
    ch = fgetc(fp);
    buf[0] = ch;
    buf[1] = 0;
    */


    ch = fgetc(fp);
    buf[i] = ch;
    buf[i + 1] = 0;
    ++i;

    /*
    for(ch = fgetc(fp); !is_special(ch) && !feof(fp); ch = fgetc(fp), ++i) {
        if(i + 1 >= sizeof(buf)) fatal_error("Token length overflow");
        buf[i] = ch;
        buf[i + 1] = 0;
    }*/

    while(!is_delimiter(ch) && !feof(fp)) {
        if(i + 1 >= sizeof(buf)) fatal_error("Token length overflow");
        ch = fgetc(fp);
        if(is_delimiter(ch)) {
            ungetc(ch, fp);
            break;
        } else {
            buf[i] = ch;
            buf[i + 1] = 0;
            ++i;
        }
    }

    if(feof(fp)) {
        return NULL;
    } else {
        return buf;
    }
}

void compile_file(Compiler * t, FILE * fp) {
    char * raw_tok = NULL;
    while((raw_tok = raw_get_token(fp))) {
        int tok = intern(t, raw_tok);
        if(tok == -1) {
            fatal_error("Null token\n");
        }
        printf("Token=%4d  ->  \"%s\"\n", tok, t->tokens[tok]);
    }
}

void compilation_start(FILE * fp) {
    Compiler t = {0};
    compile_file(&t, fp); 
}

int main(int argc, char ** argv) {

        
    printf("%zu\n", sizeof(Object));
    if(argc != 2) {
        fatal_error("Expected single filename as argument");
    } else {
        FILE * fp = fopen(argv[1], "r");
        if(fp == NULL) {
            fatal_error("Failed to open file %s", argv[1]);
        }
        compilation_start(fp);
        fclose(fp);
    }

    return 0;
}
#endif
