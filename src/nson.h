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

/**
 * @brief Alias for nson_data
 */
#define nson_str(n) nson_data(n)

struct Nson;

/**
 * @brief function pointer that is used to parse a buffer
 */
typedef int (*NsonParser)(struct Nson *, char *, size_t);

/**
 * @brief function pointer that is used to map a Nson element
 */
typedef int (*NsonMapper)(off_t index, struct Nson *);

/**
 * @brief function pointer that is used to filter a Nson element
 */
typedef int (*NsonFilter)(const struct Nson *);

/**
 * @brief type of an Nson Element
 */
enum NsonType {
	NSON_NONE = 0,

	NSON_ARR  = 1 << 0,
	NSON_OBJ  = 1 << 1,
	NSON_DATA = 1 << 2,
	NSON_STR  = 1 << 3,
	NSON_BOOL = 1 << 4,
	NSON_INT  = 1 << 5,
	NSON_REAL = 1 << 6,

	NSON_ERR  = 1 << 7,
};

enum NsonEnc {
	NSON_PLAIN,
	NSON_UTF8,
	NSON_BASE64
};

/**
 * @brief union holding the value of an Nson Element
 */
union NsonValue {
	int64_t i;
	double r;

	struct {
		const char *b;
		size_t len;
		enum NsonEnc enc;
		NsonMapper mapper;
	} d;

	struct {
		struct Nson *arr;
		size_t len;
		bool messy;
	} a;
};

/**
 * @brief allocation type of an Nson Element
 *
 * this type describes how nson should clean up the resources
 * belonging to a Nson Element
 */
enum NsonAllocType {
	NSON_ALLOC_NONE,
	NSON_ALLOC_BUF,
	NSON_ALLOC_MMAP,
	NSON_ALLOC_REF,
};

/**
 * @brief allocation information of an Nson Element
 */
union NsonAlloc {
	struct {
		void* map;
		size_t len;
	} m;
	char *b;
};

/**
 * @brief Data Container
 */
struct Nson {
	enum NsonType type;
	union NsonValue val;
	enum NsonAllocType alloc_type;
	union NsonAlloc alloc;
};

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
int nson_clean(struct Nson *nson);

/**
 * @brief returns the number of child elements of @p nson
 *
 * * For NSON_ARR the function returns the number of elements
 *   in the array.
 *
 * @return the number of child elements of @p nson
 */
size_t nson_len(const struct Nson *nson);

/**
 * @brief returns the number of child elements of @p nson
 *
 * * For NSON_OBJ the function returns the number of key value
 *   pairs in the object.
 *
 * @return the number of child elements of @p nson
 */
size_t nson_data_len(struct Nson *nson);

/**
 * @brief
 * @return
 */
const char *nson_data(struct Nson *nson);

/**
 * @brief
 * @return
 */
int nson_cmp(const void *a, const void *b);

/**
 * @brief
 * @return
 */
enum NsonType nson_type(const struct Nson *nson);

/**
 * @brief
 * @return
 */
int64_t nson_int(const struct Nson *nson);

/**
 * @brief
 * @return
 */
double nson_real(const struct Nson *nson);

/**
 * @brief
 * @return
 */
struct Nson * nson_get(const struct Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
struct Nson * nson_get_by_key(const struct Nson *nson, const char *key);

/**
 * @brief
 * @return
 */
int nson_sort(struct Nson *nson);

/**
 * @brief
 * @return
 */
const char * nson_get_key(const struct Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
int nson_add(struct Nson *nson, struct Nson *val);

/**
 * @brief
 * @return
 */
int nson_clone(struct Nson *nson, const struct Nson *src);

/**
 * @brief
 * @return
 */
int nson_add_all(struct Nson *nson, struct Nson *suff);

/**
 * @brief
 * @return
 */
int nson_add_str(struct Nson *nson, const char *val);

/**
 * @brief
 * @return
 */
int nson_add_int(struct Nson *nson, int64_t val);

/**
 * @brief
 * @return
 */
int nson_insert(struct Nson *nson, const char *key,
		struct Nson* val);

/**
 * @brief
 * @return
 */
int nson_insert_int(struct Nson *nson, const char *key,
		int64_t val);

/**
 * @brief
 * @return
 */
int nson_init(struct Nson *nson, const enum NsonType type);

/**
 * @brief
 * @return
 */
int nson_init_data(struct Nson *nson, const char *val, size_t len,
		enum NsonEnc enc);

/**
 * @brief
 * @return
 */
int nson_init_str(struct Nson *nson, const char *val);

/**
 * @brief
 * @return
 */
int nson_init_int(struct Nson *nson, const int64_t val);

/**
 * @brief
 * @return
 */
int nson_init_real(struct Nson *nson, const double val);

/**
 * @brief
 * @return
 */
int nson_load(NsonParser parser, struct Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
struct Nson *nson_mem_get(const struct Nson *nson, off_t index);

/**
 * @brief
 * @return
 */
size_t nson_mem_len(const struct Nson *nson);

/**
 * @brief
 * @return
 */
int nson_filter(struct Nson *nson, NsonFilter mapper);

/* MAP */

/**
 * @brief
 * @return
 */
int nson_map(struct Nson *nson, NsonMapper mapper);

/**
 * @brief
 * @return
 */
int nson_remove(struct Nson *nson, off_t index, size_t size);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_enc(off_t index, struct Nson *nson);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_dec(off_t index, struct Nson *nson);

/* JSON */

/**
 * @brief
 * @return
 */
int nson_load_json(struct Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
int nson_parse_json(struct Nson *nson, char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_to_json(const struct Nson *nson, char **str);

/**
 * @brief
 * @return
 */
int nson_to_json_fd(const struct Nson *nson, FILE* fd);

/* INI */

/**
 * @brief
 * @return
 */
int nson_parse_ini(struct Nson *nson, char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_load_ini(struct Nson *nson, const char *file);

/* PLIST */

/**
 * @brief
 * @return
 */
int nson_load_plist(struct Nson *nson, const char *file);

/**
 * @brief
 * @return
 */
int nson_parse_plist(struct Nson *nson, char *doc, size_t len);

/**
 * @brief
 * @return
 */
int nson_to_plist(struct Nson *nson, char **str);

/**
 * @brief
 * @return
 */
int nson_to_plist_fd(struct Nson *nson, FILE* fd);


/* POOL */

struct NsonPool {
	int worker;
};

/**
 * @brief
 * @return
 */
int nson_pool_init(struct NsonPool *pool);

/**
 * @brief
 * @return
 */
int nson_pool_filter(struct Nson* nson, struct NsonPool *pool,
		NsonFilter filter);

/**
 * @brief
 * @return
 */
int nson_pool_map(struct Nson* nson, struct NsonPool *pool, NsonMapper mapper);

#endif /* !NSON_H */
