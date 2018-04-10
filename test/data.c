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

#include "../src/nson.h"

#include "test.h"

static void
unflat_array_depth_2() {
	char *str;
	Nson container, item;
	nson_init(&container, NSON_ARR);

	nson_init(&item, NSON_ARR);
	nson_push(&container, &item);
	{
		nson_init_int(&item, 1);
		nson_push(&container, &item);

		nson_init(&item, NSON_ARR);
		nson_push(&container, &item);
		{
			nson_init_int(&item, 2);
			nson_push(&container, &item);
		}
		nson_init(&item, NSON_ARR | NSON_TERM);
		nson_push(&container, &item);

		nson_init_int(&item, 3);
		nson_push(&container, &item);
	}
	nson_init(&item, NSON_ARR | NSON_TERM);
	nson_push(&container, &item);

	nson_unflat(&container);

	nson_to_json(&container, &str);
	puts("[1,[2],3]");
	puts(str);
}

static void
unflat_array() {
	Nson container, item;
	nson_init(&container, NSON_ARR);

	nson_init(&item, NSON_ARR);
	nson_push(&container, &item);
	{
		nson_init_int(&item, 1);
		nson_push(&container, &item);
		nson_init_int(&item, 2);
		nson_push(&container, &item);
		nson_init_int(&item, 3);
		nson_push(&container, &item);
		nson_init_int(&item, 4);
		nson_push(&container, &item);
		nson_init_int(&item, 5);
		nson_push(&container, &item);
	}
	nson_init(&item, NSON_ARR | NSON_TERM);
	nson_push(&container, &item);

	nson_unflat(&container);

	printf("%li\n", nson_len(&container));
	assert(nson_len(&container) == 5);
	assert(nson_int(nson_get(&container, 0)) == 1);
	assert(nson_int(nson_get(&container, 1)) == 2);
	assert(nson_int(nson_get(&container, 2)) == 3);
	assert(nson_int(nson_get(&container, 3)) == 4);
	assert(nson_int(nson_get(&container, 4)) == 5);
}

static void
create_array() {
	Nson nson;
	nson_init(&nson, NSON_ARR);
	assert(nson_type(&nson) == NSON_ARR);
	assert(nson_len(&nson) == 0);
}

static void
add_int_to_array() {
	Nson nson;
	nson_init(&nson, NSON_ARR);
	assert(nson_type(&nson) == NSON_ARR);
	assert(nson_len(&nson) == 0);

	nson_push_int(&nson, 42);
	assert(nson_type(nson_get(&nson, 0)) == NSON_INT);
}

static void
check_messy_array() {
	Nson nson, val;
	nson_init(&nson, NSON_ARR);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_int(&val, 5);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_int(&val, 6);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_int(&val, 6);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_int(&val, 4);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) != 0);
}

static void
check_messy_object() {
	Nson nson, val;
	nson_init(&nson, NSON_OBJ);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_str(&val, "a");
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);
	nson_init_int(&val, 6);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_str(&val, "b");
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);
	nson_init_int(&val, 6);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_str(&val, "c");
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);
	nson_init_int(&val, 4);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) == 0);

	nson_init_str(&val, "b");
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) != 0);
	nson_init_int(&val, 4);
	nson_push(&nson, &val);
	assert((nson.c.info & NSON_MESSY) != 0);

}

static void
clone_array() {
	Nson nson, clone;
	NSON(&nson, [ 1, 2 ]);

	nson_clone(&clone, &nson);

	assert(nson_int(nson_get(&clone, 0)) == 1);
	assert(nson_int(nson_get(&clone, 1)) == 2);
}

static void
sort_array() {
	int rv;
	Nson nson;

	rv = NSON(&nson, [5,4,3,2,1]);
	assert(rv >= 0);

	assert(nson_sort(&nson) >= 0);

	assert(nson_int(nson_get(&nson, 0)) == 1);
	assert(nson_int(nson_get(&nson, 1)) == 2);
	assert(nson_int(nson_get(&nson, 2)) == 3);
	assert(nson_int(nson_get(&nson, 3)) == 4);
	assert(nson_int(nson_get(&nson, 4)) == 5);

	(void)rv;
}

static void
sort_object() {
	int rv;
	Nson nson;

	rv = NSON(&nson, {
			"e":"five",
			"d":"four",
			"c":"three",
			"b":"two",
			"a":"one"
	});
	assert(rv >= 0);

	assert(nson_sort(&nson) >= 0);

	assert(strcmp(nson_str(nson_get(&nson, 0)), "one") == 0);
	assert(strcmp(nson_str(nson_get(&nson, 1)), "two") == 0);
	assert(strcmp(nson_str(nson_get(&nson, 2)), "three") == 0);
	assert(strcmp(nson_str(nson_get(&nson, 3)), "four") == 0);
	assert(strcmp(nson_str(nson_get(&nson, 4)), "five") == 0);

	assert(strcmp(nson_get_key(&nson, 0), "a") == 0);
	assert(strcmp(nson_get_key(&nson, 1), "b") == 0);
	assert(strcmp(nson_get_key(&nson, 2), "c") == 0);
	assert(strcmp(nson_get_key(&nson, 3), "d") == 0);
	assert(strcmp(nson_get_key(&nson, 4), "e") == 0);

	(void)rv;
}

/*static void
filter_array() {
	int rv;
	Nson nson;

	rv = NSON(&nson, [ 1, 5, 2, 5, 5, 3, 5]);
	assert(rv >= 0);

	nson_filter(&nson, filter_lesser_5);

	assert(nson_len(&nson) == 3);

	assert(nson_int(nson_get(&nson, 0)) == 1);
	assert(nson_int(nson_get(&nson, 1)) == 2);
	assert(nson_int(nson_get(&nson, 2)) == 3);

	(void)rv;
}

static void
filter_object() {
	int rv;
	Nson nson;

	rv = NSON(&nson, {
			"a": 1,
			"b": 5,
			"c": 2,
			"d": 5,
			"e": 5,
			"f": 3,
			"g": 5
	});

	assert(rv >= 0);

	nson_filter(&nson, filter_lesser_5);

	assert(nson_len(&nson) == 3);

	assert(nson_int(nson_get(&nson, 0)) == 1);
	assert(nson_int(nson_get(&nson, 1)) == 2);
	assert(nson_int(nson_get(&nson, 2)) == 3);

	assert(strcmp(nson_get_key(&nson, 0), "a") == 0);
	assert(strcmp(nson_get_key(&nson, 1), "c") == 0);
	assert(strcmp(nson_get_key(&nson, 2), "f") == 0);

	(void)rv;
}*/

DEFINE
TEST(unflat_array);
TEST(unflat_array_depth_2);
TEST(create_array);
TEST(add_int_to_array);
TEST(clone_array);
TEST(check_messy_array);
TEST(check_messy_object);
TEST(sort_array);
TEST(sort_object);
//TEST(filter_array);
//TEST(filter_object);
DEFINE_END
