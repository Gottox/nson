/*
 * common.h
 * Copyright (C) 2019 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H

#define NSON_P(s) s, strlen(s)
#define INPUT_CHECK(name, parser, ...) static void \
name() { \
	const char static_input[] =  __VA_ARGS__; \
	size_t size = sizeof(static_input) - 1; \
	char *input = malloc(size); \
	memcpy(input, static_input, size); \
	\
	char *result = NULL; \
	Nson nson = { 0 }; \
	nson_parse_ ## parser (&nson, input, size); \
	nson_json_serialize(&result, &size, &nson, 0); \
	free(result); \
	nson_plist_serialize(&result, &size, &nson, 0); \
	free(result); \
	nson_clean(&nson); \
	free(input); \
}

#endif /* !COMMON_H */
