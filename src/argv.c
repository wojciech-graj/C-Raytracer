/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Command-line argument parsing
 **/

#include "argv.h"
#include "strhash.h"
#include "mem.h"
#include "system.h"

#include <stdint.h>
#include <stdlib.h>

int myargc;
char **myargv;
uint32_t *hash_myargv;

void argv_init(void)
{
	hash_myargv = safe_malloc(sizeof(uint32_t) * myargc);

	int i;
	for (i = 0; i < myargc; i++)
		hash_myargv[i] = hash_djb(myargv[i]);
}

void argv_deinit(void)
{
	free(hash_myargv);
}

int argv_check(const char *param)
{
	uint32_t hash_param = hash_djb(param);

	int i;
	for (i = 1; i < myargc; i++)
		if (hash_param == hash_myargv[i])
			return i;

	return 0;
}

int argv_check_with_args(const char *param, const int num_args)
{
	int idx = argv_check(param);

	return (idx + num_args < myargc) ? idx : 0;
}
