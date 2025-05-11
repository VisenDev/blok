#ifndef BLOK_H
#define BLOK_H

typedef enum {

    /* flag types */
    TAG_NIL,
    TAG_TRUE,
    TAG_FALSE,

    /* basic types */    
    TAG_INTEGER,
    TAG_FLOATING,
    TAG_SYMBOL,
    TAG_PRIMITIVE,

    /* types stored in lists (dynamic allocation) */
    TAG_LIST, /*basic list*/
    TAG_STRING, /*list containing chars instead of Objects*/
    TAG_PLIST, /*property (association) list*/
    TAG_ERROR, /*property list containing special fields like line, file, and message*/
    TAG_NAMESPACED_SYMBOL, /*list containing only symbols*/
    TAG_TYPED_SYMBOL, /*property list containing type and name*/
} blok_Tag;

typedef struct {
    int lookup_index;
} blok_Symbol;

typedef struct {
    int offset;
} blok_Ptr;

struct blok_Obj;
typedef struct blok_Obj blok_Obj;

typedef blok_Obj (*blok_Primitive) (blok_Obj * env, int argc, blok_Obj * argv);

typedef struct blok_Obj {
    blok_Tag tag;
    union {
        int integer;
        float floating;
        blok_Symbol symbol;
        blok_Primitive primitive;
        blok_Ptr ptr;
    } as;
} blok_Obj; 

_Static_assert(sizeof(blok_Obj) == 4, "hello"
              );

#ifndef BLOK_HEAP_SIZE
#   define BLOK_HEAP_SIZE 320000
#endif /*BLOK_HEAP_SIZE*/

blok_Obj blok_heap[BLOK_HEAP_SIZE] = {0};

#endif /* BLOK_H */
