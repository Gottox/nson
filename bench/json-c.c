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
	void *doc = 0;
	size_t len, f_len = 0;

	assert(mmap_file("./json/pkgdb-0.38.json", &doc, &len, &f_len));
	assert(json_tokener_parse(doc));
}

DEFINE
TEST(ucl);
DEFINE_END
