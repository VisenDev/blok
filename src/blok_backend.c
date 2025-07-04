#include "blok_obj.c"

struct blok_Backend;

typedef struct blok_Backend {
    void * ctx;
    void (*function_begin)(
            void * ctx,
            const char * name,
            blok_Type return_type,
            blok_KeyValue * params,
            int32_t param_count);
    void (*function_end)(void * ctx);
    void (*variable_create)(
            void * ctx,
            const char * name, 
            blok_Type type,
            blok_Obj value);
    void (*variable_set)(
            void * ctx,
            const char * name,
            blok_Obj value);
} blok_Backend;


const char * blok_backend_c99_translate_type(blok_Type type) {
    switch(type.tag) {
        case BLOK_TYPETAG_INT:
            return "int64_t";
        case BLOK_TYPETAG_LIST:
            return "blok_List";
        case BLOK_TYPETAG_OBJ:
            return "blok_Obj";
        case BLOK_TYPETAG_STRING:
            return "blok_String";
    }
}

void blok_c_begin_function(
        void * ctx,
        const char * name,
        blok_Type return_type,
        blok_KeyValue * params,
        int32_t param_count
        ) {
    printf("%s %s (", blok_backend_c99_translate_type(return_type), name);
    for(int i = 0; i < param_count; ++i) {

    }

}
