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
} Tag;


struct Obj;
typedef struct Obj Obj;

/* typedef struct { */
/*     void * items; */
/*     int len; */
/*     int cap; */
/* } List; */

typedef struct {
    int lookup_index;
} Symbol;

typedef Obj (*Primitive) (Obj * env, int argc, Obj * argv);

typedef struct Obj {
    Tag tag;
    union {
	int integer;
	float floating;
	Symbol symbol;
	Primitive primitive;

	void * list;
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

/* ==== PRIMITIVE CONSTRUCTORS ==== */

#define DEFINE_CONSTRUCTOR(fn_name, enum_tag)			  \
    Obj fn_name(Obj * env, int argc, Obj * argv) {		  \
	Obj result = {0};					  \
	(void) env, (void)argc, (void)argv;			  \
	result.tag = enum_tag;					  \
	return result;						  \
    }

DEFINE_CONSTRUCTOR(make_nil, TAG_NIL);
DEFINE_CONSTRUCTOR(make_true, TAG_TRUE);
DEFINE_CONSTRUCTOR(make_false, TAG_FALSE);
DEFINE_CONSTRUCTOR(make_integer, TAG_INTEGER);
DEFINE_CONSTRUCTOR(make_floating, TAG_FLOATING);
DEFINE_CONSTRUCTOR(make_symbol, TAG_SYMBOL);
DEFINE_CONSTRUCTOR(make_primitive, TAG_PRIMITIVE);
DEFINE_CONSTRUCTOR(make_list, TAG_LIST);
DEFINE_CONSTRUCTOR(make_string, TAG_STRING);
DEFINE_CONSTRUCTOR(make_plist, TAG_PLIST);
DEFINE_CONSTRUCTOR(make_error, TAG_ERROR);
DEFINE_CONSTRUCTOR(make_namespaced_symbol, TAG_NAMESPACED_SYMBOL);
DEFINE_CONSTRUCTOR(make_typed_symbol, TAG_TYPED_SYMBOL);
     
#undef DEFINE_CONSTRUCTOR



Obj make_symbol_internal(const char * name) {
    Obj result = make_symbol(); 
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

Obj make_primitive_internal(Primitive fn) {
    Obj result = make_primitive; 
    result.as.primitive = fn;
    return result;
}


Obj make_list_internal() {
    Obj result = make_list();
    assert(initial_capacity >= 0);
    result.as.list = malloc(sizeof(List));
    result.as.list->items = malloc(sizeof(Obj) * initial_capacity);
    result.as.list->len = 0;
    result.as.list->cap = initial_capacity;
    return result;
}


Obj make_integer_internal(int value) {
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

Obj make_error(int line, const char * file, const char * message)


/* ==== IMPLEMENTATION FUNCTIONS ==== */
Obj obj_free_recursive(Obj env, int argc, Obj * argv) {
    int i = 0;
    (void)env;
    return 
        
    switch(argv[0].tag) {
    case TAG_LIST:
	for(int i = 0; i < argv[0].as.list->len; ++i) {
	    obj_free_recursive(&argv[0].as.list->items[i]);
	}
	free(argv[0].as.list->items);
	free(argv[0].as.list);
	break;
    default:
	break;
    }
    argv[0].tag = TAG_NIL;

    return make_nil();
}


/*prototype for duplication*/
Obj obj_dup_recursive(Obj env, int argc, Obj * argv);


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
	case TAG_NAMESPACED_SYMBOL:
	case TAG_TYPED_SYMBOL:
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


struct Obj * plist_get(struct Obj * env, int argc, struct Obj * argv){
    Obj * start = plist->items;
    Obj * end = plist->items + plist->len;
    for(;start != end; ++start) {
	if(start->tag == TAG_LIST && start->as.list->len >= 2) {
	    Obj first = start->as.list->items[0];
	    if(eql(first, key)) {
		return &start->as.list->items[1];
	    }
	} else {
	    write(stderr, *start);
	    fatal_error("Object is not an element of a plist");
	}
    }
    return NULL;
}

void plist_set(List * plist, Obj key, Obj value) {
    Obj * found = plist_get(plist, key);
    if(found == NULL) {
	Obj new_list = make_list(2);
	list_push(new_list.as.list, key);
	list_push(new_list.as.list, value);
	list_push(plist, new_list);
    } else {
	obj_free_recursive(found);
	*found = obj_dup_recursive(value);
    }
}

/* INTERPRETER AND COMPILER */

/* A scope consists of a property list mapping symbols to values */
/* A scope may contain nested scopes, whose values can be accessed used namespaced symbols */
/* Env is a stack of property lists representing the currently available lexical scope*/
Obj * env_get(List* env, Obj sym) {
    if(sym.tag == TAG_SYMBOL) {
	Obj * start = env->items;
	Obj * end = env->items + env->len;
	for(;start < end; ++start) {
	    assert(start->tag == TAG_LIST);
	    {
		Obj * found = plist_get(start->as.list, sym);
		if(found != NULL) {
		    return found;
		}
	    }
	}
    } else if(sym.tag == TAG_NAMESPACED_SYMBOL) {
	Obj first = sym.as.list->items[0];
	if(first.tag != TAG_SYMBOL) {
	    write(stderr, sym);
	    fatal_error("Invalid namespaced symbol");
	}
	/*TODO finish this function, lookup the namespace pointed to by first, then lookup second in that namespace*/
    } else {
	write(stderr, sym);
	fatal_error("Invalid symbol type");
    }

    fatal_error("todo: figure out how to store environment frames");
    return make_nil();
}

void env_set(List * env, Symbol sym, Obj value);

void env_enter_scope(List * env);
void env_exit_scope(List * env);


Obj eval_expr(List * env, Obj expr) {
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
