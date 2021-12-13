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

#include "../src/internal.h"

#include "common.h"
#include "test.h"

static void
create_array() {
	Nson nson = {0};
	nson_init(&nson, NSON_ARR);
	assert(nson_type(&nson) == NSON_ARR);
	assert(nson_arr_len(&nson) == 0);
}

static void
add_int_to_array() {
	Nson nson = {0};
	nson_init(&nson, NSON_ARR);
	assert(nson_type(&nson) == NSON_ARR);
	assert(nson_arr_len(&nson) == 0);

	// nson_arr_push_int(&nson, 42);
	// assert(nson_type(nson_arr_get(&nson, 0)) == NSON_INT);

	nson_clean(&nson);
}

static void
check_messy_array() {
	Nson nson = {0}, val = {0};
	nson_init(&nson, NSON_ARR);

	nson_int_wrap(&val, 5);
	nson_arr_push(&nson, &val);

	nson_int_wrap(&val, 6);
	nson_arr_push(&nson, &val);

	nson_int_wrap(&val, 6);
	nson_arr_push(&nson, &val);

	nson_int_wrap(&val, 4);
	nson_arr_push(&nson, &val);

	nson_clean(&nson);
}

static void
check_messy_object() {
	Nson nson = {0}, val = {0};
	nson_init(&nson, NSON_OBJ);

	nson_int_wrap(&val, 6);
	nson_obj_put(&nson, "a", &val);

	nson_int_wrap(&val, 6);
	nson_obj_put(&nson, "b", &val);

	nson_int_wrap(&val, 4);
	nson_obj_put(&nson, "c", &val);

	nson_int_wrap(&val, 4);
	nson_obj_put(&nson, "d", &val);

	nson_clean(&nson);
}

static void
clone_array() {
	Nson nson = {0}, clone = {0};
	NSON(&nson, [ 1, 2 ]);

	nson_clone(&clone, &nson);

	assert(nson_int(nson_arr_get(&clone, 0)) == 1);
	assert(nson_int(nson_arr_get(&clone, 1)) == 2);

	nson_clean(&clone);
	nson_clean(&nson);
}

static void
sort_array() {
	int rv;
	Nson nson = {0};

	rv = NSON(&nson, [ 5, 4, 3, 2, 1 ]);
	assert(rv >= 0);

	assert(nson_arr_sort(&nson) >= 0);

	assert(nson_int(nson_arr_get(&nson, 0)) == 1);
	assert(nson_int(nson_arr_get(&nson, 1)) == 2);
	assert(nson_int(nson_arr_get(&nson, 2)) == 3);
	assert(nson_int(nson_arr_get(&nson, 3)) == 4);
	assert(nson_int(nson_arr_get(&nson, 4)) == 5);

	nson_clean(&nson);
	(void)rv;
}

static void
sort_object() {
	int rv;
	Nson nson = {0};

	rv = NSON(&nson, {
		"e" : "five",
		"d" : "four",
		"c" : "three",
		"b" : "two",
		"a" : "one"
	});
	assert(rv >= 0);

	// before getting, the object will be sorted
	assert(nson_obj_get(&nson, "not existent") == NULL);

	assert(strcmp(nson_obj_get_key(&nson, 0), "a") == 0);
	assert(strcmp(nson_obj_get_key(&nson, 1), "b") == 0);
	assert(strcmp(nson_obj_get_key(&nson, 2), "c") == 0);
	assert(strcmp(nson_obj_get_key(&nson, 3), "d") == 0);
	assert(strcmp(nson_obj_get_key(&nson, 4), "e") == 0);

	assert(strcmp(nson_str(nson_obj_get(&nson, "a")), "one") == 0);
	assert(strcmp(nson_str(nson_obj_get(&nson, "b")), "two") == 0);
	assert(strcmp(nson_str(nson_obj_get(&nson, "c")), "three") == 0);
	assert(strcmp(nson_str(nson_obj_get(&nson, "d")), "four") == 0);
	assert(strcmp(nson_str(nson_obj_get(&nson, "e")), "five") == 0);

	nson_clean(&nson);
	(void)rv;
}

static void
walk_array_empty() {
	int rv;
	off_t index = -1;
	Nson nson = {0};
	NsonStack stack = {0};
	Nson *item;
	Nson *stack_top;

	rv = NSON(&nson, []);
	assert(rv >= 0);

	item = stack_top = &nson;
	assert(nson_type(item) == NSON_ARR);
	assert(item == stack_top);
	assert(index == -1);

	item = stack_walk(&stack, &stack_top, &index);
	assert(nson_type(item) == NSON_ARR);
	assert(item == &nson);

	item = stack_walk(&stack, &stack_top, &index);
	assert(item == NULL);

	nson_clean(&nson);
}

static void
walk_array_tree() {
	int rv;
	off_t index = -1;
	Nson nson = {0};
	NsonStack stack = {0};
	Nson *item;
	Nson *stack_top;

	rv = NSON(&nson, [ 1, [2], 3, [4], 5 ]);
	assert(rv >= 0);

	assert(rv >= 0);

	{
		item = stack_top = &nson;
		assert(nson_type(item) == NSON_ARR);
		assert(item == stack_top);
		assert(index == -1);

		item = stack_walk(&stack, &stack_top, &index);
		assert(nson_type(item) == NSON_INT);
		assert(nson_int(item) == 1);
		assert(index == 0);

		{
			item = stack_walk(&stack, &stack_top, &index);
			assert(nson_type(item) == NSON_ARR);
			assert(item == stack_top);
			assert(index == -1);

			item = stack_walk(&stack, &stack_top, &index);
			assert(nson_type(item) == NSON_INT);
			assert(nson_int(item) == 2);
			assert(index == 0);
		}
		item = stack_walk(&stack, &stack_top, &index);
		assert(nson_type(item) == NSON_ARR);
		assert(item != stack_top);
		printf("%li\n", index);
		assert(index == 1);

		item = stack_walk(&stack, &stack_top, &index);
		assert(nson_type(item) == NSON_INT);
		assert(nson_int(item) == 3);
		assert(index == 2);

		{
			item = stack_walk(&stack, &stack_top, &index);
			assert(nson_type(item) == NSON_ARR);
			assert(item == stack_top);
			assert(index == -1);

			item = stack_walk(&stack, &stack_top, &index);
			assert(nson_type(item) == NSON_INT);
			assert(nson_int(item) == 4);
			assert(index == 0);
		}
		item = stack_walk(&stack, &stack_top, &index);
		assert(nson_type(item) == NSON_ARR);
		assert(item != stack_top);
		assert(index == 3);

		item = stack_walk(&stack, &stack_top, &index);
		assert(nson_type(item) == NSON_INT);
		assert(nson_int(item) == 5);
		assert(index == 4);
	}
	item = stack_walk(&stack, &stack_top, &index);
	assert(nson_type(item) == NSON_ARR);
	assert(item == &nson);
	assert(stack_top == NULL);
	assert(index == 0);

	(void)rv;

	nson_clean(&nson);
}

static void
issue_nullref() {
	int rv;
	Nson nson = {{{0}}};
	Nson item_stack = {{{0}}};
	Nson *item;

	char *src = strdup("[\"1\"]");
	int len = strlen(src);
	rv = nson_parse_json(&nson, src, len);
	assert(rv >= 0);

	item = nson_arr_get(&nson, 0);

	nson_move(&item_stack, item);
	memset(src, 'A', len);
	nson_clean(&nson);
	assert(strcmp("1", nson_str(&item_stack)) == 0);
	nson_clean(&item_stack);
	free(src);
}

static void
issue_concat_array() {
	Nson nson_1 = {0};
	Nson nson_2 = {0};
	int rv = 0;
	char *str;
	size_t len;

	rv = NSON(&nson_1, [ 1, 2, 3, 4, 5 ]);
	rv = NSON(&nson_2, [ 6, 7, 8, 9, 10 ]);

	rv = nson_arr_concat(&nson_1, &nson_2);

	nson_json_serialize(&str, &len, &nson_1, 0);
	puts(str);

	assert(strcmp("[1,2,3,4,5,6,7,8,9,10]", str) == 0);

	nson_clean(&nson_1);
	nson_clean(&nson_1);
	free(str);
	(void)rv;
}

static void
issue_concat_empty() {
	Nson nson_1 = {0};
	Nson nson_2 = {0};
	int rv = 0;
	char *str;
	size_t len;

	rv = NSON(&nson_1, []);
	rv = NSON(&nson_2, []);

	rv = nson_arr_concat(&nson_1, &nson_2);

	nson_json_serialize(&str, &len, &nson_1, 0);
	puts(str);

	assert(strcmp("[]", str) == 0);

	nson_clean(&nson_1);
	nson_clean(&nson_1);
	free(str);
	(void)rv;
}

DEFINE
TEST(create_array);
TEST(add_int_to_array);
TEST(clone_array);
TEST(check_messy_array);
TEST(check_messy_object);
TEST(sort_array);
TEST(sort_object);
TEST(walk_array_empty);
TEST(walk_array_tree);
TEST(issue_nullref);
TEST(issue_concat_array);
TEST(issue_concat_empty);
DEFINE_END
