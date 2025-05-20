#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

void fatal_error(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}

void parse_error(FILE * fp, const char * fmt, ...) {
    const long progress = ftell(fp);
    rewind(fp);
    while(ftell(fp) < progress) {
        fprintf(stderr, "%c", fgetc(fp));
    }
    fprintf(stderr, "\n\n");
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}

typedef enum {
    BLOK_TAG_NIL = 0,
    BLOK_TAG_INT,
    BLOK_TAG_STRING,
    BLOK_TAG_LIST,
    BLOK_TAG_PRIMITIVE
} blok_Tag;


typedef intptr_t blok_Obj;

typedef struct { 
    blok_Obj len;
    blok_Obj cap;
    blok_Obj items[1];
} blok_List;

#define BLOK_OBJ_SIZE sizeof(blok_Obj)
#define BLOK_OBJ_ALIGN 8

static int _function_ptr_align_check[
    __alignof(void(*)(void)) >= 8
    ? 1 : -1
];

blok_Obj blok_make_int(int value) {
    blok_Obj result = {0};
    result = value;
    result <<= BLOK_OBJ_ALIGN;
    result |= BLOK_TAG_INT;
    return result;
}

blok_Obj blok_make_nil(void) {
    blok_Obj result = BLOK_TAG_NIL;
    return result;
}

blok_Obj blok_make_list(const blok_Obj * src, long len) {
    blok_Obj result = (blok_Obj)aligned_alloc(len + 1, BLOK_OBJ_ALIGN);
    char * ptr = (char*)result;
    ptr[len] = 0;
    memcpy(ptr, src, len);
    result |= BLOK_TAG_STRING;
    return result;
}

blok_Obj blok_make_string(const char * str, long len) {
    blok_Obj result = (blok_Obj)aligned_alloc(len + 1, BLOK_OBJ_ALIGN);
    char * ptr = (char*)result;
    ptr[len] = 0;
    memcpy(ptr, str, len);
    result |= BLOK_TAG_STRING;
    return result;
}
