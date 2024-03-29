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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "common.h"
#include "test.h"

#include "../src/nson.h"

#define PLIST(x) \
	NSON_P("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
		   "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" " \
		   "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
		   "<plist>\n" x "\n</plist>\n")

static void
parse_real() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<real>2.3</real>"));
	assert(rv >= 0);
	assert(nson_real(&nson) == 2.3);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_int() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<integer>42</integer>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 42);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_true() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<true/>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_false() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<false/>"));
	assert(rv >= 0);
	assert(nson_int(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>abc</string>"));
	assert(rv >= 0);
	assert(strcmp("abc", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_lt() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>&lt;</string>"));
	assert(rv >= 0);
	assert(strcmp("<", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_gt() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>&gt;</string>"));
	assert(rv >= 0);
	assert(strcmp(">", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_gt2() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(
			&nson,
			PLIST("<array><string>xcb-util-keysyms&gt;=0.3.9_1</"
				  "string><string>xcb-util-keysyms&gt;=0.3.9_1</string></"
				  "array>"));
	assert(rv >= 0);
	assert(nson_type(&nson) == NSON_ARR);
	assert(strcmp("xcb-util-keysyms>=0.3.9_1",
				  nson_str(nson_arr_get(&nson, 0))) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_amp() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>&amp;</string>"));
	assert(rv >= 0);
	assert(strcmp("&", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_dec() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>&#126;</string>"));
	assert(rv >= 0);
	assert(strcmp("~", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_string_escape_substr_dec() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<string>Hello &#126; World</string>"));
	assert(rv >= 0);
	assert(strcmp("Hello ~ World", nson_str(&nson)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_empty() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<array></array>"));
	assert(rv >= 0);
	assert(nson_arr_len(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_1() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<array><true/></array>"));
	assert(rv >= 0);
	assert(nson_arr_len(&nson) == 1);
	assert(nson_int(nson_arr_get(&nson, 0)) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_2() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(
			&nson,
			PLIST("<array>"
				  "<true/>"
				  "<false/>"
				  "</array>"));
	assert(rv >= 0);
	assert(nson_arr_len(&nson) == 2);
	assert(nson_bool(nson_arr_get(&nson, 0)) == 1);
	assert(nson_bool(nson_arr_get(&nson, 1)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_array_spaces() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST(" <array> <true/> </array>"));
	assert(rv >= 0);
	assert(nson_arr_len(&nson) == 1);
	assert(nson_int(nson_arr_get(&nson, 0)) == 1);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_empty() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<dict></dict>"));
	assert(rv >= 0);
	assert(nson_obj_size(&nson) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_1() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST("<dict><key>k</key><true/></dict>"));
	assert(rv >= 0);
	assert(nson_obj_size(&nson) == 1);
	assert(strcmp("k", nson_obj_get_key(&nson, 0)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_2() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(
			&nson,
			PLIST("<dict>"
				  "<key>k</key><true/>"
				  "<key>i</key><true/>"
				  "</dict>"));
	assert(rv >= 0);
	assert(nson_obj_size(&nson) == 2);
	assert(strcmp("k", nson_obj_get_key(&nson, 0)) == 0);
	assert(strcmp("i", nson_obj_get_key(&nson, 1)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_object_spaces() {
	int rv;
	Nson nson = {0};
	rv = nson_parse_plist(&nson, PLIST(" <dict> <key>k</key> <true/> </dict>"));
	assert(rv >= 0);
	assert(nson_obj_size(&nson) == 1);
	assert(strcmp("k", nson_obj_get_key(&nson, 0)) == 0);
	nson_clean(&nson);

	(void)rv;
}

static void
parse_data() {
	int rv;
	Nson nson = {0};
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
stringify_object_with_2_members() {
	int rv;
	Nson nson = {0};
	char *str;
	size_t size;

	rv = NSON(&nson, {"a" : 5, "b" : 5});
	assert(rv >= 0);
	nson_plist_serialize(&str, &size, &nson, NSON_SKIP_HEADER);
	puts(str);
	assert(strcmp(str,
				  "<dict>"
				  "<key>a</key><integer>5</integer>"
				  "<key>b</key><integer>5</integer>"
				  "</dict>") == 0);

	nson_clean(&nson);
	free(str);
	(void)rv;
}

static void
stringify_data() {
	int rv;
	Nson nson = {0};
	char *str;
	size_t size;

	rv = nson_init_data(&nson, "Hello World", 11, NSON_BLOB);
	assert(rv >= 0);
	nson_plist_serialize(&str, &size, &nson, NSON_SKIP_HEADER);
	assert(strstr(str, "<data>SGVsbG8gV29ybGQ=</data>"));

	nson_clean(&nson);
	free(str);
	(void)rv;
}

static void
stringify_escape() {
	int rv;
	Nson nson = {0};
	char *str;
	size_t size;

	rv = nson_init_str(&nson, " < > & ");
	assert(rv >= 0);
	nson_plist_serialize(&str, &size, &nson, NSON_SKIP_HEADER);
	puts(str);
	assert(strstr(str, "<string> &lt; &gt; &amp; </string>"));

	nson_clean(&nson);
	free(str);
	(void)rv;
}

static void
stringify_true() {
	int rv;
	Nson nson = {0};
	char *str;
	size_t size;

	rv = NSON(&nson, true);
	assert(rv >= 0);
	nson_plist_serialize(&str, &size, &nson, NSON_SKIP_HEADER);
	assert(strstr(str, "<true/>"));

	nson_clean(&nson);
	free(str);
	(void)rv;
}

static void
fuzz_parse_memleak() {
	const char *input =
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\x0a<!DOCTYPE plist "
			"PUBLIC \"-//A    ist-0.0.dtd\">\x0a<plist "
			"version=\"1.0\">\x0a<te/ur>\x0a</plist>\x0a";
	Nson nson = {0};
	nson_parse_plist(&nson, input, strlen(input));
	nson_clean(&nson);
}

INPUT_CHECK(fuzz_parse_assert, plist,
			"<?xml i->\x0a<!DOCTYPE pl is->\x0a<plist e> \x0a</                "
			"                                                                  "
			"                </plist                             ist>\x0a";)

INPUT_CHECK(fuzz_parse_crash, plist,
			"<?xml eUarray>\x0a<!DOCTYPE>\x0a<plist v\x0b=\"en0r.0\">\x0a<data "
			"datTYPE>\x0a<plist v\x0b=\"en0r.0\">\x0a";)

INPUT_CHECK(fuzz_parse_crash2, plist,
			"<?xml eUarray>\x0a<!DOCTYPE>\x0a<plist v\x0b=\"en1r.0\">\x0a<data "
			">z\x0a";)

INPUT_CHECK(
		fuzz_parse_assert2, plist,
		"<?xml lC  ist v1.<Uarray>\x0a<!DOCTYPE>\x0a<plist >\x0a<data "
		">gue/>\x0a</plist9a >\x0aue/>\x0a</plit veray>\x0a")

INPUT_CHECK(fuzz_parse_crash3, plist, "<?xml><!DOCTYPE><plist><string>\x0a")

INPUT_CHECK(
		fuzz_parse_crash4, plist,
		"<?xml l    y>\n"
		"<!DOCTYPE>\n"
		"<plist >\n"
		"<data >\n"
		"ue/><\n"
		"<?x>\x0bm<")

INPUT_CHECK(fuzz_parse_crash5, plist, "<?xml>\x0a<!DOCTYPE><plist></array>")

INPUT_CHECK(
		fuzz_parse_crash6, plist,
		"<?xml>\n"
		"<!DOCTYPE>\n"
		"<plist>\n"
		"<real>-")

INPUT_CHECK(
		fuzz_parse_crash7, plist,
		"<?xml version=;;;<p    F-8\"?>\n"
		"<!DOCTYPE plist PUBLIC \"-//pple.com")

INPUT_CHECK(
		fuzz_parse_crash8, plist,
		"<</dictxml version=\"1.0\" enc0//EN\"                                 "
		"                                                           "
		"\"http://www.apple.com/DDPsr/Toersion=\"1.0\">\n"
		"<true/>\n"
		"</plist>\n")

INPUT_CHECK(fuzz_parse_crash9, plist, "")

DEFINE
TEST(parse_real);
TEST(parse_int);
TEST(parse_true);
TEST(parse_false);
TEST(parse_string);
TEST(parse_string_escape_lt);
TEST(parse_string_escape_gt);
TEST(parse_string_escape_gt2);
TEST(parse_string_escape_amp);
TEST(parse_string_escape_dec);
TEST(parse_string_escape_substr_dec);
TEST(parse_array_empty);
TEST(parse_array_1);
TEST(parse_array_2);
TEST(parse_array_spaces);
TEST(parse_object_empty);
TEST(parse_object_1);
TEST(parse_object_2);
TEST(parse_object_spaces);
TEST(parse_data);
TEST(stringify_object_with_2_members);
TEST(stringify_data);
TEST(stringify_escape);
TEST(stringify_true);
TEST(fuzz_parse_memleak);
TEST(fuzz_parse_assert);
TEST(fuzz_parse_crash);
TEST(fuzz_parse_crash2);
TEST(fuzz_parse_assert2);
TEST(fuzz_parse_crash3);
TEST(fuzz_parse_crash4);
TEST(fuzz_parse_crash5);
TEST(fuzz_parse_crash6);
TEST(fuzz_parse_crash7);
TEST(fuzz_parse_crash8);
TEST(fuzz_parse_crash9);
DEFINE_END
