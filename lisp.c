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
    fflush(stderr);
    abort();
}

void parse_error(FILE * fp, const char * fmt, ...) {
    const long progress = ftell(fp);
    long i = 0;
    rewind(fp);
    while(ftell(fp) < progress) {
        fprintf(stderr, "%c", fgetc(fp));
    }
    fprintf(stderr, "\n\n");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}

void * alloc(long bytes) {
    int i = 0;
    const int max_attempts = 10;
    void * mem = malloc(bytes);
    while(((long)mem & (~7)) != (long)mem) {
        if(i > max_attempts) {
            fatal_error("allocating aligned memory failed");
        }
        free(mem);
        mem = malloc(bytes);
        ++i;
    }
    return mem;
}


typedef enum {
    TAG_NIL,
    TAG_INTEGER,
    TAG_STRING,
    TAG_SYMBOL,
    TAG_PRIMITIVE,
    TAG_LIST,

    /*other types of lists*/
    TAG_PLIST,
    TAG_NAMESPACED_SYMBOL,
    TAG_TYPED_SYMBOL,
} Tag;

struct Obj;
struct Env;

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

typedef struct {
    int lookup_index;
} Symbol;

typedef struct Obj (*Primitive)(struct Obj * env, int argc, struct Obj * argv);

typedef struct Obj {
    Tag tag;
    union {
        int integer;
        Symbol symbol;
        Primitive primitive;
        String * string;
        List * list;
        /*Table table;*/
    } as;
} Obj; 


/* Symbol handling */
#define max_symbols 4096
#define max_symbol_len 128
static char symbols[max_symbols][max_symbol_len];
static int symbols_len;


int streql(const char * a, const char * b) {
    return strncmp(a, b, max_symbol_len) == 0;
}

/* Constructors*/
Obj make_symbol(const char * name) {
    Obj result = {0}; 
    result.tag = TAG_SYMBOL;
    int i = 0;
    for(i = 0; i < symbols_len; ++i) {
        if(streql(name, symbols[i])) {
            result.as.symbol.lookup_index = i;
            return result;
        }
    }
    if(symbols_len >= max_symbol_len) {
        fatal_error("Symbol storage array overflow");
    } 
    memmove(symbols[symbols_len], name, strlen(name));
    ++symbols_len;
    result.as.symbol.lookup_index = symbols_len - 1;
    return result;
}

Obj make_primitive(Primitive fn) {
    Obj result = {0}; 
    result.tag = TAG_PRIMITIVE;
    result.as.primitive = fn;
    return result;
}


Obj make_list(int initial_capacity) {
    Obj result = {0};
    assert(initial_capacity >= 0);
    result.tag = TAG_LIST;
    result.as.list = malloc(sizeof(List));
    result.as.list->items = malloc(sizeof(Obj) * initial_capacity);
    result.as.list->len = 0;
    result.as.list->cap = initial_capacity;
    return result;
}


Obj make_integer(int value) {
    Obj result = {0};
    result.tag = TAG_INTEGER;
    result.as.integer = value;
    return result;
}

Obj make_string(const char * string) {
    Obj result = {0};
    result.tag = TAG_STRING;
    result.as.list = malloc(sizeof(String));
    result.as.string->items = strdup(string);
    result.as.string->len = strlen(result.as.string->items);
    result.as.string->cap = result.as.string->len;
    return result;
}


Obj make_nil(void) {
    Obj result = {0};
    result.tag = TAG_NIL;
    return result;
}

void obj_free_recursive(Obj * self) { 
    int i = 0;
    switch(self->tag) {
    case TAG_STRING:
        free(self->as.string->items);
        break;
    case TAG_LIST:
        for(int i = 0; i < self->as.list->len; ++i) {
            obj_free_recursive(&self->as.list->items[i]);
        }
        free(self->as.list->items);
        free(self->as.list);
        break;
    default:
        break;
    }
    self->tag = TAG_NIL;
}


/*prototype for duplication*/
Obj obj_dup_recursive(const Obj self);

/* Utility functions */
void list_ensure_capacity(List * self, int new_cap) {
    if(self->cap == 0) {
        self->cap = new_cap;
        self->len= 0;
        self->items = malloc(sizeof(Obj) * self->cap);
    } else if(self->cap < new_cap) {
        self->cap = new_cap;
        self->items = realloc(self->items, self->cap * sizeof(Obj));
    }
}

/* item is copied not referenced */
void list_push(List * self, Obj item) {
    if(self->len >= self->cap) {
        list_ensure_capacity(self, self->cap * 2 + 1);
    }
    self->items[self->len] = obj_dup_recursive(item);
    ++self->len;
}

Obj list_pop(List * self) {
    --self->len;
    return self->items[self->len];
}

Obj list_get(const List * self, int i) {
    if(i <= 0 || i > self->len) {
        return make_nil();
    } else {
        return obj_dup_recursive(self->items[i]);
    }
}

void list_set(List * self, int i, Obj value) {
    assert(i >= 0);
    if(i >= self->len) {
        int j = 0;
        list_ensure_capacity(self, i + 1);
        for(j = 0; j < i + 1; ++j) {
            list_push(self, make_nil());
        }
    }
    obj_free_recursive(&self->items[i]);
    self->items[i] = obj_dup_recursive(value);
}

/*Duplication functions */
Obj list_duplicate(const List * self) {
    int i = 0;
    Obj result = make_list(self->len);
    for(i = 0; i < self->len; ++i) {
        list_push(result.as.list, obj_dup_recursive(self->items[i]));
    }
    return result;
}

/* Duplicate an object */
Obj obj_dup_recursive(const Obj self) {
    switch(self.tag) {
    case TAG_STRING:
        return make_string(self.as.string->items);
    case TAG_NIL:
        return make_nil();
    case TAG_INTEGER:
        return make_integer(self.as.integer);
    case TAG_LIST: 
        return list_duplicate(self.as.list);
    case TAG_PLIST:
    {
        Obj result = list_duplicate(self.as.list);
        result.tag = TAG_PLIST;
        return result;
    }
    case TAG_NAMESPACED_SYMBOL:
    {
        Obj result = list_duplicate(self.as.list);
        result.tag = TAG_NAMESPACED_SYMBOL;
        return result;
    }
    case TAG_TYPED_SYMBOL:
    {
        Obj result = list_duplicate(self.as.list);
        result.tag = TAG_TYPED_SYMBOL;
        return result;
    }
    case TAG_PRIMITIVE:
        return make_primitive(self.as.primitive);
    case TAG_SYMBOL:
        return self;
    default:
        fatal_error("invalid tag: %d", self.tag);
        return make_nil();
    }
}

/* Parser */
int is_number(const char * str) {
    int i = 0;
    for(i = 0; str[i] != 0; ++i) {
        if(!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

char peek(FILE * fp) {
    const char ch = fgetc(fp);
    ungetc(ch, fp);
    return ch;
}

int is_special_character(char ch) {
    return ch == '['
        || ch == ']'
        || ch == '.'
        || ch == ':'
        || ch == '\''
        || ch == '"'
        || ch == '`';
}

Obj parse_integer(FILE * fp) {
    char buf[1024] = {0};
    int i = 0;
    /*printf("parsing integer");*/
    while(isdigit(peek(fp))) {
        buf[i] = fgetc(fp);
        ++i;
    }
    return make_integer(atoi(buf));
}

void skip_whitespace(FILE * fp) {
    while(isspace(peek(fp)) || peek(fp) == '\n') {
        fgetc(fp);
    }
}

int keep_parsing_symbol(FILE * fp) {
    return (!is_special_character(peek(fp)) && !isspace(peek(fp)) && !feof(fp));
}

/*
Obj parse_symbol(FILE * fp) {
    char buf [1024] = {0};
    int i = 0;
    skip_whitespace(fp);
    while(keep_parsing_symbol(fp)) {
        buf[i] = fgetc(fp);
        ++i;
    }
    if(i == 0) {
        fatal_error("Parsing symbol failed on parsing char: %d", peek(fp));
    }
    return make_symbol(buf);
}
*/

Obj parse_expr(FILE * fp);

Obj parse_any_symbol(FILE * fp) {
    char buf [1024] = {0};
    int i = 0;
    skip_whitespace(fp);
    while(keep_parsing_symbol(fp)) {
        buf[i] = fgetc(fp);
        buf[i + 1] = 0;
        ++i;
    }
    if(i == 0) {
        parse_error(fp, "Parsing symbol failed on parsing char: %d", peek(fp));
    }
    skip_whitespace(fp);

    /*typed symbol*/
    if(peek(fp) == ':') {
        Obj result = make_list(2);
        result.tag = TAG_TYPED_SYMBOL;
        list_push(result.as.list, make_symbol(buf));
        fgetc(fp);
        list_push(result.as.list, parse_expr(fp));
        skip_whitespace(fp);
        return result;
        
    /*namespaced symbol*/
    } else if(peek(fp) == '.' ) {
        Obj result = make_list(2);
        result.tag = TAG_NAMESPACED_SYMBOL;
        list_push(result.as.list, make_symbol(buf));
        fgetc(fp);
        skip_whitespace(fp);
        while(peek(fp) == '.') {
            i = 0;
            while(keep_parsing_symbol(fp)) {
                buf[i] = fgetc(fp);
                buf[i + 1] = 0;
                ++i;
            }
            skip_whitespace(fp);
            list_push(result.as.list, make_symbol(buf));
        }
        return result;

    /*normal symbol*/
    } else {
        return make_symbol(buf);
    }

}

Obj parse_string(FILE * fp) {
    char buf[1024] = {0};
    int i = 0;
    /*printf("parsing string");*/
    assert(fgetc(fp) == '"');
    while(peek(fp) != '"') {
        if(feof(fp)) {
            parse_error(fp, "unexpected end of input when parsing string");
        }
        buf[i] = fgetc(fp);
        ++i;
    }
    assert(fgetc(fp) == '"');
    return make_string(buf);
}

Obj parse_list(FILE * fp);

Obj parse_expr(FILE * fp) {
    skip_whitespace(fp);
    if(feof(fp)) {
        return make_nil();
    }
    if(peek(fp) == '"') {
        return parse_string(fp);
    } else if(peek(fp) == '\'') {
        Obj result = make_list(2);
        fgetc(fp);
        list_push(result.as.list, make_symbol("quote"));
        list_push(result.as.list, parse_expr(fp));
        return result;
    } else if(isdigit(peek(fp))) {
        return parse_integer(fp); 
    } else if (peek(fp) == '[') {
        return parse_list(fp);
    } else if (peek(fp) == ']') {
        parse_error(fp, "Unexpected end of input");
    } else if (!is_special_character(peek(fp))) {
        return parse_any_symbol(fp);
    } else {
        parse_error(fp, "Parsing failure, expected expression");
    }
    return make_nil();
}

Obj parse_list(FILE * fp) {
    Obj result = make_list(0);
    /*printf("parsing list");*/
    assert(fgetc(fp) == '[');
    skip_whitespace(fp);
    if(peek(fp) == ']') {
        fgetc(fp);
        return result;
    }
    while(peek(fp) != ']') {
        skip_whitespace(fp);
        list_push(result.as.list, parse_expr(fp));
        skip_whitespace(fp);
    }
    assert(fgetc(fp) == ']');
    return result;
}

Obj read(FILE * fp) {
    Obj root = make_list(8);

    while(!feof(fp)) {
        list_push(root.as.list, parse_expr(fp));
    }

    return root;
}

void write(FILE * fp, Obj val) {
    int i = 0;
    switch(val.tag) {
    case TAG_NIL:
        fprintf(fp, "nil");
        break;
    case TAG_INTEGER:
        fprintf(fp, "%d", val.as.integer);
        break;
    case TAG_PRIMITIVE:
        fprintf(fp, "<Primitive %p>", val.as.primitive);
        break;
    case TAG_STRING:
        fprintf(fp, "\"%s\"", val.as.string->items);
        break;
    case TAG_SYMBOL:
        fprintf(fp, "%s", symbols[val.as.symbol.lookup_index]);
        break;
    case TAG_LIST:
        fprintf(fp, "[");
        for(i = 0; i < val.as.list->len; ++i) {
            if(i != 0) fprintf(fp, " ");
            write(fp, val.as.list->items[i]);
        }
        fprintf(fp, "]");
        break;
    case TAG_PLIST:
        fprintf(fp, "`[");
        for(i = 0; i < val.as.list->len; ++i) {
            if(i != 0) fprintf(fp, " ");
            write(fp, val.as.list->items[i]);
        }
        fprintf(fp, "]");
        break;
    case TAG_TYPED_SYMBOL:
        assert(val.as.list->len == 2);
        write(fp, val.as.list->items[0]);
        fprintf(fp, ":");
        write(fp, val.as.list->items[1]);
        break;
    case TAG_NAMESPACED_SYMBOL:
        for(i = 0; i < val.as.list->len; ++i) {
            if(i != 0) fprintf(fp, ".");
            write(fp, val.as.list->items[i]);
        }
        break;
    default:
        fatal_error("Invalid object tag: %d", val.tag);
    }
}

Obj env_lookup(Obj * env, Symbol sym) {
    int i = 0;
    assert(env->tag == TAG_LIST);

    fatal_error("todo: figure out how to store environment frames");
    return make_nil();
}

Obj eval_expr(Obj * env, Obj expr) {
    switch(expr.tag) {
    case TAG_NIL: return make_nil();
    case TAG_INTEGER: return make_integer(expr.as.integer);
    case TAG_PRIMITIVE: fatal_error("TODO: figure out primitives");
    case TAG_STRING: return make_string(expr.as.string->items);
    case TAG_SYMBOL: return env_lookup(env, expr.as.symbol);
    case TAG_LIST:  break;
    }


    return expr;
}

int eql(const Obj lhs, const Obj rhs) {
    if(lhs.tag != rhs.tag) {
        return 0;
    } else {
        switch(lhs.tag) {
        case TAG_INTEGER:
            return lhs.as.integer == rhs.as.integer;
        case TAG_NIL:
            return 1;
        case TAG_SYMBOL:
            return lhs.as.symbol.lookup_index == rhs.as.symbol.lookup_index;
        case TAG_PRIMITIVE:
            return lhs.as.primitive == rhs.as.primitive;
        case TAG_STRING:
            return strcmp(lhs.as.string->items, rhs.as.string->items) == 0;
        case TAG_LIST:
            if(lhs.as.list->len != rhs.as.list->len) {
                return 0;
            } else {
                int i = 0;
                for(i = 0; i < lhs.as.list->len; ++i) {
                    if(!eql(lhs.as.list->items[i], rhs.as.list->items[i])) {
                        return 0;
                    }
                }
                return 1;

            }
        default:
            fatal_error("Tag %d not supported yet\n");
        }
    }
    return 0;
}

Obj * plist_get(List * plist, Obj key) {
    Obj * start = plist->items;
    Obj * end = plist->items + plist->len;
    for(;start != end; ++start) {
        if(start->tag != TAG_LIST) {
            write(stderr, *start);
            fatal_error("Object is not an element of a plist");
        } else {
            Obj first = start->as.list->items[0];
        }

    }
}

void plist_set(List * plist, Symbol key, Obj value) {
    int i = 0;

}

void compile_sexpr(Obj src) {
    if(src.tag != TAG_LIST) {
        write(stderr, src);
        fatal_error("Expected top level expression to be a list");
    } else {
        List * list = src.as.list;
        Obj * elems = list->items;
        int count = list->len;
        if(count == 0) {
            write(stderr, src);
            fatal_error("Empty top level expr");
        }
        if(elems[0].tag != TAG_SYMBOL) {
            write(stderr, src);
            fatal_error("Expected sexpr to start with a symbol");
        }

        
    }
    
}

void compile_toplevel(Obj src) {
    assert(src.tag == TAG_LIST);
}


int main() {
    printf("sizeof Obj: %zu\n\n", sizeof(Obj));
    FILE * fp = fopen("test.blok", "r");
    Obj val = read(fp);
    fclose(fp);
    write(stdout, val);
    return 0;
}
