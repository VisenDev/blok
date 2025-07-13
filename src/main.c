#include "blok_arena.c"
#include "blok_vec.c"
#include "blok_obj.c"
#include "blok_reader.c"
#include "blok_evaluator.c"
#include "blok_profiler.c"

int main(void) {
    FILE * profile = fopen("main.dat", "w");

    fprintf(profile, "[");
    blok_profile(profile, "arena tests", true);
    blok_arena_run_tests();
    blok_profile(profile, "arena tests", false);

    blok_profile(profile, "slice tests", true);
    blok_slice_run_tests();
    blok_profile(profile, "slice tests", false);

    blok_profile(profile, "vec tests", false);
    blok_vec_run_tests();
    blok_profile(profile, "vec tests", false);

    blok_profile(profile, "vec tests", false);
    blok_table_run_tests();
    blok_profile(profile, "vec tests", false);
    fprintf(profile, "]");

    blok_Arena a = {0};
    blok_State s = {0};
    //blok_arena_reserve(&a, 500, 1024);
    blok_Obj source = blok_reader_read_file(&s, &a, "ideal.blok");
    blok_obj_print(&s, source, BLOK_STYLE_CODE);
    fflush(stdout);

    FILE * output = fopen("a.out.c", "w");
    blok_primitive_toplevel(&s, &a, blok_list_from_obj(source), output);
    fclose(output);

    fclose(profile);
    blok_arena_free(&a);
    return 0;
}
