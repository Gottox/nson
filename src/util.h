/*
 * util.h
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef NSON_UTIL_H
#define NSON_UTIL_H

#include <math.h>

static inline const char *
parse_int(int64_t *i, const char *p, size_t len) {
	int64_t i_val;
	int sign = *p == '-' ? -1 : 1;

	if(*p == '-' || *p == '+')
		p++;

	for(i_val = 0; *p >= '0' && *p <= '9'; p++)
		i_val += (i_val * 10) + *p - '0';
	i_val *= sign;

	*i = i_val;

	return p;
}
static inline const char *
parse_number(double *r, int64_t *i, const char *p, size_t len) {
	int sign = *p == '-' ? -1 : 1;
	int64_t nbr, upper;
	double lower;

	p = parse_int(&upper, p, len);
	*r = upper;
	if (i)
		*i = upper;
	if (*p != '.' || r == NULL) {
		if(i)
			*r = NAN;
		return p;
	}

	p++;
	if(*p == '-' || *p == '+')
		return p;

	p = parse_int(&nbr, p, len);
	for(lower = nbr; lower > 1.; lower /= 10);
	lower *= sign;

	*r = upper + lower;

	return p;
}

#endif /* !UTIL_H */
