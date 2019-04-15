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

static const char base64_table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

off_t
parse_dec(int64_t *dest, const char *src, size_t len) {
	off_t i = 0;
	int64_t val;
	if (len < 1) {
		return -1;
	}
	int sign = src[i] == '-' ? -1 : 1;

	if (src[i] == '-' || src[i] == '+') {
		i++;
	}

	for (val = 0; i < len && src[i] >= '0' && src[i] <= '9'; i++) {
		val = (val * 10) + src[i] - '0';
	}
	val *= sign;

	*dest = val;

	return i;
}

off_t
parse_hex(uint64_t *dest, const char *src, size_t len) {
	int64_t val = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		switch(src[i]) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (16 * val) + src[i] - '0';
			break;
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			val = (16 * val) + src[i] - 'a' + 10;
			break;
		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			val = (16 * val) + src[i] - 'A' + 10;
			break;
		default:
			goto out;
		}
	}

out:
	*dest = val;
	return i;
}

off_t
parse_b64(NsonBuf **dest_buf, const char *src, const size_t len) {
	char *p;
	int8_t v;
	off_t i, j;
	char *dest;

	(*dest_buf) = nson_buf_new((len + 3) / 4 * 3);
	if (*dest_buf == NULL)
		return -1;
	dest = nson_buf_unwrap(*dest_buf);

	for (i = j = 0; i < len && src[i] != '='; i++) {
		p = memchr(base64_table, src[i], 64);
		if(p == NULL) {
			goto err;
		}
		v = p - base64_table;

		switch(i % 4) {
		case 0:
			dest[j] = v << 2;
			break;
		case 1:
			dest[j++] |= v >> 4;
			dest[j] = v << 4;
			break;
		case 2:
			dest[j++] |= v >> 2;
			dest[j] = v << 6;
			break;
		case 3:
			dest[j++] |= v;
			break;
		}
	}

	for(; i % 4 != 0 && i < len && src[i] == '='; i++);

	if(i % 4 != 0) {
err:
		nson_buf_release(*dest_buf);
		return -1;
	}

	nson_buf_shrink(*dest_buf, j);

	return i;
}

off_t
parse_number(Nson *nson, const char *src, size_t len) {
	off_t i = 0;
	int64_t i_val;
	double r_val;

	i = parse_dec(&i_val, src, len);
	if (i >= len || src[i] != '.') {
		nson_init_int(nson, i_val);
		return i;
	}

	i++;
	for (r_val = 0; i < len && src[i] >= '0' && src[i] <= '9'; i++) {
		r_val = (r_val * 0.1) + src[i] - '0';
	}
	r_val *= 0.1;

	if (i_val >= 0) {
		r_val += i_val;
	} else {
		r_val -= i_val;
	}

	nson_init_real(nson, r_val);

	return i;
}

off_t
to_utf8(char *dest, const uint64_t chr, const size_t len) {
	if (chr < 0x0080 && len >= 1) {
		*dest = chr;
		return 1;
	} else if (chr < 0x0800 && len >= 2) {
		dest[0] = ((chr >> 6) & 0x1F) | 0xC0;
		dest[1] = (chr & 0x3F) | 0x80;
		return 2;
	} else if (len >= 3) {
		dest[0] = ((chr >> 12) & 0x0F) | 0xE0;
		dest[1] = ((chr >> 6) & 0x3F) | 0x80;
		dest[2] = (chr & 0x3F) | 0x80;
		return 3;
	}

	return 0;
}
