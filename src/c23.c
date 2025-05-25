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
    BLOK_TAG_KEYVALUE,
    BLOK_TAG_SYMBOL,
    BLOK_TAG_TABLE,
    BLOK_TAG_FUNCTION,
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
    char * ptr;
} blok_String;

#define BLOK_SYMBOL_MAX_LEN 64
typedef struct {
    char buf[BLOK_SYMBOL_MAX_LEN];
} blok_Symbol;

typedef struct {
    blok_Symbol key;
    blok_Obj value;
} blok_KeyValue;

typedef struct {
    int32_t count;
    int32_t cap;
    int32_t iterate_i;
    blok_KeyValue * items;
} blok_Table;

#define BLOK_FUNCTION_MAX_PARAMS 8
typedef struct {
    //key: name, value: type
    blok_KeyValue params[BLOK_FUNCTION_MAX_PARAMS];
    int32_t param_count;
    blok_Obj body;
    blok_Table static_variables;
    //TODO figure out return types
} blok_Function;


typedef struct { 
    blok_Table globals;
} blok_State; 

typedef enum {
    BLOK_PRIMITIVE_PRINT,
} blok_Primitive;

_Static_assert(sizeof(blok_Obj) == 8, "blok_Obj should be 64 bits");
_Static_assert(alignof(void*) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(blok_List *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(blok_Obj *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(void(*)(void)) >= 8, "Alignment of function pointers is too small");
_Static_assert(sizeof(void*) == 8, "Expected pointers to be 8 bytes");

blok_Obj blok_obj_allocate(char tag, int32_t bytes) {
    char * mem = malloc(bytes);
    memset(mem, 0, bytes);
    blok_Obj result = {0};
    result.ptr = mem;
    result.tag = tag;
    return result;
}

void * blok_obj_extract_ptr(blok_Obj obj) {
    if(obj.tag == BLOK_TAG_INT || BLOK_TAG_NIL) {
      fatal_error(NULL, "You tried to get the pointer out of a type that does not "
                  "contain a ptr");
    }
    obj.tag = 0;
    return obj.ptr;
}

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
    blok_String * blok_str = malloc(sizeof(blok_String));
    assert(((uintptr_t)blok_str & 0b1111) == 0);

    blok_str->len = len;
    blok_str->cap = len + 1;
    blok_str->ptr = strdup(str);

    blok_Obj result = {.ptr = blok_str};
    result.tag = BLOK_TAG_STRING;
    return result;
}

blok_Obj blok_make_symbol(const char * str) {
    blok_Symbol * sym = malloc(sizeof(blok_Symbol));
    strncpy(sym->buf, str, BLOK_SYMBOL_MAX_LEN - 1);
    blok_Obj result = {0};
    result.ptr = sym;
    result.tag = BLOK_TAG_SYMBOL;
    return result;
}

blok_List * blok_list_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_LIST);
    obj.tag = 0;
    return (blok_List *) obj.ptr;
}

blok_String * blok_string_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_STRING);
    obj.tag = 0;
    return (blok_String *) obj.ptr;
}

blok_Symbol * blok_symbol_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_SYMBOL);
    obj.tag = 0;
    return (blok_Symbol *) obj.ptr;
}


blok_KeyValue * blok_keyvalue_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_KEYVALUE);
    obj.tag = 0;
    return (blok_KeyValue *) obj.ptr;
}

blok_Table * blok_table_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_TABLE);
    obj.tag = 0;
    return (blok_Table *) obj.ptr;
}

bool blok_string_equal(blok_String * const lhs, blok_String * const rhs) {
    if(lhs->len != rhs->len) {
        return false;
    } else {
        return strncmp(lhs->ptr, rhs->ptr, lhs->len);
    }
}

bool blok_symbol_equal(blok_Symbol lhs, blok_Symbol rhs) {
    return strncmp(lhs.buf, rhs.buf, sizeof(lhs.buf));
}


bool blok_obj_equal(blok_Obj lhs, blok_Obj rhs) {
    if(lhs.tag != rhs.tag) {
        return false;
    } else {
        switch(lhs.tag) {
            case BLOK_TAG_INT:
                return lhs.data == rhs.data;
            case BLOK_TAG_NIL:
                return true;
            case BLOK_TAG_STRING:
                return blok_string_equal(blok_string_from_obj(lhs),
                        blok_string_from_obj(rhs));
            case BLOK_TAG_SYMBOL:
                return blok_symbol_equal(*blok_symbol_from_obj(lhs),
                        *blok_symbol_from_obj(rhs));
            case BLOK_TAG_KEYVALUE:
                //TODO
                fatal_error(NULL, "Comparison for this tag not implemented yet");
                return false;
            case BLOK_TAG_TABLE:
                //TODO
                fatal_error(NULL, "Comparison for this tag not implemented yet");
                return false;
            case BLOK_TAG_LIST:
                //TODO
                fatal_error(NULL, "Comparison for this tag not implemented yet");
                return false;
            case BLOK_TAG_PRIMITIVE:
                //TODO
                fatal_error(NULL, "Primitive comparison not implemented");
                return false;
            default: 
                fatal_error(NULL, "Comparison for this tag not implemented yet");
                return false;
        }
    }
}

bool blok_nil_equal(blok_Obj obj) {
    return obj.tag == BLOK_TAG_NIL;
}

bool blok_symbol_empty(blok_Symbol sym) {
    return sym.buf[0] == 0;
}

void blok_obj_free(blok_Obj obj);

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

void blok_string_append(blok_Obj str, char ch) {
    blok_String * l = blok_string_from_obj(str);
    if(l->len + 2 >= l->cap) {
        l->cap = (l->cap + 2) * 2;
        l->ptr = realloc(l->ptr, l->cap);
    }
    l->ptr[l->len++] = ch;
    l->ptr[l->len] = 0;
}

int32_t blok_hash_char_ptr(char * const ptr, int32_t len) {
    /* Inspired by djbt2 by Dan Bernstein - http://www.cse.yorku.ca/~oz/hash.html */
    int32_t hash = 5381;
    int32_t i = 0;

    for(i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char)ptr[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}


int32_t blok_hash(blok_Symbol sym) {
    return blok_hash_char_ptr(sym.buf, BLOK_SYMBOL_MAX_LEN);
}

blok_Table blok_table_init_capacity(int32_t cap) {
    blok_Table table = {0};
    table.items = malloc(sizeof(blok_KeyValue) * cap);
    table.count = 0;
    table.iterate_i = -1;
    table.cap = cap;
    return table;
}


blok_Obj blok_make_table(int32_t size) {
    blok_Obj result = {0};
    blok_Table * ptr = malloc(sizeof(blok_Table));
    *ptr = blok_table_init_capacity(size);
    result.ptr = ptr;
    result.tag = BLOK_TAG_TABLE;
    return result;
}

blok_KeyValue * blok_table_iterate_next(blok_Table * table) {
    assert(table->iterate_i >= 0);
    for(;table->iterate_i < table->cap; ++table->iterate_i) {
        if(!blok_symbol_empty(table->items[table->iterate_i].key)) {
            return &table->items[table->iterate_i++];
        }
    }
    return NULL;
}

void blok_table_iterate_start(blok_Table * table) {
    table->iterate_i = 0;
}

blok_KeyValue * blok_table_get(blok_Table * table, blok_Symbol key) {
    int32_t i = blok_hash(key) % (table->cap - 1);
    assert(i >= 0);
    assert(i < table->cap);
    const int start = i;

    while (!blok_symbol_equal(table->items[i].key, key) &&
           !blok_symbol_empty(table->items[i].key)) {
      ++i;
      if (i + 1 >= table->cap) {
          i = 0;
      } else if (i == start) {
          fatal_error(NULL, "Hash table overfilled");
      }
    }
    return &table->items[i];
}

blok_Obj blok_table_set(blok_Table * table, blok_Symbol key, blok_Obj value);
void blok_table_rehash(blok_Table * table, int32_t new_cap) {
    if(new_cap < table->count) {
        fatal_error(NULL, "new table capacity is smaller than the older one");
    }
    blok_Table new = blok_table_init_capacity(new_cap);
    for(int i = 0; i < table->cap; ++i) {
        blok_KeyValue item = table->items[i];
        if(!blok_symbol_empty(item.key)) {
            blok_table_set(&new, item.key, item.value);
        }
    }
    free(table->items);
    *table = new;
}

blok_Obj blok_table_set(blok_Table * table, blok_Symbol key, blok_Obj value) {
    if(table->cap == 0) {
        *table = blok_table_init_capacity(8);
    } else if(table->count + 1 > table->cap / 3) {
        blok_table_rehash(table, table->cap * 3);
    }
    blok_KeyValue * dest = blok_table_get(table, key);
    if(blok_symbol_empty(dest->key)) {
        ++table->count;
    }
    dest->key = key;
    blok_Obj old = dest->value;
    dest->value = value;
    return old;
}

blok_Obj blok_table_unset(blok_Table * table, blok_Symbol key) {
    blok_KeyValue * dest = blok_table_get(table, key);
    if(blok_symbol_empty(dest->key)) {
        return blok_make_nil();
    } else {
        dest->key = (blok_Symbol){0};
        --table->count;
        return dest->value;
    }
}

void blok_table_empty(blok_Table * table) {
    blok_table_iterate_start(table);
    blok_KeyValue * pair = NULL;
    while((pair = blok_table_iterate_next(table)) != NULL) {
        blok_obj_free(pair->value);
    }
    free(table->items);
    memset(table, 0, sizeof(blok_Table));
}

void blok_obj_free(blok_Obj obj) {
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
        case BLOK_TAG_TABLE:
            {
                blok_Table * table = blok_table_from_obj(obj);
                blok_table_empty(table);
                free(table);
            }
            break;
        case BLOK_TAG_STRING:
            free(blok_string_from_obj(obj)->ptr);
            free(blok_string_from_obj(obj));
            break;
        case BLOK_TAG_SYMBOL:
            free(blok_symbol_from_obj(obj));
            break;
        case BLOK_TAG_KEYVALUE:
            blok_obj_free(blok_keyvalue_from_obj(obj)->value);
            free(blok_keyvalue_from_obj(obj));
        case BLOK_TAG_INT:
        case BLOK_TAG_NIL:
            /*No freeing needed for ints or nil*/
            break;
        default:
            fatal_error(NULL, "Tag cannot be freed");
            break;
    }
}

void blok_obj_print(blok_Obj obj) {
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
                    blok_obj_print(list->items[i]);
                }
                printf(")");
            }
            break;
        case BLOK_TAG_STRING:
            printf("\"%s\"", blok_string_from_obj(obj)->ptr);
            break;
        case BLOK_TAG_SYMBOL:
            printf("%s", blok_symbol_from_obj(obj)->buf);
            break;
        case BLOK_TAG_KEYVALUE:
            {
                blok_KeyValue * kv =  blok_keyvalue_from_obj(obj);
                printf("%s:", kv->key.buf);
                blok_obj_print(kv->value);
            }
            break;
        default:
            printf("UNSUPPORTED TYPE");
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
                    blok_string_append(str, fgetc(fp));
                }
                break;
            case BLOK_READER_STATE_ESCAPE:
                assert(blok_reader_peek(fp) == '\\');
                fgetc(fp);
                char escape = fgetc(fp);
                switch(escape) {
                    case 'n':
                        blok_string_append(str, '\n');
                        break;
                    case '"':
                        blok_string_append(str, '"');
                        break;
                    case '\'':
                        blok_string_append(str, '\'');
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


blok_Obj blok_reader_parse_obj(FILE * fp);

blok_Obj blok_reader_parse_symbol_or_keyvalue(FILE * fp) {
    assert(isalpha(blok_reader_peek(fp)));
    blok_Symbol sym = {0};
    uint32_t i = 0;
    char ch = fgetc(fp);
    sym.buf[i++] = ch;
    while(blok_is_symbol_char(blok_reader_peek(fp))) {
        sym.buf[i++] = fgetc(fp);
        if(i + 2 > sizeof(sym.buf)) {
            fatal_error(fp, "symbol too long, %s\n", sym.buf);
        }
    }

    blok_reader_skip_whitespace(fp);
    if(blok_reader_peek(fp) == ':') {
        blok_Obj result = blok_obj_allocate(BLOK_TAG_KEYVALUE, sizeof(blok_KeyValue));
        blok_KeyValue* kv = blok_obj_extract_ptr(result);
        fgetc(fp); /*skip ':'*/
        kv->key = sym; 
        kv->value = blok_reader_parse_obj(fp);
        return result;
    } else {
        return blok_make_symbol(sym.buf);
    }
}

blok_Obj blok_reader_read(FILE *);
blok_Obj blok_reader_parse_obj(FILE * fp) {
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
        return blok_reader_parse_symbol_or_keyvalue(fp);
    } else {
        fatal_error(fp, "Parsing failed");
    }
    return blok_make_nil();
}

blok_Obj blok_reader_read(FILE * fp) {
    blok_Obj result = blok_make_list(8);
    blok_reader_skip_whitespace(fp);
    while(!feof(fp) && blok_reader_peek(fp) != ')') {
        blok_list_append(&result, blok_reader_parse_obj(fp));
        blok_reader_skip_whitespace(fp);
    }
    return result;
}



blok_State blok_state_init(void) {
    blok_State env = {0};
    blok_table_set(&env.globals, (blok_Symbol){.buf = "print"}, blok_make_int(BLOK_PRIMITIVE_PRINT));
    return env;
}


void blok_state_deinit(blok_State * env) {
    blok_table_empty(&env->globals);
}

blok_KeyValue * blok_state_lookup(blok_State * env, blok_Symbol sym) {
    return blok_table_get(&env->globals, sym);
}


blok_Obj blok_evaluator_apply_list(blok_State * env, int32_t argc, blok_Obj * argv) {
    /*
    (void)env;
    (void)sym;
    (void)argc;
    (void)argv;
    */

    if(argc == 0) {
        fatal_error(NULL, "cannot apply empty list");
    }
    blok_Symbol * sym = blok_symbol_from_obj(argv[0]);
    blok_KeyValue * kv = blok_state_lookup(env, *sym);
    blok_Obj value = kv->value;
    if(value.tag == BLOK_TAG_PRIMITIVE) {
        /*return blok_evaluator_apply_primitive(blok_State * env, _);*/
    } else if(value.tag == BLOK_TAG_FUNCTION) {

    } else {
        fatal_error(NULL, "Value to be applied is not applyable");
    }



    /*TODO*/
    return blok_make_nil();
}

blok_Obj blok_evaluator_eval(blok_State * env, blok_Obj obj) {
    (void)env;
    switch(obj.tag) {
        case BLOK_TAG_NIL:
        case BLOK_TAG_INT:
        case BLOK_TAG_STRING:
            return obj;
        case BLOK_TAG_SYMBOL:

    }

    return blok_make_nil();
}


int main(void) {
    FILE * fp = fopen("c23-test.lisp", "r");
    blok_Obj source = blok_reader_read(fp);
    blok_obj_print(source);
    blok_obj_free(source);
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
