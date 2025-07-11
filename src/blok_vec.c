#ifndef BLOK_VEC_C
#define BLOK_VEC_C

#include "blok_arena.c"
#include <stdint.h>

#define blok_Vec(Type) struct { Type * items; int32_t len; int32_t cap; }

#define blok_vec_item_sizeof(vec_ptr) (sizeof((vec_ptr)->items[0]))
#define blok_vec_new_cap(vec_ptr) ((vec_ptr)->cap * 2 + 1)
#define blok_vec_cap_in_bytes(vec_ptr) (blok_vec_item_sizeof(vec_ptr) * (vec_ptr)->cap)

#define blok_vec_append(vec_ptr, arena_ptr, item) do { \
    if((vec_ptr)->cap <= 0) { \
        (vec_ptr)->cap = 2;    \
        (vec_ptr)->items = blok_arena_alloc(arena_ptr, blok_vec_cap_in_bytes(vec_ptr)); \
    } else if((vec_ptr)->len + 1 > (vec_ptr)->cap) { \
        (vec_ptr)->cap = blok_vec_new_cap(vec_ptr); \
        (vec_ptr)->items = blok_arena_realloc(arena_ptr, (vec_ptr)->items, blok_vec_cap_in_bytes(vec_ptr)); \
    } \
    (vec_ptr)->items[(vec_ptr)->len++] = item; \
} while (0)

void blok_vec_run_tests(void) {
    blok_Vec(int) v = {0};
    blok_Arena a = {0};

    blok_vec_append(&v, &a, 1);
    blok_vec_append(&v, &a, 2);
    blok_vec_append(&v, &a, 3);
    blok_vec_append(&v, &a, 4);

    assert(v.items[0] == 1);
    assert(v.items[1] == 2);
    assert(v.items[2] == 3);
    assert(v.items[3] == 4);

    blok_arena_free(&a);
}



#endif /*BLOK_VEC_C*/
