/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"
#include "mmap.h"

#include "../src/nson.h"

#include <ucl.h>

void ucl() {
	void *doc = 0;
	size_t len, f_len = 0;
	struct ucl_parser *parser = NULL;

	assert(mmap_file("./json/pkgdb-0.38.json", &doc, &len, &f_len));
	parser = ucl_parser_new (0);
	ucl_parser_add_chunk (parser, doc, f_len);

	assert(ucl_parser_get_error (parser) == NULL);
}

DEFINE
TEST(ucl);
DEFINE_END
