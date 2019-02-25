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

char *
nson_buf_unwrap(NsonBuf *buf) {
	return buf->buf;
}

NsonBuf *
nson_buf_new(size_t siz) {
	NsonBuf *buf = calloc(1, sizeof(char *) * siz + sizeof(NsonBuf));
	if (buf == NULL) {
		return NULL;
	}
	buf->siz = siz;
	nson_buf_retain(buf);

	return buf;
}

NsonBuf *
nson_buf_wrap(const char *val, size_t siz) {
	NsonBuf *buf = nson_buf_new(siz);
	if (buf == NULL) {
		return NULL;
	}
	memcpy(buf->buf, val, siz);
	return buf;
}

size_t
nson_buf_siz(const NsonBuf *buf) {
	return buf->siz;
}

NsonBuf *
nson_buf_wrap_0(const char *val) {
	return nson_buf_wrap(val, strlen(val));
}

NsonBuf *
nson_buf_retain(NsonBuf *buf) {
	buf->ref_count++;
	return buf;
}

int
nson_buf_shrink(NsonBuf *buf, size_t new_siz) {
	if (new_siz > buf->siz) {
		abort();
	}
	buf->siz = new_siz;
	buf->buf[new_siz] = 0;
	return new_siz;
}

void
nson_buf_release(NsonBuf *buf) {
	buf->ref_count--;
	if (buf->ref_count == 0) {
		free(buf);
	}
}
