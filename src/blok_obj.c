#ifndef BLOK_OBJ_C
#define BLOK_OBJ_C

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <stdint.h>

#include "blok_arena.c"
#include "blok_vec.c"

#define BLOK_NORETURN __attribute__((noreturn))
#define BLOK_LOG(...) do { fprintf(stderr, "LOG:" __VA_ARGS__ ); fflush(stderr); } while(0)

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
void blok_fatal_error_internal(
        blok_SourceInfo *src_info, const char *c_file,
        int c_line, const char *restrict fmt, ...) {
    fprintf(stderr, "\n\nERROR: \n    ");
    if (src_info != NULL) {
        blok_print_sourceinfo(stderr, *src_info);
    }
    fprintf(stderr, "    Note, Error emitted from\n");
    fprintf(stderr, "        %s:%d:0:\n    ", c_file, c_line);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    blok_abort();
}

/*note, fmt is passed as part of __VA_ARGS__ to avoid the extra comma problem*/
#define blok_fatal_error(src_info, /*fmt,*/ ...) \
    blok_fatal_error_internal(src_info, __FILE__, __LINE__, __VA_ARGS__ )



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
    BLOK_TAG_TYPE,
    BLOK_TAG_VARIABLE,
} blok_Tag;

typedef int32_t blok_SymbolId;
#define BLOK_SYMBOLID_NIL 0 
typedef int32_t blok_TypeId;
#define BLOK_TYPEID_NIL 0

typedef enum {
    /*ANYTYPE*/
    BLOK_TYPETAG_OBJ,

    /*FUNDAMENTAL TYPES*/
    BLOK_TYPETAG_INT,
    BLOK_TYPETAG_BOOLEAN,
    BLOK_TYPETAG_STRING,
    BLOK_TYPETAG_SYMBOL,
    BLOK_TYPETAG_TYPE,

    /*COMPOUND TYPES*/
    BLOK_TYPETAG_LIST,
    BLOK_TYPETAG_STRUCT,
    BLOK_TYPETAG_UNION,
    BLOK_TYPETAG_OPTIONAL
} blok_TypeTag;

typedef struct {
    blok_TypeId item_type;
} blok_ListType;

typedef struct {
    blok_SymbolId name;
    blok_TypeId type;
} blok_FieldType;

typedef struct {
    blok_FieldType * items; /*fields*/
    int32_t len;
    int32_t cap;
} blok_StructType;

typedef struct {
    blok_FieldType * items; /*fields*/
    int32_t len;
    int32_t cap;
} blok_UnionType;

typedef struct {
    blok_TypeId type;
} blok_OptionalType;

typedef struct blok_Type {
    blok_TypeTag tag; 
    union {
        blok_ListType list;
        blok_UnionType union_;
        blok_StructType struct_;
        blok_OptionalType optional;
    } as;
} blok_Type;


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
        case BLOK_TAG_BOOL:      return "BLOK_TAG_BOOL";
        case BLOK_TAG_TYPE:      return "BLOK_TAG_TYPE";
        case BLOK_TAG_VARIABLE:   return "BLOK_TAG_VARIABLE";
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

typedef enum {
    BLOK_SUFFIX_NIL = '\0',
    BLOK_SUFFIX_ASTERISK = '*',
    BLOK_SUFFIX_BRACKET_PAIR = '[',
} blok_Suffix;

#define BLOK_SYMBOL_MAX_LEN 64
#define BLOK_SYMBOL_MAX_SUFFIX_COUNT 8
typedef struct {
    char buf[BLOK_SYMBOL_MAX_LEN];
    blok_Suffix suffix[BLOK_SYMBOL_MAX_SUFFIX_COUNT];
} blok_Symbol;

typedef struct {
    blok_SymbolId key;
    blok_Obj value;
} blok_KeyValue;

typedef struct {
    uint32_t count;
    uint32_t cap;
    uint32_t iterate_i;
    blok_KeyValue * items;
    blok_Arena * arena; /*the scope in which new items are allocated*/
} blok_Table;


#define BLOK_PARAMETER_COUNT_MAX 8
typedef struct blok_FunctionSignature {
    blok_Type return_type;
    blok_Type params[BLOK_PARAMETER_COUNT_MAX];
    int32_t param_count;
} blok_FunctionSignature;

typedef struct {
    blok_Arena * arena;
    blok_FunctionSignature signature;
    blok_List * params;
    blok_List * body;
} blok_Function;

typedef struct {
    blok_Type type;
    blok_Obj value;
} blok_Binding;

typedef struct blok_ToplevelScope {
    blok_Arena arena;
    blok_Table builtin_bindings;
    blok_Table interpreted_bindings;
    blok_Table compiled_bindings;

    /*DEBUG INFO*/
    const char * filename;
} blok_Toplevel;

/*
typedef struct blok_LocalScope {
    struct blok_LocalScope * parent;
    blok_Toplevel * toplevel;
    blok_Arena arena;
    blok_Table bindings;
} blok_Scope;
*/

typedef struct {
    blok_Arena persistent_arena;
    blok_Vec(blok_Type) types; 
    blok_Vec(blok_Symbol) symbols;
    blok_Vec(blok_Arena) arenas;
} blok_State;

blok_Symbol blok_symbol_from_id(const blok_State * s, blok_SymbolId id) {
    assert(id != 0 && "0 is the NULL symbol");
    assert(id > 0);
    assert(id <= s->symbols.len);
    return s->symbols.items[id - 1];
}

bool blok_symbol_equal(blok_Symbol lhs, blok_Symbol rhs);

blok_SymbolId blok_symbol_intern(blok_State * s, blok_Symbol sym) {
    for(blok_SymbolId i = 0; i < s->symbols.len; ++i) {
        if(blok_symbol_equal(sym, s->symbols.items[i])) {
            return i + 1;
        }
    }
    blok_vec_append(&s->symbols, &s->persistent_arena, sym);
    return s->symbols.len;
}

blok_SymbolId blok_symbol_intern_str(blok_State * s, const char * symbol) {
    blok_Symbol sym = {0};
    assert(strlen(symbol) < sizeof(sym.buf));
    strcpy(sym.buf, symbol);
    return blok_symbol_intern(s, sym);
}



blok_Type blok_type_from_id(const blok_State * s, blok_TypeId id) {
    assert(id >= 0);
    assert(id < s->types.len);
    return s->types.items[id];
}

/*
 * TODO: uncomment this
bool blok_type_equal(blok_Type lhs, blok_Type rhs);

blok_TypeId blok_type_intern(blok_State * s, blok_Type sym) {
    for(blok_TypeId i = 0; i < s->types.len; ++i) {
        if(blok_type_equal(sym, s->types.items[i])) {
            return i;
        }
    }
    blok_vec_append(&s->types, &s->persistent_arena, sym);
    return s->types.len - 1;
}
*/


//void blok_scope_close(blok_Scope * s) {
//    blok_arena_free(&s->arena);
//}



blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj);

//void * blok_ptr_from_obj(blok_Obj obj) {
//    if (obj.tag == BLOK_TAG_INT
//    ||  obj.tag == BLOK_TAG_NIL
//    ||  obj.tag == BLOK_TAG_BOOL
//    ||  obj.tag == BLOK_TAG_PRIMITIVE) {
//        blok_fatal_error(NULL,
//                "You tried to get the pointer out of a type that does not "
//                "contain a ptr");
//    }
//    obj.tag = 0;
//    return obj.as.ptr;
//}

//blok_SymbolId blok_symbol_from_string(blok_State * s, const char * str) {
//    blok_Symbol sym = {0};
//    //strncpy(sym.buf, ptr, sizeof(sym.buf) - 1);
//    //return sym;
//}

//blok_Symbol * blok_symbol_allocate(blok_Arena * a) {
//    blok_Symbol * result = blok_arena_alloc(a, sizeof(blok_Symbol));
//    memset(result, 0, sizeof(blok_Symbol));
//    return result;
//}

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
blok_Obj blok_obj_from_function(blok_Function * f) { return blok_obj_from_ptr(f, BLOK_TAG_FUNCTION); }

blok_Obj blok_make_int(int32_t data) {
    return (blok_Obj){.tag = BLOK_TAG_INT, .as.data = data};
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

blok_Obj blok_make_symbol(blok_SymbolId sym) {
    blok_Obj result = {0};
    result.as.data = sym;
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

blok_SymbolId blok_symbol_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_SYMBOL);
    return obj.as.data;
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

bool blok_symbol_streql(blok_State * s, const blok_SymbolId sym, const char * str) {
    blok_Symbol symbol = blok_symbol_from_id(s, sym);
    return strcmp(symbol.buf, str) == 0;
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
                return blok_symbol_from_obj(lhs) == blok_symbol_from_obj(rhs);
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


//bool blok_symbol_empty(const blok_State * s, blok_SymbolId sym) {
//}

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

//uint64_t blok_hash_char_ptr(char * const ptr, uint64_t len) {
//    /* Inspired by djbt2 by Dan Bernstein - http://www.cse.yorku.ca/~oz/hash.html */
//    uint64_t hash = 5381;
//    uint64_t i = 0;
//
//    for(i = 0; i < len; ++i) {
//        const unsigned char c = (unsigned char)ptr[i];
//        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//    }
//
//    return hash;
//}
//
//
//int32_t blok_hash(blok_Symbol sym) {
//    return blok_hash_char_ptr(sym.buf, (char*)memchr(sym.buf, 0, BLOK_SYMBOL_MAX_LEN) - sym.buf);
//}

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


//blok_Scope blok_scope_open(blok_Scope * parent_scope) {
//    blok_Scope result = {0};
//    result.parent = parent_scope;
//    result.bindings = blok_table_init_capacity(&result.arena, 8);
//    return result;
//}

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
        if(table->items[table->iterate_i].key != BLOK_SYMBOLID_NIL) {
            return &table->items[table->iterate_i++];
        }
    }
    return NULL;
}

void blok_table_iterate_start(blok_Table * table) {
    table->iterate_i = 0;
}

blok_KeyValue * blok_table_find_slot(blok_Table * table, blok_SymbolId key) {
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

    uint64_t i = key % (table->cap - 1);
    assert(i < table->cap);

    const uint64_t start = i;
    do {
        blok_KeyValue kv = table->items[i];
        if(kv.key == key) {
            return &table->items[i];
        } else if(kv.key == BLOK_SYMBOLID_NIL) {
            return &table->items[i];
        } else {
            ++i;
            i = i % table->cap;
        }
    } while(i != start);
    return NULL;
}

bool blok_table_contains_key(blok_Table * table, blok_SymbolId key) {
    return blok_table_find_slot(table, key) == NULL;
}

blok_Obj blok_table_get(blok_Arena * destination_scope, blok_Table * table, blok_SymbolId key) {
    blok_KeyValue * kv = blok_table_find_slot(table, key);
    if(kv == NULL) {
        return blok_make_nil();
    } else if(kv->key == BLOK_SYMBOLID_NIL) {
        return blok_make_nil();
    } else {
        return blok_obj_copy(destination_scope, kv->value);
    }
}

blok_Obj blok_table_set(blok_Table * table, blok_SymbolId key, blok_Obj value);

void blok_table_rehash(blok_Table * table, uint64_t new_cap) {
    if(new_cap < table->count) {
        blok_fatal_error(NULL, "new table capacity is smaller than the older one");
    }
    blok_Table new = blok_table_init_capacity(table->arena, new_cap);
    for(uint64_t i = 0; i < table->cap; ++i) {
        blok_KeyValue item = table->items[i];
        if(item.key != BLOK_SYMBOLID_NIL) {
            blok_table_set(&new, item.key, item.value);
        }
    }
    *table = new;
}

blok_Obj blok_table_set(blok_Table * table, blok_SymbolId key, blok_Obj value) {
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
          blok_fatal_error(NULL, "table cannot find slot for key: %d", key);
    if(kv->key != BLOK_SYMBOLID_NIL) {
        assert(kv->key == key);
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
    blok_State s = {0};
    blok_Table table = blok_table_init_capacity(&a, 16);
    assert(table.cap == 16);
    assert(table.count == 0);
    assert(table.iterate_i == 0);
    assert(table.items != NULL);

    blok_table_set(&table, blok_symbol_intern_str(&s, "hello"), blok_make_int(10));
    assert(table.cap == 16);
    BLOK_LOG("table.count: %d\n", table.count);
    assert(table.count == 1);
    assert(table.iterate_i == 0);
    assert(table.items != NULL);

    blok_Obj value =
        blok_table_get(&a, &table, blok_symbol_intern_str(&s, "hello"));
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
        case BLOK_TAG_SYMBOL:
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
        case BLOK_TAG_TYPE:
            blok_fatal_error(NULL, "TODO");
            return blok_make_nil();
        case BLOK_TAG_VARIABLE:
            blok_fatal_error(NULL, "TODO");
            return blok_make_nil();
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

void blok_obj_fprint(blok_State * s, FILE * fp, blok_Obj obj, blok_Style style);


void blok_symbol_fprint(blok_State *s, FILE * fp, const blok_SymbolId symbol, blok_Style style) {
    (void) style;
    blok_Symbol sym = blok_symbol_from_id(s, symbol);
    fprintf(fp, "%s", sym.buf);
    for(const blok_Suffix * suffix = sym.suffix; *suffix != BLOK_SUFFIX_NIL; ++suffix) {
        switch(*suffix) {
            case BLOK_SUFFIX_ASTERISK:
                fprintf(fp, "*");
                break;
            case BLOK_SUFFIX_BRACKET_PAIR:
                fprintf(fp, "[]");
                break;
            case BLOK_SUFFIX_NIL:
                assert(0); /*This should be unreachable*/
        }
    }
}

void blok_keyvalue_fprint(blok_State * s, FILE * fp, blok_KeyValue * const kv, blok_Style style) {
    blok_symbol_fprint(s, fp, kv->key, style);
    fprintf(fp, ":");
    blok_obj_fprint(s, fp, kv->value, style);
}

void blok_table_fprint(blok_State * s, FILE * fp, blok_Table * const table, blok_Style style) {
    blok_table_iterate_start(table);
    blok_KeyValue * kv = NULL;
    bool first = true;
    fprintf(fp, "[");
    while((kv = blok_table_iterate_next(table)) != NULL) {
        if(!first) fprintf(fp, " ");
        first = false;
        blok_keyvalue_fprint(s, fp, kv, style);
    }
    fprintf(fp, "]");
}

void blok_fprint_escape_sequences(FILE * fp, const char * str, size_t max) {
    for(size_t i = 0; i < max && *str != 0; ++str, ++i) {
        switch(*str) {
            case '\t':
                fprintf(fp, "\\t");
                break;
            case '\n':
                fprintf(fp, "\\n");
                break;
            case '\\':
                fprintf(fp, "\\\\");
                break;
            default:
                fprintf(fp, "%c", *str);
                break;
        }
    }
}


void blok_obj_fprint(blok_State * s, FILE * fp, blok_Obj obj, blok_Style style) {
    switch(obj.tag) {
        case BLOK_TAG_NIL: 
            fprintf(fp, "nil");
            break;
        case BLOK_TAG_INT:
            fprintf(fp, "%d", obj.as.data);
            break;
        case BLOK_TAG_PRIMITIVE:
            fprintf(fp, "<Primitive %d>", obj.as.data);
            break;
        case BLOK_TAG_LIST:
            {
                blok_List * list = blok_list_from_obj(obj);
                fprintf(fp, "(");
                for(int i = 0; i < list->len; ++i) {
                    if(i != 0) fprintf(fp, " ");
                    blok_obj_fprint(s, fp, list->items[i], style);
                }
                fprintf(fp, ")");
            }
            break;
        case BLOK_TAG_STRING:
            switch(style) {
                case BLOK_STYLE_AESTHETIC:
                fprintf(fp, "%s", blok_string_from_obj(obj)->ptr);
                break;
                case BLOK_STYLE_CODE:
                fprintf(fp, "\"");
                blok_String * str = blok_string_from_obj(obj);
                blok_fprint_escape_sequences(fp, str->ptr, 32);
                fprintf(fp, "\"");
                break;
            }
            break;
        case BLOK_TAG_SYMBOL:
            blok_symbol_fprint(s, fp, blok_symbol_from_obj(obj), style);
            break;
        case BLOK_TAG_KEYVALUE:
            blok_keyvalue_fprint(s, fp, blok_keyvalue_from_obj(obj), style);
            break;
        case BLOK_TAG_TABLE:
            blok_table_fprint(s, fp, blok_table_from_obj(obj), style);
            break;
        default:
            fprintf(fp, "<Unprintable %s>", blok_tag_get_name(obj.tag));
            break;
    }
}


void blok_obj_print(blok_State * s, blok_Obj obj, blok_Style style) {
    blok_obj_fprint(s, stdout, obj, style);
}


void blok_print_as_bytes(blok_Obj obj) {
    uint8_t *bytes = (uint8_t*)&obj;
    for(int i = 0; i < 8; ++i) {
        printf("%d", bytes[i]);
    }
    puts("");
}

#endif /*BLOK_OBJ_C*/
