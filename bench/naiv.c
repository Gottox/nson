/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"
#include "../src/util.h"
#include "mmap.h"
#include <assert.h>
#include <ctype.h>

#define SKIP_SPACES for(; *p && strchr("\n\f\r\t\v ", *p); p++);

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

static int
parse_plist(const char *doc) {
	int rv = 0;
	int len = strlen(doc);
	const char *p = doc;
	const char *string_tag;
	size_t stack_size = 1;
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

	SKIP_SPACES;
	do {
		switch(*p) {
		case '\0':
			goto err;
		case '<':
			p++;
			rv = 0;
			string_tag = "string";
			switch(*p) {
				case 'd':
					if((rv = skip_tag("dict", p, len - (doc - p))) <= 0)
						break;
					stack_size++;
					p += rv;
					break;
				case 'a':
					if((rv = skip_tag("array", p, len - (doc - p))) <= 0)
						break;
					stack_size++;
					p += rv;
					break;
				case '/':
					p++;
					switch(*p) {
						case 'd':
							if((rv = skip_tag("dict", p, len - (doc - p))) <= 0)
								break;
							stack_size--;
							p += rv;
							break;
						case 'a':
							if((rv = skip_tag("array", p, len - (doc - p))) <= 0)
								break;
							stack_size--;
							p += rv;
							break;
					}
				case 'k':
					string_tag = "key";
				case 's':
					if((rv = skip_tag(string_tag, p, len - (doc - p))) <= 0)
						break;
					p += rv;
					for(rv = 0; rv == 0;) {
						if(!(p = memchr(p, '<', len - (doc - p))))
							goto err;
						p++;
						if(*p != '/')
							break;
						p++;
						if((rv = skip_tag(string_tag, p, len - (doc - p))) > 0)
							p += rv;
					}
				case 'r':
					if((rv = skip_tag("real", p, len - (doc - p))) <= 0)
						break;
					for(; (*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-' || *p == 'e'; p++);
					break;
				case 'i':
					if((rv = skip_tag("integer", p, len - (doc - p))) <= 0)
						break;
					for(; (*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-'; p++);
					break;
			}
			break;
		default:
			p++;
			break;
		}
	} while(stack_size > 1);
	SKIP_SPACES;
	rv = skip_tag("</plist", p, len - (doc - p));
	if(rv > 0)
		p += rv;
	SKIP_SPACES;
	return p - doc;
err:
	return -1;
}

static size_t
parse_json(const char *doc, const size_t len) {
	const char *p = doc;
	size_t stack_size = 1;
	do {
		switch(*p) {
		case '\0':
			goto err;
		case '[':
		case '{':
			stack_size++;
			p++;
			break;
		case ',':
		case ':':
			p++;
			break;
		case ']':
		case '}':
			stack_size--;
			p++;
			break;
		case '"':
			for(++p; (p = memchr(p, '"', p + len - doc)); p++) {
				if(p[-1] != '\\')
					break;
				else if(p[-2] == '\\')
					break;
			}
			p++;
			break;
		case '\n':
		case '\f':
		case '\r':
		case '\t':
		case '\v':
		case ' ':
			p++;
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
			for(; (*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-' || *p == 'e'; p++);
			break;
		default:
			if(strncmp(p, "null", 4) == 0) {
				p += 4;
			} else if(strncmp(p, "true", 4) == 0) {
				p += 4;
			} else if(strncmp(p, "false", 5) == 0) {
				p += 5;
			} else {
				goto err;
			}
		}
	} while(stack_size > 1);

	for(; isspace(*p); p++);
	return p - doc;
err:
	return -1;
}

void naiv_plist() {
	int rv;
	char *doc = 0;
	size_t len, f_len = 0;
	int parsed;

	rv = mmap_file(BENCH_PLIST, (void **)&doc, &len, &f_len);
	assert(rv);
	parsed = parse_plist(doc);
	assert(parsed == f_len);

	(void)rv;
}

void naiv_json() {
	size_t rv;
	char *doc = 0;
	size_t len, f_len = 0, parsed;

	rv = mmap_file(BENCH_JSON, (void **)&doc, &len, &f_len);
	assert(rv);
	parsed = parse_json(doc, f_len);
	assert(parsed == f_len);

	(void)rv;
}

DEFINE
TEST(naiv_json);
TEST(naiv_plist);
DEFINE_END
