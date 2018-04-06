#
# Makefile
# tox, 2018-04-02 16:39
#
VERSION = 0.1
CFLAGS = -Wall -Werror -Wpedantic -O0 -g
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
TST_CFLAGS =

BCH = \
	bench/naiv.c \
	bench/json-c.c \
	bench/ucl.c \
	bench/proplib.c \
	bench/nson_json.c \
	bench/nson_plist.c \

BCH_EXE = $(BCH:.c=-bench)
BCH_CFLAGS = \
	$(shell pkg-config --cflags --libs proplib libucl json-c) \
	'-DBENCH_JSON="json/auctions.json"' \
	'-DBENCH_PLIST="plist/pkgdb-0.38.plist"' \

CATCHSEGV=\
	$(shell which catchsegv 2> /dev/null)

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
	@for i in $(TST_EXE); do $(CATCHSEGV) ./$$i || break; done

speed: $(BCH_EXE) bench/json/auctions.json
	@for i in $(BCH_EXE); do $(CATCHSEGV) ./$$i; done

doc: doxygen.conf $(TST) $(SRC) $(HDR) README.md
	@sed -i "/^PROJECT_NUMBER\s/ s/=.*/= $(VERSION)/" $<
	@doxygen $<

bench/json/auctions.json:
	wget http://eu.battle.net/auction-data/258993a3c6b974ef3e6f22ea6f822720/auctions.json -O $@

coverage: check
	@printf "%s\n" $(CFLAGS) | grep -qx -- '-fprofile-arcs\|-ftest-coverage' || \
		( echo "You need to enable coverage settings in the Makefile"; exit 1 )
	@mkdir cov
	@gcovr -r . --html --html-details -o cov/index.html

clean:
	@rm -rf doc cov
	@rm -f *.gcnp *.gcda
	@rm -f $(TST_EXE) $(BCH_EXE) $(OBJ)
	#@rm -f bench/json/auctions.json

.PHONY: check all clean speed coverage


# vim:ft=make
#
