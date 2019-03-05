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

#include <string.h>
#include <ctype.h>
#include <search.h>
#include <inttypes.h>
#include <assert.h>

static int
json_str_len(const char *src, const size_t len) {
	const char *chunk;

	for (chunk = src; (chunk = memchr(chunk, '"', chunk + len - src)); chunk++) {
		if (chunk[-1] != '\\' || chunk[-2] == '\\') {
			break;
		}
	}
	if (chunk == NULL) {
		return -1;
	}
	return chunk - src;
}

static int
parse_json_string(NsonBuf **dest_buf, const char *src, const size_t len) {
	const char *chunk_start, *chunk_end;
	size_t chunk_len;
	char *dest;
	uint64_t utf_val;

	(*dest_buf) = nson_buf_new(len);
	dest = nson_buf_unwrap(*dest_buf);

	for (chunk_start = src; (chunk_end = memchr(chunk_start, '\\', src + len - chunk_start));) {
		chunk_len = chunk_end - chunk_start;

		memcpy(dest, chunk_start, chunk_len);
		dest += chunk_len;
		chunk_start = chunk_end + 1;

		if (chunk_start[0] == 'u') {
			// TODO: correctly supply upper bounds.
			if (parse_hex(&utf_val, &chunk_start[1], 4) != 4) {
				break;
			}
			// TODO: correctly supply upper bounds.
			dest += to_utf8(dest, utf_val, 3);
			chunk_start += 5;
			continue;
		}

		switch(chunk_start[0]) {
		case 't':
			*dest = '\t';
			break;
		case 'n':
			*dest = '\n';
			break;
		case 'r':
			*dest = '\r';
			break;
		case '\\':
			*dest = '\\';
			break;
		default:
			*dest = chunk_start[0];
			break;
		}
		dest += 1;
		chunk_start += 1;
	}
	chunk_len = src + len - chunk_start;
	memcpy(dest, chunk_start, chunk_len);
	dest += chunk_len;

	nson_buf_shrink(*dest_buf, dest - nson_buf_unwrap(*dest_buf));

	return dest - nson_buf_unwrap(*dest_buf);
}

static int
json_escape_string(const Nson *nson, FILE *fd) {
	int rv = 0;
	off_t i = 0, last_write = 0;
	char c[] = { '\\', 0 };
	Nson tmp = { { { 0 } } };
	const char *data;
	size_t len;

	if (fputc('"', fd) < 0) {
		return -1;
	}

	nson_clone(&tmp, nson);

	data = nson_data(&tmp);
	len = nson_data_len(&tmp);

	for (; i < len; i++) {
		switch (data[i]) {
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

		if (c[1] != 0) {
			rv = -1;
			if (fwrite(&data[last_write], sizeof(*data), i - last_write, fd) == 0) {
				goto cleanup;
			}
			if (fwrite(c, sizeof(*data), 2, fd) == 0) {
				goto cleanup;
			}
			last_write = i+1;
			c[1] = 0;
		}
		else if (iscntrl(data[i])) {
			rv = -1;
			if (fwrite(&data[last_write], sizeof(*data), i - last_write, fd) == 0) {
				goto cleanup;
			}
			if (fprintf(fd, "\\u%04x", data[i]) == 0) {
				goto cleanup;
			}
			last_write = i+1;
		}
	}

	if (i != last_write &&
			fwrite(&data[last_write], sizeof(*data), i - last_write, fd) == 0) {
		return -1;
	}
	fputc('"', fd);

	rv = i;
cleanup:
	nson_clean(&tmp);
	return rv;
}

int
nson_load_json(Nson *nson, const char *file) {
	return nson_load(nson_parse_json, nson, file);
}

int
nson_parse_json(Nson *nson, const char *doc, size_t len) {
	int rv = 0;
	off_t row = 0;
	int i;
	int line_start = 0;
	Nson *stack_top;
	Nson old_top;
	Nson stack = { { { 0 } } }, tmp = { { { 0 } } };
	NsonBuf *buf;

	memset(nson, 0, sizeof(*nson));
	nson_init(&tmp, NSON_ARR);
	nson_init(&stack, NSON_ARR);
	nson_push(&stack, &tmp);

	/* UNUSED */
	(void)line_start;

	stack_top = nson_get(&stack, 0);
	// Skip leading Whitespaces
	for (i = 0; isspace(doc[i]); i++);
	do {
		switch(doc[i]) {
		case '[':
		case '{':
			nson_init(&tmp, doc[i] == '{' ? NSON_OBJ : NSON_ARR);
			tmp.a.messy = true;
			nson_push(&stack, &tmp);
			stack_top = nson_last(&stack);
			i++;
			break;
		case ',':
		case ':':
			i++;
			break;
		case ']':
		case '}':
			nson_pop(&old_top, &stack);
			stack_top = nson_last(&stack);
			nson_push(stack_top, &old_top);
			i++;
			break;
		case '"':
			i++; // Skip quote
			rv = json_str_len(&doc[i], len - i);
			if (rv < 0) {
				goto out;
			}
			if (doc[rv + i] != '"') {
				rv = -1;
				goto out;
			}
			parse_json_string(&buf, &doc[i], rv);
			nson_init_buf(&tmp, buf, NSON_STR);
			nson_push(stack_top, &tmp);
			nson_buf_release(buf);
			i += rv + 1; // Skip text + quote
			break;
		case '\n':
			line_start = i + 1;
			row++;
		case '\f':
		case '\r':
		case '\t':
		case '\v':
		case ' ':
			i++;
			break;
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			i += parse_number(&tmp, &doc[i], len - i);
			nson_push(stack_top, &tmp);
			break;
		case 'n':
			if (strncmp(&doc[i], "null", 4)) {
				rv = -1;
				goto out;
			}
			nson_init_data(&tmp, NULL, 0, NSON_STR);
			nson_push(stack_top, &tmp);
			i += 4;
			break;
		case 't':
			if (strncmp(&doc[i], "true", 4)) {
				rv = -1;
				goto out;
			}
			nson_init_bool(&tmp, 1);
			nson_push(stack_top, &tmp);
			i += 4;
			break;
		case 'f':
			if (strncmp(&doc[i], "false", 5)) {
				rv = -1;
				goto out;
			}
			nson_init_bool(&tmp, 0);
			nson_push(stack_top, &tmp);
			i += 5;
			break;
		default:
			rv = -1;
			goto out;
			nson_push(stack_top, &tmp);
		}
	} while (nson_len(&stack) > 1 && i < len);


	if (nson_len(&stack) != 1) {
		// Premature EOF
		rv = -1;
		goto out;
	}
	nson_move(nson, nson_get(stack_top, 0));

	rv = i;
out:
	nson_clean(&stack);
	return rv;
}

int
nson_to_json(Nson *nson, char **str) {
	int rv;
	size_t size = 0;
	FILE *fd = open_memstream(str, &size);
	if (fd == NULL) {
		return -1;
	}

	rv = nson_to_json_fd(nson, fd);
	fclose(fd);
	return rv;
}

static int
json_b64_enc(const Nson *nson, FILE* fd) {
	int rv = 0;
	Nson tmp;

	if (nson_clone(&tmp, nson)) {
		rv = -1;
	}
	if (nson_mapper_b64_enc(0, &tmp, NULL) < 0) {
		rv = -1;
	} else if (fputc('"', fd) < 0) {
		rv = -1;
	} else if (fwrite(nson_data(&tmp), sizeof(char), nson_data_len(&tmp), fd) == 0) {
		rv = -1;
	} else if (fputc('"', fd) < 0) {
		rv = -1;
	}
	nson_clean(&tmp);
	return rv;
}

int
nson_to_json_fd(Nson *nson, FILE* fd) {
	off_t i;
	NsonStack stack = { 0 };
	Nson *it;

	for (i = -1, it = nson; it; it = stack_walk(&stack, &nson, &i)) {
		switch(nson_type(it)) {
			case NSON_NONE:
				abort();
				break;
			case NSON_STR:
				json_escape_string(it, fd);
				break;
			case NSON_BLOB:
				json_b64_enc(it, fd);
				break;
			case NSON_REAL:
				fprintf(fd, "%f", nson_real(it));
				break;
			case NSON_INT:
				fprintf(fd, "%" PRId64, nson_int(it));
				break;
			case NSON_BOOL:
				fputs(nson_int(it) ? "true" : "false", fd);
				break;
			case NSON_ARR:
				fputc(i == -1 ? '[' : ']', fd);
				break;
			case NSON_OBJ:
				fputc(i == -1 ? '{' : '}', fd);
				break;
			default:
				break;
		}
		if (i == -1 || !nson || nson_mem_len(nson) == i + 1) {
		} else if (nson_type(nson) == NSON_OBJ) {
			fputc(i % 2 ? ',' : ':', fd);
		} else {
			fputc(',', fd);
		}
	}
	return 0;
}
