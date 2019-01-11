/*
 * stack.c
 * Copyright (C) 2019 tox <tox@rootkit>
 *
 * Distributed under terms of the MIT license.
 */

#include "util.h"

Nson *
stack_walk(NsonStack *stack, Nson **nson, off_t *index) {
	Nson *tmp;
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
			stack_push(stack, *nson, *index);
			*nson = item;
			*index = -1;
		}
	// POP
	} else if (stack_pop(stack, &tmp, index)) {
		item = *nson;
		*nson = tmp;
	// TERMINAL
	} else {
		item = *nson;
		*nson = NULL;
		*index = 0;
		stack_clean(stack);
	}

	return item;
}


int
stack_push(NsonStack *stack, Nson *element, off_t index) {
	stack->len++;
	stack->arr = realloc(stack->arr, stack->len * sizeof(NsonStack));

	NsonStackElement *last = &stack->arr[stack->len-1];
	last->element = element;
	last->index = index;
	return stack->len;
}

int
stack_pop(NsonStack *stack, Nson **element, off_t *index) {
	if (stack->len == 0)
		return 0;
	NsonStackElement *last = &stack->arr[--stack->len];

	*index = last->index;
	*element = last->element;
	return stack->len + 1;
}

void
stack_clean(NsonStack *stack) {
	free(stack->arr);
}
