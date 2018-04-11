NSON
====

Nson is a data framework for C with a very fast JSON and property list parser.

To run the tests suite:

	make check

To run the benchmarks with other parsers:

	make speed

for the benchmark the following libraries and headerfiles are needed:

 * proplib
 * libucl
 * json-c
 * jansson

To aquire the bench file `wget` is used. For converting it into plist, node and
need to be installed.

To setup the environment in VoidLinux run the following command:

	xbps-install base-devel proplib-devel libucl-devel json-c-devel jansson-devel
