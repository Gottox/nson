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

#include "internal.h"
#include "nson.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define SKIP_SPACES do { for(; i < len && doc[i] && strchr("\n\f\r\t\v ", doc[i]); i++); } while(0)

static int
parse_string(NsonBuf **dest_buf, const char *src, const size_t len) {
	const char *chunk_start, *chunk_end;
	size_t chunk_len;
	char *dest;
	int64_t val;

	(*dest_buf) = __nson_buf_new(len);
	dest = __nson_buf_unwrap(*dest_buf);

	for(chunk_start = src;
			(chunk_end = memchr(chunk_start, '&', len - (chunk_start - src)));
			dest++) {
		chunk_len = chunk_end - chunk_start;

		memcpy(dest, chunk_start, chunk_len);
		dest += chunk_len;
		chunk_start = chunk_end + 1;

		if (chunk_start[0] == '#') {
			chunk_start++;
			chunk_start += __nson_parse_dev(&val, chunk_start, len - (chunk_start - src));
			if (chunk_start[0] == ';') {
				chunk_start += __nson_to_utf8(dest, val, 3);
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
	chunk_len = src + len - chunk_start;
	memcpy(dest, chunk_start, chunk_len);
	dest += chunk_len;
	__nson_buf_shrink(*dest_buf, dest - __nson_buf_unwrap(*dest_buf));

	return dest - __nson_buf_unwrap(*dest_buf);
}

int
nson_load_plist(Nson *nson, const char *file) {
	return nson_load(nson_parse_plist, nson, file);
}

inline static int
skip_tag(const char *tag, const char *p, const size_t len) {
	const char *begin = p;
	const int tag_len = strlen(tag);
	const int cmp_siz = MIN(len, tag_len);
	if (len <= tag_len)
		return 0;
	if (strncmp(p, tag, cmp_siz) != 0)
		return 0;
	p += tag_len;

	if(*p == '>')
		return p - begin + 1;
	else if(*p != ' ')
		return 0;

	p = memchr(p, '>', len - (p - begin));
	if(!p)
		return -1;
	return p - begin + 1;
}

static off_t
measure_string_len(const char *str, const char *end_tag, size_t len) {
	int rv;
	const char *p = str;

	for (rv = 0; rv == 0;) {
		if(!(p = memchr(p, '<', len - (p - str)))) {
			return -1;
		}
		p++;
		if ((p - str) >= len){
			return -1;
		}
		if(*p != '/') {
			continue;
		}
		p++;
		rv = skip_tag(end_tag, p, len - (p - str));
		if (rv < 0)
			return -1;
	}

	return p - str - 2;
}

int
nson_parse_plist(Nson *nson, const char *doc, size_t len) {
	int rv = 0;
	off_t i = 0;
	int64_t i_val;
	off_t str_len;
	static const char *string_tag = "string";
	NsonBuf *buf;
	Nson old_top;
	Nson *stack_top;
	Nson stack = { { { 0 } } }, tmp = { { { 0 } } };


	rv = skip_tag("<?xml", &doc[i], len - i);
	if (rv <= 0) {
		return -1;
	}
	i += rv;
	SKIP_SPACES;

	rv = skip_tag("<!DOCTYPE", &doc[i], len - i);
	if (rv <= 0) {
		return -1;
	}
	i += rv;
	SKIP_SPACES;

	rv = skip_tag("<plist", &doc[i], len - i);
	if (rv <= 0) {
		return -1;
	}
	i += rv;

	memset(nson, 0, sizeof(*nson));
	nson_init(&tmp, NSON_ARR);
	nson_init(&stack, NSON_ARR);
	nson_arr_push(&stack, &tmp);
	stack_top = nson_arr_get(&stack, 0);

	do {
		SKIP_SPACES;
		if (i + 1 >= len || doc[i] != '<') {
			goto err;
		}
		i++;
		switch(doc[i]) {
		case 'a':
			if((rv = skip_tag("array", &doc[i], len - i)) <= 0) {
				break;
			}
			nson_init(&tmp, NSON_ARR);
			nson_arr_push(&stack, &tmp);
			stack_top = nson_arr_last(&stack);
			i += rv;
			break;
		case 'd':
			if((rv = skip_tag("dict", &doc[i], len - i)) > 0) {
				nson_init(&tmp, NSON_ARR);
				nson_arr_push(&stack, &tmp);
				stack_top = nson_arr_last(&stack);
				i += rv;
			} else if((rv = skip_tag("data", &doc[i], len - i)) > 0) {
				i += rv;
				rv = measure_string_len(&doc[i], "data", len - i);
				if (rv < 0) {
					goto err;
				}
				str_len = rv;
				rv = __nson_parse_b64(&buf, &doc[i], str_len);
				if (rv < 0) {
					goto err;
				}
				__nson_init_buf(&tmp, buf, NSON_BLOB);
				__nson_buf_release(buf);
				nson_arr_push(stack_top, &tmp);
				i += str_len + 2;
				rv = skip_tag("data", &doc[i], len - i);
				i += rv;
			}
			break;
		case 'k':
			string_tag = "key";
		case 's':
			if((rv = skip_tag(string_tag, &doc[i], len - i)) <= 0) {
				break;
			}
			i += rv;
			rv = measure_string_len(&doc[i], string_tag, len - i);
			if (rv < 0) {
				goto err;
			}
			str_len = rv;
			parse_string(&buf, &doc[i], str_len);
			__nson_init_buf(&tmp, buf, NSON_STR);
			__nson_buf_release(buf);
			nson_arr_push(stack_top, &tmp);
			i += str_len + 2;
			rv = skip_tag(string_tag, &doc[i], len - i);
			i += rv;
			string_tag = "string";
			break;
		case 'r':
			if((rv = skip_tag("real", &doc[i], len - i)) <= 0) {
				break;
			}
			i += rv;
			rv = __nson_parse_number(&tmp, &doc[i], len - i);
			if(rv <= 0) {
				break;
			}
			i += rv;
			if(nson_type(&tmp) == NSON_INT) {
				nson_real_wrap(&tmp, nson_real(&tmp));
			}
			nson_arr_push(stack_top, &tmp);
			if((rv = skip_tag("</real", &doc[i], len - i)) <= 0) {
				break;
			}
			i += rv;
			break;
		case 'i':
			if((rv = skip_tag("integer", &doc[i], len - i)) <= 0) {
				break;
			}
			i += rv;
			rv = __nson_parse_dev(&i_val, &doc[i], len - i);
			if(rv <= 0) {
				break;
			}
			i += rv;
			nson_int_wrap(&tmp, i_val);
			nson_arr_push(stack_top, &tmp);
			if((rv = skip_tag("</integer", &doc[i], len - i)) <= 0) {
				break;
			}
			i += rv;
			break;
		case 't':
			if((rv = skip_tag("true/", &doc[i], len - i)) <= 0) {
				break;
			}
			nson_bool_wrap(&tmp, 1);
			nson_arr_push(stack_top, &tmp);
			i += rv;
			break;
		case 'f':
			if((rv = skip_tag("false/", &doc[i], len - i)) <= 0) {
				break;
			}
			nson_bool_wrap(&tmp, 0);
			nson_arr_push(stack_top, &tmp);
			i += rv;
			break;
		case '/':
			i++;
			if (i >= len) {
				goto err;
			}
			switch(doc[i]) {
			case 'a':
				if((rv = skip_tag("array", &doc[i], len - i)) <= 0) {
					break;
				}
				if(nson_type(stack_top) != NSON_ARR) {
					goto err;
				}
				if(nson_arr_len(&stack) == 0) {
					goto err;
				}
				nson_arr_pop(&old_top, &stack);
				stack_top = nson_arr_last(&stack);
				if(stack_top == NULL) {
					goto err;
				}
				nson_arr_push(stack_top, &old_top);
				i += rv;
				break;
			case 'd':
				// TODO
				nson_obj_from_arr(stack_top);
				if((rv = skip_tag("dict", &doc[i], len - i)) <= 0) {
					break;
				}
				if(nson_type(stack_top) != NSON_OBJ) {
					goto err;
				}
				if(nson_arr_len(&stack) == 0) {
					goto err;
				}
				nson_arr_pop(&old_top, &stack);
				stack_top = nson_arr_last(&stack);
				if(stack_top == NULL) {
					goto err;
				}
				nson_arr_push(stack_top, &old_top);
				i += rv;
				break;
			}
		}
	} while(nson_arr_len(&stack) > 1 && i < len);
	if(nson_arr_len(&stack) != 1) {
		// Premature EOF
		rv = -1;
		goto err;
	}
	SKIP_SPACES;

	rv = skip_tag("</plist", &doc[i], len - i);
	if (rv <= 0) {
		goto err;
	}
	i += rv;

	if (nson_arr_len(stack_top) == 0) {
		rv = -1;
		goto err;
	}
	nson_move(nson, nson_arr_get(stack_top, 0));

	rv = i;

err:

	nson_clean(&stack);
	return rv;
}

static int
plist_escape(const Nson *nson, FILE *fd) {
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
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0) {
				return -1;
			}
			if(fputs(escape, fd) == 0) {
				return -1;
			}
			last_write = i+1;
		}
		else if(iscntrl(str[i])) {
			if(fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0) {
				return -1;
			}
			if(fprintf(fd, "&#%02x;", str[i]) == 0) {
				return -1;
			}
			last_write = i+1;
		}
	}

	if(i != last_write &&
			fwrite(&str[last_write], sizeof(*str), i - last_write, fd) == 0) {
		return -1;
	}

	return i;
}

static int
plist_b64_enc(const Nson *nson, FILE* fd) {
	int rv = 0;
	Nson tmp;

	if(nson_clone(&tmp, nson)) {
		rv = -1;
	}
	if(nson_mapper_b64_enc(0, &tmp, NULL) < 0) {
		rv = -1;
	} else if(fwrite(nson_data(&tmp), sizeof(char), nson_data_len(&tmp), fd) == 0) {
		rv = -1;
	}
	nson_clean(&tmp);
	return rv;
}

int
nson_plist_serialize(char **str, size_t *size, Nson *nson, enum NsonOptions options) {
	int rv;
	FILE *out = open_memstream(str, size);
	if (out == NULL) {
		return -1;
	}
	rv = nson_plist_write(out, nson, options);
	fclose(out);
	return rv;
}

int
nson_plist_write(FILE *out, const Nson *nson, enum NsonOptions options) {
	int rv = 0;
	static const NsonSerializerInfo info = {
		.serializer = nson_plist_write,
		.seperator = "",
		.key_value_seperator = "",
	};

	if (0 == (options & NSON_SKIP_HEADER)) {
		fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
				"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
				"<plist version=\"1.0\">", out);
	}
	switch(nson_type(nson)) {
		case NSON_NIL:
			return -1;
			break;
		case NSON_STR:
			if(options & NSON_IS_KEY) {
				fputs("<key>", out);
				plist_escape(nson, out);
				fputs("</key>", out);
			} else {
				fputs("<string>", out);
				plist_escape(nson, out);
				fputs("</string>", out);
			}
			break;
		case NSON_BLOB:
			fputs("<data>", out);
			plist_b64_enc(nson, out);
			fputs("</data>", out);
			break;
		case NSON_REAL:
			fprintf(out, "<real>%f</real>", nson_real(nson));
			break;
		case NSON_INT:
			fprintf(out, "<integer>%" PRId64 "</integer>", nson_int(nson));
			break;
		case NSON_BOOL:
			fputs(nson_int(nson) ? "<true/>" : "<false/>", out);
			break;
		case NSON_ARR:
			fputs("<array>", out);
			rv = __nson_arr_serialize(out, nson, &info, options);
			fputs("</array>", out);
			break;
		case NSON_OBJ:
			fputs("<dict>", out);
			rv = __nson_obj_serialize(out, nson, &info, options);
			fputs("</dict>", out);
			break;
		default:
			break;
	}
	if (0 == (options & NSON_SKIP_HEADER)) {
		fputs("</plist>", out);
	}
	return rv;
}
