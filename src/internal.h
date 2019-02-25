/*
 * internal.h
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef NSON_INTERNAL_H
#define NSON_INTERNAL_H

#include "nson.h"

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

const char *parse_real(double *r, const char *p, size_t len);

const char *parse_number(Nson *nson, const char *p, size_t len);

size_t to_utf8(char *dest, const uint64_t chr, const size_t len);

#endif /* !INTERNAL_H */
