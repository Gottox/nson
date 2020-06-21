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

#ifndef NSON_INTERNAL_H
#define NSON_INTERNAL_H

#include "nson.h"

#define SCAL_CMP(a, b) (a > b ? 1 : (a < b ? -1 : 0))

#define MIN(a, b) (a < b ? a : b)

typedef struct NsonBuf {
	unsigned int count;
	size_t siz;
	/* ISO C forbids zero-size array. So use 1 here and use the additional byte
	 * for zero termination. */
	char buf[1];
} NsonBuf;

typedef struct NsonPointerRef {
	unsigned int count;
	void (*dtor)(void *);
	void *ptr;
} NsonPointerRef;

typedef struct NsonStackElement {
	Nson *element;
	off_t index;
} NsonStackElement;

typedef struct NsonStack {
	NsonStackElement *arr;
	size_t len;
} NsonStack;

typedef struct NsonSerializerInfo {
	int (*serializer)(FILE *out, const Nson *object, enum NsonOptions options);
	char *seperator;

	char *key_value_seperator;
} NsonSerializerInfo;

Nson *
stack_walk(NsonStack *stack, Nson **nson, off_t *index);

int stack_push(NsonStack *stack, Nson *element, off_t index);

int stack_pop(NsonStack *stack, Nson **element, off_t *index);

void stack_clean(NsonStack *stack);

off_t __nson_parse_dev(int64_t *i, const char *p, size_t len);

off_t __nson_parse_hex(uint64_t *dest, const char *src, size_t len);

off_t __nson_parse_b64(NsonBuf **dest_buf, const char *src, const size_t len);

off_t __nson_parse_number(Nson *nson, const char *p, size_t len);

off_t __nson_to_utf8(char *dest, const uint64_t chr, const size_t len);

char *__nson_buf_unwrap(NsonBuf *buf);

size_t __nson_buf_siz(const NsonBuf *buf);

NsonBuf *__nson_buf_new(size_t len);

NsonBuf *__nson_buf_wrap(const char *val, size_t len);

NsonBuf *__nson_buf_wrap_0(const char *val);

NsonBuf *__nson_buf_retain(NsonBuf *buf);

int __nson_buf_shrink(NsonBuf *buf, size_t new_siz);

void __nson_buf_release(NsonBuf *buf);

int __nson_buf_cmp(const NsonBuf *a, const NsonBuf *b);

int __nson_init_buf(Nson *nson, NsonBuf *val, enum NsonType info);

int __nson_arr_clone(Nson *array);

int __nson_obj_clone(Nson *object);

int __nson_obj_serialize(FILE *out, const Nson *object,
		const NsonSerializerInfo *info, enum NsonOptions options);

int __nson_arr_serialize(FILE *out, const Nson *array,
		const NsonSerializerInfo *info, enum NsonOptions options);

int __nson_arr_clean(Nson *nson);

int __nson_obj_clean(Nson *nson);

NsonPointerRef *__nson_ptr_retain(NsonPointerRef *ref);

void __nson_ptr_release(NsonPointerRef *ptr);
#endif /* !INTERNAL_H */
