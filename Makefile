#
# Makefile
# tox, 2018-04-02 16:39
#
VERSION = 0.1
CFLAGS = -Wall -Werror -Wpedantic -g

HDR = src/nson.h
SRC = src/data.c src/json.c src/ini.c src/pool.c src/plist.c
OBJ = $(SRC:.c=.o)

TST = test/data.c test/json.c test/ini.c test/plist.c
TST_EXE = $(TST:.c=-test)

all: $(OBJ)

test/%-test: test/%.c $(OBJ)
	@echo CCTEST $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(OBJ)

src/%.o: src/%.c $(HDR)
	@echo CC $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

check: $(TST_EXE)
	@for i in $(TST_EXE); do ./$$i || break; done

doc: doxygen.conf $(TST) $(SRC) $(HDR) README.md
	@sed -i "/^PROJECT_NUMBER\s/ s/=.*/= $(VERSION)/" $<
	@doxygen $<

clean:
	@rm -rf doc
	@rm -f $(TST_EXE) $(OBJ)

.PHONY: check all clean


# vim:ft=make
#
