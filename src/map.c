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
#include <unistd.h>
#include <pthread.h>

#include "config.h"
#include "nson.h"

static const char base64_table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
nson_cmp_stable(const void *a, const void *b) {
	const Nson *na = a, *nb = b;
	int rv = nson_cmp(na, nb);
	return rv ? rv : (na - nb);
}

int
nson_mapper_b64_dec(off_t index, Nson *nson, void *user_data) {
	char *p;
	int8_t v;
	off_t i, j;
	char *dest;
	size_t dest_len;
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);
	const size_t src_len = nson_data_len(nson);
	const char *src = nson_data(nson);

	dest_len = (src_len + 3) / 4 * 3;
	dest = calloc(dest_len + 1, sizeof(*dest));
	if (!dest)
		return -1;

	for (i = j = 0; i < src_len && src[i] != '='; i++) {
		p = memchr(base64_table, src[i], 64);
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

	for(; i % 4 != 0 && i < src_len && src[i] == '='; i++);

	if(i % 4 != 0)
		return -1;

	nson_clean(nson);
	nson_init_data(nson, dest, j, NSON_BLOB);

	return i;
}

int
nson_mapper_b64_enc(off_t index, Nson *nson, void *user_data) {
	off_t i, j;
	char *dest;
	size_t dest_len;
	int reminder = 0;
	static const char mask = (1 << 6) - 1;
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);

	if(nson->c.mapper == nson_mapper_b64_dec) {
		nson->c.mapper = NULL;
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
			reminder = (src[i] << 4) & mask;
			break;
		case 1:
			dest[j] = base64_table[(src[i] >> 4 | reminder) & mask];
			reminder = (src[i] << 2) & mask;
			break;
		case 2:
			dest[j++] = base64_table[(src[i] >> 6 | reminder) & mask];
			dest[j] = base64_table[src[i] & mask];
			break;
		}
	}
	if(dest_len != j) {
		dest[j] = base64_table[reminder & mask];
		memset(&dest[j + 1], '=', dest_len - j - 1);
	}

	nson_clean(nson);
	nson_init_data(nson, dest, dest_len, NSON_STR);

	return dest_len;
}

Nson *
nson_get_by_key(const Nson *nson, const char *key) {
	Nson needle = { .d.b = (char *)key, .d.len = strlen(key) };
	Nson *result;
	size_t len, size;

	assert(nson_type(nson) == NSON_OBJ);
	len = nson->a.len / 2;
	size = sizeof(needle) * 2;
	if (nson->a.messy)
		result = lfind(&needle, nson->a.arr, &len, size, nson_cmp);
	else
		result = bsearch(&needle, nson->a.arr, len, size, nson_cmp);
	return result;
}

int
nson_sort(Nson *nson) {
	assert(nson_type(nson) & (NSON_OBJ | NSON_ARR));
	size_t len = nson_mem_len(nson);
	size_t size = sizeof(*nson);

	if (!nson->a.messy)
		return 0;

	if (nson_type(nson) == NSON_OBJ) {
		len /= 2;
		size *= 2;
	}
	qsort(nson->a.arr, len, size, nson_cmp_stable);
	nson->a.messy = false;

	return 0;
}

int
nson_reduce(Nson *dest, const Nson *nson, NsonReducer reducer,
		const void *user_data) {
	int rv = 0;
	off_t i;
	size_t len;

	len = nson_mem_len(nson);
	for (i = 0; rv >= 0 && i < len; i++) {
		rv = reducer(i, dest, nson_mem_get(nson, i), user_data);
	}
	return rv;
}

int
nson_map(Nson *nson, NsonMapper mapper, void *user_data) {
	int rv = 0;
	off_t i;
	size_t len;
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ
			|| nson_type(nson) == NSON_SLICE);

	len = nson_mem_len(nson);
	for (i = 0; rv >= 0 && i < len; i++) {
		rv = mapper(i, nson_mem_get(nson, i), user_data);
	}
	return rv;
}

struct ThreadInfo {
	Nson *nson;
	void *user_data;
	NsonMapper mapper;
	pthread_t thread;
	int fd;
	int rv;
	uint8_t bucket_size;
};

static void *
map_thread_wrapper(void *arg) {
	int i, rv;
	struct ThreadInfo *thread = arg;
	int *buckets = alloca(thread->bucket_size * sizeof(int));
	size_t bucket_len = 0;

	while ((bucket_len = read(thread->fd, buckets, thread->bucket_size * sizeof(int))) > 0) {
		for (i = 0; rv >= 0 && i < bucket_len / sizeof(int); i++) {
			rv = thread->mapper(buckets[i], nson_mem_get(thread->nson, buckets[i]),
					thread->user_data);
		}
	}
	thread->rv = rv;

	return NULL;
}

int
nson_map_thread(Nson *nson, NsonMapper mapper, void *user_data) {
	struct ThreadInfo *threads;
	int thread_num, bucket_size, i, p[2];
	FILE *fd;
	size_t len;

	thread_num = (int)sysconf(_SC_NPROCESSORS_ONLN);
	len = nson_mem_len(nson);
	if(thread_num <= 1 || len <= 1) {
		return nson_map(nson, mapper, user_data);
	} else if(thread_num > len) {
		thread_num = len;
		bucket_size = 1;
	} else {
		bucket_size = len / thread_num;
		if(bucket_size > 32)
			bucket_size = 32;
	}

	threads = alloca(thread_num * sizeof(*threads));
	if(pipe(p) < 0)
		return -1;

	for (i = 0; i < thread_num; i++) {
		threads[i].mapper = mapper;
		threads[i].user_data = user_data;
		threads[i].nson = nson;
		threads[i].fd = p[0];
		threads[i].bucket_size = bucket_size;
		pthread_create(&threads[i].thread, NULL, map_thread_wrapper, &threads[i]);
	}

	fd = fdopen(p[1], "w");
	for (i = 0; i < len; i++) {
		fwrite(&i, sizeof(int), 1, fd);
	}
	fclose(fd);

	for (i = 0; i < thread_num; i++) {
		pthread_join(threads[i].thread, NULL);
	}

	close(p[0]);
	return 0;
}
