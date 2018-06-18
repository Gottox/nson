/*
 * nson.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "../src/nson.h"

static struct option long_options[] = {
	{ "out-format", required_argument, 0, 'o' },
	{ "in-format", required_argument, 0, 'i' }
};

/*static struct {
	const char *suffix;
	const void *parser;
	const void *serializer;
} parsers[] = {
	{ "json",  nson_parse_json,  nson_to_json },
	{ "plist", nson_parse_plist, nson_to_plist },
	{ "ini",   nson_parse_ini,   NULL },
};*/

static int
usage(char *prog) {
	fprintf(stderr, "%s: [-i format] [-o format] input output\n", prog);
	return EXIT_FAILURE;
}

int
main(int argc, char *argv[]) {
	int c, i, fd;
	char *formats[2] = { NULL, NULL };

	for(c = 0; (c = getopt_long(argc, argv, "o:i:", long_options, NULL)) != -1; ) {
		switch(c) {
		case 'o':
			formats[1] = optarg;
			break;
		case 'i':
			formats[0] = optarg;
			break;
		default:
			return usage(argv[0]);
		}
	}

	for(i = 0; i < 2 && optind < argc; i++, optind++) {
		if (formats[i] == NULL && (formats[i] = strrchr(argv[optind], '.')))
			formats[i]++;
		if (strcmp(argv[optind], "-") == 0)
			continue;
		fd = open(argv[optind], i ? (O_WRONLY | O_CREAT) : O_RDONLY, 0664);
		if (fd < 0) {
			perror(argv[optind]);
			return EXIT_FAILURE;
		}
		dup2(fd, i);
	}
	(void)formats;
	return EXIT_SUCCESS;
}
