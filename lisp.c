#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

void fatal_error(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

typedef enum {
    TAG_NIL,
    TAG_INTEGER,
    TAG_STRING,
    TAG_SYMBOL,
    TAG_LIST,
} Tag;

struct Obj;

typedef struct {
    struct Obj * items;
    int len;
    int cap;
} List;

typedef struct {
    char * items;
    int len;
    int cap;
} String;

typedef struct Obj {
    Tag tag;
    union {
        int integer;
        String str; /*string or symbol*/
        List list;
    } as;
} Obj; 

/* Constructors*/
Obj make_integer(int value) {
    Obj result = {0};
    result.tag = TAG_INTEGER;
    result.as.integer = value;
    return result;
}

Obj make_string(const char * string) {
    Obj result = {0};
    result.tag = TAG_STRING;
    result.as.str.items = strdup(string);
    result.as.str.len = strlen(result.as.str.items);
    result.as.str.cap = result.as.str.len;
    return result;
}

Obj make_symbol(const char * symbol) {
    Obj result = make_string(symbol);
    result.tag = TAG_SYMBOL;
    return result;
}

Obj make_list(int initial_capacity) {
    Obj result = {0};
    assert(initial_capacity >= 0);
    result.tag = TAG_LIST;
    result.as.list.items = malloc(sizeof(Obj) * initial_capacity);
    result.as.list.len = 0;
    result.as.list.cap = initial_capacity;
    return result;
}

Obj make_nil(void) {
    Obj result = {0};
    result.tag = TAG_NIL;
    return result;
}

/* Utility functions */
void list_push(Obj * self, Obj item) {
    if(self->tag != TAG_LIST) {
        fatal_error("Attempted to push item to non-list: %p", self); 
    }
    if(self->as.list.cap == 0) {
        self->as.list.cap = 8;
        self->as.list.cap = 0;
        self->as.list.items = malloc(sizeof(Obj) * self->as.list.cap);
    } else if(self->as.list.len >= self->as.list.cap) {
        self->as.list.cap *= 2;
        self->as.list.items = realloc(self->as.list.items, self->as.list.cap * sizeof(Obj));
    }
    self->as.list.items[self->as.list.len] = item;
    ++self->as.list.len;
}

Obj list_pop(Obj * self) {
    if(self->tag != TAG_LIST) {
        fatal_error("Attempted to pop item from non-list: %p", self); 
    }
    --self->as.list.len;
    return self->as.list.items[self->as.list.len];
}

void obj_free_recursive(Obj * self) { 
    int i = 0;
    switch(self->tag) {
    case TAG_STRING:
    case TAG_SYMBOL:
        free(self->as.str.items);
        break;
    case TAG_LIST:
        for(int i = 0; i < self->as.list.len; ++i) {
            obj_free_recursive(&self->as.list.items[i]);
        }
        free(self->as.list.items);
        break;
    default:
        break;
    }
    self->tag = TAG_NIL;
}

int main() {
    printf("size: %zu\n", sizeof(Obj));
    return 0;
}
