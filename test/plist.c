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

#define PLIST(x) NSON_P("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
	"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
	"<plist>\n" x "\n</plist>\n")

static void
parse_real() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<real>2.3</real>"));
	assert(rv >= 0);
	assert(nson_real(&nson) == 2.3);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_int() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<integer>42</integer>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 42);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_true() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<true/>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_false() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<false/>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<string>abc</string>"));
	assert(rv >= 0);
	assert(strcmp("abc", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_lt() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<string>&lt;</string>"));
	assert(rv >= 0);
	assert(strcmp("<", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_gt() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<string>&gt;</string>"));
	assert(rv >= 0);
	assert(strcmp(">", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_amp() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<string>&amp;</string>"));
	assert(rv >= 0);
	assert(strcmp("&", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_dec() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<string>&#126;</string>"));
	assert(rv >= 0);
	assert(strcmp("~", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_empty() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<array></array>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_1() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<array><true/></array>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 1);
	assert(nson_int(nson_get(&nson, 0)) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_2() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST(
				"<array>"
				"<true/>"
				"<false/>"
				"</array>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 2);
	assert(nson_int(nson_get(&nson, 0)) == 1);
	assert(nson_int(nson_get(&nson, 1)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_spaces() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST(" <array> <true/> </array>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 1);
	assert(nson_int(nson_get(&nson, 0)) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_empty() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<dict></dict>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_1() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<dict><key>k</key><true/></dict>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 1);
	assert(strcmp("k", nson_get_key(&nson, 0)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_2() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST(
				"<dict>"
				"<key>k</key><true/>"
				"<key>i</key><true/>"
				"</dict>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 2);
	assert(strcmp("k", nson_get_key(&nson, 0)) == 0);
	assert(strcmp("i", nson_get_key(&nson, 1)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_spaces() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST(" <dict> <key>k</key> <true/> </dict>"));
	assert(rv >= 0);
	assert(nson_len(&nson) == 1);
	assert(strcmp("k", nson_get_key(&nson, 0)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_data() {
	int rv;
	struct Nson nson;
	rv = nson_parse_plist(&nson, PLIST("<data>SGVsbG8gV29ybGQ=</data>"));
	assert(rv >= 0);
	const char *data = nson_str(&nson);
	int len = nson_data_len(&nson);
	assert(len == 11);
	assert(memcmp("Hello World", data, 11) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_data() {
	int rv;
	struct Nson nson;
	char *str;

	rv = nson_init_data(&nson, "Hello World", 11, NSON_PLAIN);
	assert(rv >= 0);
	nson_to_plist(&nson, &str);
	assert(strstr(str, "<data>SGVsbG8gV29ybGQ=</data>"));
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_escape() {
	int rv;
	struct Nson nson;
	char *str;

	rv = nson_init_str(&nson, " < > & ");
	assert(rv >= 0);
	nson_to_plist(&nson, &str);
	assert(strstr(str, "<string> &lt; &gt; &amp; </string>"));
	nson_clean(&nson);

	(void)rv;
}

static void
stringify_true() {
	int rv;
	struct Nson nson;
	char *str;

	rv = NSON(&nson, true);
	assert(rv >= 0);
	nson_to_plist(&nson, &str);
	puts(str);
	assert(strstr(str, "<true/>"));
	nson_clean(&nson);

	(void)rv;
}

DEFINE
TEST(parse_real);
TEST(parse_int);
TEST(parse_true);
TEST(parse_false);
TEST(parse_string);
TEST(parse_string_escape_lt);
TEST(parse_string_escape_gt);
TEST(parse_string_escape_amp);
TEST(parse_string_escape_dec);
TEST(parse_array_empty);
TEST(parse_array_1);
TEST(parse_array_2);
TEST(parse_array_spaces);
TEST(parse_object_empty);
TEST(parse_object_1);
TEST(parse_object_2);
TEST(parse_object_spaces);
TEST(parse_data);
TEST(stringify_data);
TEST(stringify_escape);
TEST_OFF(stringify_true);
DEFINE_END
