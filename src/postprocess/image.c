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
#include "error.h"
#include "mem.h"

#include <tiffio.h>

#define TIFFTAG_Z_BUFFER 65000

struct Image image;

void image_load(void)
{
	printf_log("Loading image.");

	TIFFSetWarningHandler(0);

	const char *filename = myargv[1];
	TIFF *tif = TIFFOpen(filename, "r");
	error_check(tif, "Failed to open input file [%s].", filename);

	uint16_t samples_per_pixel, bits_per_sample, planar_config, image_width, image_length;
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
	TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar_config);
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &image_width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_length);

	error_check(samples_per_pixel == 3, "Expected 3 samples per pixel but got [%u].", samples_per_pixel);
	error_check(bits_per_sample == 32, "Expected 32 bits per sample but got [%u].", bits_per_sample);
	error_check(planar_config == PLANARCONFIG_CONTIG, "Expected contiguous planar configuration");

	image.resolution[X] = image_width;
	image.resolution[Y] = image_length;
	image.pixels = image.resolution[X] * image.resolution[Y];

	image.raster = safe_malloc(image.pixels * sizeof(v3));

	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
	uint32_t row;
	for (row = 0; row < image.resolution[Y]; row++) {
		TIFFReadScanline(tif, buf, row, 0);
		memcpy(image.raster[row * image.resolution[X]], buf, TIFFScanlineSize(tif));
	}
	_TIFFfree(buf);

	image.z_buffer = safe_malloc(image.pixels * sizeof(float));
	uint32_t count;
	float *z_buffer;
	TIFFGetField(tif, TIFFTAG_Z_BUFFER, &count, &z_buffer);
	error_check(count == image.pixels, "Corrupted Z-Buffer.");
	memcpy(image.z_buffer, z_buffer, image.pixels * sizeof(float));

	TIFFClose(tif);
}

void image_deinit(void)
{
	free(image.raster);
	free(image.z_buffer);
}

void save_image(void)
{
	printf_log("Saving image.");

	TIFF *tif;
	const char *filename = myargv[2];
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

	TIFFClose(tif);
}
