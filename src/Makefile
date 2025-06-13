ERRORS=1000
MAX_ERRORS_GCC= "--max-errors=${ERRORS}"
MAX_ERRORS_CLANG= "-ferror-limit=${ERRORS}"
SRC= main.c
STD= c99


main: ${SRC} Makefile
	clang ${SRC} -g -O3 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=$(STD) ${MAX_ERRORS_CLANG} -o main

run: main
	./main

clean: 
	trash -f a.out main *.dSYM

.PHONY run clean main:
#a.out: lisp.c
#clang lisp.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG}
