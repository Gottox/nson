/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"
Nson nson = { 0 };

// Make linter happy:
#ifndef BENCH_JSON
#define BENCH_JSON "/dev/null"
#endif

void bench_nson_json() {
	int rv;

	rv = nson_load_json(&nson, BENCH_JSON);

	assert(rv >= 0);
	(void)rv;
}

void bench_nson_to_json() {
	int rv;
	char *str;
	size_t size;

	rv = nson_json_serialize(&str, &size, &nson, 0);

	assert(rv >= 0);
	(void)rv;
}

DEFINE
TEST(bench_nson_json);
TEST(bench_nson_to_json);
DEFINE_END
