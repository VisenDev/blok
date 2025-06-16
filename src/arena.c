#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

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
        a->cap = 2;
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
            region->cap = bytes;
            region->ptr = realloc(region->ptr, region->cap);  
            return region->ptr;
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
