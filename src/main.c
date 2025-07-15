#include "blok_arena.c"
#include "blok_vec.c"
#include "blok_obj.c"
#include "blok_reader.c"
#include "blok_evaluator.c"
#include "blok_profiler.c"

int main(void) {
    blok_profiler_init("profile.json");

    blok_arena_run_tests();
    blok_slice_run_tests();
    blok_vec_run_tests();

    blok_State s = blok_state_init();
    blok_Obj source = blok_reader_read_file(&s, &s.persistent_arena, "ideal.blok");
    blok_obj_print(&s, source, BLOK_STYLE_CODE);
    fflush(stdout);

    FILE * output = fopen("a.out.c", "w");
    blok_evaluator_toplevel(&s, blok_list_from_obj(source), output);
    fclose(output);

    blok_state_deinit(&s);
    blok_profiler_deinit();
    return 0;
}
