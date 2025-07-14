ERRORS=3
MAX_ERRORS_GCC= --max-errors=${ERRORS}
MAX_ERRORS_CLANG= -ferror-limit=${ERRORS}
SRC= *.c
STD= c99
DEBUG_FLAGS= -O0 -g -fsanitize=address,undefined
RELEASE_FLAGS= -O3 -DNDEBUG -ffast-math #-DBLOK_PROFILER_DISABLE
WARN_FLAGS= -Wall -Wextra -Wpedantic -Werror

main: $(SRC) Makefile 
	clang main.c $(DEBUG_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -c
	clang main.o $(DEBUG_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main

main_release: $(SRC) Makefile 
	clang main.c $(RELEASE_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main_release

run: main
	./main

run_release: main_release
	./main_release

clean:
	if [ -e main ]; then trash main; fi
	if [ -e main_release ]; then trash main_release; fi
	if [ -e *.dSYM ]; then trash *.dSYM; fi
	if [ -e a.out.c ]; then trash a.out.c; fi
	if [ -e *.o ]; then trash *.o; fi
	if [ -e *.i ]; then trash *.i; fi
	if [ -e *.s ]; then trash *.s; fi
	if [ -e profile.json ]; then trash profile.json; fi

.PHONY clean run run_release:
