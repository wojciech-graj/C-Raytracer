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

#ifndef __MEM_H__
#define __MEM_H__

#include <stddef.h>

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
void *safe_calloc(size_t nmemb, size_t size);

#endif /* __MEM_H__ */
