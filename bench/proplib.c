/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"

#include <proplib.h>

void nson_plist() {
	prop_dictionary_t dict = prop_dictionary_internalize_from_file("./plist/pkgdb-0.38.plist");
	assert(dict != NULL);
	prop_object_release(dict);
}

DEFINE
TEST(nson_plist);
DEFINE_END
