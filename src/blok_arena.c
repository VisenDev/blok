#ifndef ARENA_C
#define ARENA_C

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    size_t cap;
    bool active;
    char * ptr;
} blok_Allocation;

typedef struct {
    blok_Allocation * allocations;
    size_t len;
    size_t cap;
} blok_Arena;

void blok_arena_fprint_contents(const blok_Arena * a, FILE * fp) {
    fprintf(fp, "(\n");
    for(size_t i = 0; i < a->len; ++i) {
        const blok_Allocation item = a->allocations[i];
        fprintf(fp, "    (active:%s cap:%zu)\n", item.active ? "true" : "false", item.cap);
    }
    fprintf(fp, ")\n");
}

blok_Allocation blok_arena_allocation_new(size_t bytes) {
    /*allocate new*/
    blok_Allocation new = (blok_Allocation){
        .active = false,
        .cap = bytes,
        .ptr = malloc(bytes),
    };
    assert(new.ptr != NULL);
    return new;
}

void blok_arena_append_allocation(blok_Arena * a, blok_Allocation new) {
    if(a->cap == 0) {
        assert(a->len == 0);
        assert(a->allocations == NULL);
        a->cap = 4;
        a->allocations = malloc(sizeof(blok_Allocation) * a->cap);
    } else if(a->len + 1 >= a->cap) {
        a->cap = a->cap * 2 + 1;
        a->allocations = realloc(a->allocations, sizeof(blok_Allocation) * a->cap);
    }
    a->allocations[a->len++] = new;
}


void blok_arena_reserve(blok_Arena * a, size_t num_allocations, size_t bytes_per_allocation) {
    for(size_t i = 0; i < num_allocations; ++i) {
        blok_arena_append_allocation(a, blok_arena_allocation_new(bytes_per_allocation));
    }
}


void * blok_arena_alloc(blok_Arena * a, size_t bytes) {

    /*search for available allocation*/
    for(size_t i = 0; i < a->len; ++i) {
        blok_Allocation * allocation = &a->allocations[i];
        if(allocation->active == false && allocation->cap >= bytes) {
            allocation->active = true; 
            return allocation->ptr;
        }
    }

    blok_Allocation new = blok_arena_allocation_new(bytes);
    new.active = true;
    blok_arena_append_allocation(a, new);
    return new.ptr;
}

void * blok_arena_realloc(blok_Arena * a, void * ptr, size_t bytes) {
    for(size_t i = 0; i < a->len; ++i) {
        blok_Allocation * allocation = &a->allocations[i];
        if(allocation->ptr == ptr) {
            allocation->ptr = realloc(allocation->ptr, bytes);
            allocation->cap = bytes;
            return allocation->ptr;
        }
    }
    assert(0 && "Tried to realloc unknown ptr");
    return NULL;
}

void blok_arena_reclaim(blok_Arena * a, void * ptr) {
    for(size_t i = 0; i < a->len; ++i) {
        blok_Allocation * allocation = &a->allocations[i];
        if(allocation->ptr == ptr) {
            allocation->active = false;
            return;
        }
    }
    assert(0 && "Tried to reclaim unknown ptr");
}

void blok_arena_reset(blok_Arena * a) {
    for(size_t i = 0; i < a->len; ++i) {
        a->allocations[i].active = false;
    }
}

void blok_arena_free(blok_Arena * a) {
    for(size_t i = 0; i < a->len; ++i) {
        free(a->allocations[i].ptr);
    }
    free(a->allocations);
    a->cap = 0;
    a->len = 0;
}

void blok_arena_run_tests(void) {
    blok_Arena a = {0};
    {
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
    }
    blok_arena_reset(&a);
    blok_arena_reserve(&a, 10, 1000);
    {
        char * mem1 = blok_arena_alloc(&a, 500);
        memset(mem1, 7, 500);
        char * mem2 = blok_arena_alloc(&a, 1500);
        memset(mem2, 9, 1500);
        for(int i = 0; i < 500; ++i) {
            assert(mem1[i] == 7);
        }
        for(int i = 0; i < 1500; ++i) {
            assert(mem2[i] == 9);
        }
    }
    blok_arena_free(&a);
}

#endif /*ARENA_C*/
