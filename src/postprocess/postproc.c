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

#include <stdlib.h>

#include "argv.h"
#include "calc.h"
#include "image.h"
#include "system.h"
#include "type.h"

void brighten(float factor);

void postprocess(void)
{
	printf_log("Commencing Postprocessing");

	int idx;
	idx = argv_check_with_args("-b", 1);
	if (idx) {
		float factor = atof(myargv[idx + 1]);
		brighten(factor);
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
