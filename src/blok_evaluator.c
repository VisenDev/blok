#ifndef BLOK_EVALUATOR_C
#define BLOK_EVALUATOR_C

#include "blok_arena.c"
#include "blok_obj.c"

#include <ctype.h>


//struct blok_State {
//    blok_Arena persistent_arena;
//    blok_Vec(blok_TypeData) types; 
//    blok_Vec(blok_SymbolData) symbols;
//    blok_Vec(blok_Arena) arenas;
//
//    blok_Primitives toplevel_primitives;
//    blok_Bindings builtins;
//
//    //Compilation specific 
//    FILE * out;
//    blok_Bindings globals;
//};

blok_Type blok_compiler_infer_typeof_value(blok_State * s, blok_Obj value);

void blok_state_bind_toplevel_primitive(blok_State * s, blok_Primitive prim) {
//#ifndef NDEBUG
    blok_Primitive * it = NULL;
    blok_vec_find(it, &s->toplevel_primitives, it->name == prim.name);
    assert(it == NULL && "Primitive is already defined");
//#endif /*NDEBUG*/

    blok_vec_append(&s->toplevel_primitives, &s->persistent_arena, prim);
}

void blok_state_create_global(blok_State * s, const char * symbol, blok_Obj obj) {
    blok_Binding * it = NULL;
    const blok_Symbol name = blok_symbol_from_string(s, symbol);
    blok_vec_find(it, &s->globals, it->name == name);
    if(it != NULL) {
        blok_fatal_error(&obj.src_info, "Multiply defined symbol");
    }
    blok_Binding binding = {
        .name = name,
        .type = blok_compiler_infer_typeof_value(s, obj),
        .value = obj,
    };
    blok_vec_append(&s->globals, &s->persistent_arena, binding);
}

bool blok_state_lookup(blok_State * s, blok_Symbol sym, blok_Obj * result) {
    blok_Binding * it = NULL;
    blok_vec_find(it, &s->globals, it->name == sym);
    if(it == NULL) {
        return false;
    } else {
        *result = it->value;
        return true;
    }
}

blok_State blok_state_init(void) {
    blok_State result = {0};
    blok_State * s = &result;

    blok_state_create_global(s, "true", blok_make_true());
    blok_state_create_global(s, "false", blok_make_false());
    blok_state_create_global(s, "nil", blok_make_nil());
    blok_state_create_global(s, "Int", blok_obj_from_type(blok_type_int(s)));
    blok_state_create_global(s, "Bool", blok_obj_from_type(blok_type_bool(s)));

    blok_state_bind_toplevel_primitive(s, (blok_Primitive){
        .name = blok_symbol_from_string(s, "#let"),
        .tag = BLOK_PRIMITIVE_TOPLEVEL_LET,
        .signature = blok_primitive_signature_intern(s, (blok_Signature){
            .param_count = 2,
            .params = {
                (blok_ParamType){.type = blok_type_symbol(s), .noeval = true },
                (blok_ParamType){.type = blok_type_obj(s)}
            },
            .return_type = blok_type_void(s),
        })
    });

    blok_Primitive * when = blok_arena_alloc(&s->persistent_arena, sizeof(blok_Primitive));
    *when = (blok_Primitive){
        .name = blok_symbol_from_string(s, "#when"),
        .tag = BLOK_PRIMITIVE_WHEN,
        .signature = blok_primitive_signature_intern(s, (blok_Signature){
            .param_count = 1,
            .params = {
                (blok_ParamType){.type = blok_type_bool(s)},
            },
            .return_type = blok_type_void(s),
            .variadic = true,
            .variadic_args_type = (blok_ParamType){.type = blok_type_obj(s), .noeval = true},
        })
    };
    blok_state_create_global(s, "#when", blok_obj_from_primitive(when));

   blok_state_bind_toplevel_primitive(s, (blok_Primitive){
       .name = blok_symbol_from_string(s, "#procedure"),
       .tag = BLOK_PRIMITIVE_TOPLEVEL_PROCEDURE,
       .signature = blok_primitive_signature_intern(s, (blok_Signature){
           .param_count = 3,
           .params = {
               (blok_ParamType){.type = blok_type_symbol(s), .noeval = true},
               (blok_ParamType){.type = blok_type_symbol(s), .noeval = true},
               (blok_ParamType){.type = blok_type_obj(s), .noeval = true},
           },
           .variadic = true,
           .variadic_args_type = {.type = blok_type_obj(s), .noeval = true},
           .return_type = blok_type_void(s),
       })
   });

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
        case BLOK_TAG_PRIMITIVE:
            //TODO("figure out a better way to store the types of primitives");
            return blok_primitive_from_obj(value)->signature;
        case BLOK_TAG_FUNCTION:
            //TODO("figure out a better way to store the types of primitives");
            return blok_function_from_obj(value)->signature;
        default:
            blok_fatal_error(NULL, "TODO: implement type inference for these types: %s", blok_tag_get_name(value.tag));
    }
}

blok_Type blok_compiler_infer_typeof_expr_list(blok_State * s, blok_SourceInfo * src, blok_Obj sexpr) {
    blok_compiler_validate_sexpr(s, sexpr);
    blok_List * l = blok_list_from_obj(sexpr);
    blok_Obj head_obj = l->items.ptr[0];
    blok_Symbol head = head_obj.as.data;
    blok_Binding * it = NULL;
    blok_vec_find(it, &s->globals, head == it->name);
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

blok_Type blok_compiler_infer_typeof_expr_symbol(blok_State * s, blok_SourceInfo * src, blok_Symbol sym) {
    (void) s;
    blok_Binding * it = NULL;
    blok_vec_find(it, &s->globals, sym == it->name);
    if(it == NULL) {
        blok_SymbolData data = blok_symbol_get_data(s, sym);
        blok_fatal_error(src, "Unbound symbol: %s", data.buf);
    } else {
        return it->type;
    }
}

//NOTE, an expr means it is evaluated, a value means it is not
blok_Type blok_compiler_infer_typeof_expr(blok_State * s, blok_SourceInfo * src, blok_Obj expr) {
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
            return blok_compiler_infer_typeof_expr_symbol(s, src, expr.as.data);
        case BLOK_TAG_LIST:
            return blok_compiler_infer_typeof_expr_list(s, src, expr);
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

void blok_compiler_typecheck_expr(blok_State * s,  blok_SourceInfo * src, blok_Type type, blok_Obj expr) {
    blok_Type value_type = blok_compiler_infer_typeof_expr(s, src, expr);
    if(!blok_compiler_type_coercible(s, type, value_type)) {
        blok_fatal_error(src, "Type mismatch");
    }
}

void blok_compiler_typecheck_arg(blok_State *s, blok_SourceInfo * src, blok_ParamType type, blok_Obj arg) {
    if(type.noeval) {
        blok_compiler_typecheck_value(s, src, type.type, arg);
    } else {
        blok_compiler_typecheck_expr(s, src, type.type, arg);
    }
}

void blok_compiler_typecheck_args(blok_State * s, blok_SourceInfo * src, blok_Signature sig, blok_ListRef args) {
    if(sig.param_count > args.len) {
        blok_fatal_error(src, "Incorrect number of arguments, expected at least %d arguments, found %d arguments", sig.param_count, args.len);
    }
    if(!sig.variadic && sig.param_count != args.len) {
        blok_fatal_error(src, "Incorrect number of arguments, expected at %d arguments, found %d arguments", sig.param_count, args.len);
    }
    for(int i = 0; i < sig.param_count; ++i) {
        blok_compiler_typecheck_arg(s, src, sig.params[i], args.ptr[i]);
    }
    for(int i = sig.param_count; i < args.len; ++i) {
        blok_compiler_typecheck_arg(s, src, sig.variadic_args_type, args.ptr[i]);
    }

    //BLOK_LOG("args are valid!\n");
}

blok_Obj blok_evaluator_eval(blok_State * s, blok_Obj obj) {
    blok_Obj result = blok_make_nil();
    switch(obj.tag) {
        case BLOK_TAG_BOOL:
        case BLOK_TAG_INT:
        case BLOK_TAG_NIL:
        case BLOK_TAG_STRING:
            return obj;
        case BLOK_TAG_SYMBOL:
            if(!blok_state_lookup(s, obj.as.data, &result)) {
                blok_SymbolData data = blok_symbol_get_data(s, obj.as.data);
                blok_fatal_error(&obj.src_info, "Undefined symbol: %s", data.buf);
            }
            return result;
        default:
            LOG("%s\n", blok_tag_get_name(obj.tag));
            TODO("IMPLEMENT EVALUATION FOR THESE TYPES");
    }
}

void blok_compiler_compile_primive_let(blok_State * s, blok_ListRef args) {
    assert(args.len == 2);
    blok_Symbol name = blok_symbol_from_obj(args.ptr[0]);
    blok_Binding b = (blok_Binding) {
        .name = name,
        .type = blok_compiler_infer_typeof_expr(s, &args.ptr[0].src_info, args.ptr[1]),
        .value = blok_evaluator_eval(s, args.ptr[1]),
    };
    blok_Binding * it = NULL;
    blok_vec_find(it, &s->globals, it->name == name);
    if(it != NULL) {
        blok_fatal_error(&args.ptr[0].src_info, "multiply defined symbol");
    }
    blok_vec_append(&s->globals, &s->persistent_arena, b);
}

void blok_compiler_codegen_type(blok_State * s, blok_Type type) {
    blok_TypeData t = blok_type_get_data(s, type);
    switch(t.tag) {
        case BLOK_TYPETAG_VOID:
            fprintf(s->out, "void");
            break;
        case BLOK_TYPETAG_BOOL:
            fprintf(s->out, "_Bool");
            break;
        case BLOK_TYPETAG_INT:
            fprintf(s->out, "long");
            break;
        case BLOK_TYPETAG_NIL:
            fprintf(s->out, "_Bool");
            break;
        default: TODO("");
    }

}

void blok_compiler_codegen_identifier(blok_State * s, blok_Symbol symbol) {
    blok_SymbolData sym = blok_symbol_get_data(s, symbol);
    fprintf(s->out, "%s", sym.buf);
}

void blok_compiler_codegen_param(blok_State * s, blok_List * param) {
    assert(param->items.len == 2);
    blok_Type type = blok_type_from_obj(blok_evaluator_eval(s, param->items.ptr[0]));
    blok_Symbol name = blok_symbol_from_obj(param->items.ptr[1]);
    blok_compiler_codegen_type(s, type);
    fprintf(s->out, " ");
    blok_compiler_codegen_identifier(s, name);
}

void blok_compiler_codegen_params(blok_State * s, blok_List * p) {
    //blok_ListRef params = params_list->items;
    fprintf(s->out, "(");
    bool first = true;
    for(blok_Obj * obj = p->items.ptr; obj < p->items.ptr + p->items.len; ++obj) {

        if(!first) {
            fprintf(s->out, ", ");
        }

        //allow non nested list for functions with a single parameter
        if(obj->tag != BLOK_TAG_LIST) {
            if(first) {
                blok_compiler_codegen_param(s, p);
                break;
            } else {
                blok_fatal_error(&obj->src_info, "Expected list, found %s", blok_tag_get_name(obj->tag));
            }
        } else {
            blok_compiler_codegen_param(s, blok_list_from_obj(*obj));
        }
        first = false;
    }
    fprintf(s->out, ")");
}


void blok_compiler_codegen_statement(blok_State * s, blok_Obj statement);

void blok_compiler_codegen_expression(blok_State * s, blok_Obj expr) {
    (void)s;
    (void)expr;
    TODO("");
}

void blok_compiler_codegen_when(blok_State *s, blok_ListRef args) {
    assert(args.len >= 2);
    fprintf(s->out, "if (");
    blok_compiler_codegen_expression(s, args.ptr[0]);
    fprintf(s->out, ") {\n");
    for(int i = 1; i < args.len; ++i) {
        blok_compiler_codegen_statement(s, args.ptr[i]);
    }
    fprintf(s->out, "}");
}

//TODO
//refactor toplevel_primitives so that 
//they show an error inside expression codegen, rather than 
//just simply being undefined outside of toplevel s-expressions

void blok_compiler_codegen_primitive(blok_State *s, const blok_Primitive * prim, blok_ListRef args) {
    switch(prim->tag) {
        case BLOK_PRIMITIVE_WHEN:
            blok_compiler_codegen_when(s, args);
            break;
        default:
            TODO("implement");
    }

}

void blok_compiler_codegen_statement(blok_State * s, blok_Obj statement) {
    blok_ListRef stmt = blok_list_from_obj(statement)->items;
    if(stmt.len < 1) {
        blok_fatal_error(&statement.src_info, "empty statement");
    }
    blok_Obj sexpr_head = blok_evaluator_eval(s, stmt.ptr[0]);
    blok_ListRef args = blok_slice_tail(stmt, 1);
    switch(sexpr_head.tag) {
        case BLOK_TAG_PRIMITIVE:
            blok_compiler_codegen_primitive(s, blok_primitive_from_obj(sexpr_head), args);
            break;
        case BLOK_TAG_FUNCTION:
            TODO("codegen function call");
        default:
            blok_fatal_error(&statement.src_info, "Invalid statement");
    }
}

void blok_compiler_codegen_body(blok_State * s, blok_ListRef args) {
    fprintf(s->out, "{\n");
    for(blok_Obj * obj = args.ptr; obj < args.ptr + args.len; ++obj) {
        if(obj->tag != BLOK_TAG_LIST) {
            blok_fatal_error(&obj->src_info, "Expected list");
        }
        blok_compiler_codegen_statement(s, *obj);
    }
    fprintf(s->out, "}\n");
}

void blok_compiler_compile_primitive_procedure(blok_State * s, blok_ListRef args) {
    assert(args.len >= 3);
    //blok_Symbol return_type_name = blok_symbol_from_obj(args.ptr[0]);
    blok_Obj return_type_obj = blok_evaluator_eval(s, args.ptr[0]); 
    blok_Type return_type = return_type_obj.as.data;
    //blok_TypeData return_type_data = blok_type_get_data(s, return_type);
    blok_Symbol name = blok_symbol_from_obj(args.ptr[1]);
    blok_List * params = blok_list_from_obj(args.ptr[2]);

    blok_compiler_codegen_type(s, return_type);
    fprintf(s->out, " ");
    blok_compiler_codegen_identifier(s, name);
    blok_compiler_codegen_params(s, params);
    fprintf(s->out, " ");
    blok_ListRef body = blok_slice_tail(args, 3);
    blok_compiler_codegen_body(s, body);
}

void blok_compiler_apply_toplevel_primitive(blok_State * s, const blok_Primitive * p, blok_ListRef args) {
    //blok_Primitive * p = blok_primitive_from_obj(prim);
    blok_Signature sig = blok_type_get_data(s, p->signature).as.primitive;
    blok_SourceInfo * src = NULL;
    if(args.len > 0) {
        src = &args.ptr[0].src_info;
    }
    blok_compiler_typecheck_args(s, src, sig, args);
    switch(p->tag) {
        case BLOK_PRIMITIVE_TOPLEVEL_LET:
            blok_compiler_compile_primive_let(s, args);
            break;
        case BLOK_PRIMITIVE_TOPLEVEL_PROCEDURE:
            blok_compiler_compile_primitive_procedure(s, args);
            break;
        default:
            blok_fatal_error(NULL, "Not a toplevel primitive");
    }
}

//void blok_compiler_apply_function(blok_State * s, blok_Obj fn, blok_ListRef args) {
//    blok_Function * f = blok_function_from_obj(fn);
//    blok_Signature sig = f->signature;
//    blok_compiler_typecheck_args(s, &fn.src_info, sig, args);
//    TODO("finish");
//}

void blok_compiler_toplevel_sexpr(blok_State * s, blok_Obj sexpr) {
        blok_List * sexpr_list = blok_list_from_obj(sexpr);
        blok_ListRef args = blok_slice_tail(sexpr_list->items, 1);
        blok_Symbol head = blok_symbol_from_obj(sexpr_list->items.ptr[0]);

        blok_Primitive * it; 
        blok_vec_find(it, &s->toplevel_primitives, it->name == head);
        if(it == NULL) {
            blok_SymbolData data = blok_symbol_get_data(s, head);
            blok_fatal_error(&sexpr.src_info, "Unknown toplevel symbol: %s", data.buf);
        } else {
            blok_compiler_apply_toplevel_primitive(s, it, args);
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
blok_Bindings blok_compiler_toplevel(blok_State * s, blok_List * toplevel) {
    fprintf(s->out, "#include <stdio.h>\n");
    for(int32_t i = 0; i < toplevel->items.len; ++i) {
        blok_Obj sexpr = toplevel->items.ptr[i];
        {
            printf("Compiling: ");
            blok_obj_print(s, sexpr, BLOK_STYLE_CODE);
            printf("\n");
        }
        blok_compiler_validate_sexpr(s, sexpr);
        blok_compiler_toplevel_sexpr(s, sexpr);
    }
    return s->globals;
}

#endif /*BLOK_EVALUATOR_C*/
