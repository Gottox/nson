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

	if (*index < nson_arr_len(*nson)) {
		item = nson_arr_get(*nson, *index);
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
	stack->arr = reallocarray(stack->arr, stack->len, sizeof(NsonStackElement));

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
