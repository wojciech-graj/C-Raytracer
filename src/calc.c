/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Math operations
 **/

#include "calc.h"

#include <math.h>

__attribute__((const))
float sqr(const float val)
{
	return val * val;
}

__attribute__((const))
float mag2(const v2 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]));
}

__attribute__((const))
float mag3(const v3 vec)
{
	return sqrtf(magsqr3(vec));
}

__attribute__((const))
float dot2(const v2 vec1, const v2 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y];
}

__attribute__((const))
float dot3(const v3 vec1, const v3 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y] + vec1[Z] * vec2[Z];
}

void cross(const v3 vec1, const v3 vec2, v3 result)
{
	result[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
	result[Y] = vec1[Z] * vec2[X] - vec1[X] * vec2[Z];
	result[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];
}

void mul2s(const v2 vec, const float mul, v2 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
}

void mul3s(const v3 vec, const float mul, v3 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
	result[Z] = vec[Z] * mul;
}

void mul3v(const v3 vec1, const v3 vec2, v3 result)
{
	result[X] = vec1[X] * vec2[X];
	result[Y] = vec1[Y] * vec2[Y];
	result[Z] = vec1[Z] * vec2[Z];
}

void inv3(v3 vec)
{
	vec[X] = 1.f / vec[X];
	vec[Y] = 1.f / vec[Y];
	vec[Z] = 1.f / vec[Z];
}

void add2v(const v2 vec1, const v2 vec2, v2 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
}

void add2s(const v2 vec1, const float summand, v2 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
}

void add3v(const v3 vec1, const v3 vec2, v3 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
	result[Z] = vec1[Z] + vec2[Z];
}

void add3s(const v3 vec1, const float summand, v3 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
	result[Z] = vec1[Z] + summand;
}

void add3v3(const v3 vec1, const v3 vec2, const v3 vec3, v3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void sub2v(const v2 vec1, const v2 vec2, v2 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
}

void sub2s(const v2 vec1, const float subtrahend, v2 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
}

void sub3v(const v3 vec1, const v3 vec2, v3 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
	result[Z] = vec1[Z] - vec2[Z];
}

void sub3s(const v3 vec1, const float subtrahend, v3 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
	result[Z] = vec1[Z] - subtrahend;
}

void norm2(v2 vec)
{
	mul2s(vec, 1.f / mag2(vec), vec);
}

void norm3(v3 vec)
{
	mul3s(vec, 1.f / mag3(vec), vec);
}

__attribute__((const))
float min3(const v3 vec)
{
    float min = vec[0];
    if (min > vec[1])
	min = vec[1];
    if (min > vec[2])
	min = vec[2];
    return min;
}

__attribute__((const))
float max3(const v3 vec)
{
    float max = vec[0];
    if (max < vec[1])
	max = vec[1];
    if (max < vec[2])
	max = vec[2];
    return max;
}

__attribute__((const))
float clamp(const float num, const float min, const float max)
{
	const float result = num < min ? min : num;
	return result > max ? max : result;
}

void clamp3(const v3 vec, const v3 min, const v3 max, v3 result)
{
	result[X] = clamp(vec[X], min[X], max[X]);
	result[Y] = clamp(vec[Y], min[Y], max[Y]);
	result[Z] = clamp(vec[Z], min[Z], max[Z]);
}

__attribute__((const))
float magsqr3(const v3 vec)
{
	return sqr(vec[X]) + sqr(vec[Y]) + sqr(vec[Z]);
}

void mulmv(m3 mat, const float *restrict vec, float *restrict result)
{
	result[X] = dot3(mat[X], vec);
	result[Y] = dot3(mat[Y], vec);
	result[Z] = dot3(mat[Z], vec);
}

void mulms(m3 mat, const float mul, m3 result)
{
	mul3s(mat[X], mul, result[X]);
	mul3s(mat[Y], mul, result[Y]);
	mul3s(mat[Z], mul, result[Z]);
}
