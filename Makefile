#
# Makefile
# tox, 2018-04-02 16:39
#
include config.mk

HDR = \
	src/nson.h \
	src/util.h \

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
	test/map.c \

TST_EXE = $(TST:.c=-test)
TST_CFLAGS =

BCH = \
	bench/naiv.c \
	bench/json-c.c \
	bench/ucl.c \
	bench/proplib.c \
	bench/jansson.c \
	bench/nson_json.c \
	bench/nson_plist.c \

BCH_EXE = $(BCH:.c=-bench)
BCH_CFLAGS = \
	$(shell pkg-config --cflags --libs proplib) \
	$(shell pkg-config --cflags --libs libucl) \
	$(shell pkg-config --cflags --libs json-c) \
	$(shell pkg-config --cflags --libs jansson) \
	'-DBENCH_JSON="$(BENCH_JSON)"' \
	'-DBENCH_PLIST="$(BENCH_PLIST)"' \

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
	@for i in $(TST_EXE); do ./$$i || exit 1; done

speed: $(BCH_EXE) $(BENCH_JSON) $(BENCH_PLIST)
	@for i in $(BCH_EXE); do ./$$i; done

doc: doxygen.conf $(TST) $(SRC) $(HDR) README.md
	@sed -i "/^PROJECT_NUMBER\s/ s/=.*/= $(VERSION)/" $<
	@doxygen $<

bench/bench-file.json:
	wget http://eu.battle.net/auction-data/258993a3c6b974ef3e6f22ea6f822720/auctions.json -O $@

bench/bench-file.plist:
	wget https://repo.voidlinux.eu/current/x86_64-repodata -O - | zcat | tar xO index.plist > $@

coverage: check
	@printf "%s\n" $(CFLAGS) | grep -qx -- '-fprofile-arcs\|-ftest-coverage' || \
		( echo "You need to enable coverage settings in the Makefile"; exit 1 )
	@mkdir cov
	@gcovr -r . --html --html-details -o cov/index.html

clean:
	@rm -rf doc cov
	@rm -f *.gcnp *.gcda
	@rm -f $(TST_EXE) $(BCH_EXE) $(OBJ)
	#@rm -f bench/json/auctions.json bench/plist/index.plist

.PHONY: check all clean speed coverage


# vim:ft=make
#
