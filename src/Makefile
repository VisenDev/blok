ERRORS=3
MAX_ERRORS_GCC= --max-errors=${ERRORS}
MAX_ERRORS_CLANG= -ferror-limit=${ERRORS}
SRC= *.c
STD= c99
DEBUG_FLAGS= -O0 -fsanitize=address,undefined -glldb
RELEASE_FLAGS= -O3 -DNDEBUG -ffast-math
WARN_FLAGS= -Wall -Wextra -Wpedantic -Werror
BUILDDIR= build

all: $(BUILDDIR)/main

release: $(BUILDDIR)/main_release

$(BUILDDIR)/main: $(SRC) Makefile $(BUILDDIR)
	cd $(BUILDDIR) && clang ../main.c $(DEBUG_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/main_release: $(SRC) Makefile $(BUILDDIR)
	cd $(BUILDDIR) && clang ../main.c $(RELEASE_FLAGS) $(WARN_FLAGS) -std=$(STD) $(MAX_ERRORS_CLANG) -o main_release

run: $(BUILDDIR)/main
	./$(BUILDDIR)/main

run_release: $(BUILDDIR)/main_release
	./$(BUILDDIR)/main_release

clean:
	trash $(BUILDDIR)

.PHONY clean run run_release:
