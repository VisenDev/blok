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


/* Parser */
#define max_token_len 1024
char * get_token(FILE * fp) {
    static char buf[max_token_len];
    long i = 0;
    if(feof(fp)) {
        return NULL;
    } else {
        char ch = fgetc(fp);
        /*skip leading whitespace*/
        while(isspace(ch) && !feof(fp)) {
            ch = fgetc(fp);
        }

        for(i = 0; !feof(fp); ++i) {
            if(i > sizeof(buf)) fatal_error("Token is too long: %s", buf);
            buf[i] = ch;
            buf[i + 1] = 0;
            ch = fgetc(fp);
            if(ch == '(' || ch == ')' || ch == '"' || ch == '\'' || isspace(ch)) {
                ungetc(ch, fp);
                break;
            }
        }
    }
    return buf;
}


int streql(const char * a, const char * b) {
    return strncmp(a, b, max_token_len) == 0;
}

/* expects that the opening " has already been parsed */
Obj parse_string(FILE * fp) {
    char buf[max_token_len] = {0};
    int i = 0;
    int slash_mode = 0;
    char ch = fgetc(fp);
    
    while(!feof(fp)) {
        if(slash_mode) {
            if(ch == '"') {
                buf[i] = '"';
                ++i;
            } else if(ch == 'n') {
                buf[i] = '\n';
                ++i;
            } else if(ch == '\\') {
                buf[i] = '\\';
                ++i;
            } else {
                fatal_error("Invalid escape sequence found in string");
            }
            slash_mode = 0;
        } else {
            if(ch == '"') {
                break;      
            } else if(ch == '\\') {
                slash_mode = 1;
            } else {
                buf[i] = ch;
                ++i;
            }
        }
        ch = fgetc(fp);
    }
    return make_string(buf);

}

/*expects that the opening bracket has been parsed*/
Obj parse_list(FILE * fp) {
    Obj root = make_list(0);
    char * tok = get_token(fp);
    while(tok != NULL && !streql("]", tok)) {
        if(streql(tok, "\"")){
            list_push(&root, parse_string(fp));
        } else if() {

        }
    }
}

Obj read(FILE * fp) {
    Obj root = make_list(8);
    char * tok = get_token(fp);

    while(tok != NULL) {
        
    }

    return root;
}

int main() {
    printf("size: %zu\n", sizeof(Obj));
    return 0;
}
