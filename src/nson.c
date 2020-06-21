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
#include <string.h>

int
nson_cmp(const void *a, const void *b) {
	int rv;
	Nson *na = (Nson *)a;
	Nson *nb = (Nson *)b;
	enum NsonType a_type = nson_type(na);
	enum NsonType b_type = nson_type(nb);

	// Reverse sort as NSON_NIL needs to be last
	if (0 != (rv = SCAL_CMP(b_type, a_type)))
		return rv;
	switch(a_type) {
		case NSON_STR:
		case NSON_BLOB:
			return __nson_buf_cmp(na->d.buf, nb->d.buf);
		case NSON_REAL:
			return SCAL_CMP(nson_real(na), nson_real(nb));
		case NSON_INT:
		case NSON_BOOL:
			return SCAL_CMP(nson_int(na), nson_int(nb));
		default:
			return 0;
	}
}

int
nson_clean(Nson *nson) {
	int rv = 0;

	switch (nson_type(nson)) {
		case NSON_BLOB:
		case NSON_STR:
			__nson_buf_release(nson->d.buf);
			break;
		case NSON_POINTER:
			__nson_ptr_release(nson->p.ref);
			break;
		case NSON_ARR:
			__nson_arr_clean(nson);
			break;
		case NSON_OBJ:
			__nson_obj_clean(nson);
			break;
		case NSON_BOOL:
		case NSON_INT:
		case NSON_REAL:
		case NSON_NIL:
			break;
	}
	memset(nson, 0, sizeof(*nson));

	return rv;
}                                                      

