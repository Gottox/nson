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

union Nson;
struct NsonBuf;
struct NsonPointerRef;

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
 * @brief function pointer that is used to map a Nson element
 */
typedef int (*NsonFilter)(off_t index, union Nson *, void *);

/**
 * @brief type of an Nson Element
 */
enum NsonType {
	NSON_NIL,

	NSON_BOOL,
	NSON_INT,
	NSON_REAL,

	NSON_POINTER,

	NSON_ARR,
	NSON_OBJ,

	NSON_BLOB,
	NSON_STR,
};

enum NsonOptions {
	NSON_IS_KEY      = 1 << 1,
	NSON_SKIP_HEADER = 1 << 2,
};

/**
 * @brief Information that are common to all data types
 * */
typedef struct NsonCommon {
	enum NsonType type;
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
	union Nson *arr;
	size_t len;
} NsonArray ;

/**
 * @brief Data that are used to reference ranges of NSON fields.
 */
typedef struct NsonObject {
	struct NsonCommon c;
	struct NsonObjectEntry *arr;
	bool messy;
	size_t len;
} NsonObject ;

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
 * @brief Data that are used to store a single integer value.
 */
typedef struct NsonPointer {
	struct NsonCommon c;
	struct NsonPointerRef *ref;
} NsonPointer;


/**
 * @brief Data Container
 */
typedef union Nson {
	struct NsonInt i;
	struct NsonReal r;
	struct NsonData d;
	struct NsonArray a;
	struct NsonObject o;
	struct NsonPointer p;
	struct NsonCommon c;
} Nson;

/**
 * @brief Container for an Object Entry
 */
typedef struct NsonObjectEntry {
	union Nson key;
	union Nson value;
} NsonObjectEntry;

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
 * * For NSON_OBJ the function returns the number of key value
 *   pairs in the object.
 *
 * @return the number of child elements of @p nson
 */
size_t nson_data_len(const Nson *nson);

/**
 * @brief Alias for nson_data
 */
#define nson_str(n) nson_data(n)

/**
 * @brief Retrieve the binary / string data from a field of type
 * NSON_STR or NSON_BLOB
 * @return a pointer to the data referenced by @p nson
 */
const char *nson_data(const Nson *nson);

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
 * @brief
 * @return
 */
int nson_arr_sort(Nson *nson);

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
int nson_arr_insert(Nson *nson, const char *key,
		Nson* val);

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
int nson_int_wrap(Nson *nson, const int64_t val);

/**
 * @brief
 * @return
 */
int nson_bool_wrap(Nson *nson, const bool val);

/**
 * @brief
 * @return
 */
int nson_real_wrap(Nson *nson, const double val);

/**
 * @brief
 * @return
 */
int nson_load(NsonParser parser, Nson *nson, const char *file);

/* MAP */

/**
 * @brief
 */
typedef struct NsonThreadMapSettings {
	int threads;
	int chunk_size;
} NsonThreadMapSettings;

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
int nson_map_thread_ext(NsonThreadMapSettings *settings, Nson *nson, NsonMapper mapper, void *user_data);

/**
 * @brief
 * @return
 */
int nson_reduce(Nson *dest, const Nson *nson, NsonReducer reducer, const void *user_data);

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

int nson_init_arr(Nson *array);
size_t nson_arr_len(const Nson *array);
Nson * nson_arr_get(const Nson *array, off_t index);
int nson_arr_push(Nson *array, Nson *value);
int nson_arr_pop(Nson *last, Nson *array);
int nson_arr_concat(Nson *array_1, Nson *array_2);
Nson * nson_arr_last(Nson *array);
int nson_arr_push_int(Nson *array, int value);

Nson * nson_obj_get(Nson *object, const char *key);
int nson_obj_put(Nson *object, const char *key, Nson *value);
size_t nson_obj_size(const Nson *object);
const char *nson_obj_get_key(Nson *object, int index);
int nson_obj_from_arr(Nson *object);

int nson_json_serialize(char **str, size_t *size, Nson *nson,
		enum NsonOptions options);
int nson_json_write(FILE *out, const Nson *nson, enum NsonOptions options);

int nson_plist_serialize(char **str, size_t *size, Nson *nson,
		enum NsonOptions options);
int nson_plist_write(FILE *out, const Nson *nson, enum NsonOptions options);
int nson_ptr_wrap(Nson *nson, void *ptr, void (*dtor)(void *));
void *nson_ptr(Nson *nson);

#endif /* !NSON_H */
