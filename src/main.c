#include "blok_arena.c"
#include "blok_vec.c"
#include "blok_obj.c"
#include "blok_reader.c"
#include "blok_evaluator.c"

int main(void) {

    blok_arena_run_tests();
    blok_slice_run_tests();
    blok_vec_run_tests();
    blok_table_run_tests();

    blok_Arena a = {0};
    blok_State s = {0};
    //blok_arena_reserve(&a, 500, 1024);
    blok_Obj source = blok_reader_read_file(&s, &a, "ideal.blok");
    blok_obj_print(&s, source, BLOK_STYLE_CODE);
    fflush(stdout);

    FILE * output = fopen("a.out.c", "w");
    blok_primitive_toplevel(&s, &a, blok_list_from_obj(source), output);
    fclose(output);

    blok_arena_free(&a);
    return 0;
}
