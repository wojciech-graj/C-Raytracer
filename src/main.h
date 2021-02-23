#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>

#include "global.h"
#include "vector.h"
#include "objects.h"
#include "stl.h"
#include "algorithm.h"
#include "scene.h"

#ifdef DISPLAY_TIME
#include <time.h>
#define PRINT_TIME(format)\
	timespec_get(&current_t, TIME_UTC);\
	printf(format, (double) ((current_t.tv_sec - start_t.tv_sec) + (current_t.tv_nsec - start_t.tv_nsec) * 1e-9f));
#else
#define PRINT_TIME(format)
#endif

#ifdef MULTITHREADING
#include <omp.h>
unsigned NUM_THREADS = 1;
#endif

enum reflection_models{REFLECTION_PHONG, REFLECTION_BLINN};

Vec3 ambient_light_intensity = {0.f, 0.f, 0.f};
unsigned max_bounces = 10;
float minimum_light_intensity_sqr = 0.01 * 0.01;
unsigned reflection_model = REFLECTION_PHONG;

#endif
