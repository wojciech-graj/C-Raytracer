#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

enum directions{X, Y, Z};

typedef float Vec2[2];
typedef float Vec3[3];

float sqr(float val);
void cross(Vec3 vec1, Vec3 vec2, Vec3 result);
float magnitude2(Vec2 vec);
float magnitude3(Vec3 vec);
float dot2(Vec2 vec1, Vec2 vec2);
float dot3(Vec3 vec1, Vec3 vec2);
void multiply2(Vec2 vec, float mul, Vec2 result);
void multiply3(Vec3 vec, float mul, Vec3 result);
void multiply3v(Vec3 vec1, Vec3 vec2, Vec3 result);
void divide2(Vec2 vec, float div, Vec2 result);
void divide3(Vec3 vec, float div, Vec3 result);
void add2(Vec2 vec1, Vec2 vec2, Vec2 result);
void add2s(Vec2 vec1, float summand, Vec2 result);
void add3(Vec3 vec1, Vec3 vec2, Vec3 result);
void add3s(Vec3 vec1, float summand, Vec3 result);
void add3_3(Vec3 vec1, Vec3 vec2, Vec3 vec3, Vec3 result);
void subtract2(Vec2 vec1, Vec2 vec2, Vec2 result);
void subtract2s(Vec2 vec1, float subtrahend, Vec2 result);
void subtract3(Vec3 vec1, Vec3 vec2, Vec3 result);
void subtract3s(Vec3 vec1, float subtrahend, Vec3 result);
void normalize2(Vec2 vec);
void normalize3(Vec3 vec);

#endif
