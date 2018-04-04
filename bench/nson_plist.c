/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"

void nson_plist() {
	int rv;
	struct Nson nson;

	rv = nson_load_plist(&nson, "./plist/pkgdb-0.38.plist");
	assert(rv >= 0);
	nson_clean(&nson);
}

DEFINE
TEST(nson_plist);
DEFINE_END
