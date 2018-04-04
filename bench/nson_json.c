/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"

void nson_json() {
	int rv;
	struct Nson nson;

	rv = nson_load_json(&nson, "./json/pkgdb-0.38.json");
	assert(rv >= 0);
	(void)rv;
}

DEFINE
TEST(nson_json);
DEFINE_END
