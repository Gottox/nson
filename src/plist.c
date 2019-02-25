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

#include "config.h"
#include "nson.h"
#include "internal.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define SKIP_SPACES do { for(; *p && strchr("\n\f\r\t\v ", *p); p++); } while(0)

static char *
nson_memdup(const char *src, const int siz) {
	char *dup = malloc(siz);
	return memcpy(dup, src, siz);
}

static int
plist_mapper_string(off_t index, Nson *nson, void *user_data) {
	size_t len;
	const char *src, *chunk_start, *chunk_end;
	char *dest;
	int64_t val;
	assert(nson_type(nson) & NSON_STR);
	NsonBuf *dest_buf;

	len = nson_data_len(nson);
	src = nson_data(nson);
	dest_buf = nson_buf_new(len);
	dest = nson_buf_unwrap(dest_buf);

	for(chunk_start = src;(chunk_end = strchr(chunk_start, '&')); dest++) {
		memcpy(dest, chunk_start, chunk_end - chunk_start);
		dest += chunk_end - chunk_start;
		chunk_start = chunk_end + 1;

		if (chunk_start[0] == '#') {
			chunk_start = parse_dec(&val, &chunk_start[1], len - (chunk_start - src));
			printf("%li\n", val);
			// TODO: UTF8 escape codes
			if (chunk_start[0] == ';') {
				chunk_start++;
				*dest = val;
			}
			else {
				*dest = '&';
			}
		} else if(strncmp("lt;", chunk_start, 3) == 0) {
			chunk_start += 3;
			*dest = '<';
		} else if(strncmp("gt;", chunk_start, 3) == 0) {
			chunk_start += 3;
			*dest = '>';
		} else if(strncmp("amp;", chunk_start, 4) == 0) {
			chunk_start += 4;
			*dest = '&';
		} else {
			*dest = '&';
		}
	}
	memcpy(dest, chunk_start, src + len - chunk_start);

	nson_buf_release(nson->d.buf);
	nson->d.buf = dest_buf;
	nson_buf_retain(nson->d.buf);

	return 0;
}

int
nson_load_plist(Nson *nson, const char *file) {
	return nson_load(nson_parse_plist, nson, file);
}

inline static int
skip_tag(const char *tag, const char *p, const size_t len) {
	const char *begin = p;
	if(strncmp(p, tag, strlen(tag)) != 0)
		return 0;
	p += strlen(tag);

	if(*p == '>')
		return p - begin + 1;
	else if(*p != ' ')
		return 0;

	p = memchr(p, '>', len - (p - begin));
	if(!p)
		return -1;
	return p - begin + 1;
}

int
nson_parse_plist(Nson *nson, const char *doc, size_t len) {
	int rv = 0;
	int64_t i_val;
	const char *begin;
	const char *p = doc;
	static const char *string_tag = "string";
	char *buf;
	Nson old_top;
	Nson *stack_top;
	Nson stack = { { { 0 } } }, tmp = { { { 0 } } };


	rv = skip_tag("<?xml", p, len - (doc - p));
	if (rv <= 0)
		return -1;
	p += rv;
	SKIP_SPACES;

	rv = skip_tag("<!DOCTYPE", p, len - (doc - p));
	if (rv <= 0)
		return -1;
	p += rv;
	SKIP_SPACES;

	rv = skip_tag("<plist", p, len - (doc - p));
	if (rv <= 0)
		return -1;
	p += rv;

	memset(nson, 0, sizeof(*nson));
	nson_init(&tmp, NSON_ARR);
	nson_init(&stack, NSON_ARR);
	nson_push(&stack, &tmp);
	stack_top = nson_get(&stack, 0);

	do {
		SKIP_SPACES;
		if(*p != '<')
			goto err;
		p++;
		switch(*p) {
		case 'a':
			if((rv = skip_tag("array", p, len - (doc - p))) <= 0)
				break;
			nson_init(&tmp, NSON_ARR);
			tmp.a.messy = true;
			nson_push(&stack, &tmp);
			stack_top = nson_last(&stack);
			p += rv;
			break;
		case 'd':
			if((rv = skip_tag("dict", p, len - (doc - p))) > 0) {
				nson_init(&tmp, NSON_OBJ);
				tmp.a.messy = true;
				nson_push(&stack, &tmp);
				stack_top = nson_last(&stack);
				p += rv;
			} else if((rv = skip_tag("data", p, len - (doc - p))) > 0) {
				string_tag = "data";
				goto string;
			}
			break;
		case 'k':
			string_tag = "key";
		case 's':
			if((rv = skip_tag(string_tag, p, len - (doc - p))) <= 0)
				break;
string:
			p += rv;
			begin = p;
			for (rv = 0; rv == 0;) {
				if(!(p = memchr(p, '<', len - (doc - p))))
					goto err;
				p++;
				if(*p != '/')
					continue;
				p++;
				rv = skip_tag(string_tag, p, len - (doc - p));
			}
			if ( (buf = nson_memdup(begin, p - begin + 1)) == NULL) {
				rv = -1;
				goto err;
			}
			if (string_tag[0] == 'd') {
				nson_init_data(&tmp, buf, p - begin - 2, NSON_BLOB);
				//tmp.c.mapper = nson_mapper_b64_dec;
				nson_mapper_b64_dec(0, &tmp, NULL);
			} else {
				nson_init_data(&tmp, buf, p - begin - 2, NSON_STR);
				//tmp.c.mapper = plist_mapper_string;
				plist_mapper_string(0, &tmp, NULL);
			}
			nson_push(stack_top, &tmp);
			p += rv;
			string_tag = "string";
			break;
		case 'r':
			if((rv = skip_tag("real", p, len - (doc - p))) <= 0)
				break;
			p += rv;
			p = parse_number(&tmp, p, len - (doc - p));
			if(nson_type(&tmp) == NSON_INT)
				nson_init_real(&tmp, nson_real(&tmp));
			nson_push(stack_top, &tmp);
			if((rv = skip_tag("</real", p, len - (doc - p))) <= 0)
				break;
			p += rv;
			break;
		case 'i':
			if((rv = skip_tag("integer", p, len - (doc - p))) <= 0)
				break;
			p += rv;
			p = parse_dec(&i_val, p, len - (doc - p));
			nson_init_int(&tmp, i_val);
			nson_push(stack_top, &tmp);
			if((rv = skip_tag("</integer", p, len - (doc - p))) <= 0)
				break;
			p += rv;
			break;
		case 't':
			if((rv = skip_tag("true/", p, len - (doc - p))) <= 0)
				break;
			nson_init_bool(&tmp, 1);
			nson_push(stack_top, &tmp);
			p += rv;
			break;
		case 'f':
			if((rv = skip_tag("false/", p, len - (doc - p))) <= 0)
				break;
			nson_init_bool(&tmp, 0);
			nson_push(stack_top, &tmp);
			p += rv;
			break;
		case '/':
			p++;
			switch(*p) {
			case 'a':
				if((rv = skip_tag("array", p, len - (doc - p))) <= 0)
					break;
				if(nson_type(stack_top) != NSON_ARR)
					goto err;
				nson_pop(&old_top, &stack);
				stack_top = nson_last(&stack);
				nson_push(stack_top, &old_top);
				p += rv;
				break;
			case 'd':
				if((rv = skip_tag("dict", p, len - (doc - p))) <= 0)
					break;
				if(nson_type(stack_top) != NSON_OBJ)
					goto err;
				nson_pop(&old_top, &stack);
				stack_top = nson_last(&stack);
				nson_push(stack_top, &old_top);
				p += rv;
				break;
			}
		}
	} while(nson_len(&stack) > 1 && p - doc < len);
	if(nson_len(&stack) != 1) {
		// Premature EOF
		rv = -1;
		goto err;
	}
	SKIP_SPACES;

	rv = skip_tag("</plist", p, len - (doc - p));
	if (rv <= 0)
		return -1;
	p += rv;
	nson_move(nson, nson_get(stack_top, 0));

	rv = p - doc;

err:

	nson_clean(&stack);
	return rv;
}

static int
plist_escape(Nson *nson, FILE *fd) {
	off_t i = 0, last_write = 0;
	size_t len;
	char *escape = NULL;
	const char *str = nson_data(nson);

	if(str == NULL) {
		return 0;
	}

	len = nson_data_len(nson);
	for(; i < len; i++) {
		switch(str[i]) {
		case '<':
			escape = "&lt;";
			break;
		case '>':
			escape = "&gt;";
			break;
		case '&':
			escape = "&amp;";
			break;
		default:
			escape = NULL;
		}
		if(escape) {
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
				return -1;
			if(fputs(escape, fd) == 0)
				return -1;
			last_write = i+1;
		}
		else if(iscntrl(str[i])) {
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
				return -1;
			if(fprintf(fd, "&#%02x;", str[i]) == 0)
				return -1;
			last_write = i+1;
		}
	}

	if(i != last_write &&
			fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0)
		return -1;

	return i;
}

static int
plist_b64_enc(const Nson *nson, FILE* fd) {
	int rv = 0;
	Nson tmp;

	if(nson_clone(&tmp, nson))
		rv = -1;
	if(nson_mapper_b64_enc(0, &tmp, NULL) < 0)
		rv = -1;
	else if(fwrite(nson_data(&tmp), sizeof(char), nson_data_len(&tmp), fd) == 0)
		rv = -1;
	nson_clean(&tmp);
	return rv;
}

int
nson_to_plist_fd(Nson *nson, FILE *fd) {
	off_t i;
	Nson *it;
	NsonStack stack = { 0 };

	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
		"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
		"<plist version=\"1.0\">", fd);
	for(i = -1, it = nson; it; it = stack_walk(&stack, &nson, &i)) {
		switch(nson_type(it)) {
			case NSON_NONE:
				abort();
				break;
			case NSON_STR:
				if(nson && nson_type(nson) == NSON_OBJ && i % 2 == 0) {
					fputs("<key>", fd);
					plist_escape(it, fd);
					fputs("</key>", fd);
				} else {
					fputs("<string>", fd);
					plist_escape(it, fd);
					fputs("</string>", fd);
				}
				break;
			case NSON_BLOB:
				fputs("<data>", fd);
				plist_b64_enc(it, fd);
				fputs("</data>", fd);
				break;
			case NSON_REAL:
				fprintf(fd, "<real>%f</real>", nson_real(it));
				break;
			case NSON_INT:
				fprintf(fd, "<integer>%" PRId64 "</integer>", nson_int(it));
				break;
			case NSON_BOOL:
				fputs(nson_int(it) ? "<true/>" : "<false/>", fd);
				break;
			case NSON_ARR:
				fputs(i == -1 ? "<array>" : "</array>", fd);
				break;
			case NSON_OBJ:
				fputs(i == -1 ? "<dict>" : "</dict>", fd);
				break;
			default:
				break;
		}
	}
	fputs("</plist>",fd);
	return 0;
}

int
nson_to_plist(Nson *nson, char **str) {
	int rv;
	size_t size = 0;
	FILE *fd = open_memstream(str, &size);
	if(fd == NULL)
		return -1;

	rv = nson_to_plist_fd(nson, fd);
	fclose(fd);
	return rv;
}
