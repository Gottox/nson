/*
 * internal.h
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef NSON_INTERNAL_H
#define NSON_INTERNAL_H

#include "nson.h"

typedef struct NsonBuf {
	unsigned int ref_count;
	size_t len;
	/* ISO C forbids zero-size array. So use 1 here and use the additional byte
	 * for zero termination. */
	char buf[1];
} NsonBuf;

typedef struct NsonStackElement {
	Nson *element;
	off_t index;
} NsonStackElement;

typedef struct NsonStack {
	NsonStackElement *arr;
	size_t len;
} NsonStack;

Nson *
stack_walk(NsonStack *stack, Nson **nson, off_t *index);

int stack_push(NsonStack *stack, Nson *element, off_t index);

int stack_pop(NsonStack *stack, Nson **element, off_t *index);

void stack_clean(NsonStack *stack);

off_t parse_dec(int64_t *i, const char *p, size_t len);

off_t parse_hex(uint64_t *dest, const char *src, size_t len);

off_t parse_number(Nson *nson, const char *p, size_t len);

off_t to_utf8(char *dest, const uint64_t chr, const size_t len);

char *nson_buf_unwrap(NsonBuf *buf);

size_t nson_buf_siz(const NsonBuf *buf);

NsonBuf *nson_buf_new(size_t len);

NsonBuf *nson_buf_wrap(const char *val, size_t len);

NsonBuf *nson_buf_wrap_0(const char *val);

NsonBuf *nson_buf_retain(NsonBuf *buf);

void nson_buf_release(NsonBuf *buf);

#endif /* !INTERNAL_H */
