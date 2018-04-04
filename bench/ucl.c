/*
 * portableproplib.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "../test/test.h"

#include "../src/nson.h"

#include <ucl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

static bool
mmap_file(const char *file, void **mmf, size_t *mmflen, size_t *filelen)
{
	struct stat st;
	size_t pgsize = (size_t)sysconf(_SC_PAGESIZE);
	size_t pgmask = pgsize - 1, mapsize;
	unsigned char *mf;
	bool need_guard = false;
	int fd;

	assert(file);

	if ((fd = open(file, O_RDONLY|O_CLOEXEC)) == -1)
		return false;

	if (fstat(fd, &st) == -1) {
		(void)close(fd);
		return false;
	}
	if (st.st_size > SSIZE_MAX - 1) {
		(void)close(fd);
		return false;
	}
	mapsize = ((size_t)st.st_size + pgmask) & ~pgmask;
	if (mapsize < (size_t)st.st_size) {
		(void)close(fd);
		return false;
	}
	/*
	 * If the file length is an integral number of pages, then we
	 * need to map a guard page at the end in order to provide the
	 * necessary NUL-termination of the buffer.
	 */
	if ((st.st_size & pgmask) == 0)
		need_guard = true;

	mf = mmap(NULL, need_guard ? mapsize + pgsize : mapsize,
	    PROT_READ, MAP_PRIVATE, fd, 0);
	(void)close(fd);
	if (mf == MAP_FAILED) {
		(void)munmap(mf, mapsize);
		return false;
	}

	*mmf = mf;
	*mmflen = mapsize;
	*filelen = st.st_size;

	return true;
}

void ucl() {
	void *doc = 0;
	size_t len, f_len = 0;
	struct ucl_parser *parser = NULL;

	assert(mmap_file("./json/pkgdb-0.38.json", &doc, &len, &f_len));
	parser = ucl_parser_new (0);
	ucl_parser_add_chunk (parser, doc, f_len);

	assert(ucl_parser_get_error (parser) == NULL);
}

DEFINE
TEST(ucl);
DEFINE_END
