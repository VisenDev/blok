#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.c"

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
    BLOK_TAG_TRUE,
    BLOK_TAG_FALSE,
} blok_Tag;


const char * blok_tag_get_name(blok_Tag tag) {

    switch(tag) {
        case BLOK_TAG_NIL:       return "BLOK_TAG_NIL";
        case BLOK_TAG_INT:       return "BLOK_TAG_INT";
        case BLOK_TAG_LIST:      return "BLOK_TAG_LIST";
        case BLOK_TAG_PRIMITIVE: return "BLOK_TAG_PRIMITIVE";
        case BLOK_TAG_STRING:    return "BLOK_TAG_STRING";
        case BLOK_TAG_KEYVALUE:  return "BLOK_TAG_KEYVALUE";
        case BLOK_TAG_SYMBOL:    return "BLOK_TAG_SYMBOL";
        case BLOK_TAG_TABLE:     return "BLOK_TAG_TABLE";
        case BLOK_TAG_FUNCTION:  return "BLOK_TAG_FUNCTION";
        case BLOK_TAG_TRUE:      return "BLOK_TAG_TRUE";
        case BLOK_TAG_FALSE:     return "BLOK_TAG_FALSE";
    }
}


struct blok_Table;
typedef struct blok_Scope { 
    struct blok_Scope * parent;
    blok_Arena arena;
    struct blok_Table * bindings;
} blok_Scope; 

typedef struct {
    blok_Tag tag; 
    union {
        void * ptr;
        int data;
    } as;
} blok_Obj;

typedef struct { 
    int32_t len;
    int32_t cap;
    blok_Obj * items;
    blok_Scope * scope; /*the scope in which new items are allocated*/
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
    uint32_t count;
    uint32_t cap;
    uint32_t iterate_i;
    blok_KeyValue * items;
    blok_Scope * scope; /*the scope in which new items are allocated*/
} blok_Table;

/*#define BLOK_FUNCTION_MAX_PARAMS 8*/
typedef struct {
    blok_List * params;
    blok_List * body;
} blok_Function;


void * blok_scope_alloc(blok_Scope * b, size_t bytes) {
    return blok_arena_alloc(&b->arena, bytes);
}

void blok_scope_close(blok_Scope * b) {
    blok_arena_free(&b->arena);
}
void blok_scope_reclaim(blok_Scope * b, void * ptr) {
    blok_arena_reclaim(&b->arena, ptr);
}


#define BLOK_PARAMETER_COUNT_MAX 8
typedef struct {
    bool variadic; 
    blok_Tag return_type;
    blok_Tag params[BLOK_PARAMETER_COUNT_MAX];
    int32_t param_count;
} blok_FunctionSignature;

/* TODO, make the contents of a file and a progn different primitives*/
typedef enum {
    BLOK_PRIMITIVE_PRINT,
    BLOK_PRIMITIVE_PROGN,
    BLOK_PRIMITIVE_SET,
    BLOK_PRIMITIVE_DEFUN,
    BLOK_PRIMITIVE_QUOTE,
    BLOK_PRIMITIVE_LIST,
    BLOK_PRIMITIVE_WHEN,
    BLOK_PRIMITIVE_NOT,
    BLOK_PRIMITIVE_EQUAL,
    BLOK_PRIMITIVE_AND,
    BLOK_PRIMITIVE_LT,
    BLOK_PRIMITIVE_LTE,
    BLOK_PRIMITIVE_GT,
    BLOK_PRIMITIVE_GTE,
    BLOK_PRIMITIVE_ADD,
    BLOK_PRIMITIVE_SUB,
    BLOK_PRIMITIVE_MUL,
    BLOK_PRIMITIVE_DIV,
    BLOK_PRIMITIVE_IF,
    BLOK_PRIMITIVE_UNLESS,
} blok_Primitive;


/*
blok_Obj blok_obj_allocate(blok_Scope * b, blok_Tag tag, int32_t bytes) {
    char * mem = blok_scope_alloc(b, bytes);
    memset(mem, 0, bytes);
    blok_Obj result = {0};
    result.as.ptr = mem;
    result.tag = tag;
    return result;
}
*/

//TODO: finish fixing this function!
void * blok_ptr_from_obj(blok_Obj obj) {
    if (obj.tag == BLOK_TAG_INT
    ||  obj.tag == BLOK_TAG_NIL
    ||  obj.tag == BLOK_TAG_FALSE
    ||  obj.tag == BLOK_TAG_TRUE
    ||  obj.tag == BLOK_TAG_PRIMITIVE) {
        fatal_error(NULL,
                "You tried to get the pointer out of a type that does not "
                "contain a ptr");
    }
    obj.tag = 0;
    return obj.as.ptr;
}

blok_Symbol blok_symbol_from_char_ptr(const char * ptr) {
    blok_Symbol sym = {0};
    strncpy(sym.buf, ptr, sizeof(sym.buf) - 1);
    return sym;
}

blok_Symbol * blok_symbol_allocate(blok_Scope * b) {
    blok_Symbol * result = blok_scope_alloc(b, sizeof(blok_Symbol));
    memset(result, 0, sizeof(blok_Symbol));
    return result;
}

blok_Obj blok_obj_from_ptr(void * ptr, blok_Tag tag) {
    assert(((uintptr_t)ptr& 0xf) == 0);
    blok_Obj obj = {0};
    obj.as.ptr = ptr;
    obj.tag = tag;
    return obj;
}

blok_Obj blok_obj_from_list(blok_List * l) { return blok_obj_from_ptr(l, BLOK_TAG_LIST); }
blok_Obj blok_obj_from_keyvalue(blok_KeyValue* l) { return blok_obj_from_ptr(l, BLOK_TAG_KEYVALUE); }
blok_Obj blok_obj_from_table(blok_Table * l) { return blok_obj_from_ptr(l, BLOK_TAG_TABLE); }
blok_Obj blok_obj_from_string(blok_String * s) { return blok_obj_from_ptr(s, BLOK_TAG_STRING); }
blok_Obj blok_obj_from_symbol(blok_Symbol * s) { return blok_obj_from_ptr(s, BLOK_TAG_SYMBOL); }
blok_Obj blok_obj_from_function(blok_Function * f) { return blok_obj_from_ptr(f, BLOK_TAG_FUNCTION); }

blok_Obj blok_make_int(int32_t data) {
    return (blok_Obj){.tag = BLOK_TAG_INT, .as.data = data};
}
blok_Obj blok_make_primitive(blok_Primitive data) {
    return (blok_Obj){.tag = BLOK_TAG_PRIMITIVE, .as.data = data};
}
blok_Obj blok_make_nil(void) {
    return (blok_Obj){.tag = BLOK_TAG_NIL};
}
blok_Obj blok_make_true(void) {
    return (blok_Obj){.tag = BLOK_TAG_TRUE};
}
blok_Obj blok_make_false(void) {
    return (blok_Obj){.tag = BLOK_TAG_FALSE};
}
blok_Obj blok_make_boolean(bool cond) {
    return cond ? blok_make_true() : blok_make_false();
}


blok_Obj blok_make_function(blok_Scope * b, blok_List * params, blok_List * body) {
    blok_Function * result = blok_scope_alloc(b, sizeof(blok_Function));
    result->params = params;
    result->body = body;
    return blok_obj_from_function(result);
}


blok_List * blok_list_allocate(blok_Scope * b, int32_t initial_capacity) {
    assert(b != NULL);
    blok_List * result = blok_scope_alloc(b, sizeof(blok_List));
    assert(result != NULL);
    result->len = 0;
    result->cap = initial_capacity;
    result->scope = b;
    result->items = blok_scope_alloc(b, result->cap * sizeof(blok_Obj));
    assert(result->items != NULL);
    return result;
}

blok_KeyValue * blok_keyvalue_allocate(blok_Scope * b) {
    void * result = blok_scope_alloc(b, sizeof(blok_KeyValue));
    return result;
}

/*
void blok_list_free(blok_List * l) {
    free(l->items);
    free(l);
}
*/

blok_String * blok_string_allocate(blok_Scope * b, uint64_t capacity) {
    blok_String * blok_str = blok_scope_alloc(b, sizeof(blok_String));
    assert(((uintptr_t)blok_str & 0xf) == 0);

    blok_str->len = 0;
    blok_str->cap = capacity + 1;
    blok_str->ptr = blok_scope_alloc(b, capacity + 1);
    memset(blok_str->ptr, 0, capacity + 1);
    return blok_str;
}

blok_Obj blok_make_string(blok_Scope * b, const char * str) {
    const int32_t len = strlen(str);
    blok_String * result = blok_string_allocate(b, len);
    strncpy(result->ptr, str, len + 1);
    return blok_obj_from_ptr(result, BLOK_TAG_STRING);
}

blok_Obj blok_make_symbol(blok_Scope * b, const char * str) {
    blok_Symbol * sym = blok_scope_alloc(b, sizeof(blok_Symbol));
    strncpy(sym->buf, str, BLOK_SYMBOL_MAX_LEN - 1);
    blok_Obj result = {0};
    result.as.ptr = sym;
    result.tag = BLOK_TAG_SYMBOL;
    return result;
}

blok_List * blok_list_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_LIST);
    obj.tag = 0;
    return (blok_List *) obj.as.ptr;
}

blok_String * blok_string_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_STRING);
    obj.tag = 0;
    return (blok_String *) obj.as.ptr;
}

blok_Symbol * blok_symbol_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_SYMBOL);
    obj.tag = 0;
    return (blok_Symbol *) obj.as.ptr;
}


blok_KeyValue * blok_keyvalue_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_KEYVALUE);
    obj.tag = 0;
    return (blok_KeyValue *) obj.as.ptr;
}


blok_Function * blok_function_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_FUNCTION);
    obj.tag = 0;
    return (blok_Function *) obj.as.ptr;
}

blok_Table * blok_table_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_TABLE);
    obj.tag = 0;
    return (blok_Table *) obj.as.ptr;
}

bool blok_string_equal(blok_String * const lhs, blok_String * const rhs) {
    if(lhs->len != rhs->len) {
        return false;
    } else {
        return strncmp(lhs->ptr, rhs->ptr, lhs->len);
    }
}

bool blok_symbol_equal(blok_Symbol lhs, blok_Symbol rhs) {
    return strncmp(lhs.buf, rhs.buf, sizeof(lhs.buf)) == 0;
}


bool blok_obj_equal(blok_Obj lhs, blok_Obj rhs) {
    if(lhs.tag != rhs.tag) {
        return false;
    } else {
        switch(lhs.tag) {
            case BLOK_TAG_INT:
                return lhs.as.data == rhs.as.data;
            case BLOK_TAG_NIL:
                return true;
            case BLOK_TAG_STRING:
                return blok_string_equal(blok_string_from_obj(lhs),
                        blok_string_from_obj(rhs));
            case BLOK_TAG_SYMBOL:
                return blok_symbol_equal(*blok_symbol_from_obj(lhs),
                        *blok_symbol_from_obj(rhs));
            default: 
                printf("Tag: %s\n", blok_tag_get_name(lhs.tag));
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


blok_Obj blok_obj_copy(blok_Scope * destination_scope, blok_Obj obj);
//void blok_obj_free(blok_Obj obj);

void blok_list_append(blok_List* l, blok_Obj item) {
    //TODO: copy items using the lists local scope
    if(l->len + 1 >= l->cap) {
        l->cap = l->cap * 2 + 1;
        l->items = realloc(l->items, l->cap * sizeof(blok_Obj));
    }
    l->items[l->len++] = item;
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

uint64_t blok_hash_char_ptr(char * const ptr, uint64_t len) {
    /* Inspired by djbt2 by Dan Bernstein - http://www.cse.yorku.ca/~oz/hash.html */
    uint64_t hash = 5381;
    uint64_t i = 0;

    for(i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char)ptr[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}


int32_t blok_hash(blok_Symbol sym) {
    return blok_hash_char_ptr(sym.buf, (char*)memchr(sym.buf, 0, BLOK_SYMBOL_MAX_LEN) - sym.buf);
}

blok_Table * blok_table_allocate(blok_Scope * b, int32_t cap) {
    blok_Table * table = blok_scope_alloc(b, sizeof(blok_Table));
    const uint64_t bytes = sizeof(blok_KeyValue) * cap;
    table->items = blok_scope_alloc(b, bytes);
    memset(table->items, 0, bytes);
    table->count = 0;
    table->iterate_i = 0;
    table->cap = cap;
    return table;
}


blok_Obj blok_make_table(blok_Scope * b, int32_t cap) {
    return blok_obj_from_table(blok_table_allocate(b, cap));
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

blok_KeyValue * blok_table_find_slot(blok_Table * table, blok_Symbol key) {
    /*
    function find_slot(key)
        i := hash(key) modulo num_slots
        // search until we either find the key, or find an empty slot.
        while (slot[i] is occupied) and (slot[i].key ≠ key)
            i := (i + 1) modulo num_slots
        return i
    */

    if(table->cap <= 0) {
        table->cap = 8;
        table->items = blok_scope_alloc(table->scope, sizeof(blok_KeyValue) * table->cap);
        memset(table->items, 0, sizeof(blok_KeyValue) * table->cap);
    }
    uint64_t i = blok_hash(key) % (table->cap - 1);
    assert(i >= 0);
    assert(i < table->cap);

    const uint64_t start = i;
    do {
        blok_KeyValue kv = table->items[i];
        if(blok_symbol_equal(kv.key, key)) {
            return &table->items[i];
        }
        if(blok_symbol_empty(kv.key)) {
            return &table->items[i];
        }


        ++i;
        i = i % table->cap;
        if(i == start) return NULL;
    } while(1);
}

blok_Obj blok_table_get(blok_Scope * destination_scope, blok_Table * table, blok_Symbol key) {
    blok_KeyValue * kv = blok_table_find_slot(table, key);
    if(kv == NULL) {
        return blok_make_nil();
    } else if(blok_symbol_empty(kv->key)) {
        return blok_make_nil();
    } else {
        //TODO actually copy this value
        return blok_obj_copy(destination_scope, kv->value);
    }
}

blok_Obj blok_table_set(blok_Table * table, blok_Symbol key, blok_Obj value);

void blok_table_rehash(blok_Table * table, uint64_t new_cap) {
    if(new_cap < table->count) {
        fatal_error(NULL, "new table capacity is smaller than the older one");
    }
    blok_Table * new_ptr = blok_table_allocate(table->scope, new_cap);
    blok_Table new = *new_ptr;
    for(uint64_t i = 0; i < table->cap; ++i) {
        blok_KeyValue item = table->items[i];
        if(!blok_symbol_empty(item.key)) {
            blok_table_set(&new, item.key, item.value);
        }
    }
    //TODO examine closely memory management in this function
    //blok_scope_reclaim(table->items);
    //blok_scope_reclaim(b, new_ptr);
    *table = new;
}

blok_Obj blok_table_set(blok_Table * table, blok_Symbol key, blok_Obj value) {
    /*
    function set(key, value)
        i := find_slot(key)
        if slot[i] is occupied   // we found our key
            slot[i].value := value
            return
        if the table is almost full
            rebuild the table larger (note 1)
            i := find_slot(key)
        mark slot[i] as occupied
        slot[i].key := key
        slot[i].value := value
    */
    assert(table->scope != NULL);
    assert(table->cap != 0);

    /*
     if(table->cap == 0) {
        *table = blok_table_init_capacity(b, 8);
    } */

    if(table->count + 1 > table->cap / 3) {
        blok_table_rehash(table, table->cap * 3);
    }

    blok_KeyValue * kv = blok_table_find_slot(table, key);
    if (kv == NULL)
          fatal_error(NULL, "table cannot find slot for key: %s", key.buf);
    if(!blok_symbol_empty(kv->key)) {
        assert(blok_symbol_equal(kv->key, key));
        //blok_obj_free(kv->value);
        /*TODO clone value here to preserve value semantics on assignment*/
        kv->value = value;
        return blok_make_nil();
    } else { 
        kv->key = key;
        /*TODO clone value here to preserve value semantics on assignment*/
        kv->value = value;
        ++table->count;
        return blok_make_nil();
    }
}

blok_Obj blok_table_unset(blok_Table * table, blok_Symbol key) {
    (void) table;
    (void) key;
    /*
    function remove(key)
        i := find_slot(key)
        if slot[i] is unoccupied
            return   // key is not in the table
        mark slot[i] as unoccupied
        j := i
        loop (note 2)
            j := (j + 1) modulo num_slots
            if slot[j] is unoccupied
                exit loop
            k := hash(slot[j].key) modulo num_slots
            // determine if k lies cyclically in (i,j]
            // i ≤ j: |    i..k..j    |
            // i > j: |.k..j     i....| or |....j     i..k.|
            if i ≤ j
                if (i < k) and (k ≤ j)
                    continue loop
            else
                if (k ≤ j) or (i < k)
                    continue loop
            mark slot[i] as occupied
            slot[i].key := slot[j].key
            slot[i].value := slot[j].value
            mark slot[j] as unoccupied
            i := j
    */
    fatal_error(NULL, "TODO implement unset");
    return blok_make_nil();
}

/*
void blok_table_empty(blok_Scope * b, blok_Table * table) {
    blok_table_iterate_start(table);
    blok_KeyValue * pair = NULL;
    while((pair = blok_table_iterate_next(table)) != NULL) {
        blok_obj_free(b, pair->value);
    }
    free(table->items);
    memset(table, 0, sizeof(blok_Table));
}
*/

void blok_table_run_tests(void) {
    blok_Scope b = {0};
    blok_Table * table = blok_table_allocate(&b, 16);
    assert(table->cap == 16);
    assert(table->count == 0);
    assert(table->iterate_i == 0);
    assert(table->items != NULL);

    blok_table_set(table, blok_symbol_from_char_ptr("hello"), blok_make_int(10));
    assert(table->cap == 16);
    assert(table->count == 1);
    assert(table->iterate_i == 0);
    assert(table->items != NULL);

    blok_Obj value =
        blok_table_get(&b, table, blok_symbol_from_char_ptr("hello"));
    assert(value.tag != BLOK_TAG_NIL);
    assert(value.as.data == 10);
    assert(table->cap == 16);
    assert(table->count == 1);
    assert(table->iterate_i == 0);
    assert(table->items != NULL);

    blok_scope_close(&b);
}

blok_Obj blok_obj_copy(blok_Scope * destination_scope, blok_Obj obj);
blok_List * blok_list_copy(blok_Scope * destination_scope, blok_List const * const list) {
    blok_List * result = blok_list_allocate(destination_scope, list->len);
    for(int32_t i = 0; i < list->len; ++i) {
        blok_list_append(result, list->items[i]);
    }
    return result;
}

blok_String * blok_string_copy(blok_Scope * destination_scope, blok_String const * const str) {
    blok_String * result = blok_string_allocate(destination_scope, str->len);
    strncpy(result->ptr, str->ptr, str->len + 1);
    return result;
}


blok_Symbol * blok_symbol_copy(blok_Scope * destination_scope, blok_Symbol const * const sym) {
    blok_Symbol * result = blok_symbol_allocate(destination_scope);
    memcpy(result->buf, sym->buf, sizeof(result->buf));
    return result;
}


/* All objects use value semantics, so they should be copied when being assigned
 * or passed as parameters
 */
blok_Obj blok_obj_copy(blok_Scope * destination_scope, blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_FALSE:
        case BLOK_TAG_TRUE:
        case BLOK_TAG_NIL:
        case BLOK_TAG_PRIMITIVE:
        case BLOK_TAG_INT:
            return obj;
        case BLOK_TAG_LIST:
            return blok_obj_from_list(blok_list_copy(destination_scope, blok_list_from_obj(obj)));
        case BLOK_TAG_STRING:
            return blok_obj_from_string(blok_string_copy(destination_scope, blok_string_from_obj(obj)));
        case BLOK_TAG_SYMBOL:
            return blok_obj_from_symbol(blok_symbol_copy(destination_scope, blok_symbol_from_obj(obj)));
            break;
        case BLOK_TAG_KEYVALUE:
            fatal_error(NULL, "TODO");
            break;
        case BLOK_TAG_TABLE:
            fatal_error(NULL, "TODO");
        case BLOK_TAG_FUNCTION:
            fatal_error(NULL, "TODO");
    }
    return blok_make_nil();
}

/*
void blok_obj_free(blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_LIST:
            blok_list_free(blok_list_from_obj(obj));
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
        case BLOK_TAG_FUNCTION:
            {
                blok_Function * fn = blok_function_from_obj(obj);
                blok_list_free(fn->body);
                blok_list_free(fn->params);
                free(fn);
            }
        case BLOK_TAG_TRUE:
        case BLOK_TAG_FALSE:
        case BLOK_TAG_INT:
        case BLOK_TAG_NIL:
        case BLOK_TAG_PRIMITIVE:
            //No freeing needed for these types
            break;
    }
}
*/

typedef enum {
    BLOK_STYLE_AESTHETIC,
    BLOK_STYLE_CODE
} blok_Style;

void blok_obj_print(blok_Obj obj, blok_Style style);

void blok_keyvalue_print(blok_KeyValue * const kv, blok_Style style) {
    printf("%s:", kv->key.buf);
    blok_obj_print(kv->value, style);
}

void blok_table_print(blok_Table * const table, blok_Style style) {
    blok_table_iterate_start(table);
    blok_KeyValue * kv = NULL;
    bool first = true;
    printf("[");
    while((kv = blok_table_iterate_next(table)) != NULL) {
        if(!first) printf(" ");
        first = false;
        printf("%s:", kv->key.buf);
        blok_obj_print(kv->value, style);
    }
    printf("]");
}

void blok_print_escape_sequences(const char * str) {
    for(;*str != 0; ++str) {
        switch(*str) {
            case '\t':
                printf("\\t");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\\':
                printf("\\\\");
                break;
            default:
                printf("%c", *str);
                break;
        }
    }
}

void blok_obj_print(blok_Obj obj, blok_Style style) {
    switch(obj.tag) {
        case BLOK_TAG_NIL: 
            printf("nil");
            break;
        case BLOK_TAG_INT:
            printf("%d", obj.as.data);
            break;
        case BLOK_TAG_PRIMITIVE:
            printf("<Primitive %d>", obj.as.data);
            break;
        case BLOK_TAG_LIST:
            {
                blok_List * list = blok_list_from_obj(obj);
                printf("(");
                for(int i = 0; i < list->len; ++i) {
                    if(i != 0) printf(" ");
                    blok_obj_print(list->items[i], style);
                }
                printf(")");
            }
            break;
        case BLOK_TAG_STRING:
            switch(style) {
                case BLOK_STYLE_AESTHETIC:
                printf("%s", blok_string_from_obj(obj)->ptr);
                break;
                case BLOK_STYLE_CODE:
                printf("\"");
                blok_print_escape_sequences(blok_string_from_obj(obj)->ptr);
                printf("\"");
                break;
            }
            break;
        case BLOK_TAG_SYMBOL:
            printf("%s", blok_symbol_from_obj(obj)->buf);
            break;
        case BLOK_TAG_KEYVALUE:
            blok_keyvalue_print(blok_keyvalue_from_obj(obj), style);
            break;
        case BLOK_TAG_TABLE:
            blok_table_print(blok_table_from_obj(obj), style);
        default:
            printf("<Unprintable %s>", blok_tag_get_name(obj.tag));
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

blok_Obj blok_reader_parse_string(blok_Scope * b, FILE * fp) {
    assert(blok_reader_peek(fp) == '"');
    fgetc(fp);
    int state = BLOK_READER_STATE_BASE;
    blok_Obj str = blok_make_string(b, "");


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
                    case 't':
                        blok_string_append(str, '\t');
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


blok_Obj blok_reader_parse_obj(blok_Scope * b, FILE * fp);

blok_Obj blok_reader_parse_symbol_or_keyvalue(blok_Scope * b, FILE * fp) {
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
        blok_KeyValue* kv = blok_keyvalue_allocate(b);
        fgetc(fp); /*skip ':'*/
        kv->key = sym; 
        kv->value = blok_reader_parse_obj(b, fp);
        return blok_obj_from_keyvalue(kv);
    } else {
        return blok_make_symbol(b, sym.buf);
    }
}

//blok_Obj blok_reader_read_obj(FILE *);
blok_Obj blok_reader_parse_obj(blok_Scope * b, FILE * fp) {
    blok_reader_skip_whitespace(fp);
    const char ch = blok_reader_peek(fp);

    if(isdigit(ch)) {
        return blok_reader_parse_int(fp); 
    } else if(ch == '(') {
        blok_List * result = blok_list_allocate(b, 8);
        blok_List * tmp = blok_list_allocate(b, 8);
        int32_t sublist_count = 0;
        fgetc(fp);
        blok_reader_skip_whitespace(fp);
        while(blok_reader_peek(fp) != ')' && !feof(fp)) {
            if(blok_reader_peek(fp) == ',') {
                fgetc(fp);
                blok_list_append(result, blok_obj_from_list(tmp));
                tmp = blok_list_allocate(b, 8);
                ++sublist_count;
            } else {
                blok_list_append(tmp, blok_reader_parse_obj(b, fp));
                blok_reader_skip_whitespace(fp);
            }
        }
        if (blok_reader_peek(fp) != ')')
            fatal_error(fp, "Expected close parenthesis, found %c",
                    blok_reader_peek(fp));
        fgetc(fp);
        if(sublist_count == 0 ) {
            free(result);
            return blok_obj_from_list(tmp);
        } else {
            blok_list_append(result, blok_obj_from_list(tmp));
            return blok_obj_from_list(result);
        }
    } else if(ch == '"') {
        return blok_reader_parse_string(b, fp);
    } else if(isalpha(ch)) {
        return blok_reader_parse_symbol_or_keyvalue(b, fp);
    } else {
        fatal_error(fp, "Parsing failed");
    }
    return blok_make_nil();
}

blok_Obj blok_reader_read_file(blok_Scope * b, FILE * fp) {
    blok_List * result = blok_list_allocate(b, 8);
    blok_list_append(result, blok_make_symbol(b, "progn"));
    blok_reader_skip_whitespace(fp);
    while(!feof(fp) && blok_reader_peek(fp) != ')') {
        blok_list_append(result, blok_reader_parse_obj(b, fp));
        blok_reader_skip_whitespace(fp);
    }
    return blok_obj_from_list(result);
}


blok_Obj blok_scope_lookup(blok_Scope * b, blok_Symbol sym) {
    blok_Obj result = blok_table_get(b, b->bindings, sym);
    if(result.tag == BLOK_TAG_NIL && b->parent != NULL) {
        result = blok_scope_lookup(b->parent, sym);
    }
    return result;
}

void blok_scope_bind(blok_Scope * b, blok_Symbol sym, blok_Obj value) {
    assert(blok_scope_lookup(b, sym) == NULL && "Multiply defined symbol");
    blok_table_set(b, &b->bindings, sym, value);
}


void blok_scope_bind_builtins(blok_Scope * b) {
    blok_scope_bind(b, blok_symbol_from_char_ptr("print"),
                   blok_make_primitive(BLOK_PRIMITIVE_PRINT));
    blok_scope_bind(b, blok_symbol_from_char_ptr("progn"),
                   blok_make_primitive(BLOK_PRIMITIVE_PROGN));
    blok_scope_bind(b, blok_symbol_from_char_ptr("defun"),
                   blok_make_primitive(BLOK_PRIMITIVE_DEFUN));
    blok_scope_bind(b, blok_symbol_from_char_ptr("list"),
                   blok_make_primitive(BLOK_PRIMITIVE_LIST));
    blok_scope_bind(b, blok_symbol_from_char_ptr("quote"),
                   blok_make_primitive(BLOK_PRIMITIVE_QUOTE));
    blok_scope_bind(b, blok_symbol_from_char_ptr("true"),
                   blok_make_true());
    blok_scope_bind(b, blok_symbol_from_char_ptr("false"),
                   blok_make_false());
    blok_scope_bind(b, blok_symbol_from_char_ptr("when"),
                   blok_make_primitive(BLOK_PRIMITIVE_WHEN));
    blok_scope_bind(b, blok_symbol_from_char_ptr("equal"),
                   blok_make_primitive(BLOK_PRIMITIVE_EQUAL));
    blok_scope_bind(b, blok_symbol_from_char_ptr("and"),
                   blok_make_primitive(BLOK_PRIMITIVE_AND));
    blok_scope_bind(b, blok_symbol_from_char_ptr("lt"),
                   blok_make_primitive(BLOK_PRIMITIVE_LT));
    blok_scope_bind(b, blok_symbol_from_char_ptr("gt"),
                   blok_make_primitive(BLOK_PRIMITIVE_GT));
    blok_scope_bind(b, blok_symbol_from_char_ptr("lte"),
                   blok_make_primitive(BLOK_PRIMITIVE_LTE));
    blok_scope_bind(b, blok_symbol_from_char_ptr("gte"),
                   blok_make_primitive(BLOK_PRIMITIVE_GTE));
    blok_scope_bind(b, blok_symbol_from_char_ptr("not"),
                   blok_make_primitive(BLOK_PRIMITIVE_NOT));
    blok_scope_bind(b, blok_symbol_from_char_ptr("add"),
                   blok_make_primitive(BLOK_PRIMITIVE_ADD));
    blok_scope_bind(b, blok_symbol_from_char_ptr("sub"),
                   blok_make_primitive(BLOK_PRIMITIVE_SUB));
    blok_scope_bind(b, blok_symbol_from_char_ptr("mul"),
                   blok_make_primitive(BLOK_PRIMITIVE_MUL));
    blok_scope_bind(b, blok_symbol_from_char_ptr("div"),
                   blok_make_primitive(BLOK_PRIMITIVE_DIV));
    blok_scope_bind(b, blok_symbol_from_char_ptr("unless"),
                   blok_make_primitive(BLOK_PRIMITIVE_UNLESS));
    blok_scope_bind(b, blok_symbol_from_char_ptr("if"),
                   blok_make_primitive(BLOK_PRIMITIVE_IF));
}

blok_Obj blok_evaluator_eval(blok_Scope * b, blok_Obj obj);

blok_Obj blok_evaluator_apply_function(blok_Scope *b, blok_Function const * fn,
                                       int32_t argc, blok_Obj *argv) {
    assert(argc == fn->params->len && "Wrong number of arguments");
    blok_Scope fn_bindings = {0};
    for(int32_t i = 0; i < argc; ++i) {
        blok_scope_bind(&fn_bindings,
                *blok_symbol_from_obj(fn->params->items[i]), argv[i]);
    }
    fn_bindings.parent = b;
    blok_Obj result = blok_make_nil();
    for(int32_t i = 0; i < fn->body->len; ++i) {
        result = blok_evaluator_eval(&fn_bindings, fn->body->items[i]);
    }
    return result;
}

blok_Obj blok_evaluator_apply_primitive(blok_Scope *b, blok_Primitive prim,
                                        int32_t argc, blok_Obj *argv) {
    switch(prim) {
        case BLOK_PRIMITIVE_PRINT:
            if(argc > 0) {
                blok_obj_print(blok_evaluator_eval(b, argv[0]), BLOK_STYLE_AESTHETIC); 
                blok_evaluator_apply_primitive(b, prim, argc - 1, ++argv);
            }
            break;
        case BLOK_PRIMITIVE_PROGN:
            {
                blok_Obj result = blok_make_nil();
                for(int32_t i = 0; i < argc; ++i) {
                    //blok_obj_free(result);
                    result = blok_evaluator_eval(b, argv[i]);
                }
                return result;
            }
        case BLOK_PRIMITIVE_LIST:
            {
                blok_List * result = blok_list_allocate(argc);
                for(int32_t i = 0; i < argc; ++i) {
                    blok_list_append(result, blok_evaluator_eval(b, argv[i]));
                }
                return blok_obj_from_list(result);
            }
        case BLOK_PRIMITIVE_QUOTE:
            assert(argc <= 1);
            return argv[0];
        case BLOK_PRIMITIVE_DEFUN:
            {
                assert(argc >= 3);
                blok_Obj name = argv[0];
                blok_List * params = blok_list_from_obj(argv[1]);
                blok_List * body = blok_list_allocate(4);
                for(int i = 2; i < argc; ++i) {
                    blok_list_append(body, argv[i]);
                }
                blok_scope_bind(b, *blok_symbol_from_obj(name), blok_make_function(params, body));

                return blok_make_nil();

                /*TODO, do typechecking of calls in the body*/
            }
        case BLOK_PRIMITIVE_EQUAL:
            assert(argc == 2);
            return blok_obj_equal(blok_evaluator_eval(b, argv[0]),
                                  blok_evaluator_eval(b, argv[1]))
                       ? blok_make_true()
                       : blok_make_false();
        case BLOK_PRIMITIVE_WHEN:
            {
                assert(argc >= 2);
                blok_Obj cond = blok_evaluator_eval(b, argv[0]);
                if(cond.tag == BLOK_TAG_FALSE) {
                    return blok_make_nil();
                } else {
                    blok_Obj result = blok_make_nil();
                    for(int32_t i = 1; i < argc; ++i) {
                        //blok_obj_free(result);
                        result = blok_evaluator_eval(b, argv[i]);
                    }
                    return result;
                }
            }
        case BLOK_PRIMITIVE_UNLESS:
            {
                assert(argc >= 2);
                blok_Obj cond = blok_evaluator_eval(b, argv[0]);
                if(cond.tag == BLOK_TAG_FALSE) {
                    blok_Obj result = blok_make_nil();
                    for(int32_t i = 1; i < argc; ++i) {
                        //blok_obj_free(result);
                        result = blok_evaluator_eval(b, argv[i]);
                    }
                    return result;
                } else {
                    return blok_make_nil();
                } 
            }
        case BLOK_PRIMITIVE_IF:
            assert(argc >= 3);
            blok_Obj cond = blok_evaluator_eval(b, argv[0]);
            if(cond.tag == BLOK_TAG_FALSE) {
                return blok_evaluator_eval(b, argv[2]);
            } else {
                return blok_evaluator_eval(b, argv[1]);
            }
        case BLOK_PRIMITIVE_SET:
            assert(argc == 2 && "Expects key then value");
            blok_Symbol sym = *blok_symbol_from_obj(argv[0]);
            blok_Obj * old = blok_scope_lookup(b, sym);
            if(old == NULL) {
                fatal_error(NULL, "Undefined symbol: %s", sym.buf);
            }
            blok_scope_bind(b, sym, blok_obj_copy(argv[1]));
        case BLOK_PRIMITIVE_AND:
            {
                assert(argc == 2);
                blok_Obj lhs = blok_evaluator_eval(b, argv[0]);
                blok_Obj rhs = blok_evaluator_eval(b, argv[1]);
                return blok_make_boolean(lhs.tag != BLOK_TAG_FALSE &&
                                         rhs.tag != BLOK_TAG_FALSE);
            }
        case BLOK_PRIMITIVE_GT:
            assert(argc == 2);
            if(argv[0].tag != BLOK_TAG_INT && argv[1].tag != BLOK_TAG_INT) {
                fatal_error(NULL, "Cannot compare non-integer values");
            }
            return blok_make_boolean(blok_evaluator_eval(b, argv[0]).as.data >
                                     blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_GTE:
            assert(argc == 2);
            if(argv[0].tag != BLOK_TAG_INT && argv[1].tag != BLOK_TAG_INT) {
                fatal_error(NULL, "Cannot compare non-integer values");
            }
            return blok_make_boolean(blok_evaluator_eval(b, argv[0]).as.data >=
                                     blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_LT:
            assert(argc == 2);
            if(argv[0].tag != BLOK_TAG_INT && argv[1].tag != BLOK_TAG_INT) {
                fatal_error(NULL, "Cannot compare non-integer values");
            }
            return blok_make_boolean(blok_evaluator_eval(b, argv[0]).as.data <
                                     blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_LTE:
            assert(argc == 2);
            if(argv[0].tag != BLOK_TAG_INT && argv[1].tag != BLOK_TAG_INT) {
                fatal_error(NULL, "Cannot compare non-integer values");
            }
            return blok_make_boolean(blok_evaluator_eval(b, argv[0]).as.data <
                                     blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_NOT:
            assert(argc == 1);
            return blok_make_boolean(blok_evaluator_eval(b, argv[0]).tag == BLOK_TAG_FALSE ? true
                                                                   : false);
        case BLOK_PRIMITIVE_ADD:
            assert(argc == 2);
            return blok_make_int(blok_evaluator_eval(b, argv[0]).as.data +
                                 blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_SUB:
            assert(argc == 2);
            return blok_make_int(blok_evaluator_eval(b, argv[0]).as.data -
                                 blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_MUL:
            assert(argc == 2);
            return blok_make_int(blok_evaluator_eval(b, argv[0]).as.data *
                                 blok_evaluator_eval(b, argv[1]).as.data);
        case BLOK_PRIMITIVE_DIV:
            assert(argc == 2);
            return blok_make_int(blok_evaluator_eval(b, argv[0]).as.data /
                                 blok_evaluator_eval(b, argv[1]).as.data);
    }

    return blok_make_nil();
}

blok_Obj blok_evaluator_apply_list(blok_Scope * b, blok_List * list) {
    /*
    (void)b;
    (void)sym;
    (void)argc;
    (void)argv;
    */


    if(list->len <= 0) {
        fatal_error(NULL, "cannot apply empty list");
    }
    blok_Obj fn = blok_evaluator_eval(b, list->items[0]);
    if(fn.tag == BLOK_TAG_PRIMITIVE) {
        return blok_evaluator_apply_primitive(b, fn.as.data, list->len - 1, &list->items[1]);
    } else if(fn.tag == BLOK_TAG_FUNCTION) {
        return blok_evaluator_apply_function(b, fn.as.ptr, list->len - 1, &list->items[1]);
        fatal_error(NULL, "TODO implement functions");
    } else {
        printf("\n\nEvaluated ");
        blok_obj_print(list->items[0], BLOK_STYLE_CODE);
        printf(" into ");
        blok_obj_print(fn, BLOK_STYLE_CODE);
        puts("\"");
        fatal_error(NULL, "Note, this value cannot be applied");
    }

    return blok_make_nil();
}

blok_Obj blok_evaluator_eval(blok_Scope * b, blok_Obj obj) {
    (void)b;
    switch(obj.tag) {
        case BLOK_TAG_NIL:
        case BLOK_TAG_INT:
        case BLOK_TAG_STRING:
            return obj;
        case BLOK_TAG_SYMBOL:
            {

                assert(blok_scope_lookup(b, blok_symbol_from_char_ptr("print"))->as.data == BLOK_PRIMITIVE_PRINT);
                blok_Symbol * sym = blok_symbol_from_obj(obj);
                blok_Symbol direct = *sym;
                blok_Obj * value = blok_scope_lookup(b, direct);
                if(value == NULL) {
                    fatal_error(NULL, "tried to evaluate null symbol");
                }
                return *value;
            }
        case BLOK_TAG_LIST:
            {
                /*printf("-->Evaluating ");
                blok_obj_print(obj, BLOK_STYLE_CODE);
                printf("\n");*/
                fflush(stdout);
                blok_List * list = blok_list_from_obj(obj);
                return blok_evaluator_apply_list(b, list);
            }
        default:
          fatal_error(NULL,
                      "Support for evaluating this type has not been "
                      "implemented yet: %s",
                      blok_char_ptr_from_tag(obj.tag));
          break;

    }

    return blok_make_nil();
}

int main(void) {
    blok_Scope global = {0};
    blok_scope_bind_builtins(&global);

    blok_table_run_tests();

    FILE * fp = fopen("test.lisp", "r");
    blok_Obj source = blok_reader_read_file(fp);
    /*blok_obj_print(source, BLOK_STYLE_CODE);*/

    blok_evaluator_eval(&global, source);
    blok_scope_close(&global);

    fclose(fp);
    return 0;
}
