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
#include "nson.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

static void
noop_dtor(void *ptr) {}

int
nson_ptr_wrap(Nson *nson, void *ptr, void (*dtor)(void *)) {
	int rv = nson_init(nson, NSON_POINTER);
	if (rv < 0) {
		return rv;
	}

	nson->p.ref = calloc(1, sizeof *nson->p.ref);
	if (NULL == nson->p.ref) {
		return -1;
	}
	nson->p.ref->dtor = dtor ? dtor : noop_dtor;
	nson->p.ref->ptr = ptr;
	nson->p.ref->count = 1;

	return rv;
}

void *
nson_ptr(const Nson *nson) {
	return nson->p.ref->ptr;
}

NsonPointerRef *
__nson_ptr_retain(NsonPointerRef *ref) {
	ref->count++;
	return ref;
}

void
__nson_ptr_release(NsonPointerRef *ref) {
	ref->count--;
	if (ref->count == 0) {
		ref->dtor(ref->ptr);
		free(ref);
	}
}
