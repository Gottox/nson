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

To aquire the bench file `wget` is used. For converting it into plist, node and npm
need to be installed.

To setup the environment in VoidLinux run the following command:

	xbps-install base-devel proplib-devel libucl-devel json-c-devel jansson-devel

Goals
-----

 * **Bootstrapable** - NSON should be useable without fancy dependencies
   or build systems. That makes it possible to use it during bootstrapping where
   only very few base dependencies are available. (gcc, libc, make)
 * **Fast** - NSON should be fast to setup. It is designed to be used for command
   line tools that won't run for long. So NSON avoids building hashmaps and uses
   linear/binary search which is expected to be faster in programs of short
   lifetime.
 * **non-refcount** NSON relies on a memory model explained below instead.
 * **mutex-/lock-less** NSON provides basic thread functionality, which will
   be used for mapping or filtering collections of data. If you need more complex
   threading setups, you must lock the data by your own.
   

Memory Model
------------

NSON does not rely on reference counting, garbage collection, or data locking.
Instead the user is adviced to follow a few simple rules that avoid memory leaks:

 * If NSON _returns_ a pointer, NSON cares about the memory. The data is
   _borrowed to you_. Never clean borrowed data.

 * If NSON _expects_ a pointer, it will not care about freeing the memory.
   The data is _borrowed to nson_ and won't be mutated after the function
   returns.

   * The big exception is `nson_init_data()`. NSON will try to free the value
     of the created element once nson_clean() is called on the field.
     This was added to have a way to save binary data in an NSON field
     without memcpy'ing it.

If this concept sounds familiar, yes, it is a similiar concept used in rust. :)
