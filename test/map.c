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

static void
check_decode_base64() {
	Nson *nson;

	nson = nson_init_str("SGVsbG8gV29ybGQ=");
	nson_mapper_b64_dec(0, nson, NULL);
	assert(strcmp("Hello World", nson_str(nson)) == 0);
	assert(nson_data_len(nson) == 11);
	nson_clean(nson);

	nson = nson_init_str("SGVsbG8gV29ybA==");
	nson_mapper_b64_dec(0, nson, NULL);
	assert(strcmp("Hello Worl", nson_str(nson)) == 0);
	assert(nson_data_len(nson) == 10);
	nson_clean(nson);

	nson = nson_init_str("SGVsbG8gV29y");
	nson_mapper_b64_dec(0, nson, NULL);
	assert(strcmp("Hello Wor", nson_str(nson)) == 0);
	assert(nson_data_len(nson) == 9);

	nson_clean(nson);
}

static void
check_encode_base64() {
	Nson *nson;

	nson = nson_init_str("Hello World");
	nson_mapper_b64_enc(0, nson, NULL);
	assert(strcmp("SGVsbG8gV29ybGQ=", nson_str(nson)) == 0);
	nson_clean(nson);

	nson = nson_init_str("Hello Worl");
	nson_mapper_b64_enc(0, nson, NULL);
	assert(strcmp("SGVsbG8gV29ybA==", nson_str(nson)) == 0);
	nson_clean(nson);

	nson = nson_init_str("Hello Wor");
	nson_mapper_b64_enc(0, nson, NULL);
	assert(strcmp("SGVsbG8gV29y", nson_str(nson)) == 0);

	nson_clean(nson);
}

int
mult_mapper(off_t index, Nson *nson, void *user_data) {
	int64_t val = nson_int(nson);
	nson_clean(nson);
	nson = nson_init_int(val * 2);

	return 0;
}

static void
check_map_big() {
	int i;
	Nson *nson;
	nson = nson_init(NSON_ARR);
	for(i = 0; i < 10240; i++) {
		nson_push_int(nson, i);
	}

	nson_map(nson, mult_mapper, NULL);

	for(i = 0; i < 10240; i++) {
		assert(i*2 == nson_int(nson_get(nson, i)));
	}

	nson_clean(nson);
}

static void
check_map_thread_big() {
	int i;
	Nson *nson = nson_init(NSON_ARR);
	for(i = 0; i < 10240; i++) {
		nson_push_int(nson, i);
	}

	nson_map_thread(nson, mult_mapper, NULL);

	for(i = 0; i < 10240; i++) {
		assert(i*2 == nson_int(nson_get(nson, i)));
	}

	nson_clean(nson);
}

static void
check_map_thread() {
	Nson *nson;
	NSON(nson, [
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10
	]);

	nson_map_thread(nson, mult_mapper, NULL);

	assert(2 == nson_int(nson_get(nson, 0)));
	assert(4 == nson_int(nson_get(nson, 1)));
	assert(6 == nson_int(nson_get(nson, 2)));
	assert(8 == nson_int(nson_get(nson, 3)));
	assert(10 == nson_int(nson_get(nson, 4)));
	assert(12 == nson_int(nson_get(nson, 5)));
	assert(14 == nson_int(nson_get(nson, 6)));
	assert(16 == nson_int(nson_get(nson, 7)));
	assert(18 == nson_int(nson_get(nson, 8)));
	assert(20 == nson_int(nson_get(nson, 9)));

	nson_clean(nson);
}

static void
check_map_thread_two() {
	Nson *nson;

	NSON(nson, [
		23, 42
	]);

	nson_map_thread(nson, mult_mapper, NULL);

	assert(2 == nson_len(nson));
	assert(46 == nson_int(nson_get(nson, 0)));
	assert(84 == nson_int(nson_get(nson, 1)));
	nson_clean(nson);
}

DEFINE
TEST(check_decode_base64);
TEST(check_encode_base64);
TEST(check_map_big);
TEST(check_map_thread);
TEST(check_map_thread_two);
TEST(check_map_thread_big);
DEFINE_END
