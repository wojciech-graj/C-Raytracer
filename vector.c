#include "vector.h"

double sqr(double val)
{
    return val * val;
}

double magnitude2(Vec2 vec)
{
	return sqrt(sqr(vec[X]) + sqr(vec[Y]));
}

double magnitude3(Vec3 vec)
{
	return sqrt(sqr(vec[X]) + sqr(vec[Y]) + sqr(vec[Z]));
}

double dot2(Vec2 vec1, Vec2 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y];
}

double dot3(Vec3 vec1, Vec3 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y] + vec1[Z] * vec2[Z];
}

void cross(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
    result[Y] = vec1[Z] * vec2[X] - vec1[X] * vec2[Z];
    result[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];
}

void multiply2(Vec2 vec, double mul, Vec2 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
}

void multiply3(Vec3 vec, double mul, Vec3 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
	result[Z] = vec[Z] * mul;
}

void divide2(Vec2 vec, double div, Vec2 result)
{
	result[X] = vec[X] / div;
	result[Y] = vec[Y] / div;
}

void divide3(Vec3 vec, double div, Vec3 result)
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

void add3(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
	result[Z] = vec1[Z] + vec2[Z];
}

void subtract2(Vec2 vec1, Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
}

void subtract3(Vec3 vec1, Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
	result[Z] = vec1[Z] - vec2[Z];
}

void normalize2(Vec2 vec)
{
	multiply2(vec, magnitude2(vec), vec);
}

void normalize3(Vec3 vec)
{
	multiply3(vec, magnitude3(vec), vec);
}
