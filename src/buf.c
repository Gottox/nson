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

#include "internal.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

char *
__nson_buf(NsonBuf *buf) {
	return buf->buf;
}

NsonBuf *
__nson_buf_new(size_t siz) {
	if (siz > (SIZE_MAX - sizeof(NsonBuf)) / sizeof(char)) {
		errno = ENOMEM;
		return NULL;
	}
	NsonBuf *buf = malloc(sizeof(char) * siz + sizeof(NsonBuf));
	if (buf == NULL) {
		return NULL;
	}
	memset(buf, 0, sizeof(char) * siz + sizeof(NsonBuf));
	buf->siz = siz;
	return __nson_buf_retain(buf);
}

NsonBuf *
__nson_buf_wrap(const char *val, size_t siz) {
	NsonBuf *buf = __nson_buf_new(siz);
	if (buf == NULL) {
		return NULL;
	}
	memcpy(buf->buf, val, siz);
	return buf;
}

NsonBuf *
__nson_buf_wrap_0(const char *val) {
	return __nson_buf_wrap(val, strlen(val));
}

size_t
__nson_buf_siz(const NsonBuf *buf) {
	return buf->siz;
}

int
__nson_buf_shrink(NsonBuf *buf, size_t new_siz) {
	assert(new_siz <= buf->siz);

	buf->siz = new_siz;
	buf->buf[new_siz] = 0;
	return new_siz;
}

NsonBuf *
__nson_buf_retain(NsonBuf *buf) {
	buf->count++;
	return buf;
}

void
__nson_buf_release(NsonBuf *buf) {
	buf->count--;
	if (buf->count == 0) {
		free(buf);
	}
}

int
__nson_buf_cmp(const NsonBuf *a, const NsonBuf *b) {
	int rv;
	const int len_a = __nson_buf_siz(a);
	const int len_b = __nson_buf_siz(b);
	const int min_len = MIN(len_a, len_b);

	rv = memcmp(a->buf, b->buf, min_len);
	if (rv == 0 && len_a != len_b)
		rv = SCAL_CMP(len_a, len_b);
	return rv;
}
