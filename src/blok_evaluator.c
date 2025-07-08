#ifndef BLOK_EVALUATOR_C
#define BLOK_EVALUATOR_C

#include "blok_arena.c"
#include "blok_obj.c"

/* TODO, make the contents of a file and a progn different primitives*/
typedef enum blok_Primitive {
    BLOK_PRIMITIVE_TOPLEVEL,
    BLOK_PRIMITIVE_PROCEDURE,
    BLOK_PRIMITIVE_INT,
    BLOK_PRIMITIVE_PRINT
} blok_Primitive;


blok_Obj blok_make_primitive(blok_Primitive data) {
    return (blok_Obj){.tag = BLOK_TAG_PRIMITIVE, .as.data = data};
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
    (void)b;//TODO, remove this function entirely
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
    (void)b;
    (void)prim;
    (void)argc;
    (void)argv;
    //TODO
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
                //assert(blok_scope_lookup(&tmp, b, blok_symbol_from_char_ptr("print")).as.data == BLOK_PRIMITIVE_PRINT);
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


void blok_primitive_procedure(blok_Table * globals, blok_List * sexpr, FILE * output) {
    (void) sexpr;
    (void) globals;
    (void) output;
}

void blok_primitive_toplevel_let(blok_Table * globals, blok_List * sexpr) {
    (void) sexpr;
    (void) globals;
}


//returns a table of globals
blok_Table * blok_primitive_toplevel(blok_Arena * a, blok_List * sexpr, FILE * output) {
    blok_Table * globals = blok_table_allocate(a, 16);
    for(int32_t i = 0; i < sexpr->len; ++i) {
        blok_Obj item = sexpr->items[i];
        if(item.tag != BLOK_TAG_LIST) {
            blok_fatal_error(
                    &item.src_info,
                    "Toplevel forms may only be s-expressions (lists), found a %s",
                    blok_tag_get_name(item.tag));
        }
        blok_List * list = blok_list_from_obj(item);
        if(list->len <= 0) {
            blok_fatal_error(&item.src_info, "Empty toplevel list");
        }
        blok_Obj head = list->items[0];
        if(head.tag != BLOK_TAG_SYMBOL) {
            blok_fatal_error(&head.src_info,
                    "Invalid toplevel form, s-expressions should start "
                    "with a list, found a %s",
                    blok_tag_get_name(head.tag));
        }
        blok_Symbol * sym = blok_symbol_from_obj(head);
        if(blok_symbol_streql(*sym, "let")) {
            blok_primitive_toplevel_let(globals, list);
        } else if(blok_symbol_streql(*sym, "procedure")) {
            blok_primitive_procedure(globals, list, output); //TODO create output file
        } else {
            blok_fatal_error(&head.src_info, "Invalid toplevel s-expression head symbol: '%s'", sym->buf);
        }
    }
    return globals;
}


/*
void blok_compiler_compile_list(blok_Scope * b, blok_List * l, FILE * output) {
    (v


}

void blok_compiler_compile(blok_Scope * b, blok_Obj obj, FILE * output) {
    switch(obj.tag) {
        case BLOK_TAG_NIL:
        case BLOK_TAG_INT:
        case BLOK_TAG_STRING:
            blok_obj_fprint(output, obj, BLOK_STYLE_CODE);
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
                //return value
            }
        case BLOK_TAG_LIST:
            {
                //printf("-->Evaluating ");
                //blok_obj_print(obj, BLOK_STYLE_CODE);
                //printf("\n");
                fflush(stdout);
                blok_List * list = blok_list_from_obj(obj);
                blok_compiler_compile_list(b, list);
            }
        default:
          blok_fatal_error(NULL,
                      "Support for evaluating this type has not been "
                      "implemented yet: %s",
                      blok_tag_get_name(obj.tag));
          break;

}

*/

#endif /*BLOK_EVALUATOR_C*/
