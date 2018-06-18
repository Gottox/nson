/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

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

	rv = nson_to_plist(&nson, &str);

	assert(rv >= 0);
	(void)rv;
}

DEFINE
TEST(nson_plist);
TEST(bench_nson_to_plist);
DEFINE_END
