/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   String hashing
 **/

#ifndef __STRHASH_H__
#define __STRHASH_H__

#include "type.h"

// D. J. Bernstein hash function
uint32_t hash_djb(const char *cp);

#endif /* __STRHASH_H__ */
