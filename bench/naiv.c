/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"
#include "mmap.h"
#include <assert.h>
#include <ctype.h>

static size_t
parse_plist(const char *doc) {
	int rv;
	int64_t i_val;
	double r_val;
	char *mp;
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
			for(++p; (p = strchr(p, '"')); p++) {
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
			i_val = strtoll(p, &mp, 10);
			p += mp - p;
			if(*p != '.') {
				break;
			}
			if(sscanf(p, "%lf%n", &r_val, &rv) == 0 || rv < 0) {
				goto err;
			}
			p += rv;
			r_val += i_val;
			break;
		default:
			if(strncmp(p, "null", 4) == 0) {
				p += 4;
			} else if(strncmp(p, "true", 4) == 0) {
				p += 4;
			} else if(strncmp(p, "false", 5) == 0) {
				p += 5;
			}
		}
	} while(stack_size > 1);
	return p - doc;
err:
	return -1;
}

static size_t
parse_json(const char *doc, const size_t len) {
	int64_t i_val;
	double r_val;
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
			if(*p == '-')
				p++;
			for(i_val = 0; *p >= '0' && *p <= '9'; p++)
				i_val += (i_val * 10) + *p - '0';
			if(*p != '.') {
				break;
			}
			p++;
			for(r_val = 0; *p >= '0' && *p <= '9'; p++)
				r_val += (r_val / 10.) + (*p - '0');
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
	printf("%li %li\n", parsed, f_len);
	for(; isspace(doc[parsed]); parsed++);
	assert(parsed == f_len);

	(void)rv;
}

DEFINE
TEST(naiv_json);
TEST_OFF(naiv_plist);
DEFINE_END
