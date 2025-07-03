ERRORS=3
MAX_ERRORS_GCC= --max-errors=${ERRORS}
MAX_ERRORS_CLANG= -ferror-limit=${ERRORS}
SRC= *.c
STD= c99
DEBUG_FLAGS= -O0 -fsanitize=address,undefined -glldb
RELEASE_FLAGS= -O3 -DNDEBUG -ffast-math
WARN_FLAGS= -Wall -Wextra -Wpedantic -Werror


main: $(SRC) Makefile 
	clang main.c $(DEBUG_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main

main_release: $(SRC) Makefile 
	clang main.c $(RELEASE_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main_release

run: main
	./main

run_release: main_release
	./main_release

clean:
	trash 

.PHONY clean run run_release:
