/*
 * plist.c
 * Copyright (C) 2019 Enno Boland <g@s01.de>
 *
 * Distributed under terms of the MIT license.
 */

#include "../src/nson.h"

int LLVMFuzzerTestOneInput(char *data, size_t size) {
	Nson nson;
	nson_parse_json(&nson, data, size);
	nson_clean(&nson);
	return 0;  // Non-zero return values are reserved for future use.
}
