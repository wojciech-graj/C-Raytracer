/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Error-checking
 **/

#ifndef __ERROR_H__
#define __ERROR_H__

#include "type.h"

#include <stdlib.h>

#include "system.h"

#define error(...)                       \
	do {                             \
		printf_log(__VA_ARGS__); \
		exit(1);                 \
		UNREACHABLE;             \
	} while (0)

#define error_check(cond, ...)              \
	do {                                \
		if (unlikely(!(cond))) {    \
			error(__VA_ARGS__); \
		}                           \
	} while (0)

#endif /* __ERROR_H__ */
