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

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "config.h"
#include "nson.h"

#define SKIP_SPACES { for(; doc[i] && isspace(doc[i]); i++); }

static int
parse_tag(char *doc, const char **tag) {
	off_t i = 0;

	*tag = NULL;

	if (doc[i] != '<')
		return -1;
	i++;
	*tag = &doc[i];

	for(; isalnum(doc[i]) || doc[i] == '/' || doc[i] == '!' || doc[i] == '?'; i++);

	if(doc[i] == '>') {
		doc[i] = '\0';
		return i + 1;
	}
	doc[i] = '\0';
	i++;
	SKIP_SPACES;
	if((*tag)[0] != '/') {
		for(; doc[i] && doc[i] != '>'; i++);
	}
	i++;

	SKIP_SPACES;

	return i;
}

static int
plist_mapper_string(off_t index, struct Nson *nson) {
	size_t len;
	off_t t_len;
	int rv, val;
	char *dest;
	char *p;
	assert(nson_type(nson) & NSON_STR);

	len = nson_len(nson);
	dest = strndup(nson_data(nson), len);

	for(p = dest; (p = strchr(p, '&')); p++) {
		if(strncmp("lt;", &p[1], 3) == 0) {
			t_len = 4;
			*p = '<';
		} else if(strncmp("gt;", &p[1], 3) == 0) {
			t_len = 4;
			*p = '>';
		} else if(strncmp("amp;", &p[1], 4) == 0) {
			t_len = 5;
			*p = '&';
		} else if ('#' == p[1]){
			t_len = -1;
			// TODO: UTF8 escape codes
			if(sscanf(p, "&#%d;%n", &val, &rv) == 0 || rv < 0)
				continue;
			t_len = rv;
			*p = val;
		} else
			continue;
		memmove(p + 1, p + t_len, len - (p - dest) - t_len);
		len -= t_len - 1;
	}
	dest[len] = 0;

	nson_clean(nson);
	nson_init_data(nson, dest, len, NSON_PLAIN);
	nson->alloc_type = NSON_ALLOC_BUF;
	nson->alloc.b = dest;

	return 0;
}

static int
plist_parse_string(struct Nson *nson, const char *start_tag, char *doc) {
	int rv;
	off_t end = 0, len;
	char *p;
	const char *tag;

	p = strstr(doc, "</");
	if(p == NULL)
		return -1;
	len = end = p - doc;

	rv = parse_tag(p, &tag);
	if(rv < 0 || tag[0] != '/' || strcmp(start_tag, &tag[1]))
		return -1;
	end += rv;

	rv = nson_init_data(nson, doc, len, NSON_UTF8);
	if(rv < 0)
		return rv;

	if (start_tag[0] == 'd')
		nson->val.d.mapper = nson_mapper_b64_dec;
	else {
		nson->type = NSON_STR;
		nson->val.d.mapper = plist_mapper_string;
	}

	return end;
}

static int
plist_parse_type(struct Nson *nson, char *doc);

static int
plist_parse_object(struct Nson *nson, const char *start_tag, char *doc) {
	int rv;
	size_t len = 0;
	off_t i = 0;
	struct Nson elem;
	const char *tag;

	nson_init(nson, start_tag[0] == 'd' ? NSON_OBJ : NSON_ARR);
	SKIP_SPACES;
	for(; &doc[i];) {
		if (doc[i] == '<' && doc[i + 1] == '/')
			break;
		if(start_tag[0] == 'd' && len % 2 == 0) {
			rv = parse_tag(&doc[i], &tag);
			if(rv < 0)
				return -1;
			else if(strcmp("key", tag))
				return -1;
			i += rv;

			rv = plist_parse_string(&elem, tag, &doc[i]);
		} else
			rv = plist_parse_type(&elem, &doc[i]);
		if(rv < 0)
			return -1;
		i += rv;

		SKIP_SPACES;

		rv = nson_add(nson, &elem);
		if(rv < 0)
			return -1;
		len++;
	}
	rv = parse_tag(&doc[i], &tag);
	if(rv < 0 || tag[0] != '/' || strcmp(start_tag, &tag[1]))
		return -1;
	return i + rv;
}

static int
plist_parse_type(struct Nson *nson, char *doc) {
	int rv;
	off_t i = 0;
	const char *tag;
	int64_t val;
	double r_val;

	rv = parse_tag(&doc[i], &tag);
	i += rv;
	if(rv < 0) {
		return -1;
	} else if(strcmp("false/", tag) == 0) {
		rv = nson_init_int(nson, 0);
	} else if(strcmp("true/", tag) == 0) {
		rv = nson_init_int(nson, 1);
	} else if(strcmp("real", tag) == 0) {
		rv = -1;
		if(*tag == 'r' && (sscanf(&doc[i], "%lf%n", &r_val, &rv) == 0 || rv < 0))
			return rv;
		i += rv;
		rv = parse_tag(&doc[i], &tag);
		if(rv < 0 || strcmp("/real", tag))
			return -1;
		nson_init_real(nson, r_val);
	} else if(strcmp("integer", tag) == 0) {
		rv = -1;
		if(sscanf(&doc[i], "%" PRId64 "%n", &val, &rv) == 0 || rv < 0)
			return rv;
		i += rv;
		rv = parse_tag(&doc[i], &tag);
		if(rv < 0 || strcmp("/integer", tag))
			return -1;
		nson_init_int(nson, val);
	} else if(strcmp("string", tag) == 0) {
		rv = plist_parse_string(nson, "string", &doc[i]);
	} else if(strcmp("array", tag) == 0 || strcmp("dict", tag) == 0) {
		rv = plist_parse_object(nson, tag, &doc[i]);
	} else if(strcmp("data", tag) == 0) {
		rv = plist_parse_string(nson, "data", &doc[i]);
		if(rv < 0)
			return rv;
	} else {
		return -1;
	}
	if (rv < 0)
		return rv;
	return i + rv;
}

int
nson_load_plist(struct Nson *nson, const char *file) {
	return nson_load(nson_parse_plist, nson, file);
}

int
nson_parse_plist(struct Nson *nson, char *doc, size_t len) {
	int rv;
	off_t i = 0;
	const char *tag;

	rv = parse_tag(&doc[i], &tag);
	if(rv < 0 || strcmp("?xml", tag))
		return -1;
	i += rv;

	rv = parse_tag(&doc[i], &tag);
	if(rv < 0 || strcmp("!DOCTYPE", tag))
		return -1;
	i += rv;

	rv = parse_tag(&doc[i], &tag);
	if(rv < 0 || strcmp("plist", tag))
		return -1;
	i += rv;

	SKIP_SPACES;

	rv = plist_parse_type(nson, &doc[i]);
	if(rv < 0)
		return -1;
	i += rv;

	SKIP_SPACES;

	rv = parse_tag(&doc[i], &tag);
	if(rv < 0 || strcmp("/plist", tag))
		return -1;
	i += rv;

	return i;
}

static int
plist_escape(const struct Nson *nson, FILE *fd) {
	off_t i = 0, last_write = 0;
	char *escape = NULL;
	const char *str = nson->val.d.b;

	if(str == NULL) {
		return 0;
	}

	for(; i < nson_len(nson); i++) {
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
to_plist(const struct Nson *nson, const char *string_overwrite, FILE *fd) {
	off_t i;
	int rv;
	enum NsonType type = nson_type(nson);
	switch(type) {
	case NSON_ARR:
	case NSON_OBJ:
		rv = fputs(type == NSON_ARR ? "<array>" : "<dict>", fd);
		if(rv <= 0)
			return -1;
		for(i = 0; i < nson_mem_len(nson); i++) {
			rv = to_plist(nson_get(nson, i), i % 2 == 0 ? "key" : "string", fd);
			if(rv < 0)
				return -1;
		}
		rv = fputs(type == NSON_ARR ? "</array>" : "</dict>", fd);
		if(rv <= 0)
			return -1;
	case NSON_DATA:
		switch(nson->val.d.enc) {
		case NSON_UTF8:
			fprintf(fd, "<%s>", string_overwrite);
			rv = plist_escape(nson, fd);
			fprintf(fd, "</%s>", string_overwrite);
			break;
		case NSON_PLAIN:
			fputs("<data>", fd);
			rv = nson_data_b64(nson, fd);
			fputs("</data>", fd);
		case NSON_BASE64:
			fputs("<data>", fd);
			rv = plist_escape(nson, fd);
			fputs("</data>", fd);
			break;
		}
		break;
	case NSON_BOOL:
		fputs(nson_int(nson) ? "<true/>" : "<false/>", fd);
		break;
	case NSON_INT:
		fprintf(fd, "<integer>%" PRId64 "</integer>", nson_int(nson));
		break;
	case NSON_REAL:
		fprintf(fd, "<real>%f</real>", nson_real(nson));
		break;
	default:
		break;
	}
	return 0;
}

int
nson_to_plist_fd(const struct Nson *nson, FILE *fd) {
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
		"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
		"<plist version=\"1.0\">", fd);
	to_plist(nson, "string", fd);
	fputs("</plist>",fd);
	return 0;
}

int
nson_to_plist(const struct Nson *nson, char **str) {
	int rv;
	size_t size = 0;
	FILE *fd = open_memstream(str, &size);
	if(fd == NULL)
		return -1;

	rv = nson_to_plist_fd(nson, fd);
	fclose(fd);
	return rv;
}
