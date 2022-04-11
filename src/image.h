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

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "type.h"

typedef uint8_t Color[3];

struct Image {
	uint32_t resolution[2];
	v2 size;
	v3 corner; //Top left corner of image
	v3 vectors[2]; //Vectors for image plane traversal by 1 pixel in X and Y directions
	Color *pixels;
};

void image_init(void);
void image_deinit(void);

void save_image(void);

extern struct Image image;

#endif /* __IMAGE_H__ */
