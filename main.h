#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <assert.h>

#include <omp.h>

#include "global.h"
#include "vector.h"
#include "objects.h"
#include "stl.h"
#include "algorithm.h"

#define NUM_THREADS 12

typedef uint8_t Color[3];
typedef struct Image Image;
typedef struct Camera Camera;
typedef struct Light Light;

Color background_color = {0, 0, 0};
Vec3 ambient_light_intensity = {.1, .1, .1};

typedef struct Image {
	int resolution[2];
	Vec2 size;
	Vec3 corner; //Top left corner of image
	Vec3 vectors[2]; //Vectors for image plane traversal by 1 pixel in X and Y directions
	Color *pixels;
} Image;

typedef struct Camera {
	Vec3 position;
	Vec3 vectors[3]; //vectors are perpendicular to eachother and normalized. vectors[3] is normal to projection_plane.
	float focal_length;
	Image image;
} Camera;

typedef struct Light {
	Vec3 position;
	Vec3 intensity;
} Light;

#endif
