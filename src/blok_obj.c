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
#include "blok_profiler.c"

#define BLOK_NORETURN __attribute__((noreturn))
#define BLOK_LOG(...) do { fprintf(stderr, "LOG:" __VA_ARGS__ ); fflush(stderr); } while(0)

BLOK_NORETURN 
void blok_abort(void) {
    fflush(stdout);
    fflush(stderr);
    exit(1);
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
    BLOK_TAG_ALIST,
    BLOK_TAG_FUNCTION,
    BLOK_TAG_BOOL,
    BLOK_TAG_TYPE,
    BLOK_TAG_VARIABLE,
} blok_Tag;

typedef int32_t blok_Symbol;
#define BLOK_SYMBOL_NIL 0 

typedef int32_t blok_Type;

typedef enum {
    /*ANYTYPE*/
    BLOK_TYPETAG_OBJ,

    /*FUNCTION POINTERS*/
    BLOK_TYPETAG_FUNCTION,
    BLOK_TYPETAG_PRIMITIVE,

    /*EMPTY TYPE*/
    BLOK_TYPETAG_VOID,

    /*FUNDAMENTAL TYPES*/
    BLOK_TYPETAG_NIL,
    BLOK_TYPETAG_INT,
    BLOK_TYPETAG_BOOL,
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
    blok_Type item_type;
} blok_ListType;

typedef struct {
    blok_Symbol name;
    blok_Type type;
} blok_FieldType;

typedef blok_Vec(blok_FieldType) blok_Fields;

typedef struct {
    blok_Fields fields;
} blok_StructType;

typedef struct {
    blok_Fields fields;
} blok_UnionType;

typedef struct {
    blok_Type type;
} blok_OptionalType;

typedef struct {
    blok_Type type;
    bool noeval;
} blok_ParamType;

#define BLOK_PARAMETER_COUNT_MAX 8
typedef struct blok_Signature {
    blok_Type return_type;
    blok_ParamType params[BLOK_PARAMETER_COUNT_MAX];
    int32_t param_count;
    bool variadic;
    blok_ParamType variadic_args_type;
} blok_Signature;

typedef struct blok_TypeData {
    blok_TypeTag tag; 
    union {
        blok_ListType list;
        blok_UnionType union_;
        blok_StructType struct_;
        blok_OptionalType optional;
        blok_Signature function;
        blok_Signature primitive;
    } as;
} blok_TypeData;


const char * blok_tag_get_name(blok_Tag tag) {

    switch(tag) {
        case BLOK_TAG_NIL:       return "BLOK_TAG_NIL";
        case BLOK_TAG_INT:       return "BLOK_TAG_INT";
        case BLOK_TAG_LIST:      return "BLOK_TAG_LIST";
        case BLOK_TAG_PRIMITIVE: return "BLOK_TAG_PRIMITIVE";
        case BLOK_TAG_STRING:    return "BLOK_TAG_STRING";
        case BLOK_TAG_KEYVALUE:  return "BLOK_TAG_KEYVALUE";
        case BLOK_TAG_SYMBOL:    return "BLOK_TAG_SYMBOL";
        case BLOK_TAG_ALIST:     return "BLOK_TAG_ALIST";
        case BLOK_TAG_FUNCTION:  return "BLOK_TAG_FUNCTION";
        case BLOK_TAG_BOOL:      return "BLOK_TAG_BOOL";
        case BLOK_TAG_TYPE:      return "BLOK_TAG_TYPE";
        case BLOK_TAG_VARIABLE:  return "BLOK_TAG_VARIABLE";
    }

    assert(0 && "Unreachable");
    return NULL;
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

typedef blok_Slice(blok_Obj) blok_ListRef;
typedef struct { blok_ListRef items; int32_t cap; blok_Obj _item;} blok_List;
STATIC_ASSERT(sizeof(blok_List) == sizeof(blok_Vec(blok_Obj)), correct_vec_structure);
typedef blok_Vec(char) blok_String;

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
} blok_SymbolData;

typedef struct {
    blok_Symbol key;
    blok_Obj value;
} blok_KeyValue;

bool blok_paramtype_equal(blok_ParamType l, blok_ParamType r) {
    return l.type == r.type && l.noeval == r.noeval;
}

bool blok_functionsignature_equal(blok_Signature l, const blok_Signature r) {
    if(l.return_type != r.return_type) return false;
    if(l.param_count != r.param_count) return false;
    if(l.variadic != r.variadic) return false;
    if(l.variadic && r.variadic && !blok_paramtype_equal(l.variadic_args_type, r.variadic_args_type)) return false;
    assert(l.param_count == r.param_count);
    for(int32_t i = 0; i < l.param_count; ++i) {
        if(!blok_paramtype_equal(l.params[i], r.params[i])) return false;
    }
    return true;
}

typedef struct {
    //blok_Arena * arena;
    blok_Type signature;
    blok_List * params;
    blok_List * body;
} blok_Function;

typedef struct {
    blok_Symbol name;
    blok_Type type;
    blok_Obj value;
} blok_Binding;

typedef blok_Vec(blok_KeyValue) blok_AList;
typedef blok_Vec(blok_Binding) blok_Bindings;

typedef enum {
    BLOK_PRIMITIVE_TOPLEVEL_LET,
    BLOK_PRIMITIVE_TOPLEVEL_PROCEDURE,
    BLOK_PRIMITIVE_PRINT_INT,
    BLOK_PRIMITIVE_RETURN,
    BLOK_PRIMITIVE_WHEN,
    BLOK_PRIMITIVE_EXPR,
} blok_PrimitiveTag;

typedef struct {
    blok_Symbol name;
    blok_PrimitiveTag tag;
    blok_Type signature;
} blok_Primitive;

typedef blok_Vec(blok_Primitive) blok_Primitives;

typedef struct {
    blok_Arena persistent_arena;
    blok_Vec(blok_TypeData) types; 
    blok_Vec(blok_SymbolData) symbols;
    blok_Vec(blok_Arena) arenas;

    //blok_Bindings builtins;
    //Compilation specific 
    
    
    
    FILE * out;
    blok_Primitives toplevel_primitives;
    blok_Bindings globals;
    int indent;
} blok_State;




//blok_Obj blok_make_primitive(blok_Primitive data) {
//    return (blok_Obj){.tag = BLOK_TAG_PRIMITIVE, .as.data = data};
//}


blok_SymbolData blok_symbol_get_data(const blok_State * s, blok_Symbol id) {
    assert(id != 0 && "0 is the NULL symbol");
    assert(id > 0);
    assert(id <= s->symbols.items.len);
    return blok_slice_get(s->symbols.items, id - 1);
}

bool blok_symboldata_equal(blok_SymbolData lhs, blok_SymbolData rhs);

blok_Symbol blok_symboldata_intern(blok_State * s, blok_SymbolData sym) {
    for(blok_Symbol i = 0; i < s->symbols.items.len; ++i) {
        if(blok_symboldata_equal(sym, blok_slice_get(s->symbols.items, i))) {
            return i + 1;
        }
    }
    blok_vec_append(&s->symbols, &s->persistent_arena, sym);
    return s->symbols.items.len;
}

blok_Symbol blok_symbol_from_string(blok_State * s, const char * symbol) {
    blok_SymbolData sym = {0};
    assert(strlen(symbol) < sizeof(sym.buf));
    strcpy(sym.buf, symbol);
    return blok_symboldata_intern(s, sym);
}


blok_TypeData blok_type_get_data(const blok_State * s, blok_Type id) {
    assert(id != 0 && "0 is the NULL type");
    assert(id > 0);
    assert(id <= s->types.items.len);
    return blok_slice_get(s->types.items, id - 1);
}

//bool blok_paramtype_equal(blok_ParamType lhs, blok_ParamType rhs) {
//    if(lhs.type != rhs.type) return false;
//    if(lhs.type != rhs.type) return false;
//    return true; 
//}

bool blok_signature_equal(blok_Signature lhs, blok_Signature rhs) {
    if(lhs.param_count != rhs.param_count) return false;
    if(lhs.return_type != rhs.return_type) return false;
    if(lhs.variadic != rhs.variadic) return false;
    if(lhs.variadic) {
        if(!blok_paramtype_equal(lhs.variadic_args_type, rhs.variadic_args_type)) return false;
    }
    for(int i = 0; i < lhs.param_count; ++i) {
        if(!blok_paramtype_equal(lhs.params[i], rhs.params[i])) return false;
    }
    return true;
}

bool blok_typedata_equal(blok_TypeData lhs, blok_TypeData rhs) {
    if(lhs.tag != rhs.tag) return false;

    assert(lhs.tag == rhs.tag);
    switch(lhs.tag) {
        case BLOK_TYPETAG_BOOL:
        case BLOK_TYPETAG_VOID:
        case BLOK_TYPETAG_INT:
        case BLOK_TYPETAG_NIL:
        case BLOK_TYPETAG_STRING:
        case BLOK_TYPETAG_OBJ:
        case BLOK_TYPETAG_TYPE:
        case BLOK_TYPETAG_SYMBOL:
            return true;
        case BLOK_TYPETAG_FUNCTION:
            return blok_signature_equal(lhs.as.function, rhs.as.function);
        case BLOK_TYPETAG_PRIMITIVE:
            return blok_signature_equal(lhs.as.primitive, rhs.as.primitive);
        case BLOK_TYPETAG_LIST:
            return lhs.as.list.item_type == rhs.as.list.item_type;
        case BLOK_TYPETAG_OPTIONAL:
            return lhs.as.optional.type == rhs.as.optional.type;
        case BLOK_TYPETAG_STRUCT: {
            blok_Fields l = lhs.as.struct_.fields;
            blok_Fields r = rhs.as.struct_.fields;
            if(l.items.len != r.items.len) {
                return false;
            } else {
                for(int i = 0; i < l.items.len; ++i) {
                    blok_FieldType lf = l.items.ptr[i];
                    blok_FieldType rf = r.items.ptr[i];
                    if(lf.name != rf.name || lf.type != rf.type) return false;
                }
            }
            return true;
        }
        case BLOK_TYPETAG_UNION: {
            blok_Fields l = lhs.as.union_.fields;
            blok_Fields r = rhs.as.union_.fields;
            if(l.items.len != r.items.len) {
                return false;
            } else {
                for(int i = 0; i < l.items.len; ++i) {
                    blok_FieldType lf = l.items.ptr[i];
                    blok_FieldType rf = r.items.ptr[i];
                    if(lf.name != rf.name || lf.type != rf.type) return false;
                }
            }
            return true;
        }
    }
    assert(0 && "Unreachable");
    return false;
}

blok_Type blok_typedata_intern(blok_State * s, blok_TypeData type) {
    for(blok_Type i = 0; i < s->types.items.len; ++i) {
        if(blok_typedata_equal(type, blok_slice_get(s->types.items, i))) {
            return i + 1;
        }
    }
    blok_vec_append(&s->types, &s->persistent_arena, type);
    return s->types.items.len;
}

blok_Type blok_primitive_signature_intern(blok_State * s, blok_Signature sig) {
    //blok_TypeData * t = blok_arena_alloc(&s->persistent_arena, sizeof(blok_TypeData));
    blok_TypeData t = {0};
    t.tag = BLOK_TYPETAG_PRIMITIVE;
    t.as.primitive = sig;
    return blok_typedata_intern(s, t);
}

blok_Type blok_type_void(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_VOID});
}

blok_Type blok_type_int(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_INT});
}

blok_Type blok_type_bool(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_BOOL});
}

blok_Type blok_type_type(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_TYPE});
}

blok_Type blok_type_symbol(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_SYMBOL});
}

blok_Type blok_type_string(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_STRING});
}


blok_Type blok_type_obj(blok_State * s) {
    return blok_typedata_intern(s, (blok_TypeData) {.tag = BLOK_TYPETAG_OBJ});
}

blok_Type blok_type_list(blok_State * s, blok_Type item_type) {
    return blok_typedata_intern(s, (blok_TypeData) {
            .tag = BLOK_TYPETAG_LIST,
            .as.list = (blok_ListType){.item_type = item_type}
        });
}



/*
 * TODO: uncomment this
bool blok_type_equal(blok_TypeData lhs, blok_TypeData rhs);

blok_Type blok_type_intern(blok_State * s, blok_TypeData sym) {
    for(blok_Type i = 0; i < s->types.len; ++i) {
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

blok_Obj blok_obj_from_ptr(void * ptr, blok_Tag tag) {
    assert(((uintptr_t)ptr& 0xf) == 0);
    blok_Obj obj = {0};
    obj.as.ptr = ptr;
    obj.tag = tag;
    return obj;
}

blok_Obj blok_obj_from_list(blok_List * l) { return blok_obj_from_ptr(l, BLOK_TAG_LIST); }
blok_Obj blok_obj_from_keyvalue(blok_KeyValue* l) { return blok_obj_from_ptr(l, BLOK_TAG_KEYVALUE); }
blok_Obj blok_obj_from_string(blok_String * s) { return blok_obj_from_ptr(s, BLOK_TAG_STRING); }
blok_Obj blok_obj_from_function(blok_Function * f) { return blok_obj_from_ptr(f, BLOK_TAG_FUNCTION); }
blok_Obj blok_obj_from_primitive(blok_Primitive * f) { return blok_obj_from_ptr(f, BLOK_TAG_PRIMITIVE); }
blok_Obj blok_obj_from_type(blok_Type t) { return (blok_Obj){.tag = BLOK_TAG_TYPE, .as.data = t}; }

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
    blok_profiler_start("blok_list_allocate");
    assert(a != NULL);
    blok_List * result = blok_arena_alloc(a, sizeof(blok_List));
    assert(result != NULL);
    result->cap = initial_capacity;
    result->items.len = 0;
    result->items.ptr = blok_arena_alloc(a, result->cap * sizeof(blok_Obj));
    assert(result->items.ptr != NULL);
    blok_profiler_stop("blok_list_allocate");
    return result;
}

blok_KeyValue * blok_keyvalue_allocate(blok_Arena * a) {
    blok_KeyValue * result = blok_arena_alloc(a, sizeof(blok_KeyValue));
    memset(result, 0, sizeof(blok_KeyValue));
    return result;
}

blok_Obj blok_make_symbol(blok_Symbol sym) {
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


blok_Primitive * blok_primitive_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_PRIMITIVE);
    obj.tag = 0;
    return (blok_Primitive *) obj.as.ptr;
}

blok_Symbol blok_symbol_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_SYMBOL);
    return obj.as.data;
}

blok_Type blok_type_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_TYPE);
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

bool blok_string_equal(blok_String * const lhs, blok_String * const rhs) {
    if(lhs->items.len != rhs->items.len) {
        return false;
    } else {
        return strncmp(lhs->items.ptr, rhs->items.ptr, lhs->items.len);
    }
}

bool blok_symbol_empty(blok_Symbol sym) {
    return sym == BLOK_SYMBOL_NIL;
}

bool blok_symboldata_equal(blok_SymbolData lhs, blok_SymbolData rhs) {
    return strncmp(lhs.buf, rhs.buf, sizeof(lhs.buf)) == 0;
}

bool blok_symbol_streql(blok_State * s, const blok_Symbol sym, const char * str) {
    blok_SymbolData symbol = blok_symbol_get_data(s, sym);
    return strcmp(symbol.buf, str) == 0;
}


bool blok_obj_equal(blok_Obj lhs, blok_Obj rhs) {
    blok_profiler_start("blok_obj_equal");
    bool result = false;
    if(lhs.tag != rhs.tag) {
        result = false;
    } else {
        switch(lhs.tag) {
            case BLOK_TAG_INT:
            case BLOK_TAG_SYMBOL:
                result = lhs.as.data == rhs.as.data;
                break;
            case BLOK_TAG_NIL:
                result = true;
                break;
            case BLOK_TAG_STRING:
                result = blok_string_equal(blok_string_from_obj(lhs),
                        blok_string_from_obj(rhs));
                break;
            default: 
                blok_fatal_error(NULL, "Comparison for this tag not implemented yet: %s", blok_tag_get_name(lhs.tag));
                break;
        }
    }
    blok_profiler_stop("blok_obj_equal");
    return result;
}

bool blok_nil_equal(blok_Obj obj) {
    return obj.tag == BLOK_TAG_NIL;
}

void blok_list_append(blok_List* l, blok_Arena * a,  blok_Obj item) {
    blok_profiler_start("blok_list_append");
    blok_vec_append(l, a, blok_obj_copy(a, item));
    blok_profiler_stop("blok_list_append");
}

blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj);

blok_List * blok_list_copy(blok_Arena * a, blok_List const * const list) {
    blok_profiler_start("blok_list_copy");
    blok_List * result = blok_arena_alloc(a, sizeof(blok_List));
    memset(result, 0, sizeof(blok_List));
    blok_vec_foreach(blok_Obj, it, list) {
        blok_list_append(result, a, *it);
    }
    blok_profiler_stop("blok_list_copy");
    return result;
}

blok_String * blok_string_copy(blok_Arena * a, blok_String * str) {
    blok_profiler_start("blok_string_copy");
    blok_String * result = blok_arena_alloc(a, sizeof(blok_String));
    memset(result, 0, sizeof(blok_List));
    blok_vec_foreach(char, ch, str) {
        blok_vec_append(result, a, *ch);
    }
    blok_profiler_stop("blok_string_copy");
    return result;
}


blok_KeyValue * blok_keyvalue_copy(blok_Arena * destination_scope, blok_KeyValue * kv) {
    blok_profiler_start("blok_keyvalue_copy");
    blok_KeyValue * result  = blok_keyvalue_allocate(destination_scope);
    result->key = kv->key;
    result->value = blok_obj_copy(destination_scope, kv->value);
    blok_profiler_stop("blok_keyvalue_copy");
    return result;
}

blok_Function * blok_function_copy(blok_Arena * destination_scope, blok_Function * function) {
    blok_profiler_start("blok_function_copy");
    blok_Function * result = blok_function_allocate(destination_scope);
    result->body = blok_list_copy(destination_scope, function->body);
    result->params = blok_list_copy(destination_scope, function->params);
    blok_profiler_stop("blok_function_copy");
    return result;
}

/* All objects use value semantics, so they should be copied when being assigned
 * or passed as parameters
 */
blok_Obj blok_obj_copy(blok_Arena * destination_scope, blok_Obj obj) {
    blok_profiler_start("blok_obj_copy");
    blok_Obj result = blok_make_nil();
    switch(obj.tag) {
        case BLOK_TAG_BOOL:
        case BLOK_TAG_NIL:
        case BLOK_TAG_PRIMITIVE:
        case BLOK_TAG_INT:
        case BLOK_TAG_SYMBOL:
            result = obj;
            break;
        case BLOK_TAG_LIST:
            result = blok_obj_from_list(blok_list_copy(destination_scope, blok_list_from_obj(obj)));
            break;
        case BLOK_TAG_STRING:
            result = blok_obj_from_string(blok_string_copy(destination_scope, blok_string_from_obj(obj)));
            break;
        case BLOK_TAG_KEYVALUE:
            result = blok_obj_from_keyvalue(blok_keyvalue_copy(destination_scope, blok_keyvalue_from_obj(obj)));
            break;
        case BLOK_TAG_ALIST:
            blok_fatal_error(NULL, "TODO");
            break;
        case BLOK_TAG_FUNCTION:
            result = blok_obj_from_function(blok_function_copy(destination_scope, blok_function_from_obj(obj)));
            break;
        case BLOK_TAG_TYPE:
            blok_fatal_error(NULL, "TODO");
            break;
        case BLOK_TAG_VARIABLE:
            blok_fatal_error(NULL, "TODO");
            break;
    }

    result.src_info = obj.src_info;
    blok_profiler_stop("blok_obj_copy");
    return result;
}

typedef enum {
    BLOK_STYLE_AESTHETIC,
    BLOK_STYLE_CODE
} blok_Style;

void blok_obj_fprint(blok_State * s, FILE * fp, blok_Obj obj, blok_Style style);


void blok_symbol_fprint(blok_State *s, FILE * fp, const blok_Symbol symbol, blok_Style style) {
    blok_profiler_do ("symbol_fprint") {
        (void) style;
        blok_SymbolData sym = blok_symbol_get_data(s, symbol);
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
}

void blok_keyvalue_fprint(blok_State * s, FILE * fp, blok_KeyValue * const kv, blok_Style style) {
    blok_profiler_do ("keyvalue_fprint") {
        blok_symbol_fprint(s, fp, kv->key, style);
        fprintf(fp, ":");
        blok_obj_fprint(s, fp, kv->value, style);
    }
}

void blok_fprint_escape_sequences(FILE * fp, const char * str, size_t max) {
    blok_profiler_do("fprint_escape_sequences") {
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
}


void blok_obj_fprint(blok_State * s, FILE * fp, blok_Obj obj, blok_Style style) {
    blok_profiler_do("blok_obj_fprint") {
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
                    bool whitespace = false;
                    fprintf(fp, "(");
                    blok_vec_foreach(blok_Obj, it, list) {
                        if(whitespace) fprintf(fp, " "); else whitespace = true;
                        blok_obj_fprint(s, fp, *it, style);
                    }
                    fprintf(fp, ")");
                }
                break;
            case BLOK_TAG_STRING:
                switch(style) {
                    case BLOK_STYLE_AESTHETIC:
                        fprintf(fp, "%s", blok_string_from_obj(obj)->items.ptr);
                        break;
                    case BLOK_STYLE_CODE:
                        fprintf(fp, "\"");
                        blok_String * str = blok_string_from_obj(obj);
                        blok_fprint_escape_sequences(fp, str->items.ptr, 32);
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
            default:
                fprintf(fp, "<Unprintable %s>", blok_tag_get_name(obj.tag));
                break;
        }
    }
}


void blok_obj_print(blok_State * s, blok_Obj obj, blok_Style style) {
    blok_profiler_do("obj_print") blok_obj_fprint(s, stdout, obj, style);
}


void blok_print_as_bytes(blok_Obj obj) {
    uint8_t *bytes = (uint8_t*)&obj;
    for(int i = 0; i < 8; ++i) {
        printf("%d", bytes[i]);
    }
    puts("");
}

#endif /*BLOK_OBJ_C*/
