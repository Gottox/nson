/*
 * BSD 2-Clause License
 * 
 * Copyright (c) 2018, Enno Boland
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "test.h"

#include "../src/nson.h"
#include <errno.h>

static void
no_such_file() {
	int rv;
	struct Nson config;
	rv = nson_load_ini(&config, "./no-such-file");
	assert(rv < 0);
	assert(errno == ENOENT);
	nson_clean(&config);
}

static void
syntax_error() {
	int rv;
	struct Nson config;
	rv = nson_parse_ini(&config, strdup("value_missing\n"));
	assert(rv < 0);
	nson_clean(&config);
}

static void
three_elements() {
	int rv;
	struct Nson config;
	rv = nson_parse_ini(&config, strdup(
			"key1 value1\n"
			"key2 value2\n"
			"key2 value3\n"
			));
	assert(rv >= 0);

	assert(nson_length(&config) == 3);

	assert(strcmp("key1", nson_get_key(&config, 0)) == 0);
	struct Nson *e1 = nson_get(&config, 0);
	assert(strcmp("value1", nson_str(e1)) == 0);

	assert(strcmp("key2", nson_get_key(&config, 1)) == 0);
	struct Nson *e2 = nson_get(&config, 1);
	assert(strcmp("value2", nson_str(e2)) == 0);

	assert(strcmp("key2", nson_get_key(&config, 2)) == 0);
	struct Nson *e3 = nson_get(&config, 2);
	assert(strcmp("value3", nson_str(e3)) == 0);
	nson_clean(&config);
}

DEFINE
TEST(no_such_file);
TEST(syntax_error);
TEST(three_elements);
DEFINE_END
