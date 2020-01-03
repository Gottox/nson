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
	char *result = NULL; \
	const char input[] =  __VA_ARGS__; \
	Nson nson; \
	nson_parse_ ## parser (&nson, input, sizeof(input)); \
	nson_to_ ## parser(&nson, &result); \
	nson_clean(&nson); \
	free(result); \
}

#endif /* !COMMON_H */
