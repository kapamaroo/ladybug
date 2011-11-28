TARGET = ladybug
OBJ1 = FuncTools.o
OBJ2 = symbol_table.o semantic_routines.o mem_reg.o ir.o err_buff.o final_code.o expressions.o expr_toolbox.o ir_toolbox.o main_app.o
OBJ = $(OBJ1) $(OBJ2)
OBJ_LEX = lex.yy.o
OBJ_YACC = bison.tab.o
OBJ_ALL = $(OBJ_YACC) $(OBJ_LEX) $(OBJ)
YACC_FILES = bison.tab.h bison.tab.c
LEX_FILES = lex.yy.c

LEX = flex
YACC = bison -d -v

CC = gcc
CFLAGS = -g -Wall
CLEX_FLAGS = -lfl

all: $(OBJ_ALL)
	$(CC) $(CFLAGS) $(OBJ_ALL) -o $(TARGET) $(CLEX_FLAGS)

debug:
	$(MAKE) all

release:
	$(MAKE) CFLAGS=" -Wall" all

lex: $(OBJ_LEX) $(OBJ1)
	$(CC) $(CFLAGS) $(OBJ_LEX) $(OBJ1) -o $(TARGET) $(CLEX_FLAGS)

clean:
	-rm -f $(OBJ_ALL) $(TARGET)

clean-devs:
	-rm -f $(OBJ_ALL) $(TARGET) $(YACC_FILES) $(LEX_FILES)

$(OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

lex.yy.o: lex.yy.c
	$(CC) -c $(CFLAGS) lex.yy.c $(CLEX_FLAGS) -o $@

lex.yy.c: flex.y
	$(LEX) flex.y

bison.tab.o: bison.tab.c
	$(CC) -c $(CFLAGS) bison.tab.c -o $@

bison.tab.h bison.tab.c: bison.y
	$(YACC) bison.y
