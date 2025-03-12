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
    while(isspace(peek(fp))) {
        fgetc(fp);
    }
}

Obj parse_symbol(FILE * fp) {
    char buf [1024] = {0};
    int i = 0;
    skip_whitespace(fp);
    while(!is_special_character(peek(fp)) && !isspace(peek(fp)) && !feof(fp)) {
        buf[i] = fgetc(fp);
        ++i;
    }
    return make_symbol(buf);
}

Obj parse_string(FILE * fp) {
    char buf[1024] = {0};
    int i = 0;
    /*printf("parsing string");*/
    assert(fgetc(fp) == '"');
    while(peek(fp) != '"') {
        if(feof(fp)) {
            fatal_error("unexpected end of input when parsing string");
        }
        buf[i] = fgetc(fp);
        ++i;
    }
    assert(fgetc(fp) == '"');
    return make_string(buf);
}

Obj parse_list(FILE * fp);

Obj parse_expr(FILE * fp) {
    if(peek(fp) == '"') {
        return parse_string(fp);
    } else if(peek(fp) == '\'') {
        /* TODO quote reader macro*/
        fatal_error("quote not implemented yet");
    } else if(isdigit(peek(fp))) {
        return parse_integer(fp); 
    } else if (peek(fp) == '[') {
        return parse_list(fp);
    } else /*if (!is_special_character(peek(fp))) {*/ {
        return parse_symbol(fp);
    }
#if 0
    else {
        /*fatal_error("parsing failed on char %c", fgetc(fp));*/
        printf("unparsed ch: %c", fgetc(fp));
    }
#endif
    return make_nil();
}

Obj parse_list(FILE * fp) {
    Obj result = make_list(8);
    /*printf("parsing list");*/
    assert(fgetc(fp) == '[');
    while(peek(fp) != ']') {
        while(isspace(peek(fp))) {
            fgetc(fp);
        }
        list_push(&result.as.list, parse_expr(fp));
    }
    assert(fgetc(fp) == ']');
    return result;
}

Obj read(FILE * fp) {
    Obj root = make_list(8);

    while(!feof(fp)) {
        list_push(&root.as.list, parse_expr(fp));
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
    case TAG_LIST:
        fprintf(fp, "[");
        for(i = 0; i < val.as.list.len; ++i) {
            if(i != 0) fprintf(fp, " ");
            write(fp, val.as.list.items[i]);
        }
        fprintf(fp, "]");
        break;
    case TAG_PRIMITIVE:
        fprintf(fp, "<Primitive %p>", val.as.primitive);
        break;
    case TAG_STRING:
        fprintf(fp, "\"%s\"", val.as.string.items);
        break;
    case TAG_SYMBOL:
        fprintf(fp, "%s", symbols[val.as.symbol.lookup_index]);
        break;
    case TAG_TABLE:
        fatal_error("writing tables not implemented yet");
        break;
    }
}

int main() {
    FILE * fp = fopen("test.blok", "r");
    Obj val = read(fp);
    fclose(fp);
    write(stdout, val);
    return 0;
}
