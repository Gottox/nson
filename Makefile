#
# Makefile
# tox, 2018-04-02 16:39
#
VERSION = 0.1
CFLAGS = -Wall -Werror -Wpedantic -g

SRC = src/data.c src/json.c src/ini.c
OBJ = $(SRC:.c=.o)

TST = test/data.c test/json.c test/ini.c
TST_EXE = $(TST:.c=-test)

all: $(OBJ)

test/%-test: test/%.c $(OBJ)
	@echo CCTEST $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(OBJ)

src/%.o: src/%.c
	@echo CC $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

check: $(TST_EXE)
	@for i in $(TST_EXE); do ./$$i || break; done

clean:
	rm -f $(TST_EXE) $(OBJ)

.PHONY: check all clean


# vim:ft=make
#
