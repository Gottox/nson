/*
 * util.h
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef NSON_UTIL_H
#define NSON_UTIL_H

#include "nson.h"

const char *parse_int(int64_t *i, const char *p, size_t len);

const char *parse_real(double *r, const char *p, size_t len);

const char *parse_number(Nson *nson, const char *p, size_t len);

char *nson_memdup(const char *src, const int siz);

#endif /* !UTIL_H */
