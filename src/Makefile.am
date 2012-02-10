bin_PROGRAMS = ladybug

BUILT_SOURCES = bison.tab.h
AM_YFLAGS = --defines=bison.tab.h -v
# AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

ladybug_SOURCES = FuncTools.c \
	symbol_table.c \
	mem.c \
	ir.c \
	err_buff.c \
	expressions.c \
	expr_toolbox.c \
	ir_toolbox.c \
	main_app.c \
	subprograms.c \
	scope.c \
	bitmap.c \
	ir_parser.c \
	reg.c \
	statements.c \
	generator.c \
	final_code.c \
	instruction_set.c \
	bison.y \
	flex.l

bison.tab.c bison.tab.h: bison.y
	$(YACC) $(AM_YFLAGS) bison.y

lex.yy.c: flex.l bison.tab.h
	$(LEX) flex.l

lex.yy.o: lex.yy.c bison.tab.h
	$(CC) -c $(CFLAGS) lex.yy.c  -o $@
