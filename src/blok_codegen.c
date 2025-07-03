#include "blok_obj.c"

struct blok_Codegen;

typedef struct blok_Codegen {
    void * ctx;
    void (*begin_function)(
            void * ctx,
            const char * name,
            blok_Type return_type,
            blok_KeyValue * params,
            int32_t param_count
            );
} blok_Codegen;


const char * blok_c_translate_type(blok_Type type) {
    switch(type.tag) {
        case BLOK_TYPETAG_INT:
            return "int64_t";
        case BLOK_TYPETAG_LIST:
            return "blok_List";
        case BLOK_TYPETAG_OBJ:
            return "blok_Obj";
        case BLOK_TYPETAG_STRING:
            return "blok_String":
    }
}

void blok_c_begin_function(
            void * ctx,
            const char * name,
            blok_Type return_type,
            blok_KeyValue * params,
            int32_t param_count
) {
    printf("%s %s (", blok_c_translate_type(return_type), name);
    for(int i = 0; i < param_count; ++i) {
        
    }

}
