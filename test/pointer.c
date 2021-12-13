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

#include "common.h"
#include "test.h"

#include "../src/internal.h"

static int dtor_was_called = 0;

static void
dtor(void *unused) {
	dtor_was_called = 1;
}

static void
check_pointer_create() {
	int pointer_target = 0;
	Nson nson;
	nson_ptr_wrap(&nson, &pointer_target, dtor);
	assert(nson_ptr(&nson) == &pointer_target);
	nson_clean(&nson);
}

static void
check_pointer_clone() {
	int pointer_target = 0;
	Nson nson, clone;
	nson_ptr_wrap(&nson, &pointer_target, dtor);
	nson_clone(&clone, &nson);

	assert(nson.p.ref->count == 2);
	assert(nson_ptr(&nson) == &pointer_target);
	nson_clean(&nson);
	nson_clean(&clone);
}

static void
check_pointer_free_dtor() {
	Nson nson;
	char *pointer_target = strdup("foobar");

	nson_ptr_wrap(&nson, pointer_target, free);
	assert(nson_ptr(&nson) == pointer_target);
	nson_clean(&nson);
}

DEFINE
TEST(check_pointer_create);
TEST(check_pointer_clone);
TEST(check_pointer_free_dtor);
DEFINE_END
