#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    size_t cap;
    bool active;
    char * ptr;
} blok_Region;

typedef struct {
    blok_Region * regions;
    size_t len;
    size_t cap;
} blok_Arena;

void * blok_arena_alloc(blok_Arena * a, size_t bytes) {
    /*search for available region*/
    for(size_t i = 0; i < a->len; ++i) {
        blok_Region * region = &a->regions[i];
        if(region->active == false && region->cap >= bytes) {
            region->active = true; 
            return region->ptr;
        }
    }

    /*allocate new*/
    blok_Region new = (blok_Region){
        .active = true,
        .cap = bytes,
        .ptr = malloc(bytes),
    };
    assert(new.ptr != NULL);

    if(a->cap == 0) {
        assert(a->len == 0);
        assert(a->regions == NULL);
        a->cap = 4;
        a->regions = malloc(sizeof(blok_Region) * a->cap);
    } else if(a->len + 1 >= a->cap) {
        a->cap = a->cap * 2 + 1;
        a->regions = realloc(a->regions, sizeof(blok_Region) * a->cap);
    }
    a->regions[a->len++] = new;

    return new.ptr;
}

void * blok_arena_realloc(blok_Arena * a, void * ptr, size_t bytes) {
    for(size_t i = 0; i < a->len; ++i) {
        blok_Region * region = &a->regions[i];
        if(region->ptr == ptr) {
            assert(region->active);
            assert(region->cap <= bytes);
            region->active = false;
            char * mem = blok_arena_alloc(a, bytes);
            memcpy(mem, region->ptr, region->cap);
            return mem;
        }
    }
    assert(0 && "Tried to realloc unknown ptr");
}

void blok_arena_reclaim(blok_Arena * a, void * ptr) {
    for(size_t i = 0; i < a->len; ++i) {
        blok_Region * region = &a->regions[i];
        if(region->ptr == ptr) {
            region->active = false;
            return;
        }
    }
    assert(0 && "Tried to reclaim unknown ptr");

}

void blok_arena_rewind(blok_Arena * a) {
    for(size_t i = 0; i < a->len; ++i) {
        a->regions[i].active = false;
    }
}

void blok_arena_free(blok_Arena * a) {
    for(size_t i = 0; i < a->len; ++i) {
        free(a->regions[i].ptr);
    }
    free(a->regions);
    a->cap = 0;
    a->len = 0;
}

void blok_arena_run_tests(void) {
    blok_Arena a = {0};
    char * mem = blok_arena_alloc(&a, 16);
    memset(mem, 0, 16);
    char * mem2 = blok_arena_alloc(&a, 1000);
    memset(mem2, 0, 1000);
    blok_arena_reclaim(&a, mem);
    char * mem3 = blok_arena_alloc(&a, 8);
    assert(mem3 == mem);
    char * mem4 = blok_arena_realloc(&a, mem3, 1000);
    memset(mem4, 1, 1000);
    char * mem5 = blok_arena_alloc(&a, 500);
    blok_arena_reclaim(&a, mem4);
    mem5 = blok_arena_realloc(&a, mem5, 1000);
    memset(mem5, 2, 1000);
    blok_arena_free(&a);
}
