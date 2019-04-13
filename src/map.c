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

#include <assert.h>
#include <string.h>
#include <search.h>
#include <unistd.h>
#include <pthread.h>

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
	NsonBuf *dest_buf;
	char *dest;
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);
	const size_t src_len = nson_data_len(nson);
	const char *src = nson_data(nson);

	dest_buf = nson_buf_new((src_len + 3) / 4 * 3);
	dest = nson_buf_unwrap(dest_buf);
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

	if(i % 4 != 0) {
		free(dest);
		return -1;
	}

	nson_buf_release(nson->d.buf);
	nson->d.buf = dest_buf;
	nson_buf_shrink(nson->d.buf, j);

	return i;
}

int
nson_mapper_b64_enc(off_t index, Nson *nson, void *user_data) {
	off_t i, j;
	char *dest;
	NsonBuf *dest_buf;
	int reminder = 0;
	static const char mask = (1 << 6) - 1;
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);

	const size_t src_len = nson_data_len(nson);
	const char *src = nson_data(nson);

	dest_buf = nson_buf_new((src_len + 2) / 3 * 4);
	dest = nson_buf_unwrap(dest_buf);
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
	if(nson_buf_siz(dest_buf) != j) {
		dest[j] = base64_table[reminder & mask];
		memset(&dest[j + 1], '=', nson_buf_siz(dest_buf) - j - 1);
	}

	nson_buf_release(nson->d.buf);
	nson->d.buf = dest_buf;

	return nson_buf_siz(dest_buf);
}

Nson *
nson_get_by_key(const Nson *nson, const char *key) {
	Nson needle = { .d.buf = nson_buf_wrap_0(key) };
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
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ);

	len = nson_mem_len(nson);
	for (i = 0; rv >= 0 && i < len; i++) {
		rv = mapper(i, nson_mem_get(nson, i), user_data);
	}
	return rv;
}

struct ThreadInfo {
	short id;
	size_t chunk_size;
	size_t len;
	Nson *nson;
	void *user_data;
	NsonMapper mapper;
	pthread_t thread;
	pthread_spinlock_t *lock;
	off_t *reserved;
	int rv;
};

static void *
map_thread_wrapper(void *arg) {
	int rv = 0;
	off_t i;
	size_t end;
	struct ThreadInfo *thread = arg;
	i = thread->id * thread->chunk_size;
	end = i + thread->chunk_size;

	do {
		for (; i < end && i < thread->len; i++) {
			thread->mapper(i, nson_mem_get(thread->nson, i), thread->user_data);
		}
		pthread_spin_lock(thread->lock);
		i = *thread->reserved;
		end = i + thread->chunk_size;
		*thread->reserved = end;
		pthread_spin_unlock(thread->lock);
	} while(i < thread->len);

	thread->rv = rv;

	return NULL;
}

int
nson_map_thread(Nson *nson, NsonMapper mapper, void *user_data) {
	struct ThreadInfo *threads;
	int thread_num, chunk_size, i;
	size_t len;
	pthread_spinlock_t lock;
	off_t reserved;

	thread_num = (int)sysconf(_SC_NPROCESSORS_ONLN);
	len = nson_mem_len(nson);

	if(thread_num <= 1 || len <= 1) {
		return nson_map(nson, mapper, user_data);
	} else if(thread_num > len) {
		thread_num = len;
		chunk_size = 1;
	} else {
		chunk_size = len / thread_num;
		if(chunk_size > 32)
			chunk_size = 32;
	}

	threads = alloca(thread_num * sizeof(*threads));
	pthread_spin_init(&lock, PTHREAD_PROCESS_SHARED);

	reserved = thread_num * chunk_size;
	for (i = 0; i < thread_num; i++) {
		threads[i].rv = 0;
		threads[i].mapper = mapper;
		threads[i].user_data = user_data;
		threads[i].nson = nson;
		threads[i].id = i;
		threads[i].len = len;
		threads[i].chunk_size = chunk_size;
		threads[i].lock = &lock;
		threads[i].reserved = &reserved;

		if (i == thread_num - 1)
			map_thread_wrapper(&threads[i]);
		else
			pthread_create(&threads[i].thread, NULL, map_thread_wrapper, &threads[i]);
	}

	for (i = 0; i < thread_num - 1; i++) {
		pthread_join(threads[i].thread, NULL);
	}

	pthread_spin_destroy(&lock);
	return 0;
}
