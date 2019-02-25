/*
 * buf.c
 * Copyright (C) 2019 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
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
