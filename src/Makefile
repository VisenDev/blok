ERRORS=3
MAX_ERRORS_GCC= "--max-errors=${ERRORS}"
MAX_ERRORS_CLANG= "-ferror-limit=${ERRORS}"
SRC= c23.c
STD= c23


c23: ${SRC} Makefile
	clang ${SRC} -g -O3 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=$(STD) ${MAX_ERRORS_CLANG} -o c23

run: c23
	./c23

clean: 
	trash -f a.out c23 *.dSYM

.PHONY run clean c23:
#a.out: lisp.c
#clang lisp.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG}
