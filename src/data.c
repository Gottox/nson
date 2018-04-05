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
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

#include "config.h"
#include "nson.h"

#define SCAL_CMP(a, b) (a > b ? 1 : (a < b ? -1 : 0))

#define MIN(a, b) (a < b ? a : b)

static int
nson_cmp_data(const struct Nson *a, const struct Nson *b) {
	int rv;
	struct Nson tmp = { 0 };
	if(a->val.d.mapper) {
		nson_clone(&tmp, a);
		nson_data(&tmp);
		rv = nson_cmp_data(b, &tmp) * -1;
		nson_clean(&tmp);
		return rv;
	}

	return memcmp(a->val.d.b, b->val.d.b, MIN(a->val.d.len, b->val.d.len));
}

int
nson_cmp(const void *a, const void *b) {
	int rv;
	enum NsonType type = nson_type(a);

	// Reverse sort as NSON_NONE needs to be last
	if ((rv = nson_type(b) - type))
		return rv;
	switch(type) {
	case NSON_STR:
	case NSON_DATA:
		return nson_cmp_data(a, b);
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
nson_realloc(struct Nson *nson, size_t size) {
	struct Nson *arr;
	size_t old = nson_mem_len(nson);

	nson->val.a.len = size;

	if(old != size) {
		arr = nson->val.a.arr;
		arr = realloc(arr, sizeof(*arr) * size);
		if(!arr) {
			nson->val.a.len = old;
			return -1;
		}
		if (size > old)
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
nson_data_len(struct Nson *nson) {
	assert(nson_type(nson) & (NSON_DATA | NSON_STR));

	if (nson->val.d.mapper)
		nson_data(nson);
	return nson->val.d.len;
}

size_t
nson_len(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));

	if(nson_type(nson) == NSON_OBJ)
		return nson_mem_len(nson) / 2;
	else
		return nson_mem_len(nson);
}

const char *
nson_data(struct Nson *nson) {
	assert(nson_type(nson) & (NSON_DATA | NSON_STR));
	NsonMapper mapper = nson->val.d.mapper;

	if(mapper) {
		nson->val.d.mapper = NULL;
		mapper(0, nson);
	}
	return nson->val.d.b;
}

enum NsonType
nson_type(const struct Nson *nson) {
	return nson->type;
}

int64_t
nson_int(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_INT | NSON_BOOL | NSON_REAL));
	if(nson_type(nson) == NSON_REAL)
		return (int64_t)nson->val.r.r;
	return nson->val.i.i;
}

double
nson_real(const struct Nson *nson) {
	assert(nson_type(nson) & (NSON_INT | NSON_BOOL | NSON_REAL));
	if(nson_type(nson) != NSON_REAL)
		return (double)nson->val.i.i;
	return nson->val.r.r;
}

struct Nson *
nson_get(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	if(nson_type(nson) == NSON_OBJ)
		index = index * 2 + 1;
	return nson_mem_get(nson, index);
}

const char *
nson_get_key(const struct Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_OBJ);
	index = index * 2;
	assert(nson->val.a.len > index);

	return nson_data(nson_mem_get(nson, index));
}

int
nson_add(struct Nson *nson, struct Nson *val) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	struct Nson *arr;
	size_t len = nson_mem_len(nson);

	assert(!(nson_type(nson) == NSON_OBJ
			&& len % 2 == 0 && nson_type(val) != NSON_STR));

	nson_realloc(nson, len+1);

	arr = nson_mem_get(nson, 0);
	memcpy(&arr[len], val, sizeof(*arr));

	if(nson->val.a.messy || len == 0) {
	} else if(nson_type(nson) == NSON_ARR) {
		nson->val.a.messy = nson_cmp(&arr[len-1], &arr[len]) < 0;
	} else if(nson_type(nson) == NSON_OBJ && len % 2 == 0) {
		nson->val.a.messy = nson_cmp(&arr[len-2], &arr[len]) < 0;
	}
	len++;

	return 0;
}

static int
nson_mapper_clone(off_t index, struct Nson *nson) {
	int rv;
	size_t len;
	struct Nson *arr;
	char *data;

	nson->alloc_type = NSON_ALLOC_NONE;
	switch(nson_type(nson)) {
		case NSON_ARR:
		case NSON_OBJ:
			arr = nson->val.a.arr;
			nson->val.a.arr = NULL;
			len = nson_mem_len(nson);

			rv = nson_realloc(nson, nson_mem_len(nson));
			if (rv < 0)
				return rv;
			memcpy(nson->val.a.arr, arr, nson_mem_len(nson) * sizeof(*arr));
			return nson_map(nson, nson_mapper_clone);
		case NSON_STR:
		case NSON_DATA:
			if(nson->val.d.mapper) {
				nson_data(nson);
				break;
			}
			len = nson_data_len(nson);
			data = calloc(len, sizeof(char));

			memcpy(data, nson_data(nson), len * sizeof(char));
			nson->alloc_type = NSON_ALLOC_BUF;
			nson->alloc.b = data;
			break;
		default:
			break;
	}
	return 0;
}

int
nson_clone(struct Nson *nson, const struct Nson *src) {
	memcpy(nson, src, sizeof(*src));
	return nson_mapper_clone(0, nson);
}

int
nson_add_all(struct Nson *nson, struct Nson *suff) {
	assert(nson_type(nson) & (NSON_ARR | NSON_OBJ));
	assert(nson_type(suff) & (NSON_ARR | NSON_OBJ));

	const size_t suff_len = nson_mem_len(suff);
	const size_t nson_len = nson_mem_len(nson);

	nson_realloc(nson, nson_len + suff_len);

	memcpy(&nson->val.a.arr[nson_len], &suff->val.a.arr, suff_len);

	suff->val.a.len = 0;
	nson_clean(suff);

	return 0;
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
nson_init_data(struct Nson *nson, const char *val, size_t len,
		enum NsonEnc enc) {
	int rv = nson_init(nson, NSON_DATA);
	nson->val.d.b = val;
	nson->val.d.len = len;
	nson->val.d.enc = enc;

	return rv;
}

int
nson_init_str(struct Nson *nson, const char *val) {
	val = strdup(val);
	if(val == 0)
		return -1;

	int rv = nson_init_data(nson, val, strlen(val), NSON_UTF8);
	nson->type = NSON_DATA;
	nson->alloc_type = NSON_ALLOC_BUF;

	return rv;
}

int
nson_init_int(struct Nson *nson, const int64_t val) {
	int rv = nson_init(nson, NSON_INT);
	nson->val.i.i = val;

	return rv;
}

int
nson_init_real(struct Nson *nson, const double val) {
	int rv = nson_init(nson, NSON_REAL);
	nson->val.r.r = val;

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

	int rv = parser(nson, (char *)mf, st.st_size);

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
	nson_realloc(nson, len - size);
	return 0;
}
