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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <ctype.h>

#include "config.h"
#include "nson.h"

static int
parse_line(struct Nson *nson, char *line) {
	off_t i = 0;
	char *key, *val;
	struct Nson elem;

	for(; isblank(line[i]) && line[i]; i++);

	if (line[i] == '#' || line[i] == '\0') {
		return 0;
	}
	key = &line[i];
	for(; !isblank(line[i]) && line[i]; i++);
	if(line[i] == '\0')
		return -1;
	line[i] = '\0';
	i++;

	for(; isblank(line[i]) && line[i]; i++);
	val = &line[i];

	for(i = strlen(line) - 1; isspace(line[i]) && i >= 0; i--)
		line[i] = '\0';

	nson_init_data(&elem, key, strlen(key), NSON_UTF8);
	elem.type = NSON_STR;
	nson_add(nson, &elem);
	nson_init_data(&elem, val, strlen(key), NSON_UTF8);
	elem.type = NSON_STR;
	nson_add(nson, &elem);

	return i+1;
}

int
nson_parse_ini(struct Nson *nson, char *doc, size_t len) {
	int rv = 0, i;
	char *p, *line;
	memset(nson, 0, sizeof(*nson));
	nson_init(nson, NSON_OBJ);
	nson->alloc_type = NSON_ALLOC_BUF;
	nson->alloc.b = doc;

	for(i = 0, p = line = doc; *p; line = ++p, i++) {
		for(; *p != '\n' && *p; p++);
		*p = '\0';
		rv = parse_line(nson, line);
	}

	return rv;
}

int
nson_load_ini(struct Nson *nson, const char *file) {
	return nson_load(nson_parse_ini, nson, file);
}
