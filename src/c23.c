#include <stdlib.h>
#include <assert.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

void fatal_error(FILE * fp, const char * fmt, ...) {
    if(fp != NULL) {
        const long progress = ftell(fp);
        rewind(fp);
        while(ftell(fp) < progress) {
            fprintf(stderr, "%c", fgetc(fp));
        }
        fprintf(stderr, "\n\n");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}

typedef enum {
    BLOK_TAG_NIL = 0,
    BLOK_TAG_INT,
    BLOK_TAG_LIST,
    BLOK_TAG_PRIMITIVE,
    BLOK_TAG_STRING,
} blok_Tag;

typedef union {
    void * ptr;
    struct {
        int32_t data;
        uint16_t __pad1;
        uint8_t  __pad2;
        uint8_t tag; 
    };
} blok_Obj;

typedef struct { 
    int32_t len;
    int32_t cap;
    blok_Obj items[];
} blok_List;

typedef struct {
    int32_t len;
    char ptr[];
} blok_String;

_Static_assert(alignof(blok_List *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(blok_Obj *) >= 8, "Alignment of pointers is too small");
_Static_assert(alignof(void(*)(void)) >= 8, "Alignment of function pointers is too small");
_Static_assert(sizeof(void*) == 8, "Expect pointers to be 8 bytes");

blok_Obj blok_make_int(int32_t data) {
    return (blok_Obj){.tag = BLOK_TAG_INT, .data = data};
}
blok_Obj blok_make_nil(void) {
    return (blok_Obj){0};
}


blok_Obj blok_make_list(int32_t initial_capacity) {
    const int32_t size = (sizeof(blok_List) + sizeof(blok_Obj) * initial_capacity);
    blok_List * list = malloc(size);
    memset(list, 0, size);
    blok_Obj result = {.ptr = list};
    result.tag = BLOK_TAG_LIST;
    return result;
}

blok_Obj blok_make_string(const char * str) {
    const int32_t len = strlen(str);
    const int32_t size = sizeof(blok_String) + len;
    blok_String * blok_str = malloc(size);
    blok_str->len = len;
    memcpy(blok_str->ptr, str, len);

    blok_Obj result = {.ptr = blok_str};
    result.tag = BLOK_TAG_STRING;
    return result;

}

blok_List * blok_list_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_LIST);
    obj.tag = 0;
    return (blok_List *) obj.ptr;
}
blok_String * blok_string_from_obj(blok_Obj obj) {
    assert(obj.tag == BLOK_TAG_STRING);
    obj.tag = 0;
    return (blok_String *) obj.ptr;
}

void blok_print(blok_Obj obj) {
    switch(obj.tag) {
        case BLOK_TAG_NIL: 
            printf("nil");
            break;
        case BLOK_TAG_INT:
            printf("%d", obj.data);
            break;
        case BLOK_TAG_PRIMITIVE:
            obj.tag = 0;
            printf("<Primitive %p>", obj.ptr);
            break;
        case BLOK_TAG_LIST:
        {
            blok_List * list = blok_list_from_obj(obj);
            printf("(");
            for(int i = 0; i < list->len; ++i) {
                if(i != 0) printf(", ");
                blok_print(list->items[i]);
            }
            printf(")");
            break;
        }
        case BLOK_TAG_STRING:
            printf("%s", blok_string_from_obj(obj)->ptr);
            break;
        default:
            printf("UNSUPPORTED TYPE");
            break;
    }
}

void blok_print_as_bytes(blok_Obj obj) {
    uint8_t *bytes = (uint8_t*)&obj;
    for(int i = 0; i < 8; ++i) {
        printf("%d", bytes[i]);
    }
    puts("");
}

int main(void) {
    blok_Obj c = blok_make_list(10);
    blok_List * l = blok_list_from_obj(c);
    l->len = 10;
    l->items[2] = blok_make_int(1234);
    l->items[8] = blok_make_string("hi there hello");
    blok_print(c);

    return 0;
}
