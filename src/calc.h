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

#ifndef __CALC_H__
#define __CALC_H__

#include "type.h"

#define SPHERICAL_TO_CARTESIAN(radius, inclination, azimuth)\
	{radius * cosf(azimuth) * sinf(inclination),\
	radius * sinf(azimuth) * sinf(inclination),\
	radius * cosf(inclination)}

float sqr(float val);
float mag2(const v2 vec);
float mag3(const v3 vec);
float dot2(const v2 vec1, const v2 vec2);
float dot3(const v3 vec1, const v3 vec2);
void cross(const v3 vec1, const v3 vec2, v3 result);
void mul2s(const v2 vec, float mul, v2 result);
void mul3s(const v3 vec, float mul, v3 result);
void mul3v(const v3 vec1, const v3 vec2, v3 result);
void inv3(v3 vec);
void add2v(const v2 vec1, const v2 vec2, v2 result);
void add2s(const v2 vec1, float summand, v2 result);
void add3v(const v3 vec1, const v3 vec2, v3 result);
void add3s(const v3 vec1, float summand, v3 result);
void add3v3(const v3 vec1, const v3 vec2, const v3 vec3, v3 result);
void sub2v(const v2 vec1, const v2 vec2, v2 result);
void sub2s(const v2 vec1, float subtrahend, v2 result);
void sub3v(const v3 vec1, const v3 vec2, v3 result);
void sub3s(const v3 vec1, float subtrahend, v3 result);
void norm2(v2 vec);
void norm3(v3 vec);
float max3(const v3 vec);
float min3(const v3 vec);
float clamp(float num, float min, float max);
void clamp3(const v3 vec, const v3 min, const v3 max, v3 result);
float magsqr3(const v3 vec);
void mulmv(m3 mat, const float *restrict vec, float *restrict result);
void mulms(m3 mat, float mul, m3 result);
void assign3(v3 dest, const v3 src);
void assignm(m3 dest, m3 src);

#endif /* __CALC_H__ */
