/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"
#include "mmap.h"

#include "../src/nson.h"

#include <json.h>

void json_c() {
	bool rv;
	struct json_object *json;
	void *doc = 0;
	size_t len, f_len = 0;

	rv = mmap_file(BENCH_JSON, &doc, &len, &f_len);
	assert(rv);
	json = json_tokener_parse(doc);
	assert(json);

	(void)json;
	(void)rv;
}

DEFINE
TEST(json_c);
DEFINE_END
