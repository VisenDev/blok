#include "blok_arena.c"
#include "blok_obj.c"
#include "blok_reader.c"
#include "blok_evaluator.c"

int main(void) {
    blok_arena_run_tests();
    blok_table_run_tests();

    BLOK_LOG("ran tests");
    blok_Arena a = {0};
    blok_Obj source = blok_reader_read_file(&a, "ideal.blok");
    BLOK_LOG("read file");
    blok_obj_print(source, BLOK_STYLE_CODE);
    fflush(stdout);

    FILE * output = fopen("a.out.c", "w");
    blok_primitive_toplevel(&a, blok_list_from_obj(source), output);
    fclose(output);

    blok_arena_free(&a);
    return 0;
}
