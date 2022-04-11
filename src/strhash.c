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

#include "strhash.h"

uint32_t hash_djb(const char *cp)
{
	uint32_t hash = 5381;
	while (*cp)
		hash = 33 * hash ^ (uint8_t)*cp++;
	return hash;
}
