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

void blok_evaluator_validate_sexpr(blok_State * s, blok_Obj sexpr) {
    (void)s;
    if(sexpr.tag != BLOK_TAG_LIST)
        blok_fatal_error(&sexpr.src_info, "Expected s-expression, found %s", blok_tag_get_name(sexpr.tag));
    blok_List * l = blok_list_from_obj(sexpr);
    if(l->items.len < 1) 
        blok_fatal_error(&sexpr.src_info, "Found empty s-expression");
    if(l->items.ptr[0].tag != BLOK_TAG_SYMBOL)
        blok_fatal_error(&sexpr.src_info, "Expected symbol inside s-expression, found %s", blok_tag_get_name(l->items.ptr[0].tag));
}

void blok_evaluator_toplevel_apply_primitive(blok_State * s, blok_Bindings * globals, blok_Primitive prim, blok_ListRef args, FILE * output) {
    (void)s;
    (void)globals;
    (void)prim;
    (void)args;
    (void)output;
    blok_fatal_error(NULL, "TODO: implement blok_evaluator_toplevel_apply_primitive");
}

void blok_evaluator_toplevel_sexpr(blok_State * s, blok_Bindings * globals, blok_Obj sexpr, FILE * output) {
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
                blok_evaluator_toplevel_apply_primitive(s, globals, head_value.as.data, args, output);
            } else {
                blok_fatal_error(NULL, "TODO");
            }
        }
}

//returns a table of globals
blok_Bindings blok_evaluator_toplevel(blok_State * s, blok_List * toplevel, FILE * output) {
    (void) output;
    blok_Bindings globals = {0};
    for(int32_t i = 0; i < toplevel->items.len; ++i) {
        blok_Obj sexpr = toplevel->items.ptr[i];
        blok_evaluator_validate_sexpr(s, sexpr);
        blok_evaluator_toplevel_sexpr(s, &globals, sexpr, output);
    }
    return globals;
}

#endif /*BLOK_EVALUATOR_C*/
