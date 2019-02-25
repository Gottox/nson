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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define NSON(n, ...) nson_parse_json(n, #__VA_ARGS__, strlen(#__VA_ARGS__))

#define NSON_P(s) s, strlen(s)

union Nson;
struct NsonBuf;

/**
 * @brief function pointer that is used to parse a buffer
 */
typedef int (*NsonParser)(union Nson *, const char *, size_t);

/**
 * @brief function pointer that is used to map a Nson element
 */
typedef int (*NsonReducer)(off_t index, union Nson *, const union Nson *, const void *);

/**
 * @brief function pointer that is used to map a Nson element
 */
typedef int (*NsonMapper)(off_t index, union Nson *, void *);

/**
 * @brief type of an Nson Element
 */
enum NsonType {
	NSON_NONE,

	NSON_BOOL,
	NSON_INT,
	NSON_REAL,

	NSON_ARR,
	NSON_OBJ,

	NSON_BLOB,
	NSON_STR,
};

/**
 * @brief Information that are common to all data types
 * */
typedef struct NsonCommon {
	enum NsonType type;
	union Nson *parent;
} NsonCommon;

/**
 * @brief fields that are used to save arbitrary binary or
 * string data.
 */
typedef struct NsonData {
	struct NsonCommon c;
	struct NsonBuf *buf;
} NsonData;

/**
 * @brief Data that are used to reference ranges of NSON fields.
 */
typedef struct NsonArray {
	struct NsonCommon c;
	bool messy;
	union Nson *arr;
	size_t len;
} NsonArray ;

/**
 * @brief Data that are used to store a single floating point value.
 */
typedef struct NsonReal {
	struct NsonCommon c;
	double r;
} NsonReal;

/**
 * @brief Data that are used to store a single integer value.
 */
typedef struct NsonInt {
	struct NsonCommon c;
	int64_t i;
} NsonInt;

/**
 * @brief Data Container
 */
typedef union Nson {
	struct NsonInt i;
	struct NsonReal r;
	struct NsonData d;
	struct NsonArray a;
	struct NsonCommon c;
} Nson;

/* DATA */

/**
 * @brief invalidates @p nson
 *
 * frees all resources belonging to @p nson and zeros
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
 * @brief Retrieve the binary / string data from a field of type
 * NSON_STR or NSON_BLOB
 * @return a pointer to the data referenced by @p nson
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
enum NsonType nson_type(const Nson *nson);

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
 * @brief returns the value at a given index.
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
 * @brief appends @p suff to @p nson.
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
int nson_init(Nson *nson, const enum NsonType info);

/**
 * @brief
 * @return
 */
int nson_init_data(Nson *nson, const char *val, const size_t len,
		const enum NsonType type);

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

/* MAP */

/**
 * @brief
 * @return
 */
int nson_map(Nson *nson, NsonMapper mapper, void *user_data);

/**
 * @brief
 * @return
 */
int nson_map_thread(Nson *nson, NsonMapper mapper, void *user_data);

/**
 * @brief
 * @return
 */
int nson_reduce(Nson *dest, const Nson *nson, NsonReducer reducer, const void *user_data);

/**
 * @brief
 * @return
 */
int nson_remove(Nson *nson, off_t index, size_t size);

/**
 * @brief
 * @return
 */
int nson_mem_capacity(Nson *nson, const size_t size);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_enc(off_t index, Nson *nson, void *user_data);

/**
 * @brief
 * @return
 */
int nson_mapper_b64_dec(off_t index, Nson *nson, void *user_data);

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
int nson_to_json(Nson *nson, char **str);

/**
 * @brief
 * @return
 */
int nson_to_json_fd(Nson *nson, FILE* fd);

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

#endif /* !NSON_H */
