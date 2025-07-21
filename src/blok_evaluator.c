#ifndef BLOK_EVALUATOR_C
#define BLOK_EVALUATOR_C

#include "blok_arena.c"
#include "blok_obj.c"

#include <ctype.h>

bool blok_symbol_is_varname(blok_State * s, blok_Symbol symbol) {
    blok_SymbolData sym = blok_symbol_get_data(s, symbol);
    const char * ch = sym.buf;
    if(!isalpha(*ch) && !islower(*ch) && *ch != '_') {
        return false;   
    }
    for(;*ch != 0; ++ch) {
        if(!isalpha(*ch) && !islower(*ch) && *ch != '_' && !isdigit(*ch)) {
            return false; 
        }
    }
    return true;
}

blok_Type blok_expr_infer_type(blok_State * s, blok_Obj expr) {
    switch(expr.tag) {
        case BLOK_TAG_NIL:
            return blok_type_void(s);
        case BLOK_TAG_INT:
            return blok_type_int(s);
        case BLOK_TAG_STRING:
            return blok_type_string(s);
        default:
            blok_fatal_error(&expr.src_info, "TODO: implement type inference for this type of obj");
    }
}

void blok_primitive_toplevel_let(blok_State * s, blok_Bindings * globals, blok_ListRef args, FILE * output) {
    (void)output;
    if(args.len != 2) {
        if(args.len <= 0) {
            blok_fatal_error(NULL, "Let forms required 2 arguments");
        } else {
            blok_fatal_error(&args.ptr[0].src_info, "Let forms required 2 arguments");
        }
    }
    blok_Symbol name = blok_symbol_from_obj(args.ptr[0]);
    if(!blok_symbol_is_varname(s, name)) {
        blok_SymbolData namesym = blok_symbol_get_data(s, name);
        blok_fatal_error(&args.ptr[0].src_info, "Invalid variable name provided inside #let form: %s", namesym.buf);
    }
    blok_Obj expr = args.ptr[1];
    blok_Type type = blok_expr_infer_type(s, expr);
    blok_Binding * it = NULL;
    blok_vec_find(it, globals, it->name == name);
    if(it != NULL) {
        blok_fatal_error(&args.ptr[0].src_info, "Multiply defined symbol");
    }
    blok_Binding binding = (blok_Binding){.name = name, .type = type, .value = args.ptr[1]};
    blok_vec_append(globals, &s->persistent_arena, binding);
}

void blok_compiler_validate_sexpr(blok_State * s, blok_Obj sexpr) {
    (void)s;
    if(sexpr.tag != BLOK_TAG_LIST)
        blok_fatal_error(&sexpr.src_info, "Expected s-expression, found %s", blok_tag_get_name(sexpr.tag));
    blok_List * l = blok_list_from_obj(sexpr);
    if(l->items.len < 1) 
        blok_fatal_error(&sexpr.src_info, "Found empty s-expression");
    if(l->items.ptr[0].tag != BLOK_TAG_SYMBOL)
        blok_fatal_error(&sexpr.src_info, "Expected symbol inside s-expression, found %s", blok_tag_get_name(l->items.ptr[0].tag));
}

//void blok_compiler_primitive_procedure(blok_State * s, blok_Bindings * globals, blok_ListRef args, FILE * output) {
//
//
//}
//


blok_Type blok_compiler_infer_typeof_value(blok_State * s, blok_Obj value);

blok_Type blok_compiler_infer_typeof_list_value(blok_State * s, blok_List * l) {
    blok_Type obj_list_type = blok_type_list(s, blok_type_obj(s));
    if(l->items.len == 0) {
        return obj_list_type;
    }
    blok_Type primary_item_type = -1;
    blok_vec_foreach(blok_Obj, it, l) {
        blok_Type item_type = blok_compiler_infer_typeof_value(s, *it);
        if(primary_item_type == -1) {
            primary_item_type = item_type;
        } else {
            if(primary_item_type != item_type) { //This means the list contains members of different types
                return obj_list_type;
            }
        }
    }
    assert(primary_item_type != -1);
    return blok_type_list(s, primary_item_type);
}

blok_Type blok_compiler_infer_typeof_value(blok_State * s, blok_Obj value) {
    switch(value.tag) {
        case BLOK_TAG_NIL:
            return blok_type_void(s);
        case BLOK_TAG_TYPE:
            return blok_type_type(s);
        case BLOK_TAG_BOOL:
            return blok_type_bool(s);
        case BLOK_TAG_INT:
            return blok_type_int(s);
        case BLOK_TAG_STRING:
            return blok_type_string(s);
        case BLOK_TAG_SYMBOL:
            return blok_type_symbol(s);
        case BLOK_TAG_LIST:
            return blok_compiler_infer_typeof_list_value(s, blok_list_from_obj(value));
        default:
            blok_fatal_error(NULL, "TODO: implement type inference for these types");
    }
}

//NOTE, an expr means it is evaluated, a value means it is not
blok_Type blok_compiler_infer_typeof_expr(blok_State * s, blok_SourceInfo * src, blok_Bindings * bindings, blok_Obj expr) {
    switch(expr.tag) {
        case BLOK_TAG_NIL:
            return blok_type_void(s);
        case BLOK_TAG_TYPE:
            return blok_type_type(s);
        case BLOK_TAG_BOOL:
            return blok_type_bool(s);
        case BLOK_TAG_INT:
            return blok_type_int(s);
        case BLOK_TAG_STRING:
            return blok_type_string(s);
        case BLOK_TAG_SYMBOL:
        {
            blok_Binding * it = NULL;
            blok_vec_find(it, bindings, expr.as.data == it->name);
            if(it == NULL) {
                blok_fatal_error(src, "Unbound symbol");
            } else {
                return it->type;
            }
        }
        case BLOK_TAG_LIST:
        {
            blok_compiler_validate_sexpr(s, expr);
            blok_List * l = blok_list_from_obj(expr);
            blok_Obj head_obj = l->items.ptr[0];
            blok_Symbol head = head_obj.as.data;
            blok_Binding * it = NULL;
            blok_vec_find(it, bindings, head == it->name);
            if(it == NULL) {
                blok_fatal_error(src, "Unbound symbol");
            } else {
                switch(it->value.tag) {
                    //TODO: get the functions return type
                    case BLOK_TAG_FUNCTION:
                    break;
                }
            }
        }
        default:
            blok_fatal_error(NULL, "TODO: implement type inference for these types");
    }
}

//TODO
//change this function to work by inferring the type of a value first,
//then check the inferred type against the expected type
void blok_compiler_typecheck_value(blok_State * s, blok_SourceInfo * src, blok_TypeData type, blok_Obj value) {
}

void blok_compiler_typecheck_expr(blok_State * s, blok_SourceInfo * src, blok_TypeData type, blok_Obj expr) {

}

void blok_compiler_typecheck_arg(blok_State *s, blok_SourceInfo * src, blok_ParamType type, blok_Obj arg) {
    blok_TypeData data = blok_type_get_data(s, type.type);
    if(type.noeval) {
        blok_compiler_typecheck_value(s, src, data, arg);
    } else {
        blok_compiler_typecheck_expr(s, src, data, arg);
    }
}

void blok_compiler_typecheck_args(blok_State * s, blok_SourceInfo * src, blok_FunctionSignature sig, blok_ListRef args) {
    if(sig.param_count > args.len) {
        blok_fatal_error(src, "Incorrect number of arguments, expected at least %d arguments, found %d arguments", sig.param_count, args.len);
    }

    
}

void blok_compiler_apply_primitive(blok_State * s, blok_Bindings * globals, blok_Primitive prim, blok_ListRef args, FILE * output) {
    switch(prim) {
        case BLOK_PRIMITIVE_LET:
            blok_fatal_error(NULL, "TODO");
        case BLOK_PRIMITIVE_PROCEDURE:
            blok_compiler_primitive_procedure(s, globals, args, output);

    }
}

void blok_compiler_toplevel_sexpr(blok_State * s, blok_Bindings * globals, blok_Obj sexpr, FILE * output) {
        blok_Binding * it; 
        blok_List * sexpr_list = blok_list_from_obj(sexpr);
        blok_ListRef args = blok_slice_tail(sexpr_list->items, 1);
        blok_Symbol head = blok_symbol_from_obj(sexpr_list->items.ptr[0]);
        blok_vec_find(it, &s->toplevel_builtins, it->name == head);
        if(it == NULL) {
            blok_SymbolData data = blok_symbol_get_data(s, head);
            blok_fatal_error(&sexpr.src_info, "Unknown toplevel symbol: %s", data.buf);
        } else {
            blok_Obj head_value = it->value;
            if(head_value.tag == BLOK_TAG_PRIMITIVE) {
                blok_compiler_apply_primitive(s, globals, head_value.as.data, args, output);
            } else if (head_value.tag == BLOK_TAG_FUNCTION) {
                blok_compiler_apply_function(s, globals, head_value.as.data, args, output);
            } else {
                blok_fatal_error(NULL, "TODO");
            }
        }
}

//returns a table of globals
blok_Bindings blok_compiler_toplevel(blok_State * s, blok_List * toplevel, FILE * output) {
    (void) output;
    blok_Bindings globals = {0};
    for(int32_t i = 0; i < toplevel->items.len; ++i) {
        blok_Obj sexpr = toplevel->items.ptr[i];
        blok_compiler_validate_sexpr(s, sexpr);
        blok_compiler_toplevel_sexpr(s, &globals, sexpr, output);
    }
    return globals;
}

#endif /*BLOK_EVALUATOR_C*/
