/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

// Make linter happy:
#ifndef BENCH_PLIST
#define BENCH_PLIST "/dev/null"
#endif
#ifndef BENCH_JSON
#define BENCH_JSON "/dev/null"
#endif

#include "../src/nson.h"
Nson nson;

static void
nson_plist() {
	int rv;

	rv = nson_load_plist(&nson, BENCH_PLIST);
	assert(rv >= 0);
	(void)rv;
}

static void
bench_nson_to_plist() {
	int rv;
	char *str;
	size_t size;

	rv = nson_plist_serialize(&str, &size, &nson, 0);

	assert(rv >= 0);
	(void)rv;
}

DEFINE
TEST(nson_plist);
TEST(bench_nson_to_plist);
DEFINE_END
