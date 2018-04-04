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
parse_from_memory() {
	int rv;
	struct Nson nson;

	rv = NSON(&nson, {"a": 5});
	assert(rv >= 0);
	nson_clean(&nson);

	(void)rv;
}

static void
object_with_multiple_elements() {
	int rv;
	struct Nson nson;

	rv = NSON(&nson, {"a": 1, "b": 2});
	assert(rv >= 0);

	assert(nson_len(&nson) == 2);

	struct Nson *e1 = nson_get(&nson, 0);
	assert(nson_int(e1) == 1);
	struct Nson *e2 = nson_get(&nson, 1);
	assert(nson_int(e2) == 2);
	nson_clean(&nson);

	(void)rv;
	(void)e1;
	(void)e2;
}

static void
access_str_as_arr() {
	int rv;
	struct Nson nson;

	rv = NSON(&nson, {"a": 1});
	assert(rv >= 0);

	assert(nson_len(&nson) == 1);

	struct Nson *e1 = nson_get(&nson, 0);
	ASSERT_ABRT(nson_get(e1, 0));
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_array() {
	int rv;
	struct Nson nson;
	rv = nson_parse_json(&nson, NSON_P("["));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_string() {
	int rv;
	struct Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"aaa"));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_array_with_one_element() {
	int rv;
	struct Nson nson;
	rv = nson_parse_json(&nson, NSON_P("[ 1 , "));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
huge_file() {
	int rv;
	struct Nson nson;
	rv = nson_load_json(&nson, "./json/huge_file.json");
	assert(rv < 0);
	assert(errno == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
page_sized() {
	int rv;
	struct Nson nson;
	rv = nson_load_json(&nson, "./json/page_sized.json");
	assert(rv < 0);
	assert(errno == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
utf8_0080() {
	int rv;
	struct Nson nson;
	rv = nson_init_str(&nson, "\u0024");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "$") == 0);

	(void)rv;
}

static void
utf8_0800() {
	int rv;
	struct Nson nson;
	rv = nson_init_str(&nson, "\u00A2");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "¢") == 0);

	(void)rv;
}

static void
utf8_FFFF() {
	int rv;
	struct Nson nson;
	rv = nson_init_str(&nson, "\u20AC");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "€") == 0);

	(void)rv;
}

void stringify_utf8() {
	int rv;
	char *str;
	struct Nson nson;
	rv = NSON(&nson, "€");

	assert(rv >= 0);
	rv = nson_to_json(&nson, &str);
	assert(rv >= 0);
	puts(str);
	assert(strcmp(str, "\"€\"") == 0);

	(void)rv;
}

void stringify_nullbyte() {
	int rv;
	char *str;
	struct Nson nson;
	nson_init_data(&nson, "a\0b", 3, NSON_UTF8);

	assert(rv >= 0);
	rv = nson_to_json(&nson, &str);
	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "a") == 0);
	assert(strcmp(str, "\"a\\u0000b\"") == 0);

	(void)rv;
}

static void
stringify_empty_array() {
	int rv;
	struct Nson nson;
	char *result;

	rv = NSON(&nson, []);

	assert(rv >= 0);
	nson_to_json(&nson, &result);
	assert(strcmp("[]", result) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_empty_object() {
	int rv;
	struct Nson nson;
	char *result;

	rv = NSON(&nson, {});

	assert(rv >= 0);
	nson_to_json(&nson, &result);
	assert(strcmp("{}", result) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_object_with_hidden_item() {
	int rv;
	struct Nson nson;
	char *result;

	rv = NSON(&nson, { "\u001bhidden": 1 });

	assert(rv >= 0);
	nson_to_json(&nson, &result);
	assert(strcmp("{}", result) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_object() {
	int rv;
	struct Nson nson;
	char *result;

	rv = NSON(&nson, { "a": 1 });

	assert(rv >= 0);
	nson_to_json(&nson, &result);
	assert(strcmp("{\"a\":1}", result) == 0);
	nson_clean(&nson);

	(void)rv;
}

void stringify_data() {
	int rv;
	struct Nson nson;
	char *result;

	rv = nson_init_data(&nson, "Hello World", 11, NSON_PLAIN);

	assert(rv >= 0);
	nson_to_json(&nson, &result);
	assert(strcmp("\"SGVsbG8gV29ybGQ=\"", result) == 0);
	nson_clean(&nson);

	(void)rv;
}

DEFINE
TEST(parse_from_memory);
TEST(object_with_multiple_elements);
TEST(access_str_as_arr);
TEST(unclosed_array);
TEST(unclosed_array_with_one_element);
TEST(unclosed_string);
TEST(page_sized);
TEST(huge_file);
TEST(utf8_0080);
TEST(utf8_0800);
TEST(utf8_FFFF);
TEST(stringify_utf8);
TEST(stringify_nullbyte);
TEST(stringify_empty_array);
TEST(stringify_empty_object);
TEST(stringify_object);
TEST(stringify_object_with_hidden_item);
TEST(stringify_data);
DEFINE_END
