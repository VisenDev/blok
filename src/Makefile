ERRORS=3
MAX_ERRORS_GCC= "--max-errors=${ERRORS}"
MAX_ERRORS_CLANG= "-ferror-limit=${ERRORS}"

c23: c23.c
	clang c23.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG} -o c23

a.out: lisp.c
	clang lisp.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG}
