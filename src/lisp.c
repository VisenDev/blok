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

    /* flag types */
    BLOK_TAG_NIL,
    BLOK_TAG_TRUE,
    BLOK_TAG_FALSE,

    /* basic types */    
    BLOK_TAG_INTEGER,
    BLOK_TAG_FLOATING,
    BLOK_TAG_SYMBOL,
    BLOK_TAG_PRIMITIVE,
    BLOK_TAG_STRING,

    /* types stored in lists (dynamic allocation) */
    BLOK_TAG_LIST, /*basic list*/
    BLOK_TAG_PLIST, /*property (association) list*/
    BLOK_TAG_NAMESPACED_SYMBOL, /*list containing only symbols*/
    BLOK_TAG_TYPED_SYMBOL, /*property list containing type and name*/
    BLOK_TAG_ERROR, /*property list containing message and stack frames list */
    BLOK_TAG_STACK_FRAME /*property list containing line, file, and function */
} blok_Tag;

static int blok_tag_is_list(blok_Tag tag) {
    return tag == BLOK_TAG_LIST 
        || tag == BLOK_TAG_PLIST
        || tag == BLOK_TAG_NAMESPACED_SYMBOL
        || tag == BLOK_TAG_TYPED_SYMBOL
        || tag == BLOK_TAG_ERROR
        || tag == BLOK_TAG_STACK_FRAME;
}

typedef struct {
    int lookup_index;
} blok_Symbol;


struct blok_Obj;

typedef struct {
    int len;
    int cap;
    struct blok_Obj * items;
} blok_List;

typedef struct {
    int len;
    int cap;
    char * items;
} blok_String;

typedef struct blok_Obj (*blok_Primitive)
    (struct blok_Obj * env, int argc, struct blok_Obj * argv[]);

typedef struct blok_Obj {
    blok_Tag tag;
    union {
        int integer;
        float floating;
        blok_Symbol symbol;
        blok_Primitive primitive;
        blok_String string;
        blok_List list;
    } as;
} blok_Obj; 

/* Symbol handling */
#define max_symbols 4096
#define max_symbol_len 128
static char symbols[max_symbols][max_symbol_len];
static int symbols_len;


/* ==== PRIMITIVE CONSTRUCTORS ==== */
#define BLOK_DEFINE_CONSTRUCTOR(fn_name, enum_tag)			  \
blok_Obj fn_name(blok_Obj * env, int argc, blok_Obj ** argv) {		  \
    blok_Obj result = {0};					  \
    (void) env, (void)argc, (void)argv;			  \
    result.tag = enum_tag;					  \
    return result;						  \
}

BLOK_DEFINE_CONSTRUCTOR(blok_make_nil, BLOK_TAG_NIL)
BLOK_DEFINE_CONSTRUCTOR(blok_make_true, BLOK_TAG_TRUE)
BLOK_DEFINE_CONSTRUCTOR(blok_make_false, BLOK_TAG_FALSE)

BLOK_DEFINE_CONSTRUCTOR(blok_make_integer, BLOK_TAG_INTEGER)
BLOK_DEFINE_CONSTRUCTOR(blok_make_floating, BLOK_TAG_FLOATING)
BLOK_DEFINE_CONSTRUCTOR(blok_make_symbol, BLOK_TAG_SYMBOL)
BLOK_DEFINE_CONSTRUCTOR(blok_make_primitive, BLOK_TAG_PRIMITIVE)
BLOK_DEFINE_CONSTRUCTOR(blok_make_string, BLOK_TAG_STRING)
BLOK_DEFINE_CONSTRUCTOR(blok_make_error, BLOK_TAG_ERROR)

BLOK_DEFINE_CONSTRUCTOR(blok_make_list, BLOK_TAG_LIST)
BLOK_DEFINE_CONSTRUCTOR(blok_make_plist, BLOK_TAG_PLIST)
BLOK_DEFINE_CONSTRUCTOR(blok_make_namespaced_symbol, BLOK_TAG_NAMESPACED_SYMBOL)
BLOK_DEFINE_CONSTRUCTOR(blok_make_typed_symbol, BLOK_TAG_TYPED_SYMBOL)
     
#undef BLOK_DEFINE_CONSTRUCTOR


blok_Obj blok_make_symbol_internal(const char * name) {
    blok_Obj result = blok_make_symbol(NULL, 0, NULL); 
    int i = 0;
    for(i = 0; i < symbols_len; ++i) {
        if(strncmp(name, symbols[i], max_symbol_len) == 0) {
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




/* ==== OBJECT AND MEMORY MANAGEMENT FUNCTIONS ==== */
blok_Obj blok_obj_free(blok_Obj * env, int argc, blok_Obj * argv[]) {
    int i = 0;
    (void)env;
    assert(argc == 1);

    {
        blok_Obj object = *argv[0];
        if(blok_tag_is_list(object.tag)) {
            for(i = 0; i < object.as.list.len; ++i) {
                blok_Obj * params[1] = {0};
                params[0] = &object.as.list.items[0];
                blok_obj_free(NULL, 1, params);
            }
            free(object.as.list.items);
        } else if (object.tag == BLOK_TAG_STRING) {
            free(object.as.string.items);
        }
        argv[0]->tag = BLOK_TAG_NIL;
        return blok_make_nil(NULL, 0, NULL);
    }
}


/*prototype for duplication*/
blok_Obj blok_obj_dup(blok_Obj * env, int argc, blok_Obj * argv);


/* Utility functions */
void blok_list_ensure_capacity(blok_Obj * env, int argc, blok_Obj * argv) {
    (void) env;
    assert(argc == 2);
    assert(blok_tag_is_list(argv[0].tag));
    assert(argv[1].tag == BLOK_TAG_INTEGER);
    {
        blok_List * self = &argv[0].as.list;
        int required_capacity = argv[1].as.integer;

        if(self->cap == 0) {
            self->cap = required_capacity;
            self->len= 0;
            self->items = malloc(sizeof(blok_Obj) * self->cap);
        } else if(self->cap < required_capacity) {
            self->cap = required_capacity;
            self->items = realloc(self->items, self->cap * sizeof(blok_Obj));
        }
    }
}

/* item is copied not referenced */
void blok_list_push(blok_Obj * env, int argc, blok_Obj * argv) {
    (void) env;
    assert(argc == 2);
    assert(blok_tag_is_list(argv[0].tag));
    {
        blok_List * self = &argv[0].as.list;
        blok_Obj item = argv[1];
        if(self->len >= self->cap) {

            /* TODO: fix this so that it isn't just the local copy of the list 
             * that gets modified
             */
            blok_Obj params[2];
            params[0] = argv[0];
            params[1] = blok_make_integer(NULL, 0, NULL);
            params[1].as.integer = self->cap * 2 + 1;
            blok_list_ensure_capacity(NULL, 2, params);
        }
        self->items[self->len] = blok_obj_dup(NULL, 1, &item);
        ++self->len;
    }
}

/* function does not free the popped item */
blok_Obj blok_list_pop (blok_Obj * env, int argc, blok_Obj * argv) {
    (void) env;
    assert(argc == 1);
    assert(blok_tag_is_list(argv[0].tag));
    {
        blok_List * self = &argv[0].as.list;
        blok_Obj result = blok_make_integer(NULL, 0, NULL);
        --self->len;
        result = self->items[self->len];
        return result;
    }
}

/* function duplicates the returned item */
blok_Obj blok_list_get (blok_Obj * env, int argc, blok_Obj * argv) {
    (void) env;
    assert(argc == 2);
    assert(blok_tag_is_list(argv[0].tag));
    assert(argv[1].tag == BLOK_TAG_INTEGER);
    {
        blok_List self = argv[0].as.list;
        int i = argv[1].as.integer;
        if(i <= 0 || i > self.len) {
            return blok_make_nil(NULL, 0, NULL);
        } else {
            return blok_obj_dup(NULL, 1, &self.items[i]);
        }
    }
}

void list_set(blok_Obj * env, int argc, blok_Obj * argv) {
    (void) env;
    assert(argc == 3);
    assert(blok_tag_is_list(argv[0].tag));
    assert(argv[1].tag == BLOK_TAG_INTEGER);
    {
        blok_List * self = &argv[0].as.list;
        int i = argv[1].as.integer;
        blok_Obj item = argv[2];

        if(i >= self->len) {
            for(j = 0; j < i + 1; ++j) {
                blok_list_push(self, make_nil());
            }
        }
        obj_free_recursive(&self->items[i]);
        self->items[i] = obj_dup_recursive(value);
    }
}

/*Duplication functions */
blok_Obj list_duplicate(const List * self) {
    int i = 0;
    blok_Obj result = make_list(self->len);
    for(i = 0; i < self->len; ++i) {
	list_push(result.as.list, obj_dup_recursive(self->items[i]));
    }
    return result;
}


/* ==== BLOK ERROR FUNCTIONS ==== */
blok_Obj blok_make_error_internal(const char * message) {
    blok_Obj err = blok_make_error(NULL, 0, NULL);
    err.as.error.stack_trace = blok_make_list(NULL, 0, NULL);
    err.as.error.message = strndup(message, 256);
    return err;
}

blok_Obj blok_error_append_stack_frame(
        blok_Error * err,
        int line,
        const char * file,
        const char * function,
        ) {

}

/* Duplicate an object */
blok_Obj obj_dup_recursive(const Obj self) {
    switch(self.tag) {
    case BLOK_TAG_STRING:
	return make_string(self.as.string->items);
    case BLOK_TAG_NIL:
	return make_nil();
    case BLOK_TAG_INTEGER:
	return make_integer(self.as.integer);
    case BLOK_TAG_LIST: 
	return list_duplicate(self.as.list);
    case BLOK_TAG_NAMESPACED_SYMBOL:
    {
	Obj result = list_duplicate(self.as.list);
	result.tag = BLOK_TAG_NAMESPACED_SYMBOL;
	return result;
    }
    case BLOK_TAG_TYPED_SYMBOL:
    {
	Obj result = list_duplicate(self.as.list);
	result.tag = BLOK_TAG_TYPED_SYMBOL;
	return result;
    }
    case BLOK_TAG_PRIMITIVE:
	return make_primitive(self.as.primitive);
    case BLOK_TAG_SYMBOL:
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

blok_Obj parse_integer(FILE * fp) {
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
  blok_Obj parse_symbol(FILE * fp) {
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

blok_Obj parse_expr(FILE * fp);

blok_Obj parse_any_symbol(FILE * fp) {
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
	result.tag = BLOK_TAG_TYPED_SYMBOL;
	list_push(result.as.list, make_symbol(buf));
	fgetc(fp);
	list_push(result.as.list, parse_expr(fp));
	skip_whitespace(fp);
	return result;
        
	/*namespaced symbol*/
    } else if(peek(fp) == '.' ) {
	Obj result = make_list(2);
	result.tag = BLOK_TAG_NAMESPACED_SYMBOL;
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

blok_Obj parse_string(FILE * fp) {
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

blok_Obj parse_list(FILE * fp);

blok_Obj parse_expr(FILE * fp) {
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

blok_Obj parse_list(FILE * fp) {
    blok_Obj result = make_list(0);
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

blok_Obj read(FILE * fp) {
    blok_Obj root = make_list(8);

    while(!feof(fp)) {
	list_push(root.as.list, parse_expr(fp));
    }

    return root;
}

void write(FILE * fp, Obj val) {
    int i = 0;
    switch(val.tag) {
    case BLOK_TAG_NIL:
	fprintf(fp, "nil");
	break;
    case BLOK_TAG_INTEGER:
	fprintf(fp, "%d", val.as.integer);
	break;
    case BLOK_TAG_PRIMITIVE:
	fprintf(fp, "<Primitive %p>", val.as.primitive);
	break;
    case BLOK_TAG_STRING:
	fprintf(fp, "\"%s\"", val.as.string->items);
	break;
    case BLOK_TAG_SYMBOL:
	fprintf(fp, "%s", symbols[val.as.symbol.lookup_index]);
	break;
    case BLOK_TAG_LIST:
	fprintf(fp, "[");
	for(i = 0; i < val.as.list->len; ++i) {
	    if(i != 0) fprintf(fp, " ");
	    write(fp, val.as.list->items[i]);
	}
	fprintf(fp, "]");
	break;
    case BLOK_TAG_TYPED_SYMBOL:
	assert(val.as.list->len == 2);
	write(fp, val.as.list->items[0]);
	fprintf(fp, ":");
	write(fp, val.as.list->items[1]);
	break;
    case BLOK_TAG_NAMESPACED_SYMBOL:
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
	case BLOK_TAG_INTEGER:
	    return lhs.as.integer == rhs.as.integer;
	case BLOK_TAG_NIL:
	    return 1;
	case BLOK_TAG_SYMBOL:
	    return lhs.as.symbol.lookup_index == rhs.as.symbol.lookup_index;
	case BLOK_TAG_PRIMITIVE:
	    return lhs.as.primitive == rhs.as.primitive;
	case BLOK_TAG_STRING:
	    return strcmp(lhs.as.string->items, rhs.as.string->items) == 0;
	case BLOK_TAG_NAMESPACED_SYMBOL:
	case BLOK_TAG_TYPED_SYMBOL:
	case BLOK_TAG_LIST:
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
    blok_Obj * start = plist->items;
    blok_Obj * end = plist->items + plist->len;
    for(;start != end; ++start) {
	if(start->tag == BLOK_TAG_LIST && start->as.list->len >= 2) {
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
    blok_Obj * found = plist_get(plist, key);
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
blok_Obj * env_get(List* env, Obj sym) {
    if(sym.tag == BLOK_TAG_SYMBOL) {
	Obj * start = env->items;
	Obj * end = env->items + env->len;
	for(;start < end; ++start) {
	    assert(start->tag == BLOK_TAG_LIST);
	    {
		Obj * found = plist_get(start->as.list, sym);
		if(found != NULL) {
		    return found;
		}
	    }
	}
    } else if(sym.tag == BLOK_TAG_NAMESPACED_SYMBOL) {
	Obj first = sym.as.list->items[0];
	if(first.tag != BLOK_TAG_SYMBOL) {
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


blok_Obj eval_expr(List * env, Obj expr) {
    switch(expr.tag) {
    case BLOK_TAG_NIL: return make_nil();
    case BLOK_TAG_INTEGER: return make_integer(expr.as.integer);
    case BLOK_TAG_PRIMITIVE: fatal_error("TODO: figure out primitives");
    case BLOK_TAG_STRING: return make_string(expr.as.string->items);
    case BLOK_TAG_SYMBOL: return env_lookup(env, expr.as.symbol);
    case BLOK_TAG_LIST:  break;
    }


    return expr;
}


void compile_sexpr(Obj src) {
    if(src.tag != BLOK_TAG_LIST) {
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
	if(elems[0].tag != BLOK_TAG_SYMBOL) {
	    write(stderr, src);
	    fatal_error("Expected sexpr to start with a symbol");
	}

        
    }
    
}

void compile_toplevel(Obj src) {
    assert(src.tag == BLOK_TAG_LIST);
}


int main(void) {
    printf("sizeof Obj: %zu\n\n", sizeof(Obj));
    FILE * fp = fopen("test.blok", "r");
    blok_Obj val = read(fp);
    fclose(fp);
    write(stdout, val);
    return 0;
}
