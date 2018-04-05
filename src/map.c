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

#include <assert.h>
#include <string.h>
#include <search.h>

#include "config.h"
#include "nson.h"

static const char base64_table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
nson_cmp_stable(const void *a, const void *b) {
	const struct Nson *na = a, *nb = b;
	int rv = nson_cmp(na, nb);
	return rv ? rv : (na - nb);
}

int
nson_mapper_b64_dec(off_t index, struct Nson *nson) {
	char *p;
	int8_t v;
	off_t i, j;
	char *dest;
	size_t dest_len;
	assert(nson_type(nson) & (NSON_DATA | NSON_STR));
	const size_t src_len = nson_data_len(nson);
	const char *src = nson_data(nson);

	dest_len = (src_len + 3) / 4 * 3;
	dest = calloc(dest_len + 1, sizeof(*dest));
	if (!dest)
		return -1;

	for (i = j = 0; i < src_len && src[i] != '='; i++) {
		p = strchr(base64_table, src[i]);
		if(p == NULL)
			break;
		v = p - base64_table;

		switch(i % 4) {
		case 0:
			dest[j] = v << 2;
			break;
		case 1:
			dest[j++] |= v >> 4;
			dest[j] = v << 4;
			break;
		case 2:
			dest[j++] |= v >> 2;
			dest[j] = v << 6;
			break;
		case 3:
			dest[j++] |= v;
			break;
		}
	}

	for(; i % 4 != 0 && i < src_len && src[i] == '='; i++)
		j--;

	if(i % 4 != 0)
		return -1;

	nson_clean(nson);
	nson_init_data(nson, dest, j + 1);
	nson->alloc_type = NSON_ALLOC_BUF;
	nson->alloc.b = dest;

	return i;
}

int
nson_mapper_b64_enc(off_t index, struct Nson *nson) {
	off_t i, j;
	char *dest;
	size_t dest_len;
	char reminder = 0;
	static const char mask = (1 << 6) - 1;
	assert(nson_type(nson) & (NSON_STR | NSON_DATA));

	if(nson->val.d.mapper == nson_mapper_b64_dec) {
		nson->val.d.mapper = NULL;
		return nson_data_len(nson);
	}

	const size_t src_len = nson_data_len(nson);
	const char *src = nson_data(nson);

	dest_len = (src_len + 2) / 3 * 4;
	dest = calloc(dest_len + 1, sizeof(*dest));
	if (!dest)
		return -1;

	for(j = i = 0; i < src_len; i++, j++) {
		switch(i % 3) {
		case 0:
			dest[j] = base64_table[(src[i] >> 2) & mask];
			reminder = src[i] << 4;
			break;
		case 1:
			dest[j] = base64_table[(src[i] >> 4 | reminder) & mask];
			reminder = src[i] << 2;
			break;
		case 2:
			dest[j++] = base64_table[(src[i] >> 6 | reminder) & mask];
			dest[j] = base64_table[src[i] & mask];
			break;
		}
	}
	if(j % 3)
		dest[j] = base64_table[reminder & mask];
	memset(&dest[j+1], '=', 3 - (j % 3));

	nson_clean(nson);
	nson_init_data(nson, dest, dest_len);
	nson->alloc_type = NSON_ALLOC_BUF;
	nson->alloc.b = dest;

	return dest_len;
}

struct Nson *
nson_get_by_key(const struct Nson *nson, const char *key) {
	struct Nson needle = { .val.d.b = (char *)key, .val.d.len = strlen(key) };
	struct Nson *result;
	size_t len, size;

	assert(nson_type(nson) == NSON_OBJ);
	len = nson->val.a.len / 2;
	size = sizeof(needle) * 2;
	if (nson->val.a.messy)
		result = lfind(&needle, nson->val.a.arr, &len, size, nson_cmp);
	else
		result = bsearch(&needle, nson->val.a.arr, len, size, nson_cmp);
	return result;
}

int
nson_sort(struct Nson *nson) {
	assert(nson_type(nson) & (NSON_OBJ | NSON_ARR));
	size_t len = nson_mem_len(nson);
	size_t size = sizeof(*nson);

	if (!nson->val.a.messy)
		return 0;

	if (nson_type(nson) == NSON_OBJ) {
		len /= 2;
		size *= 2;
	}
	qsort(nson->val.a.arr, len, size, nson_cmp_stable);
	nson->val.a.messy = 0;

	return 0;
}

int
nson_map(struct Nson *nson, NsonMapper mapper) {
	int rv = 0;
	off_t i;
	size_t len;
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	len = nson_len(nson);
	for (i = 0; rv >= 0 && i < len; i++) {
		rv = mapper(i, nson_get(nson, i));
	}
	return rv;
}

int
nson_filter(struct Nson *nson, NsonFilter filter) {
	int rv = 0;
	off_t i;
	size_t del_size = 0, len;
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	len = nson_len(nson);
	for (i = 0; rv >= 0 && i < len; i++) {
		rv = filter(nson_get(nson, i));
		if (rv == 0)
			del_size++;
		else if (del_size) {
			i -= del_size;
			len -= del_size;
			nson_remove(nson, i, del_size);
			del_size = 0;
		}
	}
	if (del_size)
		nson_remove(nson, i - del_size, del_size);
	return rv;
}

