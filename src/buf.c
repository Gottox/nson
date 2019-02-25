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
nson_buf_new(size_t len) {
	NsonBuf *buf = calloc(1, sizeof(char *) * len + sizeof(NsonBuf));
	if (buf == NULL) {
		return NULL;
	}
	buf->len = len;
	nson_buf_retain(buf);

	return buf;
}

NsonBuf *
nson_buf_wrap(const char *val, size_t len) {
	NsonBuf *buf = nson_buf_new(len);
	if (buf == NULL) {
		return NULL;
	}
	memcpy(buf->buf, val, len);
	return buf;
}

size_t
nson_buf_siz(const NsonBuf *buf) {
	return buf->len;
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

void
nson_buf_release(NsonBuf *buf) {
	buf->ref_count--;
	if (buf->ref_count == 0) {
		free(buf);
	}
}
