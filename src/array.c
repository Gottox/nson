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

#include "internal.h"
#include "nson.h"
#include <assert.h>
#include <string.h>
#include <errno.h>

static int
cmp_stable(const void *a, const void *b) {
	const Nson *na = a, *nb = b;
	int rv = nson_cmp(na, nb);
	return rv ? rv : (na - nb);
}

static int
mem_capacity(Nson *nson, const size_t size) {
	Nson *arr;
	const size_t old = nson_arr_len(nson);

	if (size == old) {
		return size;
	}

	arr = nson->a.arr;
	nson->a.len = size;

	if (size && size > SIZE_MAX / sizeof(*arr)) {
		errno = ENOMEM;
		return -1;
	}
	arr = reallocarray(arr, size, sizeof(*arr));
	if(!arr) {
		nson->a.len = old;
		return -1;
	}
	if(size > old)
		memset(&arr[old], 0, sizeof(*arr) * (size - old));

	nson->a.arr = arr;
	return size;
}

int
__nson_arr_clone(Nson *array) {
	int rv = 0;
	Nson *arr = array->a.arr;
	size_t len = nson_arr_len(array);

	array->a.arr = NULL;
	array->a.len = 0;

	rv = mem_capacity(array, len);
	if (rv < 0 || array->a.arr == NULL)
		return rv;
	memcpy(array->a.arr, arr, len * sizeof(*arr));

	return rv;
}

int
nson_init_array(Nson *array) {
	nson_init(array, NSON_ARR);
	return 0;
}

size_t
nson_arr_len(const Nson *array) {
  assert(nson_type(array) == NSON_ARR);

  return array->a.len;
}

Nson *
nson_arr_get(const Nson *array, off_t index) {
  assert(nson_type(array) == NSON_ARR);
  assert(index < nson_arr_len(array));

  return &array->a.arr[index];
}

int
nson_arr_push(Nson *array, Nson *value) {
  assert(nson_type(array) == NSON_ARR);

  Nson *new_elem;
  size_t old_len = nson_arr_len(array);

  mem_capacity(array, old_len + 1);

  new_elem = &array->a.arr[old_len];
  nson_move(new_elem, value);

  return 0;
}

int
nson_arr_push_int(Nson *array, int value) {
	Nson v = { 0 };
	nson_int_wrap(&v, value);

	return nson_arr_push(array, &v);
}

int
nson_arr_push_real(Nson *array, double value) {
	Nson v = { 0 };
	nson_real_wrap(&v, value);

	return nson_arr_push(array, &v);
}

int
nson_arr_push_str(Nson *array, char *value) {
	Nson v = { 0 };
	nson_init_str(&v, value);

	return nson_arr_push(array, &v);
}

int
nson_arr_pop(Nson *last, Nson *array) {
	assert(nson_type(array) == NSON_ARR);

	size_t len = nson_arr_len(array);
	if (len == 0) {
		return -1;
	} else {
		nson_move(last, nson_arr_get(array, len - 1));
		array->a.len = len - 1;

		return 0;
	}
}

Nson *
nson_arr_last(Nson *array) {
	assert(nson_type(array) == NSON_ARR);

	size_t len = nson_arr_len(array);
	if (len == 0) {
		return NULL;
	} else {
		return nson_arr_get(array, len - 1);
	}
}

int
nson_arr_concat(Nson *array_1, Nson *array_2) {
	assert(nson_type(array_1) == NSON_ARR);
	assert(nson_type(array_2) == NSON_ARR);

	const size_t len_1 = nson_arr_len(array_1);
	const size_t len_2 = nson_arr_len(array_2);

	if(mem_capacity(array_1, len_1 + len_2) < 0) {
		return -1;
	}

	memcpy(&array_1->a.arr[len_1], &array_2->a.arr, len_2);

	// Set length to 0 to avoid cleanup of array elements
	array_2->a.len = 0;
	nson_clean(array_2);

	return 0;
}

int
nson_arr_sort(Nson *nson) {
	assert(nson_type(nson) == NSON_ARR);

	size_t len = nson_arr_len(nson);

	qsort(nson->a.arr, len, sizeof(*nson), cmp_stable);

	return 0;
}

int
__nson_arr_clean(Nson *nson) {
	int i, rv = 0;

	for (i = 0; i < nson_arr_len(nson); i++) {
		rv |= nson_clean(nson_arr_get(nson, i));
	}
	free(nson->a.arr);

	return rv;
}

int
__nson_arr_serialize(FILE *out, const Nson *array,
		const NsonSerializerInfo *info, enum NsonOptions options) {
	int i;
	size_t size = nson_arr_len(array);
	Nson *element;

	for (i = 0; i < size; i++) {
		element = nson_arr_get(array, i);
		info->serializer(out, element, options | NSON_SKIP_HEADER);
		if (i + 1 != size) {
			fputs(info->seperator, out);
		}
	}

	return 0;
}
