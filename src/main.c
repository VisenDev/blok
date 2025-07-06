#include "blok_arena.c"
#include "blok_obj.c"
#include "blok_reader.c"
#include "blok_evaluator.c"

int main(void) {
    blok_arena_run_tests();
    blok_table_run_tests();

    blok_Scope b = {0};
    b.bindings = blok_table_init_capacity(&b.arena, 32);
    blok_scope_bind_builtins(&b);

    blok_Obj source = blok_reader_read_file(&b.arena, "ideal.blok");
    blok_obj_print(source, BLOK_STYLE_CODE);
    fflush(stdout);

    blok_evaluator_eval(&b, source);
    blok_arena_free(&b.arena);

    return 0;
}
