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

#include "argv.h"
#include "image.h"
#include "postproc.h"
#include "system.h"

static const char *HELPTEXT =
	"Apply post-processing effects to raytraced image.\n"
	"Copyright: (c) 2021-2022 Wojciech Graj\n"
	"License: https://opensource.org/licenses/MIT\n"
	"\n"
	"Usage: ./postprocess <input> <output> [OPTIONAL_PARAMETERS]\n"
	"\n"
	"REQUIRED PARAMETERS:\n"
	"<input>      (string)            : .tif raw file created by raytracer.\n"
	"<output>     (string)            : .tif file to which the image will be saved.\n"
	"OPTIONAL PARAMETERS:\n"
	"[-b] (float)                     : DEFAULT = 1.0     : brighten.\n"
	"[--dof] <scale> <bias>           : Apply depth of field effect (incompatible with --dof-camera).\n"
	"  <scale> (float)                : \n"
	"  <bias> (float)                 : \n"
	"[--dof-camera] <aperture> <focal length> <plane in focus> : Apply depth of field effect (incompatible with --dof).\n"
	"  <aperture> (float)             : \n"
	"  <focal length> (float)         : \n"
	"  <plane in focus> (float)       : \n";

int main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	myargc = argc;
	myargv = argv;
	argv_init();

	if (argv_check("--help") || argv_check("-h")) {
		puts(HELPTEXT);
		return 0;
	} else if (argc < 3) {
		puts("Too few arguments. Use --help to find out which arguments are required to call this program.");
		return 1;
	}

	system_init();

	image_load();

	postprocess();

	save_image();

	printf_log("Terminating.");
	argv_deinit();
	image_deinit();
}
