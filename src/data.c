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
nson_cmp_data(const Nson *a, const Nson *b) {
	int rv;
	Nson tmp = { { { 0 } } };
	if(a->c.mapper) {
		nson_clone(&tmp, (const Nson *)a);
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
	case NSON_BLOB:
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

int
nson_mem_capacity(Nson *nson, const size_t size) {
	Nson *arr;
	const size_t old = nson_mem_len(nson);

	if(size == old)
		return size;

	arr = nson->val.a.arr;
	nson->val.a.len = size;

	arr = realloc(arr, sizeof(*arr) * size);
	if(!arr) {
		nson->val.a.len = old;
		return -1;
	}
	if(size > old)
		memset(&arr[old], 0, sizeof(*arr) * (size - old));

	nson->val.a.arr = arr;
	return size;
}

int
nson_clean(Nson *nson) {
	off_t i;
	int rv = 0;

	if (nson->c.alloc == NULL) {
	} else if (nson->c.alloc_size != 0) {
		if(munmap(nson->c.alloc, nson->c.alloc_size) < 0)
			return -1;
	} else {
		free(nson->c.alloc);
	}

	if(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ) {
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
nson_data_len(Nson *nson) {
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);

	if (nson->c.mapper)
		nson_data(nson);
	return nson->val.d.len;
}

size_t
nson_len(const Nson *nson) {
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ);

	if(nson_type(nson) == NSON_OBJ)
		return nson_mem_len(nson) / 2;
	else
		return nson_mem_len(nson);
}

const char *
nson_data(Nson *nson) {
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);
	NsonMapper mapper = nson->c.mapper;

	if(mapper) {
		nson->c.mapper = NULL;
		mapper(0, nson, NULL);
	}
	return nson->val.d.b;
}

enum NsonType
nson_type(const Nson *nson) {
	return nson->c.type;
}

int64_t
nson_int(const Nson *nson) {
	assert(nson_type(nson) == NSON_INT || nson_type(nson) == NSON_BOOL || nson_type(nson) == NSON_REAL);
	if(nson_type(nson) == NSON_REAL)
		return (int64_t)nson->val.r.r;
	return nson->val.i.i;
}

double
nson_real(const Nson *nson) {
	assert(nson_type(nson) == NSON_INT || nson_type(nson) == NSON_BOOL || nson_type(nson) == NSON_REAL);
	if(nson_type(nson) != NSON_REAL)
		return (double)nson->val.i.i;
	return nson->val.r.r;
}

Nson *
nson_get(const Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ
			|| nson_type(nson) == NSON_SLICE);
	if(nson_type(nson) == NSON_OBJ)
		index = index * 2 + 1;
	return nson_mem_get(nson, index);
}

const char *
nson_get_key(const Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_OBJ);
	index = index * 2;
	assert(nson->val.a.len > index);

	return nson_data(nson_mem_get(nson, index));
}

int
nson_push(Nson *nson, Nson *val) {
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ);
	assert(nson_type(val) != NSON_NONE);
	Nson *arr;
	size_t len = nson_mem_len(nson);

	nson_mem_capacity(nson, len+1);

	arr = nson_mem_get(nson, len);
	nson_move(arr, val);

	if(nson->val.a.messy || len <= 2)
		return 0;

	switch(nson_type(nson)) {
		case NSON_ARR:
			nson->val.a.messy = nson_cmp(&arr[-1], arr) > 0;
			break;
		case NSON_OBJ:
			assert(len % 2 == 1 || nson_type(arr) == NSON_STR);

			nson->val.a.messy = len % 2 == 0 && nson_cmp(&arr[-2], arr) > 0;
			break;
		default:
			break;
	}

	return 0;
}

Nson *
nson_last(Nson *nson) {
	if(nson->val.a.len == 0)
		return NULL;
	return &nson->val.a.arr[nson->val.a.len - 1];
}

int
nson_pop(Nson *dest, Nson *nson) {
	int rv = 0;
	if (nson_len(nson) == 0) {
		dest = NULL;
		return 0;
	}

	if(dest)
		rv = nson_move(dest, nson_last(nson));

	if(rv < 0) {
		return rv;
	}

	return nson->val.a.len--;
}

int
nson_move(Nson *nson, Nson *src) {
	memcpy(nson, src, sizeof(*src));
	memset(src, 0, sizeof(*src));
	return 0;
}

static int
nson_mapper_clone(off_t index, Nson *nson, void *user_data) {
	int rv;
	size_t len;
	Nson *arr;
	char *data;

	switch(nson_type(nson)) {
		case NSON_ARR:
		case NSON_OBJ:
			arr = nson->val.a.arr;
			len = nson_mem_len(nson);
			nson->val.a.arr = NULL;
			nson->val.a.len = 0;

			rv = nson_mem_capacity(nson, len);
			if (rv < 0)
				return rv;
			memcpy(nson->val.a.arr, arr, nson_mem_len(nson) * sizeof(*arr));
			return nson_map(nson, nson_mapper_clone, NULL);
		case NSON_STR:
		case NSON_BLOB:
			if(nson->c.mapper) {
				nson_data(nson);
				break;
			}
			len = nson_data_len(nson);
			data = calloc(len + 1, sizeof(char));

			memcpy(data, nson_data(nson), len * sizeof(char));
			nson->c.alloc = data;
			nson->c.alloc_size = 0;
			break;
		default:
			break;
	}
	return 0;
}

int
nson_clone(Nson *nson, const Nson *src) {
	memcpy(nson, src, sizeof(*src));
	return nson_mapper_clone(0, nson, NULL);
}

int
nson_push_all(Nson *nson, Nson *src) {
	if (nson_type(src) != NSON_ARR && nson_type(src) != NSON_OBJ)
		return nson_push(nson, src);

	const size_t src_len = nson_mem_len(src);
	const size_t nson_len = nson_mem_len(nson);

	nson_mem_capacity(nson, nson_len + src_len);

	memcpy(&nson->val.a.arr[nson_len], &src->val.a.arr, src_len);

	src->val.a.len = 0;
	nson_clean(src);

	return 0;
}

int
nson_push_str(Nson *nson, const char *val) {
	Nson elem;

	nson_init_str(&elem, val);
	return nson_push(nson, &elem);
}

int
nson_push_int(Nson *nson, int64_t val) {
	Nson elem;

	nson_init_int(&elem, val);
	return nson_push(nson, &elem);
}

int
nson_insert(Nson *nson, const char *key,
		Nson* val) {
	assert(nson_type(nson) == NSON_OBJ);

	if (nson_push_str(nson, key) < 0 || nson_push(nson, val) < 0) {
		nson->val.a.len -= nson->val.a.len % 2;
		return -1;
	}
	return 0;
}

int
nson_insert_str(Nson *nson, const char *key,
		const char *val) {
	Nson elem;

	nson_init_str(&elem, val);
	return nson_insert(nson, key, &elem);
}

int
nson_insert_int(Nson *nson, const char *key,
		int64_t val) {
	Nson elem;

	nson_init_int(&elem, val);
	return nson_insert(nson, key, &elem);
}

int
nson_init(Nson *nson, const enum NsonType info) {
	assert(info != NSON_NONE);

	memset(nson, 0, sizeof(*nson));
	nson->c.type = info;

	return 0;
}

int
nson_init_ptr(Nson *nson, const char *val, size_t len, enum NsonType info) {
	int rv = nson_init(nson, info);
	if(rv < 0)
		return rv;

	nson->val.d.b = val;
	nson->val.d.len = len;
	return rv;
}

int
nson_init_data(Nson *nson, char *val, size_t len, enum NsonType type) {
	assert(type);
	if(val == NULL)
		return nson_init_ptr(nson, NULL, 0, type);
	int rv = nson_init_ptr(nson, val, len, type);
	nson->c.alloc = val;
	nson->c.alloc_size = 0;
	return rv;
}

int
nson_init_str(Nson *nson, const char *val) {
	char *dup = strdup(val);
	if(dup == 0)
		return -1;

	int rv = nson_init_data(nson, dup, strlen(dup), NSON_STR);
	return rv;
}

int
nson_init_bool(Nson *nson, bool val) {
	int rv = nson_init(nson, NSON_BOOL);
	nson->val.i.i = val;

	return rv;
}

int
nson_init_int(Nson *nson, const int64_t val) {
	int rv = nson_init(nson, NSON_INT);
	nson->val.i.i = val;

	return rv;
}

int
nson_init_real(Nson *nson, const double val) {
	int rv = nson_init(nson, NSON_REAL);
	nson->val.r.r = val;

	return rv;
}

int
nson_load(NsonParser parser, Nson *nson, const char *file) {
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

	nson->c.alloc = mf;
	nson->c.alloc_size = mapsize;
	return rv;
}

Nson *
nson_mem_get(const Nson *nson, off_t index) {
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ
			|| nson_type(nson) == NSON_SLICE);
	assert(index < nson->val.a.len);

	return &nson->val.a.arr[index];
}

size_t
nson_mem_len(const Nson *nson) {
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ
			|| nson_type(nson) == NSON_SLICE);

	return nson->val.a.len;
}

int
nson_remove(Nson *nson, off_t index, size_t size) {
	Nson *elem;
	size_t len = nson_len(nson);
	enum NsonType type = nson_type(nson);
	assert(nson_type(nson) == NSON_ARR || nson_type(nson) == NSON_OBJ);
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
	nson_mem_capacity(nson, len - size);
	return 0;
}

Nson *
nson_walk(Nson *stack, Nson **nson, off_t *index) {
	Nson stack_item = { { { 0 } } };
	Nson *item = NULL;

	if (*nson == NULL) {
		return NULL;
	}

	enum NsonType type = nson_type(*nson);

	if (type != NSON_OBJ && type != NSON_ARR) {
		return NULL;
	}

	(*index)++;

	if (*index < nson_mem_len(*nson)) {
		item = nson_mem_get(*nson, *index);
		type = nson_type(item);
		// PUSH
		if (type == NSON_OBJ || type == NSON_ARR) {
			nson_init_ptr(&stack_item, (void *)*nson, *index, NSON_BLOB);
			nson_push(stack, &stack_item);
			*nson = item;
			*index = -1;
		}
	// POP
	} else if (nson_pop(&stack_item, stack)) {
		item = *nson;
		*nson = (void *)nson_data(&stack_item);
		*index = nson_data_len(&stack_item);
	// TERMINAL
	} else {
		item = *nson;
		*nson = NULL;
		*index = 0;
	}

	return item;
}

int
nson_slice(Nson *nson, Nson *src, off_t start, size_t len) {
	if(nson_len(src) < start + len)
		return -1;

	int rv = nson_init(nson, NSON_SLICE);
	if(rv < 0)
		return rv;
	nson->val.a.arr = nson_get(src, start);
	nson->val.a.len = len;
	return rv;
}
