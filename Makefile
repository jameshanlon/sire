CC         := gcc
LD         := gcc
CCFLAGS    := -c -Wall -Wextra -std=c99 -pedantic -g -ggdb -O3 -fno-stack-protector
LDFLAGS    := -g -std=c99
LIBS       := -lm -ll
FLEX       := flex
BISON      := bison
BISONFLAGS := -d -v -t --report=all

CMP_SRCS := \
    compiler/util.c \
    compiler/list.c \
    compiler/table.c \
    compiler/error.c \
    compiler/main.c \
    compiler/set.c \
    compiler/symbol.c \
    compiler/temp.c \
    compiler/label.c \
    compiler/statistics.c \
    compiler/signature.c \
    compiler/ast.c \
    compiler/astprinter.c \
    compiler/irt.c \
    compiler/irtprinter.c \
    compiler/ir.c \
    compiler/frame.c \
    compiler/structures.c \
    compiler/sem.c \
    compiler/translate.c \
    compiler/block.c \
    compiler/liveness.c \
    compiler/linearscan.c \
    compiler/spill.c \
    compiler/regalloc.c \
    compiler/codegen.c \
    compiler/instructions.c 

CMP_HDRS  := $(subst compiler/main.h,, $(CMP_SRCS:.c=.h))
CMP_OBJS  := $(addprefix obj/, compiler/x.tab.o compiler/lex.yy.o $(CMP_SRCS:.c=.o))

ALL_SRCS  := $(CMP_SRCS)
ALL_HDRS  := $(CMP_HDRS)
ALL_OBJS  := $(CMP_OBJS)

CMP       := bin/sire

# Targets
.PHONY: all compiler dirs clean count
all:  dirs $(CMP)
compiler:  dirs $(CMP)

depend: depends.mk
Makefile: depends.mk
	touch $@

# If not 'make clean' then include dependency makefiles
ifneq ($(findstring clean,$(MAKECMDGOALS)), clean)
ifneq ($(findstring count,$(MAKECMDGOALS)), count)

# Include the dependencies file if it exists
#ifneq ($(strip $($(wildcard depends.mk))),)
include depends.mk
#endif

endif
endif

# Create dependencies
depends.mk: $(ALL_SRCS) $(ALL_HDRS)
	@echo Generating dependencies
	@makedepend -f- -Y -m -pobj/ $(ALL_SRCS) 2> \
        /dev/null | sed -e 's:obj/src/:obj/:' > $@

# Link the objects and create the executables
$(CMP): $(CMP_OBJS)
	@echo Linking objects to $@
	@$(LD) $(LDFLAGS) $(CMP_OBJS) -o $@ $(LIBS)

# Compile a .c file to a .o file
obj/%.o: %.c
	@echo Compiling $<
	@$(CC) -c $(CCFLAGS) $< -o $@

# Generate compiler parser with Bison
obj/compiler/x.tab.o: compiler/x.y
	echo Generating $@
	$(BISON) $(BISONFLAGS) compiler/x.y -o compiler/x.tab.c
	$(CC) -g -c compiler/x.tab.c -o x.tab.o
	mv x.tab.o obj/compiler/

# Generate compiler lexer with Flex
obj/compiler/lex.yy.o: compiler/x.lex compiler/x.tab.c compiler/x.tab.h
	@echo Generating $@
	@$(FLEX) -o compiler/lex.yy.c compiler/x.lex 
	@$(CC) -g -c compiler/lex.yy.c -o lex.yy.o
	@mv lex.yy.o obj/compiler/

# Create required build directories if they don't exist
dirs:
	@if !(test -d bin);          then mkdir bin;          fi
	@if !(test -d obj);          then mkdir obj;          fi
	@if !(test -d obj/compiler); then mkdir obj/compiler; fi

# Clean up
clean:
	@rm -f depends.mk
	@rm -rd bin/ obj/
	@rm -f compiler/lex.yy.c compiler/x.tab.c compiler/x.tab.h compiler/x.output

# Count the number of lines of code
count:
	@echo 'Compiler sources'
	@wc -l $(CMP_SRCS) $(CMP_HDRS) compiler/x.y compiler/x.lex
