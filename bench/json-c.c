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

void ucl() {
	bool rv;
	struct json_object *json;
	void *doc = 0;
	size_t len, f_len = 0;

	rv = mmap_file("./json/pkgdb-0.38.json", &doc, &len, &f_len);
	assert(rv);
	json = json_tokener_parse(doc);
	assert(json);

	(void)json;
	(void)rv;
}

DEFINE
TEST(ucl);
DEFINE_END
