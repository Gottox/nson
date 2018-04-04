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

#include <string.h>
#include <search.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

#include "nson.h"

#define BUFFER_SIZE 16

#define SCAL_CMP(a, b) (a > b ? 1 : (a < b ? -1 : 0))

static const char base64_table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
nson_compare(const void *a, const void *b) {
	int rv;
	enum NsonType type = nson_type(a);

	// Reverse sort as NSON_NONE needs to be last
	if ((rv = nson_type(b) - type))
		return rv;
	switch(type) {
	case NSON_PTR:
		return 0;
	case NSON_STR:
		return strcmp(nson_ptr(a), nson_ptr(b));
	case NSON_REAL:
		return SCAL_CMP(nson_real(a), nson_real(b));
	case NSON_INT:
	case NSON_BOOL:
		return SCAL_CMP(nson_int(a), nson_int(b));
	default:
		return 0;
	}
}

static int
nson_compare_stable(const void *a, const void *b) {
	int rv = nson_compare(a, b);
	return rv ? rv : SCAL_CMP(a, b);
}

static int
nson_set_len(struct Nson *nson, size_t size) {
	struct Nson *arr;
	size_t old = nson_mem_len(nson);

	nson->val.a.len = size;

	if(size % BUFFER_SIZE != 0)
		size += BUFFER_SIZE - (size % BUFFER_SIZE);

	if(old != size) {
		arr = nson->val.a.arr;
		arr = realloc(arr, sizeof(*arr) * size);
		if(!arr) {
			nson->val.a.len = old;
			return -1;
		}
		memset(&arr[old], 0, sizeof(*arr) * (size - old));
		nson->val.a.arr = arr;
	}

	return nson_mem_len(nson);
}

int
nson_clean(struct Nson *nson) {
	off_t i;
	int rv = 0;

	switch(nson->alloc_type) {
	case NSON_ALLOC_MMAP:
		if(munmap(nson->alloc.m.map, nson->alloc.m.len) < 0)
			return -1;
		break;
	case NSON_ALLOC_BUF:
		free(nson->alloc.b);
		break;
	default:
		break;
	}

	if(nson_type(nson) & (NSON_ARR | NSON_OBJ)) {
		for (i = 0; i < nson_mem_len(nson); i++) {
			rv = nson_clean(nson_mem_get(nson, i));
			if(rv < 0)
				break;
		}
		free(nson->val.a.arr);
	}
	memset(nson, 0, sizeof(*nson));

	return rv;
}

size_t
nson_len(const struct Nson *nson) {
	if(nson_type(nson) & (NSON_PTR | NSON_STR))
		return nson->val.c.len;
	else if(nson_type(nson) == NSON_OBJ)
		return nson_mem_len(nson) / 2;
	else
		return nson_mem_len(nson);
}

const char *
nson_ptr(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_STR | NSON_PTR));
	return nson->val.c.b;
}

enum NsonType
nson_type(const struct Nson *nson) {
	return nson->type;
}

int64_t
nson_int(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_INT | NSON_BOOL | NSON_REAL));
	if(nson_type(nson) == NSON_REAL)
		return (int64_t)nson->val.r;
	return nson->val.i;
}

double
nson_real(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_INT | NSON_BOOL | NSON_REAL));
	if(nson_type(nson) == NSON_REAL)
		return nson->val.r;
	return (double)nson->val.i;
}

struct Nson *
nson_get(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	if(nson_type(nson) == NSON_OBJ)
		index = index * 2 + 1;
	return nson_mem_get(nson, index);
}

struct Nson *
nson_get_by_key(const struct Nson *nson, const char *key) {
	struct Nson needle = { .val.c.b = (char *)key };
	struct Nson *result;
	size_t len, size;

	assert(nson_type(nson) == NSON_OBJ);
	len = nson->val.a.len / 2;
	size = sizeof(needle) * 2;
	if (nson->val.a.messy)
		result = lfind(&needle, nson->val.a.arr, &len, size, nson_compare);
	else
		result = bsearch(&needle, nson->val.a.arr, len, size, nson_compare);
	return result;
}

int
nson_sort(struct Nson *nson) {
	assert(nson->type == NSON_OBJ || nson->type == NSON_ARR);
	size_t len = nson->val.a.len;
	size_t size = sizeof(*nson);

	if (!nson->val.a.messy)
		return 0;
	if (nson_type(nson) == NSON_OBJ) {
		len = nson->val.a.len / 2;
		size = sizeof(*nson) * 2;
	}
	qsort(nson->val.a.arr, len, size, nson_compare_stable);
	nson->val.a.messy = 0;
	return 0;
}

const char *
nson_get_key(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_OBJ);
	index = index * 2;
	assert(nson->val.a.len > index);

	return nson->val.a.arr[index].val.c.b;
}

int
nson_add(struct Nson *nson, struct Nson *val) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	struct Nson *arr;
	size_t last = nson_mem_len(nson);

	if(nson_type(nson) == NSON_OBJ && last % 2 == 0 && nson_type(val) != NSON_STR) {
		return -1;
	}

	nson_set_len(nson, last+1);

	arr = nson_mem_get(nson, 0);
	memcpy(&arr[last], val, sizeof(*arr));

	if(!nson->val.a.messy && last >= 1 &&
			(nson_type(nson) == NSON_ARR || last % 2 == 0)) {
		nson->val.a.messy = nson_compare(&arr[last-1], &arr[last]) > 0;
	}

	return 0;
}

int
nson_clone(struct Nson *nson, struct Nson *src) {
	int rv;
	off_t i = 0;

	enum NsonType type = nson_type(src);
	switch(type) {
		case NSON_ARR:
		case NSON_OBJ:
			nson_init(nson, type);
			rv = nson_set_len(nson, nson_mem_len(src));
			for(i = 0; rv >= 0 && i < nson_mem_len(src); i++) {
				rv = nson_clone(nson_mem_get(nson, i), nson_mem_get(src, i));
			}
			return rv;
		case NSON_PTR:
		case NSON_STR:
			return nson_init_str(nson, nson_ptr(src));
		default:
			nson_init(nson, type);
			memcpy(&nson->val, &src->val, sizeof(src->val));
			return 0;
	}
}

int
nson_add_all(struct Nson *nson, struct Nson *suff) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	assert(nson_type(suff) & (NSON_ARR | NSON_OBJ));

	const size_t suff_len = nson_mem_len(suff);
	const size_t nson_len = nson_mem_len(nson);

	nson_set_len(nson, nson_len + suff_len);

	memcpy(&nson->val.a.arr[nson_len], &suff->val.a.arr, suff_len);

	suff->val.a.len = 0;
	nson_clean(suff);

	return 0;
}

int
nson_add_ptr(struct Nson *nson, const char *val, size_t len) {
	struct Nson elem;

	nson_init_ptr(&elem, val, len);
	return nson_add(nson, &elem);
}

int
nson_add_str(struct Nson *nson, const char *val) {
	struct Nson elem;

	nson_init_str(&elem, val);
	return nson_add(nson, &elem);
}

int
nson_add_int(struct Nson *nson, int64_t val) {
	struct Nson elem;

	nson_init_int(&elem, val);
	return nson_add(nson, &elem);
}

int
nson_insert(struct Nson *nson, const char *key,
		struct Nson* val) {
	assert(nson_type(nson) == NSON_OBJ);

	if (nson_add_str(nson, key) < 0 || nson_add(nson, val) < 0) {
		nson->val.a.len -= nson->val.a.len % 2;
		return -1;
	}
	return 0;
}

int
nson_insert_ptr(struct Nson *nson, const char *key,
		const char *val, const size_t len) {
	struct Nson elem;

	nson_init_ptr(&elem, val, len);
	return nson_insert(nson, key, &elem);
}

int
nson_insert_str(struct Nson *nson, const char *key,
		const char *val) {
	struct Nson elem;

	nson_init_str(&elem, val);
	return nson_insert(nson, key, &elem);
}

int
nson_insert_int(struct Nson *nson, const char *key,
		int64_t val) {
	struct Nson elem;

	nson_init_int(&elem, val);
	return nson_insert(nson, key, &elem);
}

int
nson_init(struct Nson *nson, const enum NsonType type) {
	assert(type != NSON_NONE);

	memset(nson, 0, sizeof(*nson));
	nson->type = type;

	return 0;
}

int
nson_init_ptr(struct Nson *nson, const char *val, size_t len) {
	int rv = nson_init(nson, NSON_PTR);
	nson->val.c.b = val;
	nson->val.c.len = len;

	return rv;
}

int
nson_init_ptr_b64(struct Nson *nson, char *buf) {
	char *p;
	int8_t v;
	off_t i, j;
	nson_init(nson, NSON_PTR);

	for(i = j = 0; buf[i] && buf[i] != '='; i++) {
		p = strchr(base64_table, buf[i]);
		if(p == NULL)
			break;
		v = p - base64_table;

		switch(i % 4) {
		case 0:
			buf[j] = v << 2;
			break;
		case 1:
			buf[j++] |= v >> 4;
			buf[j] = v << 4;
			break;
		case 2:
			buf[j++] |= v >> 2;
			buf[j] = v << 6;
			break;
		case 3:
			buf[j++] |= v;
			break;
		}
	}
	for(; i % 4 != 0 && buf[i] && buf[i] == '='; i++)
		j--;
	if(i % 4 != 0)
		return -1;
	nson->val.c.b = buf;
	nson->val.c.len = j + 1;

	return i;
}

int
nson_ptr_b64(const struct Nson *nson, FILE *fd) {
	int l;
	off_t i;
	char reminder = 0;
	static const char mask = (1 << 6) - 1;

	assert(nson_type(nson) & (NSON_PTR | NSON_STR));
	const char *buf = nson_ptr(nson);

	for(l = i = 0; i < nson_len(nson); i++, l++) {
		switch(i % 3) {
		case 0:
			fputc(base64_table[(buf[i] >> 2) & mask], fd);
			reminder = buf[i] << 4;
			break;
		case 1:
			fputc(base64_table[(buf[i] >> 4 | reminder) & mask], fd);
			reminder = buf[i] << 2;
			break;
		case 2:
			fputc(base64_table[(buf[i] >> 6 | reminder) & mask], fd);
			fputc(base64_table[buf[i] & mask], fd);
			l++;
			break;
		}
	}
	if(i % 3)
		fputc(base64_table[reminder & mask], fd);
	for(; i % 3 != 0; i++, l++)
		fputc('=', fd);
	return l;
}

int
nson_init_str(struct Nson *nson, const char *val) {
	int rv = nson_init_ptr(nson, strdup(val), strlen(val));
	nson->type = NSON_STR;
	nson->alloc_type = NSON_ALLOC_BUF;

	return rv;
}

int
nson_init_int(struct Nson *nson, const int64_t val) {
	int rv = nson_init(nson, NSON_INT);
	nson->val.i = val;

	return rv;
}

int
nson_init_real(struct Nson *nson, const double val) {
	int rv = nson_init(nson, NSON_REAL);
	nson->val.r = val;

	return rv;
}

int
nson_load(NsonParser parser, struct Nson *nson, const char *file) {
	struct stat st;
	size_t pgsize = (size_t)sysconf(_SC_PAGESIZE);
	size_t pgmask = pgsize - 1, mapsize;
	unsigned char *mf;
	int need_guard = 0;
	int fd;
	memset(nson, 0, sizeof(*nson));

	assert(nson);
	assert(file);

	if ((fd = open(file, O_RDONLY|O_CLOEXEC)) == -1)
		return -1;

	if (fstat(fd, &st) == -1) {
		(void)close(fd);
		return -1;
	}
	if (st.st_size > SSIZE_MAX - 1) {
		(void)close(fd);
		return -1;
	}
	mapsize = ((size_t)st.st_size + 1 + pgmask) & ~pgmask;
	if (mapsize < (size_t)st.st_size + 1) {
		(void)close(fd);
		return -1;
	}
	/*
	 * If the file length is an integral number of pages, then we
	 * need to map a guard page at the end in order to provide the
	 * necessary NUL-termination of the buffer.
	 */
	if (((st.st_size + 2) & pgmask) == 0)
		need_guard = 1;

	mf = mmap(NULL, need_guard ? mapsize + pgsize : mapsize,
	    PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	(void)close(fd);
	if (mf == MAP_FAILED) {
		(void)munmap(mf, mapsize);
		return -1;
	}

	int rv = parser(nson, (char *)mf);

	nson->alloc.m.map = mf;
	nson->alloc.m.len = mapsize;
	nson->alloc_type = NSON_ALLOC_MMAP;
	return rv;
}

struct Nson *
nson_mem_get(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	assert(index < nson->val.a.len);

	return &nson->val.a.arr[index];
}

size_t
nson_mem_len(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	return nson->val.a.len;
}

int
nson_map(struct Nson *nson, NsonMapper mapper) {
	int rv = 0;
	off_t i;
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	for (i = 0; rv >= 0 && i < nson_len(nson); i++) {
		rv = mapper(i, nson_get(nson, i));
	}
	return rv;
}

int
nson_remove(struct Nson *nson, off_t index, size_t size) {
	struct Nson *elem;
	size_t len = nson_len(nson);
	enum NsonType type = nson_type(nson);
	assert(type & (NSON_ARR | NSON_OBJ));
	assert(len >= index + size);

	if(type == NSON_OBJ) {
		index *= 2;
		size *= 2;
		len *= 2;
		nson_clean(nson_mem_get(nson, index + 1));
	}
	elem = nson_mem_get(nson, index);
	nson_clean(elem);
	memmove(elem, &elem[size], (len - index - size) * sizeof(*elem));
	nson_set_len(nson, len - size);
	return 0;
}

int
nson_filter(struct Nson *nson, NsonFilter filter) {
	int rv = 0;
	off_t i;
	size_t del_size = 0;
	enum NsonType type = nson_type(nson);
	assert(type & (NSON_ARR | NSON_OBJ));

	for (i = 0; rv >= 0 && i < nson_len(nson); i++) {
		rv = filter(nson_get(nson, i));
		if (rv == 0)
			del_size++;
		else if (del_size) {
			i -= del_size;
			nson_remove(nson, i, del_size);
			del_size = 0;
		}
	}
	if (del_size)
		nson_remove(nson, i - del_size, del_size);
	return rv;
}
