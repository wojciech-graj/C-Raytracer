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

#include <tiffio.h>

#define TIFFTAG_Z_BUFFER 65000

void save_tiff_raw(TIFF *tif);
void save_tiff(TIFF *tif);

struct Image image;

void image_init(void)
{
	printf_log("Initializing image.");

	image.resolution[X] = abs(atoi(myargv[ARG_RESOLUTION_X]));
	image.resolution[Y] = abs(atoi(myargv[ARG_RESOLUTION_Y]));
	image.pixels = image.resolution[X] * image.resolution[Y];

	image.size[X] = 2 * camera.focal_length * tanf(camera.fov * PI / 360.f);
	image.size[Y] = image.size[X] * image.resolution[Y] / image.resolution[X];

	image.raster = safe_malloc(image.pixels * sizeof(v3));
	image.z_buffer = safe_malloc(image.pixels * sizeof(float));

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
	free(image.raster);
	free(image.z_buffer);
}

void save_tiff_raw(TIFF *tif)
{
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);

	static const TIFFFieldInfo xtiffFieldInfo[] = {
		{ TIFFTAG_Z_BUFFER, TIFF_VARIABLE, TIFF_VARIABLE, TIFF_FLOAT, FIELD_CUSTOM, true, true, "ZBuffer" },
	};

	TIFFMergeFieldInfo(tif, xtiffFieldInfo, arrlen(xtiffFieldInfo));
	TIFFSetField(tif, TIFFTAG_Z_BUFFER, image.pixels, image.z_buffer);

	tdata_t buf;
	tstrip_t strip;

	buf = _TIFFmalloc(TIFFStripSize(tif));
	for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++) {
		memcpy(buf, image.raster[strip * image.resolution[X]], TIFFStripSize(tif));
		TIFFWriteScanline(tif, buf, strip, 0);
	}

	_TIFFfree(buf);
}

void save_tiff(TIFF *tif)
{
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);

	uint8_t(*pixels)[3] = safe_malloc(image.resolution[X] * image.resolution[Y] * sizeof(uint8_t[3]));

	size_t i;
	for (i = 0; i < image.pixels; i++) {
		uint8_t *pixel = pixels[i];
		pixel[0] = (uint8_t)fmaxf(fminf(image.raster[i][0] * 255.f, 255.f), 0.f);
		pixel[1] = (uint8_t)fmaxf(fminf(image.raster[i][1] * 255.f, 255.f), 0.f);
		pixel[2] = (uint8_t)fmaxf(fminf(image.raster[i][2] * 255.f, 255.f), 0.f);
	}

	tdata_t buf;
	tstrip_t strip;

	buf = _TIFFmalloc(TIFFStripSize(tif));
	for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++) {
		memcpy(buf, pixels[strip * image.resolution[X]], TIFFStripSize(tif));
		TIFFWriteScanline(tif, buf, strip, 0);
	}

	_TIFFfree(buf);
	free(pixels);
}

void save_image(void)
{
	printf_log("Saving image.");

	TIFF *tif;
	char *filename = myargv[ARG_OUTPUT_FILENAME];
	if (unlikely(!strstr(filename, ".tif")))
		printf_log("Expected output file [%s] with extension .tif.", filename);
	tif = TIFFOpen(filename, "w");
	error_check(tif, "Failed to open output file [%s].", filename);

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, image.resolution[X]);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, image.resolution[Y]);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);

	if (argv_check("-f"))
		save_tiff_raw(tif);
	else
		save_tiff(tif);

	TIFFClose(tif);
}
