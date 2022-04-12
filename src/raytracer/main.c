/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   main
 **/

#include <stdio.h>

#include "accel.h"
#include "argv.h"
#include "image.h"
#include "material.h"
#include "object.h"
#include "render.h"
#include "scene.h"
#include "system.h"

static const char *HELPTEXT =
	"Render a scene using raytracing.\n"
	"Copyright: (c) 2021-2022 Wojciech Graj\n"
	"License: https://opensource.org/licenses/MIT\n"
	"Features: "
#ifdef MULTITHREADING
	"Multithreading "
#endif
#ifdef UNBOUND_OBJECTS
	"Planes "
#endif
	"\n"
	"Usage: ./engine <input> <output> <resolution> [OPTIONAL_PARAMETERS]\n"
	"\n"
	"REQUIRED PARAMETERS:\n"
	"<input>      (string)            : .json scene file which will be used to generate the image. Example files can be found in ./scenes.\n"
	"<output>     (string)            : .tif file to which the image will be saved.\n"
	"<resolution> (integer) (integer) : resolution of the output image.\n"
	"OPTIONAL PARAMETERS:\n"
	"[-m] (integer | \"max\")           : DEFAULT = 1       : number of CPU cores\n"
	"[-b] (integer)                   : DEFAULT = 10      : maximum number of times that a light ray can bounce.\n"
	"[-a] (float)                     : DEFAULT = 0.01    : minimum light intensity for which a ray is cast.\n"
	"[-s] (\"phong\" | \"blinn\")         : DEFAULT = phong   : reflection model.\n"
	"[-n] (integer)                   : DEFAULT = 1       : number of samples which are rendered per pixel.\n"
	"[-r] (\"norm\" | float)            : DEFAULT = 1.0     : scene scaling factor.\n"
	"[-l] (\"none\" | \"lin\" | \"sqr\")    : DEFAULT = sqr     : light attenuation.\n"
	"[-p] (\"real\" | \"cpu\")            : DEFAULT = real    : time to print with status messages.\n"
	"[-g] (string)                    : DEFAULT = ambient : global illumination model.\n"
	"    ambient    : ambient lighting\n"
	"    path       : path-tracing\n"
	"[-f]                             : DEFAULT = OFF     : save raw output for post-processing.\n";

int main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	myargc = argc;
	myargv = argv;
	argv_init();

	if (argv_check("--help") || argv_check("-h")) {
		puts(HELPTEXT);
		return 0;
	} else if (argc < 5) {
		puts("Too few arguments. Use --help to find out which arguments are required to call this program.");
		return 1;
	}

	system_init();

	scene_load();
	image_init();

	accel_init();
	render_init();

	render();

	save_image();

	printf_log("Terminating.");
	accel_deinit();
	argv_deinit();
	image_deinit();
	materials_deinit();
	objects_deinit();
}
