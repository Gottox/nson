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

// Make linter happy:
#ifndef BENCH_JSON
#define BENCH_JSON "/dev/null"
#endif

#include <ucl.h>

void
ucl() {
	bool rv;
	void *doc = 0;
	const char *err;
	size_t len, f_len = 0;
	struct ucl_parser *parser = NULL;

	rv = mmap_file(BENCH_JSON, &doc, &len, &f_len);
	assert(rv);
	parser = ucl_parser_new(0);
	ucl_parser_add_chunk(parser, doc, f_len);

	err = ucl_parser_get_error(parser);
	assert(err == NULL);
	(void)rv;
	(void)err;
}

DEFINE
TEST(ucl);
DEFINE_END
