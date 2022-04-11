/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Nil
 **/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "type.h"

#include <stdio.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

void system_init(void);

double system_time(void);
float rand_flt(void);

#pragma GCC system_header /* Ignore -Wpedantic warning about ##__VA_ARGS__ trailing comma */
#define printf_log(fmt, ...)                                               \
	do {                                                               \
		printf("[%08.3f] %10s:%16s:%3u: " fmt "\n", system_time(), \
			__FILENAME__, __func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#endif /* __SYSTEM_H__ */
