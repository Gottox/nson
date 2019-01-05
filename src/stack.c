/*
 * stack.c
 * Copyright (C) 2018 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "config.h"
#include "nson.h"

Nson *
nson_walk(Nson *stack, Nson **nson, off_t *index) {
	Nson *stack_item = NULL;
	Nson *item = NULL;

	if (*nson == NULL) {
		return NULL;
	}

	enum NsonType type = nson_type(*nson);

	if (type != NSON_OBJ && type != NSON_ARR) {
		return NULL;
	}

	(*index)++;

	if (*index < nson_mem_len(*nson)) {
		item = nson_mem_get(*nson, *index);
		type = nson_type(item);
		// PUSH
		if (type == NSON_OBJ || type == NSON_ARR) {
			stack_item = nson_init_ptr((void *)*nson, *index, NSON_BLOB);
			nson_push(stack, stack_item);
			*nson = item;
			*index = -1;
		}
	// POP
	} else if ( (stack_item = nson_pop(stack)) ) {
		item = *nson;
		*nson = (void *)nson_data(&stack_item);
		*index = nson_data_len(&stack_item);
	// TERMINAL
	} else {
		item = *nson;
		*nson = NULL;
		*index = 0;
	}

	return item;
}

