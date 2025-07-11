#ifndef BLOK_READER_C
#define BLOK_READER_C

#include <ctype.h>
#include <errno.h>

#include "blok_obj.c"

/* READER */
typedef struct {
    FILE * fp;
    blok_SourceInfo src_info;
} blok_Reader;

bool blok_reader_is_whitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\t';
}

char blok_reader_getc(blok_Reader * r) {
    char ch = fgetc(r->fp);
    if(ch == '\n') {
        ++r->src_info.line;
        r->src_info.column = 0;
    } else {
        ++r->src_info.column;
    }
    return ch;
}

bool blok_reader_eof(blok_Reader * r) {
    return feof(r->fp);
}

char blok_reader_peek(blok_Reader * r) {
    char ch = fgetc(r->fp);
    ungetc(ch, r->fp);
    return ch;
}

void blok_reader_skip_whitespace(blok_Reader * r) {
    if(blok_reader_eof(r)) return;
    while(blok_reader_is_whitespace(blok_reader_peek(r))) blok_reader_getc(r);
}

void blok_reader_skip_char(blok_Reader * r, char expected) {
    char ch = blok_reader_getc(r);
    if(ch != expected) {
        blok_fatal_error(&r->src_info, "Reader expected '%c', got '%c'", expected, ch);
    }
}

blok_Obj blok_reader_parse_int(blok_Reader * r) {
    assert(isdigit(blok_reader_peek(r)));
    char buf[1024] = {0};
    size_t i = 0;
    while(isdigit(blok_reader_peek(r))) {
        buf[i++] = blok_reader_getc(r);
        assert(i < sizeof(buf));
    }
    char * end;
    errno = 0;
    const long num = strtol(buf, &end, 10);
    if(errno != 0) {
        blok_fatal_error(&r->src_info, "Failed to parse integer, errno: %n", errno);
    }
    blok_Obj result = blok_make_int(num);
    result.src_info = r->src_info;
    return result;
}


#define BLOK_READER_STATE_BASE 0
#define BLOK_READER_STATE_ESCAPE 1

blok_Obj blok_reader_parse_string(blok_Arena * b, blok_Reader * r) {
    blok_reader_skip_char(r, '"');
    int state = BLOK_READER_STATE_BASE;
    blok_String * str = blok_string_allocate(b, 8);
    while(1) {
        char ch = blok_reader_peek(r);
        if(blok_reader_eof(r)) blok_fatal_error(&r->src_info, "Unexpected end of file when parsing string");
        switch(state) {
            case BLOK_READER_STATE_BASE:
                if(ch == '\\') {
                    state = BLOK_READER_STATE_ESCAPE;
                } else if (ch == '"') {
                    blok_reader_skip_char(r, '"');
                    blok_Obj result =  blok_obj_from_string(str);
                    result.src_info = r->src_info;
                    return result;
                } else {
                    blok_string_append(str, blok_reader_getc(r));
                }
                break;
            case BLOK_READER_STATE_ESCAPE:
                blok_reader_skip_char(r, '\\');
                char escape = blok_reader_getc(r);
                switch(escape) {
                    case 'n':
                        blok_string_append(str, '\n');
                        break;
                    case 't':
                        blok_string_append(str, '\t');
                        break;
                    case '"':
                        blok_string_append(str, '"');
                        break;
                    case '\'':
                        blok_string_append(str, '\'');
                        break;
                    default:
                        blok_fatal_error(&r->src_info, "Unknown string escape: \"\\%c\"", escape);
                        break;
                }
                state = BLOK_READER_STATE_BASE;
                break;
        }
    }
}


bool blok_reader_is_operator_symbol_char(char ch) {
    return ch == '<' || ch == '|' || ch == '>' || ch == '=' || ch == '&';
}

bool blok_reader_is_begin_symbol_char(char ch) {
    return isalpha(ch) || ch == '#' || ch == '_'|| blok_reader_is_operator_symbol_char(ch);
}

bool blok_reader_is_symbol_char(char ch) {
    return ch == '_' || ch == '#' || blok_reader_is_operator_symbol_char(ch) || isalpha(ch) || isdigit(ch);
}

blok_Obj blok_reader_parse_obj(blok_Arena * b, blok_Reader * r);

blok_Obj blok_reader_parse_symbol(blok_Arena * a, blok_Reader* r) {
    blok_Symbol sym = {0};
    uint32_t i = 0;

    while(blok_reader_is_symbol_char(blok_reader_peek(r))) {
        sym.buf[i++] = blok_reader_getc(r);
        if(i + 2 > sizeof(sym.buf)) {
            blok_fatal_error(&r->src_info, "symbol too long, %s\n", sym.buf);
        }
    }
    blok_reader_skip_whitespace(r);
    int suffix_i = 0;
    while(blok_reader_peek(r) == '[' || blok_reader_peek(r) == '*') {
        char ch = blok_reader_getc(r);
        if(suffix_i + 1 > BLOK_SYMBOL_MAX_SUFFIX_COUNT) {
          blok_fatal_error(&r->src_info,
                           "Suffix provided for symbol is too long, the "
                           "maximum length of a suffix is %d items",
                           BLOK_SYMBOL_MAX_SUFFIX_COUNT);
        }
        if(ch == '*') {
            sym.suffix[suffix_i++] = BLOK_SUFFIX_ASTERISK;
            sym.suffix[suffix_i]= BLOK_SUFFIX_NIL;
        } else if (ch == '[') {
            blok_reader_skip_whitespace(r);
            char second_ch = blok_reader_getc(r);
            if(second_ch != ']') {
                blok_fatal_error(&r->src_info, "Expected closing bracket, found %c", second_ch);
            }
            sym.suffix[suffix_i++] = BLOK_SUFFIX_BRACKET_PAIR;
            sym.suffix[suffix_i]= BLOK_SUFFIX_NIL;
        }
        blok_reader_skip_whitespace(r);
    }
    blok_reader_skip_whitespace(r);

    if(blok_reader_peek(r) == ':') {
        blok_KeyValue* kv = blok_keyvalue_allocate(a);
        blok_reader_skip_char(r, ':');
        kv->key = sym; 
        kv->value = blok_reader_parse_obj(a, r);
        blok_Obj result = blok_obj_from_keyvalue(kv);
        result.src_info = r->src_info;
        return result;
    } else {
        blok_Symbol * result = blok_arena_alloc(a, sizeof(blok_Symbol));
        *result = sym;
        blok_Obj result_obj = blok_obj_from_symbol(result);
        result_obj.src_info = r->src_info;
        return result_obj;
    }
}

blok_Obj blok_reader_parse_list(blok_Arena * a, blok_Reader * r) {
        blok_reader_skip_char(r, '(');
        blok_reader_skip_whitespace(r);

#       define BLOK_LIST_MAX_SUBLISTS 32
        int sublist_count = 0;
        blok_List * sublists[BLOK_LIST_MAX_SUBLISTS];

        do {
            if(blok_reader_peek(r) == ',') {
                blok_reader_skip_char(r, ',');
            }
            if(sublist_count > BLOK_LIST_MAX_SUBLISTS) {
                blok_fatal_error(&r->src_info, "List contains too many sublists, the maximum amount of sublists is %d", BLOK_LIST_MAX_SUBLISTS);
            }
            sublists[sublist_count++] = blok_list_allocate(a, 4);

            while(blok_reader_peek(r) != ')' && blok_reader_peek(r) != ',' && !blok_reader_eof(r)) {
                blok_list_append(sublists[sublist_count - 1], blok_reader_parse_obj(a, r));
                blok_reader_skip_whitespace(r);
            }

        } while(blok_reader_peek(r) == ',');
        blok_reader_skip_char(r, ')');
        blok_reader_skip_whitespace(r);
        blok_List * result = NULL;
        if(sublist_count <= 1) {
            result = sublists[0];
        } else {
            result = blok_list_allocate(a, sublist_count);
            for(int i = 0; i < sublist_count; ++i) {
                blok_list_append(result, blok_obj_from_list(sublists[i]));
            }
        }

        blok_Obj result_obj = blok_obj_from_list(result);
        result_obj.src_info = r->src_info;
        return result_obj;
}

//blok_Obj blok_reader_read_obj(FILE *);
blok_Obj blok_reader_parse_obj(blok_Arena * a, blok_Reader * r) {
    blok_reader_skip_whitespace(r);
    const char ch = blok_reader_peek(r);

    if(isdigit(ch)) {
        return blok_reader_parse_int(r); 
    } else if(ch == '(') {
        return blok_reader_parse_list(a, r);
    } else if(ch == '"') {
        return blok_reader_parse_string(a, r);
    } else if(blok_reader_is_begin_symbol_char(ch)) {
        return blok_reader_parse_symbol(a, r);
    } else {
        blok_fatal_error(&r->src_info, "Encountered unexpected character '%c' when parsing object", ch);
    }
    /*return blok_make_nil();*/
}

blok_Obj blok_reader_read_file(blok_Arena * a, char const * path) {
    blok_List * result = blok_list_allocate(a, 32);

    blok_Reader r = {0};
    r.fp = fopen(path, "r");

    //size_t len = strlen(path);
    //char * path_copy = blok_arena_alloc(a, len + 1);
    //strncpy(path_copy, path, len);
    strncpy(r.src_info.file, path, sizeof(r.src_info.file));
    r.src_info.line = 1;

    if(r.fp == NULL) {
        blok_fatal_error(NULL, "Failed to open file: %s\n", path);
    }
    //blok_list_append(result, blok_make_symbol(a, "toplevel"));
    blok_reader_skip_whitespace(&r);
    while(!blok_reader_eof(&r) && blok_reader_peek(&r) != ')') {
        blok_list_append(result, blok_reader_parse_obj(a, &r));
        blok_reader_skip_whitespace(&r);
    }
    fclose(r.fp);
    return blok_obj_from_list(result);
}

#endif /*BLOK_READER_C*/
