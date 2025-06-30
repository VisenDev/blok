ERRORS=3
MAX_ERRORS_GCC= --max-errors=${ERRORS}
MAX_ERRORS_CLANG= -ferror-limit=${ERRORS}
SRC= main.c
STD= c99
DEBUG_FLAGS= -O0 -fsanitize=address,undefined -glldb
RELEASE_FLAGS= -O3 -DNDEBUG -ffast-math
WARN_FLAGS= -Wall -Wextra -Wpedantic -Werror


main: $(SRC) Makefile arena.c
	clang $(SRC) $(DEBUG_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main

release: $(SRC) Makefile arena.c
	clang $(SRC) $(RELEASE_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main_release

run: main
	./main

clean: 
	trash -f a.out main *.dSYM main_release

.PHONY run clean main:
#a.out: lisp.c
#clang lisp.c -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -std=c89 ${MAX_ERRORS_CLANG}
