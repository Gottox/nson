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
#include "common.h"

#include "../src/nson.h"
#include <errno.h>

static void
parse_true() {
	int rv;
	Nson nson = { 0 };

	rv = NSON(&nson, true);
	assert(rv >= 0);
	assert(nson_int(&nson) != 0);

	nson_clean(&nson);
	(void)rv;
}

static void
parse_double() {
	int rv;
	Nson nson = { 0 };

	rv = NSON(&nson, 5.2);
	assert(rv >= 0);
	assert(nson_int(&nson) == 5);
	assert(nson_real(&nson) == 5.2);

	nson_clean(&nson);
	(void)rv;
}

static void
parse_number() {
	int rv;
	Nson nson = { 0 };

	rv = NSON(&nson, 5);
	assert(rv >= 0);
	assert(nson_int(&nson) == 5);

	nson_clean(&nson);
	(void)rv;
}

static void
parse_empty_string() {
	int rv;
	Nson nson = { 0 };

	rv = NSON(&nson, "");
	assert(rv >= 0);

	nson_clean(&nson);
	(void)rv;
}

static void
parse_string_escape_newline() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"a\\nb\""));
	assert(rv >= 0);
	assert(strcmp("a\nb", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_newline2() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("[\"a\\nb\",\"c\nd\"]"));
	assert(rv >= 0);
	assert(strcmp("a\nb", nson_str(nson_arr_get(&nson, 0))) == 0);
	assert(strcmp("c\nd", nson_str(nson_arr_get(&nson, 1))) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
object_with_one_element() {
	int rv;
	Nson nson;

	rv = NSON(&nson, {"a": "b"});
	assert(rv >= 0);

	assert(nson_obj_size(&nson) == 1);

	assert(strcmp(nson_obj_get_key(&nson, 0), "a") == 0);
	assert(strcmp(nson_str(nson_obj_get(&nson, "a")), "b") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
object_with_multiple_elements() {
	int rv;
	Nson nson;

	rv = NSON(&nson, {"a": 1, "b": 2});
	assert(rv >= 0);

	assert(nson_obj_size(&nson) == 2);

	Nson *e1 = nson_obj_get(&nson, "a");
	assert(nson_int(e1) == 1);
	Nson *e2 = nson_obj_get(&nson, "b");
	assert(nson_int(e2) == 2);

	nson_clean(&nson);
	(void)rv;
	(void)e1;
	(void)e2;
}

static void
access_str_as_arr() {
	int rv;
	Nson nson;

	rv = NSON(&nson, {"a": ""});
	assert(rv >= 0);

	assert(nson_obj_size(&nson) == 1);

	Nson *e1 = nson_obj_get(&nson, "a");
	ASSERT_ABRT(nson_arr_get(e1, 0));
	nson_clean(&nson);

	(void)rv;
}

static void
leading_whitespace() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("  {}"));
	assert(rv >= 0);

	assert(nson_obj_size(&nson) == 0);

	assert(nson_type(&nson) == NSON_OBJ);
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_array() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("["));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_string() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"aaa"));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
unclosed_array_with_one_element() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("[ 1 , "));
	assert(rv < 0);
	nson_clean(&nson);

	(void)rv;
}

static void
huge_file() {
	int rv;
	Nson nson;
	rv = nson_load_json(&nson, "./json/huge_file.json");
	assert(rv < 0);
	assert(errno == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
page_sized() {
	int rv;
	Nson nson;
	rv = nson_load_json(&nson, "./json/page_sized.json");
	assert(rv < 0);
	assert(errno == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
utf8_0080() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"\\u0024\""));

	assert(rv >= 0);
	printf("%s\n", nson_str(&nson));
	assert(strcmp(nson_str(&nson), "$") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
utf8_substr_0080() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"Hello \\u0024 World\""));

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "Hello $ World") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
utf8_substr_0080_2() {
	int rv;
	Nson nson;
	rv = nson_parse_json(&nson, NSON_P("\"Hello \\u002400\""));

	assert(rv >= 0);
	printf("%s\n", nson_str(&nson));
	assert(strcmp(nson_str(&nson), "Hello $00") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
utf8_incorrect_08() {
	int rv;
	Nson nson;
	rv = nson_init_str(&nson, "Hello \\u08");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "Hello \\u08") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
utf8_0800() {
	int rv;
	Nson nson;
	rv = nson_init_str(&nson, "\u00A2");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "¢") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
utf8_FFFF() {
	int rv;
	Nson nson;
	rv = nson_init_str(&nson, "\u20AC");

	assert(rv >= 0);
	assert(strcmp(nson_str(&nson), "€") == 0);

	nson_clean(&nson);
	(void)rv;
}

void stringify_utf8() {
	int rv;
	char *str;
	size_t size;
	Nson nson;
	rv = NSON(&nson, "€");

	assert(rv >= 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	assert(strcmp(str, "\"€\"") == 0);

	free(str);
	nson_clean(&nson);
	(void)rv;
}

void stringify_nullbyte() {
	int rv;
	char *str;
	size_t size;
	Nson nson;
	rv = nson_init_data(&nson, "a\0b", 3, NSON_STR);
	assert(rv >= 0);

	assert(strcmp(nson_str(&nson), "a") == 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	assert(strcmp(str, "\"a\\u0000b\"") == 0);

	free(str);
	nson_clean(&nson);
	(void)rv;
}

static void
stringify_empty_array() {
	int rv;
	Nson nson;
	char *str;
	size_t size;

	rv = NSON(&nson, []);

	assert(rv >= 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	assert(strcmp("[]", str) == 0);

	nson_clean(&nson);
	free(str);
	(void)rv;
}

static void
stringify_empty_object() {
	int rv;
	Nson nson;
	char *str;
	size_t size;

	rv = NSON(&nson, {});

	assert(rv >= 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	assert(strcmp("{}", str) == 0);

	free(str);
	nson_clean(&nson);
	(void)rv;
}

static void
stringify_object() {
	int rv;
	Nson nson;
	char *str;
	size_t size;

	rv = NSON(&nson, { "a": 1 });

	assert(rv >= 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	puts(str);
	assert(strcmp("{\"a\":1}", str) == 0);

	nson_clean(&nson);
	free(str);
	(void)rv;
}

void stringify_data() {
	int rv;
	Nson nson;
	char *str;
	size_t size;

	rv = nson_init_data(&nson, "Hello World", 11, NSON_BLOB);

	assert(rv >= 0);
	rv = nson_json_serialize(&str, &size, &nson, 0);
	assert(rv >= 0);
	puts(str);
	assert(strcmp("\"SGVsbG8gV29ybGQ=\"", str) == 0);

	free(str);
	nson_clean(&nson);
	(void)rv;
}

static void
fuzz_parse_crash() {
	const char input[1] = ",";
	Nson nson;
	nson_parse_json(&nson, input, sizeof(input));
	nson_clean(&nson);
}

static void
fuzz_parse_leak() {
	const char input[1] = " ";
	Nson nson;
	nson_parse_json(&nson, input, sizeof(input));
	nson_clean(&nson);
}

INPUT_CHECK(
		fuzz_parse_leak2, json,
		"{ [[[}}}}"
		)

INPUT_CHECK(
		fuzz_parse_crash_string, json,
		"\"\\\" "
		)

INPUT_CHECK(
		fuzz_parse_crash2, json,
		"\"0L~\\\\\\\"\\"
		)

INPUT_CHECK(
		fuzz_parse_deadlock, json,
		"{{99999999999999999999999999T]]]"
		)


DEFINE
TEST(parse_true);
TEST(parse_double);
TEST(parse_number);
TEST(parse_empty_string);
TEST(parse_string_escape_newline);
TEST(parse_string_escape_newline2);
TEST(object_with_one_element);
TEST(object_with_multiple_elements);
TEST(access_str_as_arr);
TEST(leading_whitespace);
TEST(unclosed_array);
TEST(unclosed_array_with_one_element);
TEST(unclosed_string);
TEST_OFF(page_sized);
TEST_OFF(huge_file);
TEST(utf8_incorrect_08);
TEST(utf8_0080);
TEST(utf8_substr_0080);
TEST(utf8_substr_0080_2);
TEST(utf8_0800);
TEST(utf8_FFFF);
TEST(stringify_utf8);
TEST(stringify_nullbyte);
TEST(stringify_empty_array);
TEST(stringify_empty_object);
TEST(stringify_object);
TEST(stringify_data);
TEST(fuzz_parse_crash);
TEST(fuzz_parse_leak);
TEST(fuzz_parse_leak2);
TEST(fuzz_parse_crash_string);
TEST(fuzz_parse_crash2);
TEST(fuzz_parse_deadlock);
DEFINE_END
