/*
 * util.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "internal.h"
#include <string.h>

off_t
parse_dec(int64_t *i, const char *src, size_t len) {
	int64_t val;
	const char *p = src;
	int sign = *p == '-' ? -1 : 1;

	if(*p == '-' || *p == '+')
		p++;

	for(val = 0; *p >= '0' && *p <= '9'; p++)
		val = (val * 10) + *p - '0';
	val *= sign;

	*i = val;

	return p - src;
}

off_t
parse_hex(uint64_t *dest, const char *src, size_t len) {
	int64_t val = 0;
	size_t i;

	for(i = 0; i < len; i++) {
		switch(src[i]) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (16 * val) + src[i] - '0';
			break;
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			val = (16 * val) + src[i] - 'a' + 10;
			break;
		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			val = (16 * val) + src[i] - 'A' + 10;
			break;
		default:
			goto out;
		}
	}

out:
	*dest = val;
	return i;
}

off_t
parse_number(Nson *nson, const char *src, size_t len) {
	const char *p = src;
	int64_t i_val;
	double r_val;

	p += parse_dec(&i_val, p, len);
	if (*p != '.') {
		nson_init_int(nson, i_val);
		return p - src;
	}

	p++;
	for(r_val = 0; *p >= '0' && *p <= '9'; p++)
		r_val = (r_val * 0.1) + *p - '0';
	r_val *= 0.1;

	if(i_val >= 0)
		r_val += i_val;
	else
		r_val -= i_val;

	nson_init_real(nson, r_val);

	return p - src;
}

size_t
to_utf8(char *dest, const uint64_t chr, const size_t len) {
	if (chr < 0x0080 && len >= 1) {
		*dest = chr;
		return 1;
	} else if (chr < 0x0800 && len >= 2) {
		dest[0] = ((chr >> 6) & 0x1F) | 0xC0;
		dest[1] = (chr & 0x3F) | 0x80;
		return 2;
	} else if (len >= 3){
		dest[0] = ((chr >> 12) & 0x0F) | 0xE0;
		dest[1] = ((chr >> 6) & 0x3F) | 0x80;
		dest[2] = (chr & 0x3F) | 0x80;
		return 3;
	}

	return 0;
}
