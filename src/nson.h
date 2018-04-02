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

#ifndef NSON_H
#define NSON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define NSON(n, ...) nson_parse_json(n, strdup(#__VA_ARGS__))

enum NsonType {
	NSON_NONE = 0,

	NSON_ARR  = 1 << 0,
	NSON_OBJ  = 1 << 1,
	NSON_STR  = 1 << 2,
	NSON_BOOL = 1 << 3,
	NSON_INT  = 1 << 4,
	NSON_REAL = 1 << 5,

	NSON_ERR  = 1 << 6,
};

union NsonValue {
	int64_t i;
	double r;
	const char *c;
	struct {
		struct Nson *arr;
		size_t len;
		bool sorted;
	} a;
};

enum NsonAllocType {
	NSON_ALLOC_NONE,
	NSON_ALLOC_BUFFER,
	NSON_ALLOC_MMAP,
};

union NsonAlloc {
	struct {
		void* map;
		size_t len;
	} m;
	char *b;
};

struct Nson {
	enum NsonType type;
	union NsonValue val;
	enum NsonAllocType alloc_type;
	union NsonAlloc alloc;
};

typedef int (*NsonParser)(struct Nson *, char *);

/* DATA */

int nson_clean(struct Nson *nson);

size_t nson_length(const struct Nson *nson);

const char * nson_str(const struct Nson *nson);

enum NsonType nson_type(const struct Nson *nson);

int64_t nson_int(const struct Nson *nson);

double nson_real(const struct Nson *nson);

struct Nson * nson_get(const struct Nson *nson, off_t index);

struct Nson * nson_get_by_key(const struct Nson *nson, const char *key);

int nson_sort(struct Nson *nson);

const char * nson_get_key(const struct Nson *nson, off_t index);

int nson_add(struct Nson *nson, struct Nson *val);

int nson_add_ptr(struct Nson *nson, const char *val);

int nson_add_str(struct Nson *nson, const char *val);

int nson_add_int(struct Nson *nson, int64_t val);

int nson_insert(struct Nson *nson, const char *key,
		struct Nson* val);

int nson_insert_str(struct Nson *nson, const char *key,
		const char *val);

int nson_insert_int(struct Nson *nson, const char *key,
		int64_t val);

int nson_init(struct Nson *nson, const enum NsonType type);

int nson_init_ptr(struct Nson *nson, const char *val);

int nson_init_str(struct Nson *nson, const char *val);

int nson_init_int(struct Nson *nson, const int64_t val);

int nson_init_real(struct Nson *nson, const double val);

int nson_load(NsonParser parser, struct Nson *nson, const char *file);

struct Nson *nson_mem_get(const struct Nson *nson, off_t index);

size_t nson_mem_length(const struct Nson *nson);

/* JSON */
int nson_load_json(struct Nson *nson, const char *file);

int nson_parse_json(struct Nson *nson, char *doc);

int nson_to_json(const struct Nson *nson, char **str);

int nson_to_json_fd(const struct Nson *nson, FILE* fd);

/* INI */
int nson_parse_ini(struct Nson *nson, char *doc);

int nson_load_ini(struct Nson *nson, const char *file);

#endif /* !NSON_H */
