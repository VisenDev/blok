CC=clang
MAX_ERRORS=3
GCC_FLAGS=--max-errors=${MAX_ERRORS}
CLANG_FLAGS=-ferror-limit=${MAX_ERRORS}
SRC=*.c
STD=c99
CFLAGS=-Wall -Wextra -Wpedantic -Werror -Wimplicit-fallthrough -Wshadow -std=$(STD)
DEBUG_FLAGS= -O0 -g -fsanitize=address,undefined
RELEASE_FLAGS= -O3 -DNDEBUG -ffast-math -DBLOK_PROFILER_DISABLE

ifeq ($(CC), gcc)
	CFLAGS+=$(GCC_FLAGS)
endif
ifeq ($(CC), clang)
	CFLAGS+=$(CLANG_FLAGS)
endif

main: main.o $(SRC) Makefile 
	$(CC) main.o $(DEBUG_FLAGS) $(CFLAGS) -o main

main.o: $(SRC) Makefile 
	$(CC) main.c $(DEBUG_FLAGS) $(CFLAGS) -c -o main.o

release: $(SRC) Makefile 
	$(CC) main.c $(RELEASE_FLAGS) $(CFLAGS) -std=$(STD) -o release

run: main
	./main

run_release: release
	./release

clean:
	if [ -e main ]; then trash main; fi
	if [ -e main_release ]; then trash main_release; fi
	if [ -e release ]; then trash release; fi
	if [ -e *.dSYM ]; then trash *.dSYM; fi
	if [ -e a.out.c ]; then trash a.out.c; fi
	if [ -e *.o ]; then trash *.o; fi
	if [ -e *.i ]; then trash *.i; fi
	if [ -e *.s ]; then trash *.s; fi
	if [ -e profile.json ]; then trash profile.json; fi
	if [ -e a.out ]; then trash a.out; fi

.PHONY clean run run_release:
