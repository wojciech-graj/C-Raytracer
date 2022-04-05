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

#define SPHERICAL_TO_CARTESIAN(radius, inclination, azimuth)\
	{radius * cosf(azimuth) * sinf(inclination),\
	radius * sinf(azimuth) * sinf(inclination),\
	radius * cosf(inclination)}

typedef float Vec2[2];
typedef float Vec3[3];
typedef Vec3 Mat3[3];

float sqr(float val);
float mag2(const Vec2 vec);
float mag3(const Vec3 vec);
float dot2(const Vec2 vec1, const Vec2 vec2);
float dot3(const Vec3 vec1, const Vec3 vec2);
void cross(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void mul2s(const Vec2 vec, float mul, Vec2 result);
void mul3s(const Vec3 vec, float mul, Vec3 result);
void mul3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void inv3(Vec3 vec);
void add2v(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void add2s(const Vec2 vec1, float summand, Vec2 result);
void add3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void add3s(const Vec3 vec1, float summand, Vec3 result);
void add3v3(const Vec3 vec1, const Vec3 vec2, const Vec3 vec3, Vec3 result);
void sub2v(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void sub2s(const Vec2 vec1, float subtrahend, Vec2 result);
void sub3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void sub3s(const Vec3 vec1, float subtrahend, Vec3 result);
void norm2(Vec2 vec);
void norm3(Vec3 vec);
float max3(const Vec3 vec);
float min3(const Vec3 vec);
float clamp(float num, float min, float max);
void clamp3(const Vec3 vec, const Vec3 min, const Vec3 max, Vec3 result);
float magsqr3(const Vec3 vec);
void mulm3(Mat3 mat, const float *restrict vec, float *restrict result);
void mulms(Mat3 mat, float mul, Mat3 result);

#endif /* __CALC_H__ */
