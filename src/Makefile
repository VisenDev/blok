ERRORS=3
MAX_ERRORS_GCC= "--max-errors=${ERRORS}"
MAX_ERRORS_CLANG= "-ferror-limit=${ERRORS}"
SRC= c23.c 3rdparty/rpmalloc/*c

.PHONY run:

run: c23
	./c23

c23: ${SRC} Makefile
	clang ${SRC} -g -O2 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c23 ${MAX_ERRORS_CLANG} -o c23

#a.out: lisp.c
#clang lisp.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG}
