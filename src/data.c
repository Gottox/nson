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

static int
nson_compare(const void *a, const void *b) {
	int rv;
	enum NsonType type = nson_type(a);

	if ((rv = type - nson_type(b)))
		return rv;
	switch(type) {
	case NSON_STR:
		rv = strcmp(nson_str(a), nson_str(b));
		break;
	case NSON_REAL:
		rv = SCAL_CMP(nson_real(a), nson_real(b));
		break;
	case NSON_INT:
	case NSON_BOOL:
		rv = SCAL_CMP(nson_int(a), nson_int(b));
		break;
	default:
		rv = 0;
	}
	return rv ? rv : SCAL_CMP(nson_real(a), nson_real(b));
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
	case NSON_ALLOC_BUFFER:
		free(nson->alloc.b);
		break;
	default:
		break;
	}

	if(nson_type(nson) & (NSON_ARR | NSON_OBJ)) {
		for (i = 0; i < nson_length(nson); i++) {
			rv = nson_clean(nson_get(nson, i));
			if(rv < 0)
				break;
		}
		free(nson->val.a.arr);
	}
	memset(nson, 0, sizeof(*nson));

	return rv;
}

size_t
nson_length(const struct Nson *nson) {
	if(nson_type(nson) == NSON_OBJ)
		return nson_mem_length(nson) / 2;
	else
		return nson_mem_length(nson);
}

const char *
nson_str(const struct Nson *nson) {
	assert(nson_type(nson) == NSON_STR);
	return nson->val.c;
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
	struct Nson needle = { .val.c = (char *)key };
	struct Nson *result;
	size_t len, size;

	assert(nson_type(nson) == NSON_OBJ);
	len = nson->val.a.len / 2;
	size = sizeof(needle) * 2;
	if (nson->val.a.sorted)
		result = bsearch(&needle, nson->val.a.arr, len, size, nson_compare);
	else
		result = lfind(&needle, nson->val.a.arr, &len, size, nson_compare);
	return result;
}

int
nson_sort(struct Nson *nson) {
	assert(nson->type == NSON_OBJ || nson->type == NSON_ARR);
	size_t len = nson->val.a.len;
	size_t size = sizeof(*nson);

	if (nson->val.a.sorted)
		return 0;
	if (nson_type(nson) == NSON_OBJ) {
		len = nson->val.a.len / 2;
		size = sizeof(*nson) * 2;
	}
	qsort(nson->val.a.arr, len, size, nson_compare);
	nson->val.a.sorted = true;
	return 0;
}

const char *
nson_get_key(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_OBJ);
	index = index * 2;
	assert(nson->val.a.len > index);

	return nson->val.a.arr[index].val.c;
}

int
nson_add(struct Nson *nson, struct Nson *val) {
	assert(nson_type(nson) == NSON_ARR);

	struct Nson *elem = nson->val.a.arr;

	if(nson->val.a.len % BUFFER_SIZE == 0) {
		elem = realloc(elem, sizeof(*elem) * (nson->val.a.len + BUFFER_SIZE));
		if(!elem)
			return -1;
		memset(&elem[nson->val.a.len], 0, BUFFER_SIZE * sizeof(*elem));
		nson->val.a.arr = elem;
	}

	elem = &elem[nson->val.a.len];
	nson->val.a.len++;

	memcpy(elem, val, sizeof(*elem));

	return 0;
}

int
nson_add_ptr(struct Nson *nson, const char *val) {
	struct Nson elem;

	nson_init_ptr(&elem, val);
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
	nson->type = NSON_ARR;
	if (nson_add_str(nson, key) < 0 || nson_add(nson, val) < 0) {
		nson->val.a.len -= nson->val.a.len % 2;
		return -1;
	}
	nson->type = NSON_OBJ;
	return 0;
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
nson_init_ptr(struct Nson *nson, const char *val) {
	int rv = nson_init(nson, NSON_STR);
	nson->val.c = val;

	return rv;
}

int
nson_init_str(struct Nson *nson, const char *val) {
	int rv = nson_init_ptr(nson, strdup(val));
	nson->alloc_type = NSON_ALLOC_BUFFER;

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
nson_mem_length(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	return nson->val.a.len;
}
