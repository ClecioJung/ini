# ----------------------------------------
# Compiler and linker options
# ----------------------------------------

# Name of the executables to be generated
EXEC      := examples/ini_file_read \
             examples/ini_file_search \
             examples/ini_file_create

# Library files
LIB_FILES := ini_file.c ini_file.h

# Flags for compiler
CFLAGS    := -W -Wall -Wextra -pedantic -Wconversion \
             -Werror -flto -std=c89 -O2

# ----------------------------------------
# Compilation and linking rules
# ----------------------------------------

all: $(EXEC)

%: %.c $(LIB_FILES) Makefile
	$(CC) $(filter %.c,$^) -o $@ $(CFLAGS)

# ----------------------------------------
# Script rules
# ----------------------------------------

clean:
	$(RM) $(EXEC)

remade: clean all

.PHONY: all clean remade

# ----------------------------------------
