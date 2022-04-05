/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Memory allocation with error-checking
 **/

#include "mem.h"
#include "error.h"

void *safe_malloc(const size_t size)
{
	void *ptr = malloc(size);
	error_check(ptr, "Unable to allocate [%zu] bytes on heap.", size);
	return ptr;
}

void *safe_realloc(void *ptr, const size_t size)
{
	void *new_ptr = realloc(ptr, size);
	error_check(ptr, "Unable to reallocate [%zu] bytes on heap.", size);
	return new_ptr;
}

void *safe_calloc(const size_t nmemb, const size_t size)
{
	void *ptr = calloc(nmemb, size);
	error_check(ptr, "Unable to calloc [%zu] bytes on the heap.", nmemb * size);
	return ptr;
}
