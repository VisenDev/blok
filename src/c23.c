#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

void fatal_error(FILE * fp, const char * fmt, ...) {
    if(fp != NULL) {
        const long progress = ftell(fp);
        rewind(fp);
        while(ftell(fp) < progress) {
            fprintf(stderr, "%c", fgetc(fp));
        }
        fprintf(stderr, "\n\n");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}

typedef enum {
    BLOK_TAG_NIL = 0,
    BLOK_TAG_INT,
    BLOK_TAG_LIST,
    BLOK_TAG_PRIMITIVE,
    BLOK_TAG_STRING,
    BLOK_TAG_SYMBOL,
} blok_Tag;

typedef union {
    void * ptr;
    struct {
        int32_t data;
        uint16_t __pad1;
        uint8_t  __pad2;
        uint8_t tag; 
    };
} blok_Obj;

typedef struct { 
    int32_t len;
    int32_t cap;
    blok_Obj items[];
} blok_List;

typedef struct {
    int32_t len;
    int32_t cap;
    char ptr[];
} blok_String;

_Static_assert(sizeof(blok_Obj) == 8, "blok_Obj should be 64 bits");
_Static_assert(alignof(void*) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(blok_List *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(blok_Obj *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(void(*)(void)) >= 8, "Alignment of function pointers is too small");
_Static_assert(sizeof(void*) == 8, "Expected pointers to be 8 bytes");

blok_Obj blok_make_int(int32_t data) {
    return (blok_Obj){.tag = BLOK_TAG_INT, .data = data};
}
blok_Obj blok_make_nil(void) {
    return (blok_Obj){0};
}


blok_Obj blok_make_list(int32_t initial_capacity) {
    const int32_t size = (sizeof(blok_List) + sizeof(blok_Obj) * initial_capacity);
    blok_List * list = malloc(size);
    memset(list, 0, size);
    blok_Obj result = {.ptr = list};
    result.tag = BLOK_TAG_LIST;
    return result;
}


blok_Obj blok_make_string(const char * str) {
    const int32_t len = strlen(str);
    const int32_t size = sizeof(blok_String) + 2*len + 1;
    blok_String * blok_str = malloc(size);
    assert(((uintptr_t)blok_str & 0b1111) == 0);
    blok_str->len = len;
    blok_str->cap = 2*len;
    memcpy(blok_str->ptr, str, len + 1);

    blok_Obj result = {.ptr = blok_str};
    result.tag = BLOK_TAG_STRING;
    return result;
}

blok_Obj blok_make_symbol(const char * str) {
    blok_Obj result = blok_make_string(str);
    result.tag = BLOK_TAG_SYMBOL;
    return result;
}

blok_List * blok_list_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_LIST);
    obj.tag = 0;
    return (blok_List *) obj.ptr;
}
blok_String * blok_string_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_STRING | obj.tag == BLOK_TAG_SYMBOL);
    obj.tag = 0;
    return (blok_String *) obj.ptr;
}

void blok_list_append(blok_Obj * list, blok_Obj item) {
    blok_List * l = blok_list_from_obj(*list);
    if(l->len + 1 >= l->cap) {
        l->cap = l->cap * 2 + 1;
        l = realloc(l, sizeof(blok_List) + l->cap * sizeof(blok_Obj));
    }
    l->items[l->len++] = item;


    list->ptr = l;
    list->tag = BLOK_TAG_LIST;
}

void blok_string_append(blok_Obj * str, char ch) {
    char tag = str->tag;
    blok_String * l = blok_string_from_obj(*str);
    if(l->len + 2 >= l->cap) {
        l->cap = l->cap * 2 + 2;
        l = realloc(l, sizeof(blok_String) + l->cap * sizeof(char));
        /*printf("string realloc: %d\n", l->cap);*/
        /*fflush(stdout);*/
    }
    l->ptr[l->len++] = ch;
    l->ptr[l->len] = 0;

    str->ptr = l;
    str->tag = tag;
}

void blok_print(blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_NIL: 
            printf("nil");
            break;
        case BLOK_TAG_INT:
            printf("%d", obj.data);
            break;
        case BLOK_TAG_PRIMITIVE:
            obj.tag = 0;
            printf("<Primitive %p>", obj.ptr);
            break;
        case BLOK_TAG_LIST:
        {
            blok_List * list = blok_list_from_obj(obj);
            printf("(");
            for(int i = 0; i < list->len; ++i) {
                if(i != 0) printf(" ");
                blok_print(list->items[i]);
            }
            printf(")");
            break;
        }
        case BLOK_TAG_STRING:
            printf("\"%s\"", blok_string_from_obj(obj)->ptr);
            break;
        case BLOK_TAG_SYMBOL:
            printf("%s", blok_string_from_obj(obj)->ptr);
            break;
        default:
            printf("UNSUPPORTED TYPE");
            break;
    }
}

void blok_obj_free(const blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_LIST:
            {
                blok_List * list = blok_list_from_obj(obj);
                for(int i = 0; i < list->len; ++i) {
                    blok_obj_free(list->items[i]);
                }
                free(list);
            }
            break;
        case BLOK_TAG_STRING:
        case BLOK_TAG_SYMBOL:
            free(blok_string_from_obj(obj));
            break;
        default:
            break;
    }
}

void blok_print_as_bytes(blok_Obj obj) {
    uint8_t *bytes = (uint8_t*)&obj;
    for(int i = 0; i < 8; ++i) {
        printf("%d", bytes[i]);
    }
    puts("");
}


/* READER */

bool blok_reader_is_whitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\t';
}

char blok_reader_peek(FILE * fp) {
    char ch = fgetc(fp);
    ungetc(ch, fp);
    return ch;
}

void blok_reader_skip_whitespace(FILE * fp) {
    if(feof(fp)) return;
    while(blok_reader_is_whitespace(blok_reader_peek(fp))) getc(fp);
}

blok_Obj blok_reader_parse_int(FILE * fp) {
    assert(isdigit(blok_reader_peek(fp)));
    char buf[1024] = {0};
    int i = 0;
    while(isdigit(blok_reader_peek(fp))) buf[i++] = fgetc(fp);
    char * end;
    errno = 0;
    const long num = strtol(buf, &end, 10);
    if(errno != 0) {
        fatal_error(fp, "Failed to parse integer");
    }
    return blok_make_int(num);
}

#define BLOK_READER_STATE_BASE 0
#define BLOK_READER_STATE_ESCAPE 1

blok_Obj blok_reader_parse_string(FILE * fp) {
    assert(blok_reader_peek(fp) == '"');
    fgetc(fp);
    int state = BLOK_READER_STATE_BASE;
    blok_Obj str = blok_make_string("");


    while(1) {
        char ch = blok_reader_peek(fp);
        if(feof(fp)) fatal_error(fp, "Unexpected end of file");
        switch(state) {
            case BLOK_READER_STATE_BASE:
                if(ch == '\\') {
                    state = BLOK_READER_STATE_ESCAPE;
                } else if (ch == '"') {
                    fgetc(fp);
                    return str;
                } else {
                    blok_string_append(&str, fgetc(fp));
                }
                break;
            case BLOK_READER_STATE_ESCAPE:
                assert(blok_reader_peek(fp) == '\\');
                fgetc(fp);
                char escape = fgetc(fp);
                switch(escape) {
                    case 'n':
                        blok_string_append(&str, '\n');
                        break;
                    case '"':
                        blok_string_append(&str, '"');
                        break;
                    case '\'':
                        blok_string_append(&str, '\'');
                        break;
                    default:
                        fatal_error(fp, "Unknown string escape: \"\\%c\"", escape);
                        break;
                }
                state = BLOK_READER_STATE_BASE;
                break;
        }
    }
    return str;
}

bool blok_is_symbol_char(char ch) {
    return ch == '_' || isalpha(ch) || isdigit(ch);
}

blok_Obj blok_reader_parse_symbol(FILE * fp) {
    blok_Obj sym = blok_make_symbol("");
    assert(isalpha(blok_reader_peek(fp)));
    char ch = fgetc(fp);
    blok_string_append(&sym, ch);

    while(blok_is_symbol_char(blok_reader_peek(fp))) {
        blok_string_append(&sym, fgetc(fp));
    }
    
    return sym;
}

blok_Obj blok_reader_read(FILE *);
blok_Obj blok_reader_read_obj(FILE * fp) {
    blok_reader_skip_whitespace(fp);
    const char ch = blok_reader_peek(fp);

    if(isdigit(ch)) {
        return blok_reader_parse_int(fp); 
    } else if(ch == '(') {
        fgetc(fp);
        blok_Obj result = blok_reader_read(fp);
        if (blok_reader_peek(fp) != ')')
          fatal_error(fp, "Expected close parenthesis, found %c",
                      blok_reader_peek(fp));
        fgetc(fp);
        return result;
    } else if(ch == '"') {
        return blok_reader_parse_string(fp);
    } else if(isalpha(ch)) {
        return blok_reader_parse_symbol(fp);
    } else {
        fatal_error(fp, "Parsing failed");
    }
    return blok_make_nil();
}

blok_Obj blok_reader_read(FILE * fp) {
    blok_Obj result = blok_make_list(8);
    blok_reader_skip_whitespace(fp);
    while(!feof(fp) && blok_reader_peek(fp) != ')') {
        blok_list_append(&result, blok_reader_read_obj(fp));
        blok_reader_skip_whitespace(fp);
    }
    return result;
}

typedef struct {
    blok_String name;
    blok_Obj value;
} blok_Variable;

typedef struct {
} blok_Env;

blok_Obj blok_evaluator_eval(blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_NIL:
        case BLOK_TAG_INT:
        case BLOK_TAG_STRING:
            return obj;



    }

    return blok_make_nil();
}


int main(void) {
    FILE * fp = fopen("c23-test.lisp", "r");
    blok_print(blok_reader_read(fp));

    fclose(fp);
    return 0;
}


    /*
    blok_Obj c = blok_make_list(10);
    blok_List * l = blok_list_from_obj(c);
    l->len = 10;
    l->items[2] = blok_make_int(1234);
    l->items[8] = blok_make_string("hi there hello");
    blok_print(c);
    blok_obj_free(c);
    */
