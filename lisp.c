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
    TAG_PRIMITIVE,
    TAG_TABLE,
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

typedef struct {
    List dense;
    List dense_to_sparse_mapping;
    List sparse;
} Table;

typedef struct Obj (*Primitive)(struct Obj * env, int argc, struct Obj * argv);

typedef struct Obj {
    char is_const; /*this is a boolean (stored in a char)*/
    Tag tag;
    union {
        int integer;
        Symbol symbol;
        String string;
        Primitive primitive;
        List list;
        Table table;
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
    result.as.list.items = malloc(sizeof(Obj) * initial_capacity);
    result.as.list.len = 0;
    result.as.list.cap = initial_capacity;
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
    result.as.string.items = strdup(string);
    result.as.string.len = strlen(result.as.string.items);
    result.as.string.cap = result.as.string.len;
    return result;
}


Obj make_nil(void) {
    Obj result = {0};
    result.tag = TAG_NIL;
    return result;
}

Obj make_table(void) {
    Obj result = {0};
    result.tag = TAG_TABLE;
    result.as.table.sparse = make_list(0).as.list;
    result.as.table.dense = make_list(0).as.list;
    result.as.table.dense_to_sparse_mapping = make_list(0).as.list;
    return result;
}


void obj_free_recursive(Obj * self) { 
    int i = 0;
    switch(self->tag) {
    case TAG_STRING:
        free(self->as.string.items);
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
        list_push(&result.as.list, obj_dup_recursive(self->items[i]));
    }
    return result;
}

Obj table_duplicate(const Table * self) {
    Obj result = make_table();
    result.as.table.dense = list_duplicate(&self->dense).as.list;
    result.as.table.sparse = list_duplicate(&self->sparse).as.list;
    result.as.table.dense_to_sparse_mapping = list_duplicate(&self->dense_to_sparse_mapping).as.list;
    return result;
}


/* Duplicate an object */
Obj obj_dup_recursive(const Obj self) {
    switch(self.tag) {
    case TAG_STRING:
        return make_string(self.as.string.items);
    case TAG_NIL:
        return make_nil();
    case TAG_INTEGER:
        return make_integer(self.as.integer);
    case TAG_LIST: 
        return list_duplicate(&self.as.list);
    case TAG_PRIMITIVE:
        return make_primitive(self.as.primitive);
    case TAG_SYMBOL:
        return self;
    case TAG_TABLE:
        return table_duplicate(&self.as.table);
    }
}

/* Table utilities */
Obj table_get(Table * self, Symbol key) {
    const Obj i = list_get(&self->sparse, key.lookup_index);
    if(i.tag == TAG_NIL) {
        return make_nil();
    } else {
        assert(i.tag == TAG_INTEGER && "invalid type"); 
        assert(i.as.integer >= 0);
        assert(i.as.integer < self->dense.len);
        return obj_dup_recursive(list_get(&self->dense, i.as.integer));
    }
}

void table_set(Table * self, Symbol key, Obj value) {
    const Obj i = list_get(&self->sparse, key.lookup_index);
    if(i.tag == TAG_NIL) {
        /*append new*/
        
        /*get dense index*/
        const int j = self->dense.len;
        list_push(&self->dense, obj_dup_recursive(value));
        list_push(&self->dense_to_sparse_mapping, make_integer(key.lookup_index));
        
        /* set sparse index */
        list_set(&self->sparse, key.lookup_index, make_integer(j));
    } else {
        /*replace existing*/
        assert(i.tag == TAG_INTEGER && "invalid type"); 
        assert(i.as.integer >= 0);
        assert(i.as.integer < self->dense.len);
        obj_free_recursive(&self->dense.items[i.as.integer]);
        self->dense.items[i.as.integer] = obj_dup_recursive(value);
    }
}




/* Parser */
/*
#define max_token_len 1024
char * get_token(FILE * fp) {
    static char buf[max_token_len];
    long i = 0;
    if(feof(fp)) {
        return NULL;
    } else {
        char ch = fgetc(fp);
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
*/

int is_number(const char * str) {
    int i = 0;
    for(i = 0; str[i] != 0; ++i) {
        if(!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

#if 0
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

Obj parse_expr(FILE * fp) {
    char * tok = get_token(fp);
    if(streql(tok, "\"")){
        return parse_string(fp);
    } else if(is_number(tok)) {
        return make_integer(atoi(tok));
    } else if(streql(tok, "'")) {
        Obj quoted = make_list(2);
        list_push(&quoted, make_symbol("quote"));
        list_push(&quoted, parse_expr(fp));
        return quoted;
    }
}

/*expects that the opening bracket has been parsed*/
Obj parse_list(FILE * fp) {
    Obj root = make_list(0);
    /*char * tok = get_token(fp);*/
    while(tok != NULL) {
        list_push(&root, parse_expr(fp));
        /*tok = get_token(fp);*/
    }
}

Obj read(FILE * fp) {
    Obj root = make_list(8);
    char * tok = get_token(fp);

    while(tok != NULL) {
        
    }

    return root;
}

#endif
int main() {
    printf("size: %zu\n", sizeof(Obj));
    return 0;
}
