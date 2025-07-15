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

blok_Type blok_expr_infer_type(blok_Arena * a, blok_Obj expr) {
    (void) a;
    (void) expr;
    blok_fatal_error(NULL, "TODO: implement type inference");
    return (blok_Type){0}; 
}

void blok_primitive_toplevel_let(blok_State * s, blok_AList * globals, blok_Obj sexpr_obj) {
    (void)globals;
    blok_List * sexpr = blok_list_from_obj(sexpr_obj);
    if(sexpr->items.len != 3) {
        blok_fatal_error(&sexpr_obj.src_info, "Invalid #let form, expected (#let <name> <value>)");
    }
    blok_Obj let = sexpr->items.ptr[0];
    assert(let.tag == BLOK_TAG_SYMBOL);
    assert(blok_symbol_streql(s, blok_symbol_from_obj(let), "#let"));
    blok_Obj name_obj = sexpr->items.ptr[1];
    if(name_obj.tag != BLOK_TAG_SYMBOL) {
        blok_fatal_error(&name_obj.src_info, "Expected symbol inside #let form, found %s", blok_tag_get_name(name_obj.tag));
    }
    blok_Symbol name = blok_symbol_from_obj(name_obj);
    if(!blok_symbol_is_varname(s, name)) {
        blok_SymbolData namesym = blok_symbol_get_data(s, name);
        blok_fatal_error(&name_obj.src_info, "Invalid variable name provided inside #let form: %s", namesym.buf);
    }
    blok_Obj expr = sexpr->items.ptr[2];
    (void)expr;
    (void) let;
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

void blok_evaluator_toplevel_apply_primitive(blok_State * s, blok_AList * globals, blok_Primitive prim, blok_ListRef args, FILE * output) {
    (void)s;
    (void)globals;
    (void)prim;
    (void)args;
    (void)output;
}

void blok_evaluator_toplevel_sexpr(blok_State * s, blok_AList * globals, blok_Obj sexpr, FILE * output) {
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
blok_AList blok_evaluator_toplevel(blok_State * s, blok_List * toplevel, FILE * output) {
    (void) output;
    blok_AList globals = {0};
    for(int32_t i = 0; i < toplevel->items.len; ++i) {
        blok_Obj sexpr = toplevel->items.ptr[i];
        blok_evaluator_validate_sexpr(s, sexpr);
        blok_evaluator_toplevel_sexpr(s, &globals, sexpr, output);
    }
    return globals;
}

#endif /*BLOK_EVALUATOR_C*/
