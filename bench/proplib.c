/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"

#include <proplib.h>

// Make linter happy:
#ifndef BENCH_PLIST
#define BENCH_PLIST "/dev/null"
#endif

void proplib() {
	prop_dictionary_t dict = prop_dictionary_internalize_from_file(BENCH_PLIST);
	assert(dict != NULL);
	prop_object_release(dict);
}

DEFINE
TEST(proplib);
DEFINE_END
