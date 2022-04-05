/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Common types and macros
 **/

#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef float v2[2];
typedef float v3[3];
typedef float m3[3][3];

#define UNREACHABLE  __builtin_unreachable()

#define likely(x)       __builtin_expect((x), true)
#define unlikely(x)     __builtin_expect((x), false)

#define clz __builtin_clz

#define PI 3.1415927f

enum _Directions {
	X = 0,
	Y,
	Z,
};

#endif /* __TYPE_H__ */
