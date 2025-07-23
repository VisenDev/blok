#ifndef BLOK_EVALUATOR_C
#define BLOK_EVALUATOR_C

#include "blok_arena.c"
#include "blok_obj.c"

#include <ctype.h>

blok_Type blok_compiler_infer_typeof_value(blok_State * s, blok_Obj value);


//void blok_state_bind_builtin(blok_State * s, const char * symbol, blok_Obj value, bool toplevel) {
//    blok_Binding binding = {
//        .name = blok_symbol_from_string(s, symbol),
//        .value = value,
//        .type = blok_compiler_infer_typeof_value(s, value),
//    };
//    if(toplevel) {
//        blok_vec_append(&s->toplevel_builtins, &s->persistent_arena, binding);
//    } else {
//        blok_vec_append(&s->local_builtins, &s->persistent_arena, binding);
//    }
//}

void blok_state_bind_toplevel_primitive(blok_State * s, blok_Primitive prim) {
#ifndef NDEBUG
    blok_Primitive * it = NULL;
    blok_vec_find(it, &s->toplevel_primitives, it->name == prim.name);
    assert(it == NULL && "Primitive is already defined");
#endif /*NDEBUG*/

    blok_vec_append(&s->toplevel_primitives, &s->persistent_arena, prim);
}

blok_State blok_state_init(void) {
    blok_State result = {0};
    blok_State * s = &result;
    (void) s;

    blok_state_bind_toplevel_primitive(s, (blok_Primitive){
        .name = blok_symbol_from_string(s, "#let"),
        .tag = BLOK_PRIMITIVE_LET,
        .signature = (blok_FunctionSignature){
            .param_count = 2,
            .params = {
                (blok_ParamType){.type = blok_type_symbol(s), .noeval = true },
                (blok_ParamType){.type = blok_type_obj(s)}
            },
            .return_type = blok_type_void(s),
        }
    });

    blok_state_bind_toplevel_primitive(s, (blok_Primitive){
        .name = blok_symbol_from_string(s, "#procedure"),
        .tag = BLOK_PRIMITIVE_PROCEDURE,
        .signature = (blok_FunctionSignature){
            .param_count = 3,
            .params = {
                (blok_ParamType){.type = blok_type_symbol(s), .noeval = true},
                (blok_ParamType){.type = blok_type_symbol(s), .noeval = true},
                (blok_ParamType){.type = blok_type_obj(s), .noeval = true},
            },
            .variadic = true,
            .variadic_args_type = {.type = blok_type_obj(s), .noeval = true},
            .return_type = blok_type_void(s),
        }
    });

    //TOPLEVEL
    //blok_state_bind_builtin(s, "#let", blok_obj_from_primitive(let)       ,        true);
    //blok_state_bind_builtin(s, "#procedure", blok_make_primitive(BLOK_PRIMITIVE_PROCEDURE),  true);
    //blok_state_bind_builtin(s, "print_int",  blok_make_primitive(BLOK_PRIMITIVE_PRINT_INT),  true);

    ////LOCAL
    //blok_state_bind_builtin(s, "return",     blok_make_primitive(BLOK_PRIMITIVE_RETURN),     false);

    return result;
}

void blok_state_deinit(blok_State * s) {
    blok_arena_free(&s->persistent_arena);
}

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

blok_Type blok_compiler_infer_typeof_value_list(blok_State * s, blok_List * l) {
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
            return blok_compiler_infer_typeof_value_list(s, blok_list_from_obj(value));
        default:
            blok_fatal_error(NULL, "TODO: implement type inference for these types");
    }
}

blok_Type blok_compiler_infer_typeof_expr_list(blok_State * s, blok_SourceInfo * src, blok_Bindings * bindings, blok_Obj sexpr) {
    blok_compiler_validate_sexpr(s, sexpr);
    blok_List * l = blok_list_from_obj(sexpr);
    blok_Obj head_obj = l->items.ptr[0];
    blok_Symbol head = head_obj.as.data;
    blok_Binding * it = NULL;
    blok_vec_find(it, bindings, head == it->name);
    if(it == NULL) {
        blok_fatal_error(src, "Unbound symbol");
    } else {
        blok_TypeData type_data = blok_type_get_data(s, it->type);
        if(type_data.tag == BLOK_TYPETAG_FUNCTION) {
            return type_data.as.function.return_type;
        } else {
            blok_fatal_error(src, "Expected function");
        }
    }
}

blok_Type blok_compiler_infer_typeof_expr_symbol(blok_State * s, blok_SourceInfo * src, blok_Bindings * bindings, blok_Symbol sym) {
    (void) s;
    blok_Binding * it = NULL;
    blok_vec_find(it, bindings, sym == it->name);
    if(it == NULL) {
        blok_fatal_error(src, "Unbound symbol");
    } else {
        return it->type;
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
            return blok_compiler_infer_typeof_expr_symbol(s, src, bindings, expr.as.data);
        case BLOK_TAG_LIST:
            return blok_compiler_infer_typeof_expr_list(s, src, bindings, expr);
        default:
            blok_fatal_error(NULL, "TODO: implement type inference for these types");
    }
}

bool blok_compiler_type_coercible(blok_State * s, blok_Type to, blok_Type from) {
    if(to == from) {
        return true;
    } else {
        blok_TypeData t = blok_type_get_data(s, to);
        blok_TypeData f = blok_type_get_data(s, from);
        switch(t.tag) {
            case BLOK_TYPETAG_OBJ:
                return true;
            case BLOK_TYPETAG_OPTIONAL: 
                if(f.tag == BLOK_TYPETAG_NIL || from == t.as.optional.type) {
                    return true;
                } else {
                    return false;
                }
            default:
                BLOK_LOG("Types cannot be coerced");
                return false;
        }
    }
}

void blok_compiler_typecheck_value(blok_State * s, blok_SourceInfo * src, blok_Type type, blok_Obj value) {
    blok_Type value_type = blok_compiler_infer_typeof_value(s, value);
    if(!blok_compiler_type_coercible(s, type, value_type)) {
        blok_fatal_error(src, "Type mismatch");
    }
}

void blok_compiler_typecheck_expr(blok_State * s,  blok_SourceInfo * src, blok_Bindings * bindings, blok_Type type, blok_Obj expr) {
    blok_Type value_type = blok_compiler_infer_typeof_expr(s, src, bindings, expr);
    if(!blok_compiler_type_coercible(s, type, value_type)) {
        blok_fatal_error(src, "Type mismatch");
    }
}

void blok_compiler_typecheck_arg(blok_State *s, blok_SourceInfo * src, blok_Bindings * bindings, blok_ParamType type, blok_Obj arg) {
    if(type.noeval) {
        blok_compiler_typecheck_value(s, src, type.type, arg);
    } else {
        blok_compiler_typecheck_expr(s, src, bindings, type.type, arg);
    }
}

void blok_compiler_typecheck_args(blok_State * s, blok_SourceInfo * src, blok_Bindings * bindings, blok_FunctionSignature sig, blok_ListRef args) {
    if(sig.param_count > args.len) {
        blok_fatal_error(src, "Incorrect number of arguments, expected at least %d arguments, found %d arguments", sig.param_count, args.len);
    }
    if(!sig.variadic && sig.param_count != args.len) {
        blok_fatal_error(src, "Incorrect number of arguments, expected at %d arguments, found %d arguments", sig.param_count, args.len);
    }
    for(int i = 0; i < sig.param_count; ++i) {
        blok_compiler_typecheck_arg(s, src, bindings, sig.params[i], args.ptr[i]);
    }
    for(int i = sig.param_count; i < args.len; ++i) {
        blok_compiler_typecheck_arg(s, src, bindings, sig.variadic_args_type, args.ptr[i]);
    }

    BLOK_LOG("args are valid!\n");
}

void blok_compiler_apply_primitive(blok_State * s, blok_Bindings * globals, const blok_Primitive * p, blok_ListRef args, FILE * output) {
    //blok_Primitive * p = blok_primitive_from_obj(prim);
    blok_FunctionSignature sig = p->signature;
    blok_SourceInfo * src = NULL;
    if(args.len > 0) {
        src = &args.ptr[0].src_info;
    }
    blok_compiler_typecheck_args(s, src, globals, sig, args);
    (void)globals;
    (void)output;
    //TODO implement the primitives
//    switch(prim) {
//        case BLOK_PRIMITIVE_LET:
//            blok_fatal_error(NULL, "TODO");
//        case BLOK_PRIMITIVE_PROCEDURE:
//            blok_compiler_primitive_procedure(s, globals, args, output);
//
    //}
}

void blok_compiler_apply_function(blok_State * s, blok_Bindings * globals, blok_Obj fn, blok_ListRef args, FILE * output) {
    blok_Function * f = blok_function_from_obj(fn);
    blok_FunctionSignature sig = f->signature;
    blok_compiler_typecheck_args(s, &fn.src_info, globals, sig, args);
    (void)output;
}

void blok_compiler_toplevel_sexpr(blok_State * s, blok_Bindings * globals, blok_Obj sexpr, FILE * output) {
        blok_List * sexpr_list = blok_list_from_obj(sexpr);
        blok_ListRef args = blok_slice_tail(sexpr_list->items, 1);
        blok_Symbol head = blok_symbol_from_obj(sexpr_list->items.ptr[0]);

        blok_Primitive * it; 
        blok_vec_find(it, &s->toplevel_primitives, it->name == head);
        if(it == NULL) {
            blok_SymbolData data = blok_symbol_get_data(s, head);
            blok_fatal_error(&sexpr.src_info, "Unknown toplevel symbol: %s", data.buf);
        } else {
            blok_compiler_apply_primitive(s, globals, it, args, output);
            //blok_Obj head_value = it->value;
            //if(head_value.tag == BLOK_TAG_PRIMITIVE) {
            //    blok_compiler_apply_primitive(s, globals, head_value, args, output);
            //} else if (head_value.tag == BLOK_TAG_FUNCTION) {
            //    blok_compiler_apply_function(s, globals, head_value, args, output);
            //} else {
            //    blok_fatal_error(NULL, "TODO");
            //}
        }
}

//returns a table of globals
blok_Bindings blok_compiler_toplevel(blok_State * s, blok_List * toplevel, FILE * output) {
    (void) output;
    blok_Bindings globals = {0};
    for(int32_t i = 0; i < toplevel->items.len; ++i) {
        blok_Obj sexpr = toplevel->items.ptr[i];
        {
            printf("Compiling: ");
            blok_obj_print(s, sexpr, BLOK_STYLE_CODE);
            printf("\n");
        }
        blok_compiler_validate_sexpr(s, sexpr);
        blok_compiler_toplevel_sexpr(s, &globals, sexpr, output);
    }
    return globals;
}

#endif /*BLOK_EVALUATOR_C*/
