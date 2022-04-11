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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "argv.h"
#include "calc.h"
#include "camera.h"
#include "error.h"
#include "mem.h"

struct Image image;

void image_init(void)
{
	printf_log("Initializing image.");

	image.resolution[X] = abs(atoi(myargv[ARG_RESOLUTION_X]));
	image.resolution[Y] = abs(atoi(myargv[ARG_RESOLUTION_Y]));

	image.size[X] = 2 * camera.focal_length * tanf(camera.fov * PI / 360.f);
	image.size[Y] = image.size[X] * image.resolution[Y] / image.resolution[X];
	image.pixels = safe_malloc(image.resolution[X] * image.resolution[Y] * sizeof(Color));

	v3 focal_vector, plane_center, corner_offset_vectors[2];
	mul3s(camera.vectors[2], camera.focal_length, focal_vector);
	add3v(focal_vector, camera.position, plane_center);
	mul3s(camera.vectors[0], image.size[X] / image.resolution[X], image.vectors[0]);
	mul3s(camera.vectors[1], image.size[Y] / image.resolution[Y], image.vectors[1]);
	mul3s(image.vectors[X], .5f - image.resolution[X] / 2.f, corner_offset_vectors[X]);
	mul3s(image.vectors[Y], .5f - image.resolution[Y] / 2.f, corner_offset_vectors[Y]);
	add3v3(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], image.corner);
}

void image_deinit(void)
{
	free(image.pixels);
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

	size_t nmemb_written = fprintf(file, "P6\n%u %u\n255\n", image.resolution[X], image.resolution[Y]);
	error_check(nmemb_written > 0, "Failed to write header to output file [%s].", myargv[2]);
	size_t num_pixels = image.resolution[X] * image.resolution[Y];
	nmemb_written = fwrite(image.pixels, sizeof(Color), num_pixels, file);
	error_check(nmemb_written == num_pixels, "Failed to write pixels to output file [%s].", myargv[2]);

	fclose(file);
}
