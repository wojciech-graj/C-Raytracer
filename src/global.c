#include "global.h"

const char *ERR_FORMAT_STRING = "ERROR:%s\n";
const char *ERR_MESSAGES[ERR_END] = {
	[ERR_ARGC] = 			"Too few arguments. Use --help to find out which arguments are required to call this program.",
	[ERR_ARGV_FILE_EXT] = 		"ARGV: Output file must have the .ppm extension.",
	[ERR_ARGV_NUM_THREADS] = 	"ARGV: Specified number of threads is greater than the available number of threads.",
	[ERR_ARGV_MULTITHREADING] = 	"ARGV: Multithreading is disabled. To enable it, recompile the program with the -DMULTITHREADING parameter.",
	[ERR_ARGV_REFLECTION] = 	"ARGV: Unrecognized reflection model.",
	[ERR_ARGV_FOV] = 		"ARGV: FOV is outside of interval (0, 180).",
	[ERR_ARGV_UNRECOGNIZED] = 	"ARGV: Unrecognized argument. Use --help to find out which arguments can be used.",
	[ERR_ARGV_IO_OPEN_SCENE] = 	"ARGV:I/O : Unable to open scene file.",
	[ERR_ARGV_IO_OPEN_OUTPUT] = 	"ARGV:I/O : Unable to open output file.",
	[ERR_IO_WRITE_IMG] = 		"I/O : Unable to write to image file.",
	[ERR_JSON_KEY_NOT_STRING] = 	"JSON: Key is not a string.",
	[ERR_JSON_UNRECOGNIZED_KEY] = 	"JSON: Unrecognized key.",
	[ERR_JSON_VALUE_NOT_FLOAT] = 	"JSON: Value is not a float.",
	[ERR_JSON_VALUE_NOT_ARRAY] = 	"JSON: Value is not an array.",
	[ERR_JSON_ARRAY_SIZE] = 	"JSON: Array contains an incorrect amount of values.",
	[ERR_JSON_FILENAME_NOT_STRING] ="JSON: Filename is not string.",
	[ERR_JSON_IO_OPEN] = 		"JSON:I/O : Unable to open file specified in JSON file.",
	[ERR_JSON_ARGC] = 		"JSON: Object has an incorrect number of elements.",
	[ERR_JSON_UNRECOGNIZED] = 	"JSON: Unrecognized element in Object.",
	[ERR_JSON_IO_READ] = 		"JSON:I/O : Unable to read file.",
	[ERR_JSON_NUM_TOKENS] = 	"JSON: Too many tokens.",
	[ERR_JSON_READ_TOKENS] = 	"JSON: Unable to read correct amount of tokens.",
	[ERR_JSON_FIRST_TOKEN] = 	"JSON: First token is not Object.",
	[ERR_JSON_ARGC_SCENE] = 	"JSON: Unrecognized Object in scene.",
	[ERR_MALLOC] = 			"MEM : Unable to allocate memory on heap.",
	[ERR_JSON_NO_CAMERA] = 		"JSON: Unable to find camera in scene.",
	[ERR_JSON_NO_OBJECTS] = 	"JSON: Unable to find objects in scene.",
	[ERR_JSON_NO_LIGHTS] = 		"JSON: Unable to find lights in scene.",
	[ERR_STL_IO_FP] = 		"STL :I/O : Unable to move file pointer.",
	[ERR_STL_IO_READ] = 		"STL :I/O : Unable to read file.",
	[ERR_STL_ENCODING] = 		"STL : File uses ASCII encoding.",
};
const char *HELPTEXT = "\
Renders a scene using raytracing.\n\
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
