/*
 * util.h
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef NSON_UTIL_H
#define NSON_UTIL_H

#include "nson.h"
#include <string.h>

const char *
parse_int(int64_t *i, const char *p, size_t len) {
	int64_t val;
	int sign = *p == '-' ? -1 : 1;

	if(*p == '-' || *p == '+')
		p++;

	for(val = 0; *p >= '0' && *p <= '9'; p++)
		val = (val * 10) + *p - '0';
	val *= sign;

	*i = val;

	return p;
}

const char *
parse_real(double *r, const char *p, size_t len) {
	double val;

	for(val = 0; *p >= '0' && *p <= '9'; p++)
		val = (val * 0.1) + *p - '0';

	*r = val * 0.1;

	return p;
}

const char *
parse_number(Nson *nson, const char *p, size_t len) {
	int64_t i_val;
	double r_val;

	p = parse_int(&i_val, p, len);
	if (*p != '.') {
		nson_init_int(nson, i_val);
		return p;
	}

	p++;
	p = parse_real(&r_val, p, len);

	if(i_val >= 0)
		r_val += i_val;
	else
		r_val -= i_val;

	nson_init_real(nson, r_val);

	return p;
}

char *
nson_memdup(const char *src, const int siz) {
	char *dup = malloc(siz);
	return memcpy(dup, src, siz);
}

#endif /* !UTIL_H */
