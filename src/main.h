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
	printf(format, (current_t.tv_sec - start_t.tv_sec) + (current_t.tv_nsec - start_t.tv_nsec) * 1e-9f);
#else
#define PRINT_TIME(format)
#endif

#ifdef MULTITHREADING
#include <omp.h>
int NUM_THREADS = 1;
#endif

enum reflection_models{REFLECTION_PHONG, REFLECTION_BLINN};

Vec3 ambient_light_intensity = {0.f, 0.f, 0.f};
int max_bounces = 10;
float minimum_light_intensity_sqr = 0.01 * 0.01;
int reflection_model = REFLECTION_PHONG;

const char *HELPTEXT = \
"Renders a scene using raytracing.\n\
\n\
REQUIRED PARAMS:\n\
--file       [-f] (string)            : specifies the scene file which will be used to generate the image. Example files can be found in scenes/\n\
--output     [-o] (string)            : specifies the file to which the image will be saved. Must end in .ppm\n\
--resolution [-r] (integer) (integer) : specifies the resolution of the output image\n\
OPTIONAL PARAMS:\n\
-m (integer)   : DEFAULT = 1     : specifies the number of CPU cores\n\
    Accepted values:\n\
    (integer)  : allocates (integer) amount of CPU cores\n\
    max        : allocates the maximum number of cores\n\
-b (integer)   : DEFAULT = 10    : specifies the maximum number of times that a light ray can bounce. Large values: (integer) > 100 may cause stack overflows\n\
-a (decimal)   : DEFAULT = 0.01  : specifies the minimum light intensity for which a ray is cast\n\
-s (string)    : DEFAULT = phong : specifies the reflection model\n\
    Accepted values:\n\
    phong      : phong reflection model\n\
    blinn      : blinn-phong reflection model\n\
-fov (integer) : DEFAULT = 90    : specifies the angle of the horizontal field of view in degress. Must be in range: 0 < (integer) < 180\n";

#endif
