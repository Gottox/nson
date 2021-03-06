#
# Makefile
# tox, 2018-04-02 16:39
#
include config.mk

HDR = \
	src/nson.h \
	src/internal.h \

SRC = \
	src/nson.c \
	src/buf.c \
	src/scalar.c \
	src/data.c \
	src/object.c \
	src/pointer.c \
	src/array.c \
	src/util.c \
	src/map_reduce.c \
	src/json.c \
	src/ini.c \
	src/stack.c \
	src/plist.c \

OBJ = $(SRC:.c=.o)

BIN = bin/nson

TST = \
	test/pointer.c \
	test/data.c \
	test/json.c \
	test/ini.c \
	test/plist.c \
	test/map_reduce.c \

TST_BIN = $(TST:.c=-test)

FZZ = \
	fuzzer/json.c \
	fuzzer/plist.c \

FZZ_BIN = $(FZZ:.c=-fuzz)

BCH = \
	bench/naiv.c \
	bench/json-c.c \
	bench/ucl.c \
	bench/proplib.c \
	bench/jansson.c \
	bench/nson_json.c \
	bench/nson_plist.c \

BCH_BIN = $(BCH:.c=-bench)
BCH_CFLAGS = \
	$(shell pkg-config --cflags --libs proplib) \
	$(shell pkg-config --cflags --libs libucl) \
	$(shell pkg-config --cflags --libs json-c) \
	$(shell pkg-config --cflags --libs jansson) \
	'-DBENCH_JSON="$(BENCH_JSON)"' \
	'-DBENCH_PLIST="$(BENCH_PLIST)"' \

MAJOR=$(shell echo $(VERSION) | cut -d . -f 1)

all: $(BIN) libnson.a libnson.so

libnson.a: $(OBJ)
	@echo AR $@
	@ar rc $@ $(OBJ)
	@ranlib $@

libnson.so: $(OBJ)
	@echo SH $@
	@$(CC) -shared $(LDFLAGS) $(OBJ) -o $@.$(VERSION) -Wl,-soname=$@.$(MAJOR)
	@ln -sf $@.$(VERSION) $@.$(MAJOR)
	@ln -sf $@.$(VERSION) $@

bin/%: bin/%.o $(OBJ)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJ) $< -o $@

bench/%-bench: bench/%.c test/test.h $(OBJ)
	@echo CCBENCH $@
	@$(CC) $(CFLAGS) $(LDFLAGS) $(BCH_CFLAGS) $(OBJ) $< -o $@

test/%-test: test/%.c test/test.h $(SRC)
	@echo CCTEST $@
	@$(CC) $(CFLAGS) $(LDFLAGS) $(TST_CFLAGS) $(SRC) $< -o $@

fuzzer/%-fuzz: fuzzer/%.c $(OBJ)
	@echo CCFUZZ $@
	@$(CC) $(CFLAGS) $(LDFLAGS) $(FZZ_CFLAGS) $(SRC) $< -o $@

%.o: %.c $(HDR)
	@echo CC $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

check: $(TST_BIN)
	@for i in $(TST_BIN); do ./$$i || exit 1; done

speed: $(BCH_BIN) $(BENCH_JSON) $(BENCH_PLIST)
	@for i in $(BCH_BIN); do ./$$i; done

fuzz: $(FZZ_BIN)
	@for i in $(FZZ_BIN); do ./$$i -only_ascii=1 $${i%-fuzz}-corpus -max_total_time=120 -dict=$${i%-fuzz}-dict.txt || exit 1; done

doc: doxygen.conf $(TST) $(SRC) $(HDR) README.md
	@sed -i "/^PROJECT_NUMBER\s/ s/=.*/= $(VERSION)/" $<
	@doxygen $<

bench/bench-file.json:
	@wget http://eu.battle.net/auction-data/258993a3c6b974ef3e6f22ea6f822720/auctions.json -O $@

bench/bench-file.plist: bench/bench-file.json
	@npm install plist-cli
	@node_modules/.bin/plist-cli < $< > $@
	@rm -rf package-lock.json node_modules

coverage: check
	@mkdir cov
	@gcovr -r . --html --html-details -o cov/index.html

clean:
	@echo cleaning...
	@rm -rf doc cov
	@rm -f *.gcnp *.gcda
	@rm -f $(TST_BIN) $(BCH_BIN) $(FZZ_BIN) $(OBJ) $(BIN) libnson.so* libnson.a
	#@rm -f $(BENCH_JSON) $(BENCH_PLIST)

.PHONY: check all clean speed coverage fuzz


# vim:ft=make
#
