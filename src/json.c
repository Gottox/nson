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
#include <search.h>
#include <inttypes.h>

#include "nson.h"

#define SKIP_SPACES { for(; doc[i] && isspace(doc[i]); i++); }

static int
json_parse_object(struct Nson *nson, char *doc);

static int
json_parse_utf8(char *dest, const char *doc) {
	off_t i = 0;
	uint16_t chr = 0;

	if(doc[i] != 'u')
		return -1;
	i++;

	for (; i < 5; i++) {
		if(isdigit(doc[i]))
			chr = (16 * chr) + doc[i] - '0';
		else if(isxdigit(doc[i]))
			chr = (16 * chr) + tolower(doc[i]) - 'a' + 10;
	}

	if (chr < 0x0080) {
		*dest = chr;
		return 1;
	} else if (chr < 0x0800) {
		dest[0] = ((chr >> 6) & 0x1F) | 0xC0;
		dest[1] = (chr & 0x3F) | 0x80;
		return 2;
	} else {
		dest[0] = ((chr >> 12) & 0x0F) | 0xE0;
		dest[1] = ((chr >> 6) & 0x3F) | 0x80;
		dest[2] = (chr & 0x3F) | 0x80;
		return 3;
	}
}

static int
json_unescape(struct Nson* nson, char *doc) {
	off_t i = 0, j;
	int rv;
	const char *str;
	if(doc[i] != '"')
		return -1;
	i++;

	str = &doc[i];

	for(; doc[i] && doc[i] != '\\' && doc[i] != '"'; i++);

	for(j = i; doc[i] && doc[i] != '"'; i++, j++) {
		if(doc[i] != '\\') {
			doc[j] = doc[i];
			continue;
		}
		i++;
		switch(doc[i]) {
		case 't':
			doc[j] = '\t';
			break;
		case 'n':
			doc[j] = '\n';
			break;
		case 'r':
			doc[j] = '\r';
			break;
		case 'u':
			rv = json_parse_utf8(&doc[j], &doc[i]);
			if(rv < 0)
				return -1;
			j += rv - 1;
			i += 4;
			break;
		default:
			doc[j] = doc[i];
			break;
		}
	}
	if(doc[i] != '"')
		return -1;
	doc[j] = '\0';
	nson_init_ptr(nson, str, j - 1);
	nson->type = NSON_STR;
	return i + 1;
}

static int
json_parse_type(struct Nson *nson, char *doc) {
	int rv;
	int64_t val;
	double r_val;
	off_t i = 0;

	SKIP_SPACES;

	switch(doc[i]) {
	case '"':
		return json_unescape(nson, &doc[i]);
		break;
	case '{':
	case '[':
		return json_parse_object(nson, &doc[i]);
		break;
	default:
		if (strncmp("true", &doc[i], 4) == 0) {
			nson_init_int(nson, 1);
			return 4;
		} else if (strncmp("false", &doc[i], 5) == 0) {
			nson_init_int(nson, 0);
			return 5;
		} else {
			rv = -1;
			if(sscanf(&doc[i], "%" PRId64 "%n", &val, &rv) == 0 || rv < 0)
				return rv;
			if(doc[i + rv] != '.') {
				nson_init_int(nson, val);
				return i + rv;
			}
			rv = -1;
			if(sscanf(&doc[i], "%lf%n", &r_val, &rv) == 0 || rv < 0)
				return rv;
			nson_init_real(nson, r_val);
			return i + rv;
		}
	}
}

static int
json_parse_object(struct Nson *nson, char *doc) {
	struct Nson elem;
	char terminal = ']';
	const char *seperator = ",,";
	size_t len = 0;
	off_t i = 0;
	int rv;


	if(doc[i] == '{') {
		terminal = '}';
		seperator = ",:";
		nson_init(nson, NSON_OBJ);
	}
	else {
		nson_init(nson, NSON_ARR);
	}
	for(doc[i] = seperator[0]; doc[i] == seperator[len % 2]; len++) {
		i++;
		SKIP_SPACES;
		if(doc[i] == terminal)
			break;
		rv = json_parse_type(&elem, &doc[i]);
		if(rv < 0)
			return rv;
		i += rv;
		nson_add(nson, &elem);
		SKIP_SPACES;
	}

	if (doc[i] != terminal)
		return -1;

	return i+1;
}

static int
json_escape(const struct Nson *nson, FILE *fd) {
	off_t i = 0, last_write = 0;
	char c[] = { '\\', 0 };
	const char *str = nson_ptr(nson);

	if(str == NULL) {
		fputs("null", fd);
		return 4;
	}

	fputc('"', fd);
	for(; i < nson_len(nson); i++) {
		switch(str[i]) {
		case '\t':
			c[1] = 't';
			break;
		case '\n':
			c[1] = 'n';
			break;
		case '\r':
			c[1] = 'r';
			break;
		case '"':
			c[1] = '"';
			break;
		}
		if(c[1] != 0) {
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
				return -1;
			if(fwrite(c, sizeof(*str), 2, fd) == 0)
				return -1;
			last_write = i+1;
			c[1] = 0;
		}
		else if(iscntrl(str[i])) {
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
				return -1;
			if(fprintf(fd, "\\u%04x", str[i]) == 0)
				return -1;
			last_write = i+1;
		}
	}

	if(i != last_write &&
			fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
		return -1;
	fputc('"', fd);

	return i;
}

int
nson_load_json(struct Nson *nson, const char *file) {
	return nson_load(nson_parse_json, nson, file);
}

int
nson_parse_json(struct Nson *nson, char *doc) {
	memset(nson, 0, sizeof(*nson));
	nson->alloc_type = NSON_ALLOC_BUF;
	nson->alloc.b = doc;

	return json_parse_type(nson, doc);
}

int
nson_to_json(const struct Nson *nson, char **str) {
	int rv;
	size_t size = 0;
	FILE *fd = open_memstream(str, &size);
	if(fd == NULL)
		return -1;

	rv = nson_to_json_fd(nson, fd);
	fclose(fd);
	return rv;
}

int
nson_to_json_fd(const struct Nson *nson, FILE* fd) {
	char start = '[', *seperator = ",,", terminal = ']';
	off_t i;

	switch(nson_type(nson)) {
		case NSON_NONE:
			abort();
			break;
		case NSON_PTR:
		case NSON_STR:
			json_escape(nson, fd);
			break;
		case NSON_REAL:
			fprintf(fd, "%f", nson_real(nson));
			break;
		case NSON_INT:
			fprintf(fd, "%" PRId64, nson_int(nson));
			break;
		case NSON_BOOL:
			fputs(nson_int(nson) ? "true" : "false", fd);
			break;
		case NSON_ARR:
		case NSON_OBJ:
			if(nson_type(nson) == NSON_OBJ) {
				start = '{';
				seperator = ":,";
				terminal = '}';
			}

			for(i = 0; i < nson_mem_len(nson); i++) {
				if(terminal == '}' && i % 2 == 0 && nson_ptr(nson_mem_get(nson, i))[0] == '\x1b') {
					i+=2;
					continue;
				}
				fputc(start, fd);
				start = seperator[i % 2];
				nson_to_json_fd(nson_mem_get(nson, i), fd);
			}
			if(start == '[' || start == '{')
				fputc(start, fd);
			fputc(terminal, fd);
		default:
			break;
	}
	return 0;
}
