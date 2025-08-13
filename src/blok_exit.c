#ifndef BLOK_EXIT_C
#define BLOK_EXIT_C

#include <stdlib.h>
#include <assert.h>


#define BLOK_NORETURN __attribute__((noreturn))

typedef struct {
    void (*fn)(void*);
    void *ctx;
} blok_Cleanup;

#define BLOK_ON_EXIT_CLEANUP_MAX 64
static blok_Cleanup blok_on_exit_cleanup[BLOK_ON_EXIT_CLEANUP_MAX] = {0};
static int blok_on_exit_cleanup_i = 0;

void blok_on_exit(void (*fn)(void*), void * ctx) {
    assert(blok_on_exit_cleanup_i < BLOK_ON_EXIT_CLEANUP_MAX && "Too much cleanup");
    blok_on_exit_cleanup[blok_on_exit_cleanup_i++] = (blok_Cleanup){fn, ctx};
}

BLOK_NORETURN
void blok_exit(int code) {
    for(int i = 0; i < blok_on_exit_cleanup_i; ++i) {
        blok_Cleanup item = blok_on_exit_cleanup[blok_on_exit_cleanup_i];
        item.fn(item.ctx);
    }
    exit(code);
}

#endif /*BLOK_EXIT_C*/
