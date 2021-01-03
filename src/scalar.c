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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "internal.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int64_t
nson_int(const Nson *nson) {
	assert(nson_type(nson) == NSON_INT || nson_type(nson) == NSON_BOOL ||
		   nson_type(nson) == NSON_REAL);
	if (nson_type(nson) == NSON_REAL)
		return (int64_t)nson->r.r;
	return nson->i.i;
}

double
nson_real(const Nson *nson) {
	assert(nson_type(nson) == NSON_INT || nson_type(nson) == NSON_BOOL ||
		   nson_type(nson) == NSON_REAL);
	if (nson_type(nson) != NSON_REAL)
		return (double)nson->i.i;
	return nson->r.r;
}

int
nson_bool_wrap(Nson *nson, bool val) {
	int rv = nson_init(nson, NSON_BOOL);
	nson->i.i = val;

	return rv;
}

int
nson_int_wrap(Nson *nson, const int64_t val) {
	int rv = nson_init(nson, NSON_INT);
	nson->i.i = val;

	return rv;
}

int
nson_real_wrap(Nson *nson, const double val) {
	int rv = nson_init(nson, NSON_REAL);
	nson->r.r = val;

	return rv;
}
