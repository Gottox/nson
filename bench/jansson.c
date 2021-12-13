/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#define _GNU_SOURCE

#include "../test/test.h"
#include "mmap.h"

#include "../src/nson.h"

#include <jansson.h>

// Make linter happy:
#ifndef BENCH_JSON
#define BENCH_JSON "/dev/null"
#endif

void
jansson() {
	bool rv;
	struct json_t *json;
	json_error_t error;
	void *doc = 0;
	size_t len, f_len = 0;

	rv = mmap_file(BENCH_JSON, &doc, &len, &f_len);
	assert(rv);
	json = json_loadb(doc, f_len, 0, &error);
	assert(json);

	(void)json;
	(void)rv;
}

DEFINE
TEST(jansson);
DEFINE_END
