#include "internal.h"
#include "nson.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

static int
cmp_stable(const void *a, const void *b) {
	const NsonObjectEntry *na = a, *nb = b;
	int rv = nson_cmp(&na->key, &nb->key);
	return rv ? rv : (na - nb);
}

static int
mem_capacity(Nson *nson, const size_t size) {
	NsonObjectEntry *arr;
	const size_t old = nson_obj_size(nson);

	if (size == old) {
		return size;
	}

	arr = nson->o.arr;
	nson->o.len = size;

	if (size && size > SIZE_MAX / sizeof(*arr)) {
		errno = ENOMEM;
		return -1;
	}
	arr = reallocarray(arr, size, sizeof(*arr));
	if (!arr) {
		nson->o.len = old;
		return -1;
	}
	if (size > old)
		memset(&arr[old], 0, sizeof(*arr) * (size - old));

	nson->o.arr = arr;
	return size;
}

static int
obj_sort(Nson *object) {
	qsort(object->o.arr, object->o.len, sizeof *object->o.arr, cmp_stable);
	object->c.type = NSON_OBJ;

	return 0;
}

static int
search_key(const void *key, const void *elem) {
	return strcmp(key, nson_str(elem));
}

static NsonObjectEntry *
obj_search(Nson *object, const char *key) {
	if (object->o.messy) {
		obj_sort(object);
	}

	return bsearch(key, object->o.arr, object->o.len, sizeof *object->o.arr,
				   search_key);
}

static NsonObjectEntry *
obj_get(const Nson *object, int index) {
	if (index < nson_obj_size(object)) {
		return &object->o.arr[index];
	} else {
		return NULL;
	}
}

static NsonObjectEntry *
obj_last(Nson *object) {
	size_t len = nson_obj_size(object);
	if (len == 0) {
		return NULL;
	} else {
		return &object->o.arr[len - 1];
	}
}

int
__nson_obj_clone(Nson *object) {
	int rv = 0;
	NsonObjectEntry *arr = object->o.arr;
	size_t len = nson_arr_len(object);

	object->o.arr = NULL;
	object->o.len = 0;

	rv = mem_capacity(object, len);
	if (rv < 0 || object->o.arr == NULL)
		return rv;
	memcpy(object->o.arr, arr, len * sizeof(*arr));

	return rv;
}

Nson *
nson_obj_get(Nson *object, const char *key) {
	assert(nson_type(object) == NSON_OBJ);
	NsonObjectEntry *result = obj_search(object, key);
	if (result) {
		return &result->value;
	} else {
		return NULL;
	}
}

const char *
nson_obj_get_key(Nson *object, int index) {
	assert(nson_type(object) == NSON_OBJ);
	NsonObjectEntry *result = obj_get(object, index);
	if (result) {
		return nson_str(&result->key);
	} else {
		return NULL;
	}
}

int
nson_obj_put(Nson *object, const char *key, Nson *value) {
	assert(nson_type(object) == NSON_OBJ);
	NsonObjectEntry *new_elem;
	Nson obj_key = {0};

	nson_init_str(&obj_key, key);

	if (object->o.messy && nson_obj_size(object) > 0 &&
		nson_cmp(&obj_last(object)->key, &obj_key) > 0) {
		object->o.messy = 1;
	}

	size_t old_len = nson_obj_size(object);
	mem_capacity(object, old_len + 1);

	new_elem = &object->o.arr[old_len];
	nson_move(&new_elem->key, &obj_key);
	nson_move(&new_elem->value, value);

	return 0;
}

size_t
nson_obj_size(const Nson *object) {
	return object->o.len;
}

int
__nson_obj_clean(Nson *object) {
	assert(nson_type(object) == NSON_OBJ);
	int i, rv = 0;
	NsonObjectEntry *entry;
	size_t len = nson_obj_size(object);

	for (i = 0; i < len; i++) {
		entry = obj_get(object, i);
		rv |= nson_clean(&entry->key);
		rv |= nson_clean(&entry->value);
	}
	free(object->o.arr);

	return rv;
}

int
nson_obj_from_arr(Nson *array) {
	assert(nson_type(array) == NSON_ARR);
	Nson obj;

	if (nson_arr_len(array) % 2 != 0) {
		return -1;
	}

	nson_init(&obj, NSON_OBJ);

	obj.c.type = NSON_OBJ;
	obj.o.messy = true;
	obj.o.arr = (NsonObjectEntry *)array->a.arr;
	obj.o.len = array->a.len / 2;

	nson_move(array, &obj);
	return 0;
}

int
nson_obj_to_arr(Nson *object) {
	assert(nson_type(object) == NSON_ARR);

	object->c.type = NSON_OBJ;
	object->o.len = object->a.len / 2;
	object->o.messy = true;

	return 0;
}

int
__nson_obj_serialize(FILE *out, const Nson *object,
					 const NsonSerializerInfo *info, enum NsonOptions options) {
	int i;
	size_t size = nson_obj_size(object);
	NsonObjectEntry *entry;

	for (i = 0; i < size; i++) {
		entry = obj_get(object, i);
		info->serializer(out, &entry->key,
						 options | NSON_IS_KEY | NSON_SKIP_HEADER);
		fputs(info->key_value_seperator, out);
		info->serializer(out, &entry->value, options | NSON_SKIP_HEADER);
		if (i + 1 != size) {
			fputs(info->seperator, out);
		}
	}

	return 0;
}
