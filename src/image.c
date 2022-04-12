/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Image parameters and frame buffer
 **/

#include "image.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "argv.h"
#include "calc.h"
#include "camera.h"
#include "error.h"
#include "mem.h"

struct Image image;

static bool normalize_colors = false;
static float brightness = 1.f;

void image_init(void)
{
	printf_log("Initializing image.");

	image.resolution[X] = abs(atoi(myargv[ARG_RESOLUTION_X]));
	image.resolution[Y] = abs(atoi(myargv[ARG_RESOLUTION_Y]));
	image.pixels = image.resolution[X] * image.resolution[Y];

	image.size[X] = 2 * camera.focal_length * tanf(camera.fov * PI / 360.f);
	image.size[Y] = image.size[X] * image.resolution[Y] / image.resolution[X];
	image.raster = safe_malloc(image.pixels * sizeof(v3));

	v3 focal_vector, plane_center, corner_offset_vectors[2];
	mul3s(camera.vectors[2], camera.focal_length, focal_vector);
	add3v(focal_vector, camera.position, plane_center);
	mul3s(camera.vectors[0], image.size[X] / image.resolution[X], image.vectors[0]);
	mul3s(camera.vectors[1], image.size[Y] / image.resolution[Y], image.vectors[1]);
	mul3s(image.vectors[X], .5f - image.resolution[X] / 2.f, corner_offset_vectors[X]);
	mul3s(image.vectors[Y], .5f - image.resolution[Y] / 2.f, corner_offset_vectors[Y]);
	add3v3(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], image.corner);

	int idx;
	idx = argv_check("-c");
	if (idx)
		normalize_colors = true;

	idx = argv_check_with_args("-i", 1);
	if (idx)
		brightness = atof(myargv[idx + 1]);
}

void image_deinit(void)
{
	free(image.raster);
}

void image_postprocess(void)
{
	printf_log("Postprocessing.");
	size_t i;
	float slope = brightness;
	if (normalize_colors) {
		float min = FLT_MAX, max = FLT_MIN;
		for (i = 0; i < image.pixels; i++) {
			float *raw_pixel = image.raster[i];
			float pixel_min = min3(raw_pixel), pixel_max = max3(raw_pixel);
			if (pixel_min < min)
				min = pixel_min;
			if (pixel_max > max)
				max = pixel_max;
		}

		slope /= (max - min);

		for (i = 0; i < image.pixels; i++) {
			sub3s(image.raster[i], min, image.raster[i]);
		}
	}

	if (slope != 1.f) {
		for (i = 0; i < image.pixels; i++) {
			mul3s(image.raster[i], slope, image.raster[i]);
		}
	}
}

void save_image(void)
{
	printf_log("Saving image.");

	const char *filename = myargv[ARG_OUTPUT_FILENAME];
	if (!strstr(filename, ".ppm")) {
		error("Expected output file [%s] with extension .ppm.", filename);
	}
	FILE *file = fopen(filename, "wb");
	error_check(file, "Failed to open output file [%s].", filename);

	uint8_t(*pixels)[3] = safe_malloc(image.resolution[X] * image.resolution[Y] * sizeof(uint8_t[3]));

	size_t i;
	for (i = 0; i < image.pixels; i++) {
		uint8_t *pixel = pixels[i];
		pixel[0] = (uint8_t)fmaxf(fminf(image.raster[i][0] * 255.f, 255.f), 0.f);
		pixel[1] = (uint8_t)fmaxf(fminf(image.raster[i][1] * 255.f, 255.f), 0.f);
		pixel[2] = (uint8_t)fmaxf(fminf(image.raster[i][2] * 255.f, 255.f), 0.f);
	}

	size_t nmemb_written = fprintf(file, "P6\n%u %u\n255\n", image.resolution[X], image.resolution[Y]);
	error_check(nmemb_written > 0, "Failed to write header to output file [%s].", myargv[2]);
	nmemb_written = fwrite(pixels, sizeof(Color), image.pixels, file);
	error_check(nmemb_written == image.pixels, "Failed to write pixels to output file [%s].", myargv[2]);

	fclose(file);
	free(pixels);
}
