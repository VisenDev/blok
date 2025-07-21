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

void blok_compiler_primitive_procedure(blok_State * s, blok_Bindings * globals, blok_ListRef args, FILE * output) {


}

//TODO
//change this function to work by inferring the type of a value first,
//then check the inferred type against the expected type
void blok_compiler_typecheck_value(blok_State * s, blok_SourceInfo * src, blok_TypeData type, blok_Obj value) {
    switch(type.tag) {
        case BLOK_TYPETAG_VOID:
            blok_fatal_error(src, "Attempted to typecheck a value against a void type");
            break;
        case BLOK_TYPETAG_BOOLEAN:
            if(value.tag != BLOK_TAG_BOOL) {
                blok_fatal_error(src, "Expected value of type Boolean");
            }
            break;
        case BLOK_TYPETAG_FUNCTION:
            if(value.tag != BLOK_TAG_FUNCTION) {
                blok_fatal_error(src, "Expected value of type Function");
            } else {
                blok_Function * value_sig = blok_function_from_obj(value);
                if(!blok_functionsignature_equal(value_sig->signature, type.as.function)) {
                    blok_fatal_error(src, "Unmatching function signature");
                }
            }
            break;
        case BLOK_TYPETAG_INT:
            if(value.tag != BLOK_TAG_INT) {
                blok_fatal_error(src, "Expected value of type Int");
            }
            break;
        case BLOK_TYPETAG_LIST:
            if(value.tag != BLOK_TAG_INT) {

            }


        



    }
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
