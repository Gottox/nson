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
	const size_t in_size = sizeof(static_input) - 1; \
	size_t size = 0; \
	char *input = NULL; \
	if (in_size) { \
		input = malloc(in_size); \
		memcpy(input, static_input, in_size); \
	} \
	\
	char *result = NULL; \
	Nson nson = { 0 }; \
	nson_parse_ ## parser (&nson, input, size); \
	nson_json_serialize(&result, &size, &nson, 0); \
	free(result); \
	nson_plist_serialize(&result, &size, &nson, 0); \
	free(result); \
	nson_clean(&nson); \
	if (in_size) { \
		free(input); \
	} \
}

#endif /* !COMMON_H */
