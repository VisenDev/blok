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

#define BLOK_NORETURN __attribute__((noreturn))

BLOK_NORETURN 
void blok_abort(void) {
    fflush(stdout);
    fflush(stderr);
    abort();
}


typedef struct {
    int line;
    int column;
    char file[128];
} blok_SourceInfo;

void blok_print_sourceinfo(FILE * fp, blok_SourceInfo src_info) {
    fprintf(fp, "%s:%d:%d\n", src_info.file, src_info.line, src_info.column); 
}

BLOK_NORETURN 
void blok_fatal_error(blok_SourceInfo * src_info, const char * restrict fmt, ...) {
    fprintf(stderr, "ERROR: \n");
    if(src_info != NULL) {
        blok_print_sourceinfo(stderr, *src_info);
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    blok_abort();
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
    BLOK_TAG_BOOL,
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
        case BLOK_TAG_BOOL:     return "BLOK_TAG_BOOL";
    }
}


typedef struct {
    blok_Tag tag; 
    union {
        void * ptr;
        int data;
    } as;

    /*debugging info*/
    blok_SourceInfo src_info;
} blok_Obj;

typedef struct { 
    int32_t len;
    int32_t cap;
    blok_Obj * items;
    blok_Arena * arena; /*the scope in which new items are allocated*/
} blok_List;

typedef struct {
    size_t len;
    size_t cap;
    char * ptr;
    blok_Arena * arena;
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
    blok_Arena * arena; /*the scope in which new items are allocated*/
} blok_Table;

typedef struct {
    blok_List * params;
    blok_List * body;
} blok_Function;


typedef struct blok_Scope {
    struct blok_Scope * parent;
    blok_Arena arena;
    blok_Table bindings;
    /*blok_Table * const builtins;*/
} blok_Scope;


void blok_scope_close(blok_Scope * b) {
    blok_arena_free(&b->arena);
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
    BLOK_PRIMITIVE_MOD,
    BLOK_PRIMITIVE_IF,
    BLOK_PRIMITIVE_UNLESS,
    BLOK_PRIMITIVE_ASSERT,
    BLOK_PRIMITIVE_WHILE,
} blok_Primitive;


blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj);

void * blok_ptr_from_obj(blok_Obj obj) {
    if (obj.tag == BLOK_TAG_INT
    ||  obj.tag == BLOK_TAG_NIL
    ||  obj.tag == BLOK_TAG_BOOL
    ||  obj.tag == BLOK_TAG_PRIMITIVE) {
        blok_fatal_error(NULL,
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

blok_Symbol * blok_symbol_allocate(blok_Arena * a) {
    blok_Symbol * result = blok_arena_alloc(a, sizeof(blok_Symbol));
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
    return (blok_Obj){.tag = BLOK_TAG_BOOL, .as.data = 1};
}
blok_Obj blok_make_false(void) {
    return (blok_Obj){.tag = BLOK_TAG_BOOL, .as.data = 0};
}
blok_Obj blok_make_bool(bool cond) {
    return cond ? blok_make_true() : blok_make_false();
}

blok_Function * blok_function_allocate(blok_Arena * a) {
    blok_Function * fn = blok_arena_alloc(a, sizeof(blok_Function));
    memset(fn, 0, sizeof(blok_Function));
    return fn;
}

blok_List * blok_list_copy(blok_Arena * destination_scope, blok_List const * const list);
blok_Obj blok_make_function(blok_Arena * a, blok_List * params, blok_List * body) {
    blok_Function * result = blok_function_allocate(a);
    result->params = blok_list_copy(a, params);
    result->body = blok_list_copy(a, body);
    return blok_obj_from_function(result);
}


blok_List * blok_list_allocate(blok_Arena * a, int32_t initial_capacity) {
    assert(a != NULL);
    blok_List * result = blok_arena_alloc(a, sizeof(blok_List));
    assert(result != NULL);
    result->len = 0;
    result->cap = initial_capacity;
    result->arena = a;
    result->items = blok_arena_alloc(a, result->cap * sizeof(blok_Obj));
    assert(result->items != NULL);
    return result;
}

blok_KeyValue * blok_keyvalue_allocate(blok_Arena * a) {
    blok_KeyValue * result = blok_arena_alloc(a, sizeof(blok_KeyValue));
    //result->arena = a;
    //assert(result->arena != NULL);
    return result;
}
/*
void blok_keyvalue_set(blok_KeyValue * kv, blok_Obj value) {
    kv->value = blok_obj_copy(kv->arena, value);
}
blok_Obj blok_keyvalue_get(blok_Arena * destination_arena, blok_KeyValue * kv) {
    return blok_obj_copy(destination_arena, kv->value);
}
*/

blok_String * blok_string_allocate(blok_Arena * b, uint64_t capacity) {
    blok_String * blok_str = blok_arena_alloc(b, sizeof(blok_String));
    assert(((uintptr_t)blok_str & 0xf) == 0);

    blok_str->len = 0;
    blok_str->cap = 2 * capacity + 1;
    blok_str->ptr = blok_arena_alloc(b, blok_str->cap);
    blok_str->arena = b;
    memset(blok_str->ptr, 0, capacity);
    return blok_str;
}

blok_Obj blok_make_string(blok_Arena * b, const char * str) {
    const int32_t len = strlen(str);
    blok_String * result = blok_string_allocate(b, len);
    strcpy(result->ptr, str);
    result->len = len;
    return blok_obj_from_ptr(result, BLOK_TAG_STRING);
}

blok_Obj blok_make_symbol(blok_Arena * b, const char * str) {
    blok_Symbol * sym = blok_arena_alloc(b, sizeof(blok_Symbol));
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
                blok_fatal_error(NULL, "Comparison for this tag not implemented yet");
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

void blok_list_append(blok_List* l, blok_Obj item) {
    if(l->len + 1 >= l->cap) {
        l->cap = l->cap * 2 + 1;
        l->items = blok_arena_realloc(l->arena, l->items, l->cap * sizeof(blok_Obj));
    }
    l->items[l->len++] = blok_obj_copy(l->arena, item);
}

void blok_string_append(blok_String * l, char ch) {
    if(l->len + 2 >= l->cap) {
        l->cap = (l->cap + 2) * 2;
        l->ptr = blok_arena_realloc(l->arena, l->ptr, l->cap);
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

blok_Table blok_table_init_capacity(blok_Arena * a, size_t cap) {
    blok_Table table = {0};
    const uint64_t bytes = sizeof(blok_KeyValue) * cap;
    table.items = blok_arena_alloc(a, bytes);
    memset(table.items, 0, bytes);
    table.count = 0;
    table.iterate_i = 0;
    table.cap = cap;
    table.arena = a;
    return table;
}


blok_Scope blok_scope_open(blok_Scope * parent_scope) {
    blok_Scope result = {0};
    result.parent = parent_scope;
    result.bindings = blok_table_init_capacity(&result.arena, 8);
    return result;
}

blok_Table * blok_table_allocate(blok_Arena * a, size_t cap) {
    blok_Table * ptr = blok_arena_alloc(a, sizeof(blok_Table));
    *ptr = blok_table_init_capacity(a, cap);
    return ptr;
}


blok_Obj blok_make_table(blok_Arena * a, int32_t cap) {
    return blok_obj_from_table(blok_table_allocate(a, cap));
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
    assert(table != NULL);
    assert(table->arena != NULL);
    assert(table->cap > 0);

    uint64_t i = blok_hash(key) % (table->cap - 1);
    assert(i < table->cap);

    const uint64_t start = i;
    do {
        blok_KeyValue kv = table->items[i];
        if(blok_symbol_equal(kv.key, key)) {
            return &table->items[i];
        } else if(blok_symbol_empty(kv.key)) {
            return &table->items[i];
        } else {
            ++i;
            i = i % table->cap;
        }
    } while(i != start);
    return NULL;
}

bool blok_table_contains_key(blok_Table * table, blok_Symbol key) {
    return blok_table_find_slot(table, key) == NULL;
}

blok_Obj blok_table_get(blok_Arena * destination_scope, blok_Table * table, blok_Symbol key) {
    blok_KeyValue * kv = blok_table_find_slot(table, key);
    if(kv == NULL) {
        return blok_make_nil();
    } else if(blok_symbol_empty(kv->key)) {
        return blok_make_nil();
    } else {
        return blok_obj_copy(destination_scope, kv->value);
    }
}

blok_Obj blok_table_set(blok_Table * table, blok_Symbol key, blok_Obj value);

void blok_table_rehash(blok_Table * table, uint64_t new_cap) {
    if(new_cap < table->count) {
        blok_fatal_error(NULL, "new table capacity is smaller than the older one");
    }
    blok_Table new = blok_table_init_capacity(table->arena, new_cap);
    for(uint64_t i = 0; i < table->cap; ++i) {
        blok_KeyValue item = table->items[i];
        if(!blok_symbol_empty(item.key)) {
            blok_table_set(&new, item.key, item.value);
        }
    }
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

    assert(table->arena != NULL);
    assert(table->cap != 0);

    if(table->count + 1 > table->cap / 3) {
        blok_table_rehash(table, table->cap * 3);
    }

    blok_KeyValue * kv = blok_table_find_slot(table, key);
    if (kv == NULL)
          blok_fatal_error(NULL, "table cannot find slot for key: %s", key.buf);
    if(!blok_symbol_empty(kv->key)) {
        assert(blok_symbol_equal(kv->key, key));
        //blok_obj_free(kv->value);
        kv->value = blok_obj_copy(table->arena, value);
        return blok_make_nil();
    } else { 
        kv->key = key;
        kv->value = blok_obj_copy(table->arena, value);
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
    blok_fatal_error(NULL, "TODO implement unset");
    return blok_make_nil();
}

/*
void blok_table_empty(blok_Arena * b, blok_Table * table) {
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
    blok_Arena a = {0};
    blok_Table table = blok_table_init_capacity(&a, 16);
    assert(table.cap == 16);
    assert(table.count == 0);
    assert(table.iterate_i == 0);
    assert(table.items != NULL);

    blok_table_set(&table, blok_symbol_from_char_ptr("hello"), blok_make_int(10));
    assert(table.cap == 16);
    assert(table.count == 1);
    assert(table.iterate_i == 0);
    assert(table.items != NULL);

    blok_Obj value =
        blok_table_get(&a, &table, blok_symbol_from_char_ptr("hello"));
    assert(value.tag != BLOK_TAG_NIL);
    assert(value.as.data == 10);
    assert(table.cap == 16);
    assert(table.count == 1);
    assert(table.iterate_i == 0);
    assert(table.items != NULL);
    (void) value;

    blok_arena_free(&a);
}

blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj);

blok_List * blok_list_copy(blok_Arena * destination_scope, blok_List const * const list) {
    blok_List * result = blok_list_allocate(destination_scope, list->len);
    for(int32_t i = 0; i < list->len; ++i) {
        blok_list_append(result, list->items[i]);
    }
    return result;
}

blok_String * blok_string_copy(blok_Arena * destination_scope, blok_String * str) {
    size_t len = strlen(str->ptr);
    if(str->len != len) {
        printf("str->len: %zu, strlen(str->ptr): %zu", str->len, len);
    }
    blok_String * result = blok_string_allocate(destination_scope, str->cap);

    strcpy(result->ptr, str->ptr);
    result->len = str->len;
    return result;
}


blok_Symbol * blok_symbol_copy(blok_Arena * destination_scope, blok_Symbol * sym) {
    blok_Symbol * result = blok_symbol_allocate(destination_scope);
    memcpy(result->buf, sym->buf, sizeof(result->buf));
    return result;
}

blok_KeyValue * blok_keyvalue_copy(blok_Arena * destination_scope, blok_KeyValue * kv) {
    blok_KeyValue * result  = blok_keyvalue_allocate(destination_scope);
    result->key = kv->key;
    result->value = blok_obj_copy(destination_scope, kv->value);
    return result;
}

blok_Table * blok_table_copy(blok_Arena * destination_scope, blok_Table * table) {
    blok_Table * result = blok_table_allocate(destination_scope, table->cap);
    blok_table_iterate_start(table);
    blok_KeyValue * item = NULL;
    while((item = blok_table_iterate_next(table)) != NULL) {
        blok_table_set(result, item->key, item->value);
    }
    return result;
}

blok_Function * blok_function_copy(blok_Arena * destination_scope, blok_Function * function) {
    blok_Function * result = blok_function_allocate(destination_scope);
    result->body = blok_list_copy(destination_scope, function->body);
    result->params = blok_list_copy(destination_scope, function->params);
    return result;
}

/* All objects use value semantics, so they should be copied when being assigned
 * or passed as parameters
 */
blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_BOOL:
        case BLOK_TAG_NIL:
        case BLOK_TAG_PRIMITIVE:
        case BLOK_TAG_INT:
            return obj;
        case BLOK_TAG_LIST:
            {
                blok_Obj result = blok_obj_from_list(blok_list_copy(destination_scope, blok_list_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
        case BLOK_TAG_STRING:
            {
                blok_Obj result = blok_obj_from_string(blok_string_copy(destination_scope, blok_string_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
        case BLOK_TAG_SYMBOL:
            {
                blok_Obj result = blok_obj_from_symbol(blok_symbol_copy(destination_scope, blok_symbol_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
        case BLOK_TAG_KEYVALUE:
            {
                blok_Obj result =  blok_obj_from_keyvalue(blok_keyvalue_copy(destination_scope, blok_keyvalue_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
        case BLOK_TAG_TABLE:
            {
                blok_Obj result =  blok_obj_from_table(blok_table_copy(destination_scope, blok_table_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
        case BLOK_TAG_FUNCTION:
            {
                blok_Obj result =blok_obj_from_function(blok_function_copy(destination_scope, blok_function_from_obj(obj)));
                result.src_info = obj.src_info;
                return result;
            }
    }
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

void blok_print_escape_sequences(const char * str, size_t max) {
    for(size_t i = 0; i < max && *str != 0; ++str, ++i) {
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
                blok_String * str = blok_string_from_obj(obj);
                blok_print_escape_sequences(str->ptr, 32);
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
typedef struct {
    FILE * fp;
    blok_SourceInfo src_info;
} blok_Reader;

bool blok_reader_is_whitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\t';
}

char blok_reader_getc(blok_Reader * r) {
    char ch = fgetc(r->fp);
    if(ch == '\n') {
        ++r->src_info.line;
        r->src_info.column = 0;
    } else {
        ++r->src_info.column;
    }
    return ch;
}

bool blok_reader_eof(blok_Reader * r) {
    return feof(r->fp);
}

char blok_reader_peek(blok_Reader * r) {
    char ch = fgetc(r->fp);
    ungetc(ch, r->fp);
    return ch;
}

void blok_reader_skip_whitespace(blok_Reader * r) {
    if(blok_reader_eof(r)) return;
    while(blok_reader_is_whitespace(blok_reader_peek(r))) blok_reader_getc(r);
}

void blok_reader_skip_char(blok_Reader * r, char expected) {
    char ch = blok_reader_getc(r);
    if(ch != expected) {
        blok_fatal_error(&r->src_info, "Reader expected '%c', got '%c'", expected, ch);
    }
}

blok_Obj blok_reader_parse_int(blok_Reader * r) {
    assert(isdigit(blok_reader_peek(r)));
    char buf[1024] = {0};
    size_t i = 0;
    while(isdigit(blok_reader_peek(r))) {
        buf[i++] = blok_reader_getc(r);
        assert(i < sizeof(buf));
    }
    char * end;
    errno = 0;
    const long num = strtol(buf, &end, 10);
    if(errno != 0) {
        blok_fatal_error(&r->src_info, "Failed to parse integer, errno: %n", errno);
    }
    blok_Obj result = blok_make_int(num);
    result.src_info = r->src_info;
    return result;
}


#define BLOK_READER_STATE_BASE 0
#define BLOK_READER_STATE_ESCAPE 1

blok_Obj blok_reader_parse_string(blok_Arena * b, blok_Reader * r) {
    blok_reader_skip_char(r, '"');
    int state = BLOK_READER_STATE_BASE;
    blok_String * str = blok_string_allocate(b, 8);
    while(1) {
        char ch = blok_reader_peek(r);
        if(blok_reader_eof(r)) blok_fatal_error(&r->src_info, "Unexpected end of file when parsing string");
        switch(state) {
            case BLOK_READER_STATE_BASE:
                if(ch == '\\') {
                    state = BLOK_READER_STATE_ESCAPE;
                } else if (ch == '"') {
                    blok_reader_skip_char(r, '"');
                    blok_Obj result =  blok_obj_from_string(str);
                    result.src_info = r->src_info;
                    return result;
                } else {
                    blok_string_append(str, blok_reader_getc(r));
                }
                break;
            case BLOK_READER_STATE_ESCAPE:
                blok_reader_skip_char(r, '\\');
                char escape = blok_reader_getc(r);
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
                        blok_fatal_error(&r->src_info, "Unknown string escape: \"\\%c\"", escape);
                        break;
                }
                state = BLOK_READER_STATE_BASE;
                break;
        }
    }
}

bool blok_reader_is_symbol_char(char ch) {
    return ch == '_' || ch == '[' || ch == ']' || isalpha(ch) || isdigit(ch);
}

blok_Obj blok_reader_parse_obj(blok_Arena * b, blok_Reader * r);

blok_Obj blok_reader_parse_symbol(blok_Arena * b, blok_Reader* r) {
    assert(isalpha(blok_reader_peek(r)));
    blok_Symbol sym = {0};
    uint32_t i = 0;
    char ch = blok_reader_getc(r);
    sym.buf[i++] = ch;
    while(blok_reader_is_symbol_char(blok_reader_peek(r))) {
        sym.buf[i++] = blok_reader_getc(r);
        if(i + 2 > sizeof(sym.buf)) {
            blok_fatal_error(&r->src_info, "symbol too long, %s\n", sym.buf);
        }
    }

    if(blok_reader_peek(r) == ':') {
        blok_KeyValue* kv = blok_keyvalue_allocate(b);
        blok_reader_skip_char(r, ':');
        kv->key = sym; 
        kv->value = blok_reader_parse_obj(b, r);
        blok_Obj result = blok_obj_from_keyvalue(kv);
        result.src_info = r->src_info;
        return result;
    } else {
        blok_Obj result =  blok_make_symbol(b, sym.buf);
        result.src_info = r->src_info;
        return result;
    }
}

blok_Obj blok_reader_parse_list(blok_Arena * a, blok_Reader * r) {
        blok_reader_skip_char(r, '(');
        blok_reader_skip_whitespace(r);

#       define BLOK_LIST_MAX_SUBLISTS 32
        int sublist_count = 0;
        blok_List * sublists[BLOK_LIST_MAX_SUBLISTS];

        do {
            if(blok_reader_peek(r) == ',') {
                blok_reader_skip_char(r, ',');
            }
            if(sublist_count > BLOK_LIST_MAX_SUBLISTS) {
                blok_fatal_error(&r->src_info, "List contains too many sublists, the maximum amount of sublists is %d", BLOK_LIST_MAX_SUBLISTS);
            }
            sublists[sublist_count++] = blok_list_allocate(a, 4);

            while(blok_reader_peek(r) != ')' && blok_reader_peek(r) != ',' && !blok_reader_eof(r)) {
                blok_list_append(sublists[sublist_count - 1], blok_reader_parse_obj(a, r));
                blok_reader_skip_whitespace(r);
            }

        } while(blok_reader_peek(r) == ',');
        blok_reader_skip_char(r, ')');
        blok_reader_skip_whitespace(r);
        blok_List * result = NULL;
        if(sublist_count <= 1) {
            result = sublists[0];
        } else {
            result = blok_list_allocate(a, sublist_count);
            for(int i = 0; i < sublist_count; ++i) {
                blok_list_append(result, blok_obj_from_list(sublists[i]));
            }
        }

        blok_Obj result_obj = blok_obj_from_list(result);
        result_obj.src_info = r->src_info;
        return result_obj;
}

//blok_Obj blok_reader_read_obj(FILE *);
blok_Obj blok_reader_parse_obj(blok_Arena * a, blok_Reader * r) {
    blok_reader_skip_whitespace(r);
    const char ch = blok_reader_peek(r);

    if(isdigit(ch)) {
        return blok_reader_parse_int(r); 
    } else if(ch == '(') {
        return blok_reader_parse_list(a, r);
    } else if(ch == '"') {
        return blok_reader_parse_string(a, r);
    } else if(isalpha(ch)) {
        return blok_reader_parse_symbol(a, r);
    } else {
        blok_fatal_error(&r->src_info, "Encountered unexpected character '%c' when parsing object", ch);
    }
    /*return blok_make_nil();*/
}

blok_Obj blok_reader_read_file(blok_Arena * a, char const * path) {
    blok_List * result = blok_list_allocate(a, 32);

    blok_Reader r = {0};
    r.fp = fopen(path, "r");

    //size_t len = strlen(path);
    //char * path_copy = blok_arena_alloc(a, len + 1);
    //strncpy(path_copy, path, len);
    strncpy(r.src_info.file, path, sizeof(r.src_info.file));
    r.src_info.line = 1;

    if(r.fp == NULL) {
        blok_fatal_error(NULL, "Failed to open file: %s\n", path);
    }
    blok_list_append(result, blok_make_symbol(a, "progn"));
    blok_reader_skip_whitespace(&r);
    while(!blok_reader_eof(&r) && blok_reader_peek(&r) != ')') {
        blok_list_append(result, blok_reader_parse_obj(a, &r));
        blok_reader_skip_whitespace(&r);
    }
    fclose(r.fp);
    return blok_obj_from_list(result);
}


blok_Obj blok_scope_lookup(blok_Arena * destination_arena, blok_Scope * b, blok_Symbol sym) {
    blok_Obj result = blok_table_get(destination_arena, &b->bindings, sym);
    if(result.tag == BLOK_TAG_NIL && b->parent != NULL) {
        result = blok_scope_lookup(destination_arena, b->parent, sym);
    }
    return result;
}

bool blok_scope_symbol_bound(blok_Scope * b, blok_Symbol sym) {
    for(;b != NULL; b = b->parent) {
        if(blok_table_contains_key(&b->bindings, sym)) return true;
    }
    return false;
}

void blok_scope_bind(blok_Scope * b, blok_Symbol sym, blok_Obj value) {
    if(blok_scope_symbol_bound(b, sym)) {
        blok_fatal_error(NULL, "Symbol already bound: %s", sym.buf);
    }
    blok_table_set(&b->bindings, sym, value);
}

void blok_scope_rebind(blok_Scope * b, blok_Symbol sym, blok_Obj value) {
    if(blok_scope_symbol_bound(b, sym)) {
        blok_fatal_error(NULL, "Symbol is unbound: %s", sym.buf);
    }
    blok_table_set(&b->bindings, sym, value);
}


void blok_scope_bind_builtins(blok_Scope * b) {
#   define BLOK_BIND_PRIMITIVE(scope, name, primitive) \
        blok_scope_bind(scope, blok_symbol_from_char_ptr(name), \
           blok_make_primitive(primitive));

    BLOK_BIND_PRIMITIVE(b, "print", BLOK_PRIMITIVE_PRINT);
    BLOK_BIND_PRIMITIVE(b, "progn", BLOK_PRIMITIVE_PROGN);
    BLOK_BIND_PRIMITIVE(b, "defun", BLOK_PRIMITIVE_DEFUN);
    BLOK_BIND_PRIMITIVE(b, "list", BLOK_PRIMITIVE_LIST);
    BLOK_BIND_PRIMITIVE(b, "quote", BLOK_PRIMITIVE_QUOTE);
    BLOK_BIND_PRIMITIVE(b, "when", BLOK_PRIMITIVE_WHEN);
    BLOK_BIND_PRIMITIVE(b, "equal", BLOK_PRIMITIVE_EQUAL);
    BLOK_BIND_PRIMITIVE(b, "and", BLOK_PRIMITIVE_AND);
    BLOK_BIND_PRIMITIVE(b, "lt", BLOK_PRIMITIVE_LT);
    BLOK_BIND_PRIMITIVE(b, "gt", BLOK_PRIMITIVE_GT);
    BLOK_BIND_PRIMITIVE(b, "lte", BLOK_PRIMITIVE_LTE);
    BLOK_BIND_PRIMITIVE(b, "gte", BLOK_PRIMITIVE_GTE);
    BLOK_BIND_PRIMITIVE(b, "not", BLOK_PRIMITIVE_NOT);
    BLOK_BIND_PRIMITIVE(b, "add", BLOK_PRIMITIVE_ADD);
    BLOK_BIND_PRIMITIVE(b, "sub", BLOK_PRIMITIVE_SUB);
    BLOK_BIND_PRIMITIVE(b, "mul", BLOK_PRIMITIVE_MUL);
    BLOK_BIND_PRIMITIVE(b, "div", BLOK_PRIMITIVE_DIV);
    BLOK_BIND_PRIMITIVE(b, "unless", BLOK_PRIMITIVE_UNLESS);
    BLOK_BIND_PRIMITIVE(b, "if", BLOK_PRIMITIVE_IF);
    BLOK_BIND_PRIMITIVE(b, "assert", BLOK_PRIMITIVE_ASSERT);
    BLOK_BIND_PRIMITIVE(b, "while", BLOK_PRIMITIVE_WHILE);
    BLOK_BIND_PRIMITIVE(b, "set", BLOK_PRIMITIVE_SET);

#   undef BLOK_BIND_PRIMITIVE

    //blok_scope_bind(b, blok_symbol_from_char_ptr("true"),
    //               blok_make_true());
    //blok_scope_bind(b, blok_symbol_from_char_ptr("false"),
    //               blok_make_false());
}

blok_Obj blok_evaluator_eval(blok_Scope * b, blok_Obj obj);

blok_Obj blok_evaluator_apply_function(
        blok_Scope *b,
        blok_Function const * fn,
        int32_t argc,
        blok_Obj *argv
        ) {
    assert(argc == fn->params->len && "Wrong number of arguments");
    blok_Scope fn_local = blok_scope_open(b);

    for(int32_t i = 0; i < argc; ++i) {
      blok_scope_bind(&fn_local, *blok_symbol_from_obj(fn->params->items[i]),
                      blok_evaluator_eval(b, argv[i]));
    }
    blok_Obj result = blok_make_nil();
    for(int32_t i = 0; i < fn->body->len; ++i) {
        result = blok_evaluator_eval(&fn_local, fn->body->items[i]);
    }

    blok_Obj return_value = blok_obj_copy(&b->arena, result);
    blok_scope_close(&fn_local);
    return return_value;
}

blok_Obj blok_evaluator_apply_primitive(
        blok_Scope * b,
        blok_Primitive prim,
        int32_t argc,
        blok_Obj *argv
        ) {
    switch(prim) {
        case BLOK_PRIMITIVE_PRINT:
            if(argc > 0) {
                assert(argv != NULL);
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
                blok_List * result = blok_list_allocate(&b->arena, argc);
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
                blok_List * body = blok_list_allocate(&b->arena, 4);
                for(int i = 2; i < argc; ++i) {
                    blok_list_append(body, argv[i]);
                }
                blok_scope_bind(b, *blok_symbol_from_obj(name), blok_make_function(&b->arena, params, body));

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
                if(cond.tag != BLOK_TAG_BOOL) {
                    blok_fatal_error(NULL, "Expected a boolean");
                } else if(cond.as.data) {
                    blok_Obj result = blok_make_nil();
                    for(int32_t i = 1; i < argc; ++i) {
                        result = blok_evaluator_eval(b, argv[i]);
                    }
                    return result;
                } else {
                    return blok_make_nil();
                }
            }
        case BLOK_PRIMITIVE_UNLESS:
            {

                assert(argc >= 2);
                blok_Obj cond = blok_evaluator_eval(b, argv[0]);
                if(cond.tag != BLOK_TAG_BOOL) {
                    blok_fatal_error(NULL, "Expected a boolean");
                } else if(!cond.as.data) {
                    blok_Obj result = blok_make_nil();
                    for(int32_t i = 1; i < argc; ++i) {
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
            if(cond.tag != BLOK_TAG_BOOL) {
                blok_fatal_error(NULL, "Expected a boolean");
            } else if(cond.as.data) {
                return blok_evaluator_eval(b, argv[1]);
            } else {
                return blok_evaluator_eval(b, argv[2]);
            }
        case BLOK_PRIMITIVE_SET:
            {
                assert(argc == 2 && "Expects key then value");
                blok_Symbol * sym = blok_symbol_from_obj(argv[0]);
                blok_Obj val = blok_evaluator_eval(b, argv[1]);
                blok_scope_bind(b, *sym, val);
                return val;
            }
        case BLOK_PRIMITIVE_AND:
            {
                assert(argc == 2);
                blok_Obj lhs = blok_evaluator_eval(b, argv[0]);
                blok_Obj rhs = blok_evaluator_eval(b, argv[1]);
                if(lhs.tag != BLOK_TAG_BOOL || rhs.tag != BLOK_TAG_BOOL) {
                    printf("(and ");
                    blok_obj_print(lhs, BLOK_STYLE_CODE);
                    printf(" ");
                    blok_obj_print(rhs, BLOK_STYLE_CODE);
                    printf(")\n");
                    fflush(stdout);
                    blok_fatal_error(NULL, "expected two booleans for 'and'");
                }
                return blok_make_bool(lhs.as.data && rhs.as.data);
            }
        case BLOK_PRIMITIVE_NOT:
            {
                assert(argc == 1);
                blok_Obj val = blok_evaluator_eval(b, argv[0]);
                if(val.tag != BLOK_TAG_BOOL) {
                    blok_fatal_error(NULL, "Cannot NOT a non-boolean");
                }
                return blok_make_bool(!val.as.data);
            }

        case BLOK_PRIMITIVE_WHILE:
            {
                assert(argc >= 1);
                blok_Obj result = blok_make_nil();
                while(blok_evaluator_eval(b, argv[0]).as.data) {
                    for(int32_t i = 1; i < argc; ++i) {
                        result = blok_evaluator_eval(b, argv[i]);
                    }
                }
                return result;
            }
        case BLOK_PRIMITIVE_ASSERT:
            assert(argc == 1);
            {
                blok_Obj val = blok_evaluator_eval(b, argv[0]);
                if(val.tag != BLOK_TAG_BOOL) {
                    blok_fatal_error(NULL, "Expected a boolean inside the assert");
                }
                if(!val.as.data) {
                    printf("\n\nAssertion Failed: ");
                    blok_obj_print(argv[0], BLOK_STYLE_CODE);
                    blok_fatal_error(NULL, "");
                }
                return blok_make_true();
            }

#       define BLOK_PRIMITIVE_IMPL_BOOL(primitive, operator) \
            case primitive: \
                { \
                    assert(argc == 2); \
                    blok_Obj lhs = blok_evaluator_eval(b, argv[0]); \
                    blok_Obj rhs = blok_evaluator_eval(b, argv[1]); \
                    /*if(lhs.tag != BLOK_TAG_BOOL || rhs.tag != BLOK_TAG_BOOL) { \
                        blok_fatal_error(NULL, "Cannot use boolean operator on non-boolean values"); \
                    } \*/ \
                    return blok_make_bool(lhs.as.data operator rhs.as.data); \
                }

        BLOK_PRIMITIVE_IMPL_BOOL(BLOK_PRIMITIVE_GT, >);
        BLOK_PRIMITIVE_IMPL_BOOL(BLOK_PRIMITIVE_GTE, >=);
        BLOK_PRIMITIVE_IMPL_BOOL(BLOK_PRIMITIVE_LT, <);
        BLOK_PRIMITIVE_IMPL_BOOL(BLOK_PRIMITIVE_LTE, <=);

#       undef BLOK_PRIMITIVE_IMPL_BOOL
#       define BLOK_PRIMITIVE_IMPL_OPERATOR(primitive, operator) \
            case primitive: \
                { \
                    assert(argc == 2); \
                    blok_Obj lhs = blok_evaluator_eval(b, argv[0]); \
                    blok_Obj rhs = blok_evaluator_eval(b, argv[1]); \
                    if(lhs.tag != BLOK_TAG_INT || rhs.tag != BLOK_TAG_INT) { \
                        printf("Cannot perform " ); \
                        printf("(%s ", #operator); \
                        blok_obj_print(lhs, BLOK_STYLE_CODE); \
                        printf(" "); \
                        blok_obj_print(rhs, BLOK_STYLE_CODE); \
                        printf(")"); \
                        fflush(stdout); \
                        blok_fatal_error(NULL, ""); \
                    } \
                    return blok_make_int(lhs.as.data operator rhs.as.data); \
                }

        BLOK_PRIMITIVE_IMPL_OPERATOR(BLOK_PRIMITIVE_ADD, +);
        BLOK_PRIMITIVE_IMPL_OPERATOR(BLOK_PRIMITIVE_SUB, -);
        BLOK_PRIMITIVE_IMPL_OPERATOR(BLOK_PRIMITIVE_MUL, *);
        BLOK_PRIMITIVE_IMPL_OPERATOR(BLOK_PRIMITIVE_DIV, /);
        BLOK_PRIMITIVE_IMPL_OPERATOR(BLOK_PRIMITIVE_MOD, %);

#       undef BLOK_PRIMITIVE_IMPL_OPERATOR


    }

    return blok_make_nil();
}

blok_Obj blok_evaluator_apply_list(blok_Scope * b,  blok_List * list) {
    if(list->len <= 0) {
        blok_fatal_error(NULL, "cannot apply empty list");
    }
    blok_Obj fn = blok_evaluator_eval(b, list->items[0]);
    if(fn.tag == BLOK_TAG_PRIMITIVE) {
        return blok_evaluator_apply_primitive(b, fn.as.data, list->len - 1, &list->items[1]);
    } else if(fn.tag == BLOK_TAG_FUNCTION) {
        return blok_evaluator_apply_function(b, fn.as.ptr, list->len - 1, &list->items[1]);
    } else {
        printf("\n\nEvaluated ");
        blok_obj_print(list->items[0], BLOK_STYLE_CODE);
        printf(" into ");
        blok_obj_print(fn, BLOK_STYLE_CODE);
        puts("\"");
        blok_fatal_error(NULL, "Note, this value cannot be applied");
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

                blok_Arena tmp = {0};
                assert(blok_scope_lookup(&tmp, b, blok_symbol_from_char_ptr("print")).as.data == BLOK_PRIMITIVE_PRINT);
                blok_arena_free(&tmp);
                blok_Symbol * sym = blok_symbol_from_obj(obj);
                blok_Symbol direct = *sym;
                blok_Obj value = blok_scope_lookup(&b->arena, b, direct);
                if(value.tag == BLOK_TAG_NIL) {
                    blok_fatal_error(&obj.src_info, "Tried to evaluate undefined symbol '%s'", sym->buf);
                }
                return value;
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
          blok_fatal_error(NULL,
                      "Support for evaluating this type has not been "
                      "implemented yet: %s",
                      blok_tag_get_name(obj.tag));
          break;

    }

    return blok_make_nil();
}

void blok_sema_analyze(blok_Obj src) {
    (void)src;
    
}

int main(void) {
    blok_arena_run_tests();
    blok_table_run_tests();

    blok_Scope b = {0};
    b.bindings = blok_table_init_capacity(&b.arena, 32);
    blok_scope_bind_builtins(&b);

    blok_Obj source = blok_reader_read_file(&b.arena, "ideal.blok");
    //blok_obj_print(source, BLOK_STYLE_CODE);
    //fflush(stdout);

    blok_evaluator_eval(&b, source);
    blok_arena_free(&b.arena);

    return 0;
}
