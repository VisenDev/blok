struct Backend;

typedef void (*FunctionFn)
    (struct Backend self,
     char * type,
     char * name,
     char ** param_names,
     char ** param_types
    );

typedef void (*LetFn) (struct Backend self, char * type, char * name, char * value);
typedef void (*SetFn) (struct Backend self, char * name, char * value);
typedef void (*AddFn) (struct Backend self, char * lhs, char * rhs);
typedef void (*SubFn) (struct Backend self, char * lhs, char * rhs);
typedef void (*MulFn) (struct Backend self, char * lhs, char * rhs);
typedef void (*DivFn) (struct Backend self, char * lhs, char * rhs);

typedef struct Backend {
    

} Backend;
