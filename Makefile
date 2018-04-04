#
# Makefile
# tox, 2018-04-02 16:39
#
VERSION = 0.1
CFLAGS = -Wall -Wpedantic -O0 -g
#CFLAGS += -DNDEBUG
## Comment this in for profiling
#CFLAGS += -pg -no-pie
## Comment this in for coverage reports
#CFLAGS += -fprofile-arcs -ftest-coverage


HDR = src/nson.h

SRC = \
	src/data.c \
	src/map.c \
	src/json.c \
	src/ini.c \
	src/pool.c \
	src/plist.c \


OBJ = $(SRC:.c=.o)

TST = \
	test/data.c \
	test/json.c \
	test/ini.c \
	test/plist.c \

TST_EXE = $(TST:.c=-test)

BCH = \
	bench/json-c.c \
	bench/ucl.c \
	bench/proplib.c \
	bench/nson_json.c \
	bench/nson_plist.c \

BCH_EXE = $(BCH:.c=-bench)
BCH_CFLAGS = \
			$(shell pkg-config --cflags --libs proplib libucl json-c) \

all: $(OBJ)

bench/%-bench: bench/%.c test/test.h $(OBJ)
	@echo CCBENCH $@
	$(CC) $(CFLAGS) $(LDFLAGS) $(BCH_CFLAGS) $(OBJ) $< -o $@

test/%-test: test/%.c test/test.h $(OBJ)
	@echo CCTEST $@
	$(CC) $(CFLAGS) $(LDFLAGS) $(TST_CFLAGS) $(OBJ) $< -o $@

src/%.o: src/%.c $(HDR)
	@echo CC $@
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

check: $(TST_EXE)
	@for i in $(TST_EXE); do ./$$i || break; done

speed: $(BCH_EXE)
	@for i in $(BCH_EXE); do ./$$i; done

doc: doxygen.conf $(TST) $(SRC) $(HDR) README.md
	@sed -i "/^PROJECT_NUMBER\s/ s/=.*/= $(VERSION)/" $<
	@doxygen $<

clean:
	@rm -rf doc
	@rm -f $(TST_EXE) $(BCH_EXE) $(OBJ)

.PHONY: check all clean speed


# vim:ft=make
#
