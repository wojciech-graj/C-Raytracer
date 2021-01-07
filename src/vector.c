#include "vector.h"

float sqr(float val)
{
    return val * val;
}

float magnitude2(Vec2 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]));
}

float magnitude3(Vec3 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]) + sqr(vec[Z]));
}

float dot2(Vec2 vec1, Vec2 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y];
}

float dot3(Vec3 vec1, Vec3 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y] + vec1[Z] * vec2[Z];
}

void cross(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
    result[Y] = vec1[Z] * vec2[X] - vec1[X] * vec2[Z];
    result[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];
}

void multiply2(Vec2 vec, float mul, Vec2 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
}

void multiply3(Vec3 vec, float mul, Vec3 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
	result[Z] = vec[Z] * mul;
}

void multiply3v(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] * vec2[X];
	result[Y] = vec1[Y] * vec2[Y];
	result[Z] = vec1[Z] * vec2[Z];
}

void divide2(Vec2 vec, float div, Vec2 result)
{
	result[X] = vec[X] / div;
	result[Y] = vec[Y] / div;
}

void divide3(Vec3 vec, float div, Vec3 result)
{
	result[X] = vec[X] / div;
	result[Y] = vec[Y] / div;
	result[Z] = vec[Z] / div;
}

void add2(Vec2 vec1, Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
}

void add2s(Vec2 vec1, float summand, Vec2 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
}

void add3(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
	result[Z] = vec1[Z] + vec2[Z];
}

void add3s(Vec3 vec1, float summand, Vec3 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
	result[Z] = vec1[Z] + summand;
}

void add3_3(Vec3 vec1, Vec3 vec2, Vec3 vec3, Vec3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void subtract2(Vec2 vec1, Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
}

void subtract2s(Vec2 vec1, float subtrahend, Vec2 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
}

void subtract3(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
	result[Z] = vec1[Z] - vec2[Z];
}

void subtract3s(Vec3 vec1, float subtrahend, Vec3 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
	result[Z] = vec1[Z] - subtrahend;
}

void normalize2(Vec2 vec)
{
	divide2(vec, magnitude2(vec), vec);
}

void normalize3(Vec3 vec)
{
	divide3(vec, magnitude3(vec), vec);
}
