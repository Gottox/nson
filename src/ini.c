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

#include <ctype.h>
#include <string.h>

static int
parse_line(Nson *nson, const char *line, size_t len) {
	off_t i = 0;
	const char *key, *val;
	size_t key_len = 0, val_len = 0;
	Nson elem;
	char s_key[64] = {0};

	for (; isblank(line[i]) && line[i]; i++)
		;

	if (line[i] == '#' || line[i] == '\0') {
		return 0;
	}
	key = &line[i];
	for (; !isblank(line[i]) && line[i]; key_len++, i++)
		;
	if (line[i] == '\0')
		return -1;
	i++;

	for (; isblank(line[i]) && line[i]; i++)
		;
	val = &line[i];
	val_len = len - i;

	// for(i = len - 1; isspace(line[i]) && i >= 0; i--, val_len--);

	memcpy(s_key, key, MIN(63, key_len));

	nson_init_data(&elem, val, val_len, NSON_STR);
	nson_obj_put(nson, s_key, &elem);

	return i;
}

int
nson_parse_ini(Nson *nson, const char *doc, size_t len) {
	int rv = 0, i;
	const char *p, *line;
	memset(nson, 0, sizeof(*nson));
	rv = nson_init(nson, NSON_OBJ);
	if (rv < 0)
		return rv;

	for (i = 0, p = line = doc; *p; line = ++p, i++) {
		for (; *p != '\n' && *p; p++)
			;
		rv = parse_line(nson, line, p - line);
	}

	return rv;
}

int
nson_load_ini(Nson *nson, const char *file) {
	return nson_load(nson_parse_ini, nson, file);
}
