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

#ifndef __ARGV_H__
#define __ARGV_H__

#include "type.h"

enum Argv {
	ARG_INPUT_FILENAME = 1,
	ARG_OUTPUT_FILENAME = 2,
	ARG_RESOLUTION_X = 3,
	ARG_RESOLUTION_Y = 4,
};

/* Requires myargv, myargc to be set */
void argv_init(void);
void argv_deinit(void);

/* Check is argument exists. Returns 0 on failure. */
int argv_check(const char *param);
int argv_check_with_args(const char *param, int num_args);

extern int myargc;
extern char **myargv;
extern uint32_t *hash_myargv;

#endif /* __ARGV_H__ */
