#ifndef BLOK_VEC_C
#define BLOK_VEC_C

#include "blok_arena.c"
#include <stdint.h>

#define blok_Slice(Type)         \
    struct {                     \
        Type * ptr;              \
        int32_t len;             \
    }

#define blok_slice_from_array(array) {array, sizeof(array) / sizeof(array[0])}

#define blok_slice_tail(slice, i) { (slice).ptr + i, (slice).len - i}

#define blok_slice_foreach(Type, iterator, slice) \
    for(Type * iterator = (slice).ptr; iterator < (slice).ptr + (slice).len; ++iterator)

#define blok_slice_get(slice, i) (slice).ptr[(i)]

void blok_slice_run_tests(void) {
    blok_profiler_do("slice_run_tests") {
        int buf[10] = {0};
        blok_Slice(int) slice = blok_slice_from_array(buf);
        for(int i = 0; i < slice.len; ++i) {
            blok_slice_get(slice, i) = i;
            printf("%d", blok_slice_get(slice, i));
        }
        printf("\n");
        blok_Slice(int) subrange = blok_slice_tail(slice, 5);
        blok_slice_foreach(int, it, subrange) {
            printf("%d", *it);
        }
        printf("\n");
        (void)subrange;

    }
}

#define blok_Vec(Type)           \
    struct {                     \
        blok_Slice(Type) items;  \
        int32_t cap;             \
        Type _item;              \
    }


#define blok_vec_append(vec_ptr, arena_ptr, item) do { \
    blok_profiler_do("blok_vec_append") { \
        if((vec_ptr)->cap <= 0) { \
            assert((vec_ptr)->items.ptr == NULL); \
            (vec_ptr)->cap = 2;    \
            (vec_ptr)->items.ptr = blok_arena_alloc(arena_ptr, (vec_ptr)->cap * sizeof((vec_ptr)->_item)); \
        } else if((vec_ptr)->items.len + 1 > (vec_ptr)->cap) { \
            (vec_ptr)->cap = (vec_ptr)->cap * 2 + 1; \
            (vec_ptr)->items.ptr = blok_arena_realloc(arena_ptr, (vec_ptr)->items.ptr, (vec_ptr)->cap * sizeof(vec_ptr)->_item); \
        } \
        (vec_ptr)->items.ptr[(vec_ptr)->items.len++] = item; \
    } \
} while (0)

#define blok_vec_get(vec_ptr, i) blok_slice_get((vec_ptr)->items, i)


#define blok_vec_foreach(Type, iterator, vec) \
    for(Type * iterator = (vec)->items.ptr; iterator < (vec)->items.ptr + (vec)->items.len; ++iterator)

void blok_vec_run_tests(void) {
    blok_profiler_do("vec_run_tests") {
        blok_Vec(int) v = {0};
        blok_Arena a = {0};

        blok_vec_append(&v, &a, 1);
        blok_vec_append(&v, &a, 2);
        blok_vec_append(&v, &a, 3);
        blok_vec_append(&v, &a, 4);

        assert(v.items.ptr[0] == 1);
        assert(v.items.ptr[1] == 2);
        assert(v.items.ptr[2] == 3);
        assert(v.items.ptr[3] == 4);

        blok_arena_free(&a);
    }
}

#define blok_Optional(Type) struct { bool valid; Type value; }

#define blok_optional_nil(Type) (blok_Optional(Type) {false}


#endif /*BLOK_VEC_C*/
