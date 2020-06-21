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

#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

size_t
nson_data_len(const Nson *nson) {
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);

	return __nson_buf_siz(nson->d.buf);
}

const char *
nson_data(const Nson *nson) {
	assert(nson_type(nson) == NSON_STR || nson_type(nson) == NSON_BLOB);

	return __nson_buf_unwrap(nson->d.buf);
}

enum NsonType
nson_type(const Nson *nson) {
	return nson->c.type;
}

int
nson_move(Nson *nson, Nson *src) {
	memcpy(nson, src, sizeof(*src));
	memset(src, 0, sizeof(*src));
	return 0;
}

static int
nson_mapper_clone(off_t index, Nson *nson, void *user_data) {
	switch(nson_type(nson)) {
		case NSON_ARR:
			__nson_arr_clone(nson);
			return nson_map(nson, nson_mapper_clone, nson);
		case NSON_OBJ:
			__nson_obj_clone(nson);
			break;
		case NSON_STR:
		case NSON_BLOB:
			__nson_buf_retain(nson->d.buf);
			break;
		case NSON_POINTER:
			__nson_ptr_retain(nson->p.ref);
			break;
		case NSON_NIL:
		case NSON_BOOL:
		case NSON_INT:
		case NSON_REAL:
			// noop
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
nson_init(Nson *nson, const enum NsonType info) {
	assert(info != NSON_NIL);

	memset(nson, 0, sizeof(*nson));
	nson->c.type = info;

	return 0;
}

int
__nson_init_buf(Nson *nson, NsonBuf *val, enum NsonType info) {
	int rv = nson_init(nson, info);
	if(rv < 0) {
		return rv;
	}

	nson->d.buf = __nson_buf_retain(val);
	return rv;
}

int
nson_init_data(Nson *nson, const char *val, const size_t len, enum NsonType info) {
	int rv = nson_init(nson, info);
	if(rv < 0)
		return rv;

	nson->d.buf = __nson_buf_wrap(val, len);
	return rv;
}

int
nson_init_str(Nson *nson, const char *val) {
	return nson_init_data(nson, val, strlen(val), NSON_STR);
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

	if (munmap(mf, mapsize) < 0) {
		rv = -1;
	}

	return rv;
}
