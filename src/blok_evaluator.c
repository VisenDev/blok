#ifndef BLOK_EVALUATOR_C
#define BLOK_EVALUATOR_C

#include "blok_arena.c"
#include "blok_obj.c"

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
#endif /*BLOK_EVALUATOR_C*/
