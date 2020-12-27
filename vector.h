#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

enum directions{X, Y, Z};

typedef double Vec2[2];
typedef double Vec3[3];

double sqr(double val);
void cross(Vec3 vec1, Vec3 vec2, Vec3 result);
double magnitude2(Vec2 vec);
double magnitude3(Vec3 vec);
double dot2(Vec2 vec1, Vec2 vec2);
double dot3(Vec3 vec1, Vec3 vec2);
void multiply2(Vec2 vec, double mul, Vec2 result);
void multiply3(Vec3 vec, double mul, Vec3 result);
void multiply3v(Vec3 vec1, Vec3 vec2, Vec3 result);
void divide2(Vec2 vec, double div, Vec2 result);
void divide3(Vec3 vec, double div, Vec3 result);
void add2(Vec2 vec1, Vec2 vec2, Vec2 result);
void add3(Vec3 vec1, Vec3 vec2, Vec3 result);
void subtract2(Vec2 vec1, Vec2 vec2, Vec2 result);
void subtract3(Vec3 vec1, Vec3 vec2, Vec3 result);
void normalize2(Vec2 vec);
void normalize3(Vec3 vec);

#endif
