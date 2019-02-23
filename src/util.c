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
parse_dec(int64_t *i, const char *p, size_t len) {
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
parse_hex(uint64_t *i, const char *p, size_t len) {
	int64_t val;

	for(val = 0; ; p++) {
		switch(*p) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (16 * val) + *p - '0';
			break;
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			val = (16 * val) + *p - 'a' + 10;
			break;
		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			val = (16 * val) + *p - 'A' + 10;
			break;
		default:
			break;
		}
	}

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

	p = parse_dec(&i_val, p, len);
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

size_t
to_utf8(char *dest, const uint64_t chr, const size_t len) {
	if (chr < 0x0080 && len <= 1) {
		*dest = chr;
		return 1;
	} else if (chr < 0x0800 && len <= 2) {
		dest[0] = ((chr >> 6) & 0x1F) | 0xC0;
		dest[1] = (chr & 0x3F) | 0x80;
		return 2;
	} else if (len <= 3){
		dest[0] = ((chr >> 12) & 0x0F) | 0xE0;
		dest[1] = ((chr >> 6) & 0x3F) | 0x80;
		dest[2] = (chr & 0x3F) | 0x80;
		return 3;
	}

	return 0;
}

#endif /* !UTIL_H */
