/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Postprocessing operations
 **/

#include "postproc.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "argv.h"
#include "calc.h"
#include "image.h"
#include "mem.h"
#include "system.h"
#include "type.h"

void brighten(float factor);
void depth_of_field(float scale, float bias);

void postprocess(void)
{
	printf_log("Commencing Postprocessing");

	int idx;
	idx = argv_check_with_args("-b", 1);
	if (idx) {
		float factor = atof(myargv[idx + 1]);
		brighten(factor);
	}

	idx = argv_check_with_args("--dof", 2);
	if (idx) {
		float scale = atof(myargv[idx + 1]);
		float bias = atof(myargv[idx + 2]);
		depth_of_field(scale, bias);
	} else if ((idx = argv_check_with_args("--dof-camera", 3))) {
		float aperture = atof(myargv[idx + 1]);
		float focal_length = atof(myargv[idx + 2]);
		float plane_in_focus = atof(myargv[idx + 3]);

		float z_min = FLT_MAX, z_max = FLT_MIN;
		size_t i;
		for (i = 0; i < image.pixels; i++) {
			if (image.z_buffer[i] < z_min)
				z_min = image.z_buffer[i];
			if (image.z_buffer[i] > z_max)
				z_max = image.z_buffer[i];
		}

		float scale = (aperture * focal_length * plane_in_focus * (z_max - z_min)) / ((plane_in_focus - focal_length) * z_min * z_max);
		float bias = (aperture * focal_length * (z_min - plane_in_focus)) / ((plane_in_focus * focal_length) * z_min);
		depth_of_field(scale, bias);
	}
}

void brighten(float factor)
{
	printf_log("Brightening by factor %f.", (double)factor);

	uint32_t i;
	for (i = 0; i < image.pixels; i++) {
		mul3s(image.raster[i], factor, image.raster[i]);
	}
}

void depth_of_field(float scale, float bias)
{
	printf_log("Applying depth of field with scale [%f] and bias [%f].", (double)scale, (double)bias);

	v3 *raster = safe_calloc(image.pixels, sizeof(v3));
	float *alpha_buffer = safe_calloc(image.pixels, sizeof(float));

	size_t i;
	for (i = 0; i < image.pixels; i++) {
		const float depth = image.z_buffer[i];
		const float coc = fabsf(depth * scale + bias);
		const int radius = (int)(coc * .5f);
		const int radius_sqr = radius * radius;
		const float alpha = MIN(1.f / radius_sqr, 1.f);
		v3 pix_val;
		mul3s(image.raster[i], alpha, pix_val);

		const int cx = i % image.resolution[X];
		const int cy = i / image.resolution[X];
		/* clang-format off */
		if (unlikely(cx - radius < 0
			|| cx + radius >= (int)image.resolution[X]
			|| cy - radius < 0
			|| cy + radius >= (int)image.resolution[Y])) { /* Need bounds checking */
			/* clang-format on */
			int x, y;
			for (x = -MIN(cx, radius); x <= MIN((int)image.resolution[X] - cx - 1, radius); x++) {
				const int hh = (int)sqrtf(radius_sqr - x * x);
				const int rx = cx + x;
				for (y = MAX(cy - hh, 0); y <= MIN(cy + hh, (int)image.resolution[Y] - 1); y++) {
					const int idx = rx + y * image.resolution[X];
					if (depth <= image.z_buffer[idx]) {
						add3v(raster[idx], pix_val, raster[idx]);
						alpha_buffer[idx] += alpha;
					}
				}
			}
		} else {
			int x, y;
			for (x = -radius; x <= radius; x++) {
				const int hh = (int)sqrtf(radius_sqr - x * x);
				const int rx = cx + x;
				for (y = cy - hh; y <= cy + hh; y++) {
					const int idx = rx + y * image.resolution[X];
					if (depth <= image.z_buffer[idx]) {
						add3v(raster[idx], pix_val, raster[idx]);
						alpha_buffer[idx] += alpha;
					}
				}
			}
		}
	}

	free(image.raster);
	image.raster = raster;

	for (i = 0; i < image.pixels; i++) /* Normalize */
		mul3s(image.raster[i], 1.f / alpha_buffer[i], image.raster[i]);

	free(alpha_buffer);
}