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

/**
 * @file nson.h
 * @author Enno Boland <mail@eboland.de>
 * @date 3 Apr 2018
 * @brief File Containing all public symbols of JSON
 *
 * @see https://github.com/Gottox/nson
 */

#ifndef NSON_H
#define NSON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define NSON(n, ...) nson_parse_json(n, strdup(#__VA_ARGS__), strlen(#__VA_ARGS__))

#define NSON_P(s) strdup(s), strlen(s)

union Nson;

/**
 * @brief function pointer that is used to parse a buffer
 */
typedef int (*NsonParser)(union Nson *, const char *, size_t);

/**
 * @brief function pointer that is used to map a Nson element
 */
typedef int (*NsonMapper)(off_t index, union Nson *);

/**
 * @brief function pointer that is used to filter a Nson element
 */
typedef int (*NsonFilter)(const union Nson *);

/**
 * @brief type of an Nson Element
 */
enum NsonInfo {
	NSON_NONE = 0,

	NSON_MALLOC  = 1 << 0,
	NSON_MMAP    = 2 << 0,
	NSON_ALLOC   = NSON_MALLOC | NSON_MMAP,

	NSON_BOOL = 1 << 2,
	NSON_INT  = 2 << 2,
	NSON_REAL = 3 << 2,
	NSON_PRIM = NSON_BOOL | NSON_INT | NSON_REAL,

	NSON_ARR  = 1 << 4,
	NSON_OBJ  = 2 << 4,
	NSON_STOR = NSON_ARR | NSON_OBJ,

	NSON_BLOB = 1 << 6,
	NSON_STR  = 2 << 6,
	NSON_DATA  = NSON_BLOB | NSON_STR,

	NSON_TYPE = NSON_PRIM | NSON_STOR | NSON_DATA
};

typedef struct NsonCommon {
	enum NsonInfo info;
	NsonMapper mapper;
	void *alloc;
	size_t alloc_size;
} NsonCommon;

typedef struct NsonData {
	NsonCommon n;
	const char *b;
	size_t len;
} NsonData;

typedef struct NsonArray {
	NsonCommon n;
	union Nson *arr;
	size_t len;
	bool messy;
} NsonArray ;

typedef struct NsonReal {
	NsonCommon n;
	double r;
} NsonReal;

typedef struct NsonInt {
	NsonCommon n;
	int64_t i;
} NsonInt;

/**
 * @brief Data Container
 */
typedef union Nson {
	struct NsonCommon c;
	struct NsonInt i;
	struct NsonReal r;
	struct NsonData d;
	struct NsonArray a;
} Nson;

/* DATA */

/**
 * @brief invalidates @p nson
 *
 * frees all resources belonging to @p nson and NULLs
 * @p nson. Does not free @p nson itself.
 *
 * @p nson can be resident on the stack and on the heap.
 *
 * @return 0 on success, < 0 on error
 */
int nson_clean(Nson *nson);

/**
 * @brief returns the number of child elements of @p nson
 *
 * * For NSON_ARR the function returns the number of elements
 *   in the array.
 *
 * @return the number of child elements of @p nson
 */
size_t nson_len(const Nson *nson);

/**
 * @brief returns the number of child elements of @p nson
 *
 * * For NSON_OBJ the function returns the number of key value
 *   pairs in the object.
 *
 * @return the number of child elements of @p nson
 */
size_t nson_data_len(Nson *nson);

/**
 * @brief Alias for nson_data
 */
#define nson_str(n) nson_data(n)

/**
 * @brief
 * @return
 */
const char *nson_data(Nson *nson);

/**
 * @brief Compares two Nson Objects.
 *
 * @return > 0 if @p a is greater, < 0 if @p b is greater, 0 on equality
 */
int nson_cmp(const void *a, const void *b);

/**
 * @brief returns the type of an object
 *
 * @return the type of @p nson
 */
enum NsonInfo nson_type(const Nson *nson);

/**
 * @brief returns the integer value of an object
 *
 * if the object isn't a primitive value, 0
 * is returned.
 *
 * real and booleans are casted to int.
 *
 * @return the integer value of @p nson
 */
int64_t nson_int(const Nson *nson);

#define nson_bool(x) nson_int(x)

/**
 * @brief returns the real value of an object
 *
 * if the object isn't a primitive value, 0
 * is returned.
 *
 * integer and booleans are casted to int.
 *
 * @return the integer value of @p nson
 */
double nson_real(const Nson *nson);

/**
 * @brief
 * @return
 */
Nson *nson_get(const Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
Nson *nson_get_by_key(const Nson *nson, const char *key);

/**
 * @brief
 * @return
 */
int nson_sort(Nson *nson);

/**
 * @brief
 * @return
 */
const char * nson_get_key(const Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
int nson_push(Nson *nson, Nson *val);

/**
 * @brief
 * @return
 */
Nson *nson_last(Nson *nson);

/**
 * @brief
 * @return
 */
int nson_pop(Nson *dest, Nson *nson);

/**
 * @brief Moves the value of @p src into @p nson
 *
 * @warning if @p src is an array element, the array
 * will contain an invalidated object
 *
 * invalidates src.
 *
 * @return 0
 */
int nson_move(Nson *nson, Nson *src);

/**
 * @brief clones the value and all children of @p src into @p nson
 *
 * @return 0 on success, < 0 on failure
 */
int nson_clone(Nson *nson, const Nson *src);

/**
 * @brief
 * @return
 */
int nson_push_all(Nson *nson, Nson *suff);

/**
 * @brief
 * @return
 */
int nson_push_str(Nson *nson, const char *val);

/**
 * @brief
 * @return
 */
int nson_push_int(Nson *nson, int64_t val);

/**
 * @brief
 * @return
 */
int nson_insert(Nson *nson, const char *key,
		Nson* val);

/**
 * @brief
 * @return
 */
int nson_insert_int(Nson *nson, const char *key,
		int64_t val);

/**
 * @brief
 * @return
 */
int nson_init(Nson *nson, const enum NsonInfo info);

/**
 * @brief
 * @return
 */
int nson_init_ptr(Nson *nson, const char *val, size_t len,
		const enum NsonInfo info);

/**
 * @brief
 * @return
 */
int nson_init_data(Nson *nson, char *val, size_t len,
		const enum NsonInfo type);

/**
 * @brief
 * @return
 */
int nson_init_str(Nson *nson, const char *val);

/**
 * @brief
 * @return
 */
int nson_init_int(Nson *nson, const int64_t val);

/**
 * @brief
 * @return
 */
int nson_init_bool(Nson *nson, const bool val);

/**
 * @brief
 * @return
 */
int nson_init_real(Nson *nson, const double val);

/**
 * @brief
 * @return
 */
int nson_load(NsonParser parser, Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
Nson *nson_mem_get(const Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
size_t nson_mem_len(const Nson *nson);

/**
 * @brief
 * @return
 */
int nson_filter(Nson *nson, NsonFilter mapper);

/* MAP */

/**
 * @brief
 * @return
 */
int nson_map(Nson *nson, NsonMapper mapper);

/**
 * @brief
 * @return
 */
int nson_remove(Nson *nson, off_t index, size_t size);

/**
 * @brief
 * @return
 */
int nson_mem_capacity(Nson *nson, size_t size);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_enc(off_t index, Nson *nson);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_dec(off_t index, Nson *nson);

/* JSON */

/**
 * @brief
 * @return
 */
int nson_load_json(Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
int nson_parse_json(Nson *nson, const char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_to_json(const Nson *nson, char **str);

/**
 * @brief
 * @return
 */
int nson_to_json_fd(const Nson *nson, FILE* fd);

/* INI */

/**
 * @brief
 * @return
 */
int nson_parse_ini(Nson *nson, const char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_load_ini(Nson *nson, const char *file);

/* PLIST */

/**
 * @brief
 * @return
 */
int nson_load_plist(Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
int nson_parse_plist(Nson *nson, const char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_to_plist(Nson *nson, char **str);

/**
 * @brief
 * @return
 */
int nson_to_plist_fd(Nson *nson, FILE* fd);


/* POOL */

typedef struct NsonPool {
	int worker;
} NsonPool;

/**
 * @brief
 * @return
 */
int nson_pool_init(NsonPool *pool);

/**
 * @brief
 * @return
 */
int nson_pool_filter(Nson* nson, NsonPool *pool,
		NsonFilter filter);

/**
 * @brief
 * @return
 */
int nson_pool_map(Nson* nson, NsonPool *pool, NsonMapper mapper);

#endif /* !NSON_H */
