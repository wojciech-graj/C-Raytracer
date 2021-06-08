/*
* Author: Wojciech Graj
* License: MIT
* Description: A simple raytracer in C
* Libraries used:
* 	cJSON (https://github.com/DaveGamble/cJSON)
*	SimplexNoise (https://github.com/SRombauts/SimplexNoise)
* Setting macros:
* 	MULTITHREADING - enables multithreading
* 	UNBOUND_OBJECTS - allows for unbound objects such as planes
*
* Certain sections of code adapted from:
* 	https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/
* 	http://people.csail.mit.edu/amy/papers/box-jgt.pdf
*/

/*
* TODO:
*	Lighting:
*		Treat emittant objects as lights
*		Remove point lights
*		Normalization functions
*	Textures:
*		More procedural textures
*	Optimization:
*		Consider LOD
*	Other:
* 		Denoising
*		Documentation
*		Port scenes to new format
*		Update readme
*/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lib/cJSON.h"
#include "../lib/SimplexNoise.h"

/*******************************************************************************
*	MACRO
*******************************************************************************/

#ifdef MULTITHREADING
#include <omp.h>
uint32_t NUM_THREADS = 1;
#endif

#define clz __builtin_clz

#define PI 3.1415927f
#define MATERIAL_THRESHOLD 1e-6f

/* ERROR */
#define err_assert(expr, err_code)\
	do {\
		if(!(expr))\
			err(err_code);\
	} while(false)

/* Object */
#define NUM_OBJECT_MEMBERS 2

/*******************************************************************************
*	TYPE DEFINITION
*******************************************************************************/

typedef uint8_t Color[3];
typedef float Vec2[2];
typedef float Vec3[3];
typedef Vec3 Mat3[3];

typedef struct Context Context;
typedef struct Line Line;

/* Camera */
typedef struct Image Image;
typedef struct Camera Camera;

/* Light */
typedef struct Light Light;

/* BoundingCuboid */
typedef struct BoundingCuboid BoundingCuboid;

/* Material */
typedef struct Material Material;
typedef struct Texture Texture;
typedef struct TextureUniform TextureUniform;
typedef struct TextureCheckerboard TextureCheckerboard;
typedef struct TextureBrick TextureBrick;
typedef struct TextureNoisySin TextureNoisySin;

/* Object */
typedef struct Object Object;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
#ifdef UNBOUND_OBJECTS
typedef struct Plane Plane;
#endif
typedef struct ObjectData ObjectData;

/* BVH */
typedef struct BVH BVH;
typedef union BVHChild BVHChild;
typedef struct BVHWithMorton BVHWithMorton;

/*******************************************************************************
*	GLOBAL
*******************************************************************************/

enum directions{
	X = 0,
	Y,
	Z,
};

typedef enum {
	REFLECTION_PHONG,
	REFLECTION_BLINN,
} ReflectionModel;

typedef enum {
	GLOBAL_ILLUMINATION_AMBIENT,
	GLOBAL_ILLUMINATION_PATH_TRACING,
} GlobalIlluminationModel;

typedef enum {
	LIGHT_ATTENUATION_NONE,
	LIGHT_ATTENUATION_LINEAR,
	LIGHT_ATTENUATION_SQUARE,
} LightAttenuation;

typedef enum {
	LOG_NONE,
	LOG_REALTIME,
	LOG_CPUTIME,
} LogOption;

/* Object */
typedef enum {
#ifdef UNBOUND_OBJECTS
	OBJECT_PLANE,
#endif
	OBJECT_SPHERE,
	OBJECT_TRIANGLE,
} ObjectType;

/* ERROR */
typedef enum {
	ERR_ARGC,
	ERR_ARGV_FILE_EXT,
	ERR_ARGV_NUM_THREADS,
	ERR_ARGV_MULTITHREADING,
	ERR_ARGV_REFLECTION,
	ERR_ARGV_GLOBAL_ILLUMINATION,
	ERR_ARGV_LIGHT_ATTENUATION,
	ERR_ARGV_UNRECOGNIZED,
	ERR_ARGV_IO_OPEN_SCENE,
	ERR_ARGV_IO_OPEN_OUTPUT,
	ERR_IO_WRITE_IMG,
	ERR_JSON_KEY_NOT_STRING,
	ERR_JSON_UNRECOGNIZED_KEY,
	ERR_JSON_VALUE_NOT_NUMBER,
	ERR_JSON_VALUE_NOT_ARRAY,
	ERR_JSON_VALUE_NOT_STRING,
	ERR_JSON_VALUE_NOT_OBJECT,
	ERR_JSON_ARRAY_SIZE,
	ERR_JSON_FILENAME_NOT_STRING,
	ERR_JSON_IO_OPEN,
	ERR_JSON_ARGC,
	ERR_JSON_UNRECOGNIZED,
	ERR_JSON_IO_READ,
	ERR_JSON_NUM_TOKENS,
	ERR_JSON_READ_TOKENS,
	ERR_JSON_FIRST_TOKEN,
	ERR_JSON_ARGC_SCENE,
	ERR_JSON_CAMERA_FOV,
	ERR_JSON_INVALID_MATERIAL,
	ERR_MALLOC,
	ERR_JSON_NO_CAMERA,
	ERR_JSON_NO_OBJECTS,
	ERR_JSON_NO_MATERIALS,
	ERR_JSON_NO_LIGHTS,
	ERR_STL_IO_FP,
	ERR_STL_IO_READ,
	ERR_STL_ENCODING,
	ERR_END, /* Used for determining number of error codes */
} ErrorCode;
const char *ERR_FORMAT_STRING = "ERROR:%s\n";
const char *ERR_MESSAGES[ERR_END] = {
	[ERR_ARGC] = 			"Too few arguments. Use --help to find out which arguments are required to call this program.",
	[ERR_ARGV_FILE_EXT] = 		"ARGV: Output file must have the .ppm extension.",
	[ERR_ARGV_NUM_THREADS] = 	"ARGV: Specified number of threads is greater than the available number of threads.",
	[ERR_ARGV_MULTITHREADING] = 	"ARGV: Multithreading is disabled. To enable it, recompile the program with the -DMULTITHREADING parameter.",
	[ERR_ARGV_REFLECTION] = 	"ARGV: Unrecognized reflection model.",
	[ERR_ARGV_GLOBAL_ILLUMINATION] ="ARGV: Unrecognized global illumination model.",
	[ERR_ARGV_LIGHT_ATTENUATION] =  "ARGV: Unrecognized light attenuation.",
	[ERR_ARGV_UNRECOGNIZED] = 	"ARGV: Unrecognized argument. Use --help to find out which arguments can be used.",
	[ERR_ARGV_IO_OPEN_SCENE] = 	"ARGV:I/O : Unable to open scene file.",
	[ERR_ARGV_IO_OPEN_OUTPUT] = 	"ARGV:I/O : Unable to open output file.",
	[ERR_IO_WRITE_IMG] = 		"I/O : Unable to write to image file.",
	[ERR_JSON_KEY_NOT_STRING] = 	"JSON: Key is not a string.",
	[ERR_JSON_UNRECOGNIZED_KEY] = 	"JSON: Unrecognized key.",
	[ERR_JSON_VALUE_NOT_NUMBER] = 	"JSON: Value is not a number.",
	[ERR_JSON_VALUE_NOT_ARRAY] = 	"JSON: Value is not an array.",
	[ERR_JSON_VALUE_NOT_STRING] = 	"JSON: Value is not a string.",
	[ERR_JSON_VALUE_NOT_OBJECT] = 	"JSON: Value is not an object.",
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
	[ERR_JSON_CAMERA_FOV] = 	"JSON: Camera FOV is outside of interval (0, 180).",
	[ERR_JSON_INVALID_MATERIAL] = 	"JSON: Material with stated id does not exist.",
	[ERR_MALLOC] = 			"MEM : Unable to allocate memory on heap.",
	[ERR_JSON_NO_CAMERA] = 		"JSON: Unable to find camera in scene.",
	[ERR_JSON_NO_OBJECTS] = 	"JSON: Unable to find objects in scene.",
	[ERR_JSON_NO_LIGHTS] = 		"JSON: Unable to find lights in scene.",
	[ERR_JSON_NO_MATERIALS] = 	"JSON: Unable to find materials in scene.",
	[ERR_STL_IO_FP] = 		"STL :I/O : Unable to move file pointer.",
	[ERR_STL_IO_READ] = 		"STL :I/O : Unable to read file.",
	[ERR_STL_ENCODING] = 		"STL : File uses ASCII encoding.",
};
const char *HELPTEXT = "\
Renders a scene using raytracing.\n\
\n\
./engine <input> <output> <resolution> [OPTIONAL_PARAMETERS]\n\
\n\
REQUIRED PARAMETERS:\n\
<input>      (string)            : .json scene file which will be used to generate the image. Example files can be found in scenes.\n\
<output>     (string)            : .ppm file to which the image will be saved.\n\
<resolution> (integer) (integer) : resolution of the output image.\n\
OPTIONAL PARAMETERS:\n\
[-m] (integer | \"max\")           : DEFAULT = 1       : number of CPU cores\n\
[-b] (integer)                   : DEFAULT = 10      : maximum number of times that a light ray can bounce.\n\
[-a] (float)                     : DEFAULT = 0.01    : minimum light intensity for which a ray is cast\n\
[-s] (\"phong\" | \"blinn\")         : DEFAULT = phong   : reflection model\n\
[-n] (integer)                   : DEFAULT = 1       : number of samples which are rendered per pixel\n\
[-c]                             : DEFAULT = OFF     : normalize values of pixels so that entire color spectrum is utilized\n\
[-l] (\"none\" | \"lin\" | \"sqr\")    : DEFAULT = sqr     : light attenuation\n\
[-p] (\"none\" | \"real\" | \"cpu\")   : DEFAULT = real    : time to print with status messages\n\
[-g] (string)                    : DEFAULT = ambient : global illumination model\n\
    ambient    : ambient lighting\n\
    path       : path-tracing\n";

/*******************************************************************************
*	STRUCTURE DEFINITION
*******************************************************************************/

typedef struct Context {
	FILE *scene_file;
	FILE *output_file;

	Object **objects;
	size_t num_objects;
	size_t objects_size; //Stores capacity of objects. After objects are loaded, should be equal to num_objects
#ifdef UNBOUND_OBJECTS
	Object **unbound_objects; //Since planes do not have a bounding cuboid and cannot be included in the BVH, they must be looked at separately
	size_t num_unbound_objects;
#endif
	Light *lights;
	size_t num_lights;
	Material *materials;
	size_t num_materials;
	Camera *camera;
	BVH *bvh;

	struct timespec start_timespec;
	clock_t start_clock;

	Vec3 global_ambient_light_intensity;
	uint32_t max_bounces;
	float minimum_light_intensity_sqr;
	ReflectionModel reflection_model;
	GlobalIlluminationModel global_illumination_model;
	LogOption log_option;
	uint32_t resolution[2];
	size_t samples_per_pixel;
	bool normalize_colors;
	LightAttenuation light_attenuation;
} Context;

typedef struct Line {
	float vector[3];
	float position[3];
} Line;

typedef struct  STLTriangle {
	float normal[3]; //normal is unreliable so it is not used.
	float vertices[3][3];
	uint16_t attribute_bytes; //attribute bytes is unreliable so it is not used.
} __attribute__ ((packed)) STLTriangle;

/* Camera */
typedef struct Image {
	uint32_t resolution[2];
	Vec2 size;
	Vec3 corner; //Top left corner of image
	Vec3 vectors[2]; //Vectors for image plane traversal by 1 pixel in X and Y directions
	Color *pixels;
} Image;

typedef struct Camera {
	Vec3 position;
	Vec3 vectors[3]; //vectors are perpendicular to eachother and normalized. vectors[3] is normal to projection_plane.
	float fov;
	float focal_length;
	Image image;
} Camera;

/* Light */
typedef struct Light {
	Vec3 position;
	Vec3 intensity;
} Light;

/* BoundingCuboid */
typedef struct BoundingCuboid {
	float epsilon;
	Vec3 corners[2];
} BoundingCuboid;

/* Material */
typedef struct Material {
	int32_t id;
	Vec3 ks;  /*specular reflection constant*/
	Vec3 ka; /*ambient reflection constant*/
	Vec3 kr; /*specular interreflection constant*/
	Vec3 kt; /*transparency constant*/
	Vec3 ke; /*emittance constant*/
	float shininess; /*shininess constant*/
	float refractive_index;
	Texture *texture;
	bool reflective;
	bool transparent;
} Material;

typedef struct Texture {
	void (*get_color)(const Texture*, const Vec3, Vec3);
} Texture;

typedef struct TextureUniform {
	Texture texture;
	Vec3 color;
} TextureUniform;

typedef struct TextureCheckerboard {
	Texture texture;
	Vec3 colors[2];
	float scale;
} TextureCheckerboard;

typedef struct TextureBrick {
	Texture texture;
	Vec3 colors[2];
	float scale;
	float mortar_width;
} TextureBrick;

typedef struct TextureNoisySin {
	Texture texture;
	Vec3 color;
	Vec3 color_gradient;
	float noise_feature_scale;
	float noise_scale;
	float frequency_scale;
} TextureNoisySin;

/* Object */
typedef struct Object {
	ObjectData const *object_data;
	float epsilon;
	Material *material;
} Object;

typedef struct Sphere {
	Object object;
	Vec3 position;
	float radius;
} Sphere;

typedef struct Triangle {//triangle ABC
	Object object;
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
	Vec3 normal;
} Triangle;

#ifdef UNBOUND_OBJECTS
typedef struct Plane {//normal = {a,b,c}, ax + by + cz = d
	Object object;
	Vec3 normal;
	float d;
} Plane;
#endif

typedef struct ObjectData {
	char *name;
#ifdef UNBOUND_OBJECTS
	bool is_bounded;
#endif
	bool (*get_intersection)(const Object*, const Line*, float*, Vec3);
	bool (*intersects_in_range)(const Object*, const Line*, float);
	void (*delete)(Object*);
	BoundingCuboid *(*generate_bounding_cuboid)(const Object*);
	void (*get_corners)(const Object*, Vec3[2]);
	void (*scale)(const Object*, const Vec3, const float);
} ObjectData;

/* BVH */
typedef union BVHChild {
	BVH *bvh;
	Object *object;
} BVHChild;

typedef struct BVH {
	bool is_leaf;
	BoundingCuboid *bounding_cuboid;
	BVHChild children[];
} BVH;

typedef struct BVHWithMorton { //Only used when constructing BVH tree
	uint32_t morton_code;
	BVH *bvh;
} BVHWithMorton;

/*******************************************************************************
*	FUNCTION PROTOTYPE
*******************************************************************************/

/* ALGORITHM */
float sqr(const float val);
float mag2(const Vec2 vec);
float mag3(const Vec3 vec);
float dot2(const Vec2 vec1, const Vec2 vec2);
float dot3(const Vec3 vec1, const Vec3 vec2);
void cross(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void mul2s(const Vec2 vec, const float mul, Vec2 result);
void mul3s(const Vec3 vec, const float mul, Vec3 result);
void mul3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void inv3(Vec3 vec);
void add2v(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void add2s(const Vec2 vec1, const float summand, Vec2 result);
void add3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void add3s(const Vec3 vec1, const float summand, Vec3 result);
void add3v3(const Vec3 vec1, const Vec3 vec2, const Vec3 vec3, Vec3 result);
void sub2v(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void sub2s(const Vec2 vec1, const float subtrahend, Vec2 result);
void sub3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void sub3s(const Vec3 vec1, const float subtrahend, Vec3 result);
void norm2(Vec2 vec);
void norm3(Vec3 vec);
float max3(const Vec3 vec);
float min3(const Vec3 vec);
float clamp(const float num, const float min, const float max);
void clamp3(const Vec3 vec, const Vec3 min, const Vec3 max, Vec3 result);
float magsqr3(const Vec3 vec);
void mulm3(Mat3 mat, const float *restrict vec, float *restrict result);
void mulms(Mat3 mat, const float mul, Mat3 result);
float rand_flt(void);
bool moller_trumbore(const Vec3 vertex, Vec3 edges[2], const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance);
bool line_intersects_sphere(const Vec3 sphere_position, const float sphere_radius, const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance);
uint32_t djb_hash(const char* cp);

/* Context */
Context *context_new(void);
void context_delete(Context *context);

/* Camera */
Camera *camera_new(const Vec3 position, Vec3 vectors[2], const float fov, const float focal_length, Context *context);
void camera_delete(Camera *camera);
Camera *camera_load(const cJSON *json, Context *context);
void save_image(FILE *file, const Image *image);
void camera_scale(Camera *camera, const Vec3 neg_shift, const float scale);

/* Light */
void light_init(Light *light, const Vec3 position, const Vec3 intensity);
void light_load(const cJSON *json, Light *light);
void light_scale(Light *light, const Vec3 neg_shift, const float scale);

/* Material */
void material_init(Material *material, const int32_t id, const Vec3 ks, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, const float shininess, const float refractive_index, Texture * const texture);
void material_load(const cJSON *json, Material *material);
Material *get_material(const Context *context, const int32_t id);
Texture *texture_load(const cJSON *json);
void texture_get_color_uniform(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_checkerboard(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_brick(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_noisy_sin(const Texture *tex, const Vec3 point, Vec3 color);

/* Object */
void object_add(Object * const object, Context *context);
void object_load(const cJSON *json, const Context *context, Object *object, const ObjectType object_type);
#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const Context *context, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool unbound_objects_is_light_blocked(const Context *context, const Line *ray, const float distance, Vec3 light_intensity);
#endif

/* Sphere */
void sphere_delete(Object *object);
Sphere *sphere_load(const cJSON *json, Context *context);
bool sphere_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool sphere_intersects_in_range(const Object *object, const Line *ray, const float min_distance);
BoundingCuboid *sphere_generate_bounding_cuboid(const Object *object);
void sphere_get_corners(const Object *object, Vec3 corners[2]);
void sphere_scale(const Object *object, const Vec3 neg_shift, const float scale);

/* Triangle */
void triangle_init(Triangle *triangle);
void triangle_delete(Object *object);
Triangle *triangle_load(const cJSON *json, Context *context);
bool triangle_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool triangle_intersects_in_range(const Object *object, const Line *ray, float min_distance);
BoundingCuboid *triangle_generate_bounding_cuboid(const Object *object);
void triangle_get_corners(const Object *object, Vec3 corners[2]);
void triangle_scale(const Object *object, const Vec3 neg_shift, const float scale);

/* Plane */
#ifdef UNBOUND_OBJECTS
void plane_delete(Object *object);
Plane *plane_load(const cJSON *json, Context *context);
bool plane_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool plane_intersects_in_range(const Object *object, const Line *ray, float min_distance);
void plane_scale(const Object *object, const Vec3 neg_shift, const float scale);
#endif

/* Mesh */
void mesh_load(const cJSON *json, Context *context);
uint32_t stl_get_num_triangles(FILE *file);
void stl_load_objects(Context *context, FILE *file, const Object *object, const Vec3 position, const Vec3 rot, const float scale);

/* BoundingCuboid */
BoundingCuboid *bounding_cuboid_new(const float epsilon, Vec3 corners[2]);
void bounding_cuboid_delete(BoundingCuboid *bounding_cuboid);
bool bounding_cuboid_intersects(const BoundingCuboid *cuboid, const Line *ray, float *tmax, float *tmin);

/* BVH */
BVH *bvh_new(const bool is_leaf, BoundingCuboid *bounding_cuboid);
void bvh_delete(BVH *bvh);
int bvh_morton_code_compare(const void *p1, const void *p2);
BoundingCuboid *bvh_generate_bounding_cuboid_leaf(const BVHWithMorton *leaf_array, const size_t first, const size_t last);
BoundingCuboid *bvh_generate_bounding_cuboid_node(const BVH *bvh_left, const BVH *bvh_right);
BVH *bvh_generate_node(const BVHWithMorton *leaf_array, const size_t first, const size_t last);
void bvh_generate(Context *context);
void bvh_get_closest_intersection(const BVH *bvh, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool bvh_is_light_blocked(const BVH *bvh, const Line *ray, const float distance, Vec3 light_intensity);
#ifdef DEBUG
void bvh_print(const BVH *bvh, const uint32_t depth);
#endif

/* SCENE */
void cJSON_parse_float_array(const cJSON *json, float *array);
void scene_load(Context *context);

/* MISC */
void err(const ErrorCode error_code);
void get_closest_intersection(const Context *context, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool is_light_blocked(const Context *context, const Line *ray, const float distance, Vec3 light_intensity);
void normalize_scene(Context *context);
void cast_ray(const Context *context, const Line *ray, const Vec3 kr, Vec3 color, const uint32_t bounce_count, Object *inside_object);
void create_image(const Context *context);
void process_arguments(int argc, char *argv[], Context *context);
void log_msg(const Context *context, const char *msg);
int main(int argc, char *argv[]);

/* ObjectData */
const ObjectData OBJECT_DATA[] = {
#ifdef UNBOUND_OBJECTS
	[OBJECT_PLANE] = {
		.name = "Plane",
		.is_bounded = false,
		.get_intersection = &plane_get_intersection,
		.intersects_in_range = &plane_intersects_in_range,
		.delete = &plane_delete,
		.scale = &plane_scale,
	},
#endif
	[OBJECT_SPHERE] = {
		.name = "Sphere",
#ifdef UNBOUND_OBJECTS
		.is_bounded = true,
#endif
		.get_intersection = &sphere_get_intersection,
		.intersects_in_range = &sphere_intersects_in_range,
		.delete = &sphere_delete,
		.generate_bounding_cuboid = &sphere_generate_bounding_cuboid,
		.get_corners = &sphere_get_corners,
		.scale = &sphere_scale,
	},
	[OBJECT_TRIANGLE] = {
		.name = "Triangle",
#ifdef UNBOUND_OBJECTS
		.is_bounded = true,
#endif
		.get_intersection = &triangle_get_intersection,
		.intersects_in_range = &triangle_intersects_in_range,
		.delete = &triangle_delete,
		.generate_bounding_cuboid = &triangle_generate_bounding_cuboid,
		.get_corners = &triangle_get_corners,
		.scale = &triangle_scale,
	},
};

/*******************************************************************************
*	ALGORITHM
*******************************************************************************/
__attribute__((const))
float sqr(const float val)
{
	return val * val;
}

__attribute__((const))
float mag2(const Vec2 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]));
}

__attribute__((const))
float mag3(const Vec3 vec)
{
	return sqrtf(magsqr3(vec));
}

__attribute__((const))
float dot2(const Vec2 vec1, const Vec2 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y];
}

__attribute__((const))
float dot3(const Vec3 vec1, const Vec3 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y] + vec1[Z] * vec2[Z];
}

void cross(const Vec3 vec1, const Vec3 vec2, Vec3 result)
{
	result[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
	result[Y] = vec1[Z] * vec2[X] - vec1[X] * vec2[Z];
	result[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];
}

void mul2s(const Vec2 vec, const float mul, Vec2 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
}

void mul3s(const Vec3 vec, const float mul, Vec3 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
	result[Z] = vec[Z] * mul;
}

void mul3v(const Vec3 vec1, const Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] * vec2[X];
	result[Y] = vec1[Y] * vec2[Y];
	result[Z] = vec1[Z] * vec2[Z];
}

void inv3(Vec3 vec)
{
	vec[X] = 1.f / vec[X];
	vec[Y] = 1.f / vec[Y];
	vec[Z] = 1.f / vec[Z];
}

void add2v(const Vec2 vec1, const Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
}

void add2s(const Vec2 vec1, const float summand, Vec2 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
}

void add3v(const Vec3 vec1, const Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
	result[Z] = vec1[Z] + vec2[Z];
}

void add3s(const Vec3 vec1, const float summand, Vec3 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
	result[Z] = vec1[Z] + summand;
}

void add3v3(const Vec3 vec1, const Vec3 vec2, const Vec3 vec3, Vec3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void sub2v(const Vec2 vec1, const Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
}

void sub2s(const Vec2 vec1, const float subtrahend, Vec2 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
}

void sub3v(const Vec3 vec1, const Vec3 vec2, Vec3 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
	result[Z] = vec1[Z] - vec2[Z];
}

void sub3s(const Vec3 vec1, const float subtrahend, Vec3 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
	result[Z] = vec1[Z] - subtrahend;
}

void norm2(Vec2 vec)
{
	mul2s(vec, 1.f / mag2(vec), vec);
}

void norm3(Vec3 vec)
{
	mul3s(vec, 1.f / mag3(vec), vec);
}

__attribute__((const))
float min3(const Vec3 vec)
{
    float min = vec[0];
    if (min > vec[1])
    	min = vec[1];
    if (min > vec[2])
    	min = vec[2];
    return min;
}

__attribute__((const))
float max3(const Vec3 vec)
{
    float max = vec[0];
    if (max < vec[1])
    	max = vec[1];
    if (max < vec[2])
    	max = vec[2];
    return max;
}

__attribute__((const))
float clamp(const float num, const float min, const float max)
{
	const float result = num < min ? min : num;
	return result > max ? max : result;
}

void clamp3(const Vec3 vec, const Vec3 min, const Vec3 max, Vec3 result)
{
	result[X] = clamp(vec[X], min[X], max[X]);
	result[Y] = clamp(vec[Y], min[Y], max[Y]);
	result[Z] = clamp(vec[Z], min[Z], max[Z]);
}

__attribute__((const))
float magsqr3(const Vec3 vec)
{
	return sqr(vec[X]) + sqr(vec[Y]) + sqr(vec[Z]);
}

void mulm3(Mat3 mat, const float *restrict vec, float *restrict result)
{
	result[X] = dot3(mat[X], vec);
	result[Y] = dot3(mat[Y], vec);
	result[Z] = dot3(mat[Z], vec);
}

void mulms(Mat3 mat, const float mul, Mat3 result)
{
	mul3s(mat[X], mul, result[X]);
	mul3s(mat[Y], mul, result[Y]);
	mul3s(mat[Z], mul, result[Z]);
}

void addmat(Mat3 mat1, Mat3 mat2, Mat3 result)
{
	add3v(mat1[X], mat2[X], result[X]);
	add3v(mat1[Y], mat2[Y], result[Y]);
	add3v(mat1[Z], mat2[Z], result[Z]);
}

float rand_flt(void)
{
	return rand() / (float) RAND_MAX;
}

//Möller–Trumbore intersection algorithm
bool moller_trumbore(const Vec3 vertex, Vec3 edges[2], const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance)
{
	Vec3 h, s, q;
	cross(line_vector, edges[1], h);
	float a = dot3(edges[0], h);
	if (a < epsilon && a > -epsilon) //ray is parallel to line
		return false;
	float f = 1.f / a;
	sub3v(line_position, vertex, s);
	float u = f * dot3(s, h);
	if (u < 0.f || u > 1.f)
		return false;
	cross(s, edges[0], q);
	float v = f * dot3(line_vector, q);
	if (v < 0.f || u + v > 1.f)
		return false;
	*distance = f * dot3(edges[1], q);
	return *distance > epsilon;
}

bool line_intersects_sphere(const Vec3 sphere_position, const float sphere_radius, const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance)
{
	Vec3 relative_position;
	sub3v(line_position, sphere_position, relative_position);
	float b = -2 * dot3(line_vector, relative_position);
	float c = dot3(relative_position, relative_position) - sqr(sphere_radius);
	float determinant = sqr(b) - 4 * c;
	if (determinant < 0) //no collision
		return false;
	float sqrt_determinant = sqrtf(determinant);
	*distance = (b - sqrt_determinant) * .5f;
	if (*distance > epsilon)//if in front of origin of ray
		return true;
	*distance = (b + sqrt_determinant) * .5f;
	return *distance > epsilon; //check if the further distance is positive
}

// D. J. Bernstein hash function
uint32_t djb_hash(const char* cp)
{
	uint32_t hash = 5381;
	while (*cp)
		hash = 33 * hash ^ (uint8_t) *cp++;
	return hash;
}

//Expands a number to only use 1 in every 3 bits
uint32_t expand_bits(uint32_t num)
{
    num = (num * 0x00010001u) & 0xFF0000FFu;
    num = (num * 0x00000101u) & 0x0F00F00Fu;
    num = (num * 0x00000011u) & 0xC30C30C3u;
    num = (num * 0x00000005u) & 0x49249249u;
    return num;
}

//Calcualtes 30-bit morton code for a point in cube [0,1]. Does not perform bounds checking
uint32_t morton_code(const Vec3 vec)
{
	return expand_bits((uint32_t)(1023.f * vec[X])) * 4
		+ expand_bits((uint32_t)(1023.f * vec[Y])) * 2
		+ expand_bits((uint32_t)(1023.f * vec[Z]));
}

/*******************************************************************************
*	Context
*******************************************************************************/

Context *context_new(void)
{
	Context *context = malloc(sizeof(Context));
	*context = (Context) {
			.max_bounces = 10,
			.minimum_light_intensity_sqr = .01f * .01f,
			.global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT,
			.reflection_model = REFLECTION_PHONG,
			.log_option = LOG_REALTIME,
			.samples_per_pixel = 1,
			.normalize_colors = false,
			.light_attenuation = LIGHT_ATTENUATION_SQUARE,
	};
	timespec_get(&context->start_timespec, TIME_UTC);
	context->start_clock = clock();
	return context;
}

void context_delete(Context *context)
{
	size_t i;
	for (i = 0; i < context->num_objects; i++)
		context->objects[i]->object_data->delete(context->objects[i]);
	camera_delete(context->camera);
	bvh_delete(context->bvh);
	free(context->objects);
#ifdef UNBOUND_OBJECTS
	free(context->unbound_objects);
#endif
	free(context->lights);
	free(context->materials);
	free(context);
}

/*******************************************************************************
*	Camera
*******************************************************************************/

Camera *camera_new(const Vec3 position, Vec3 vectors[2], const float fov, const float focal_length, Context *context)
{
	Camera *camera = malloc(sizeof(Camera));

	camera->fov = fov;
	camera->focal_length = focal_length;

	memcpy(camera->position, position, sizeof(Vec3));
	memcpy(camera->vectors, vectors, sizeof(Vec3[2]));
	norm3(camera->vectors[0]);
	norm3(camera->vectors[1]);
	cross(camera->vectors[0], camera->vectors[1], camera->vectors[2]);
	camera->focal_length = focal_length;
	memcpy(camera->image.resolution, context->resolution, 2 * sizeof(int));
	camera->image.size[X] = 2 * focal_length * tanf(fov * PI / 360.f);
	camera->image.size[Y] = camera->image.size[X] * camera->image.resolution[Y] / camera->image.resolution[X];
	camera->image.pixels = malloc(context->resolution[X] * context->resolution[Y] * sizeof(Color));

	Vec3 focal_vector, plane_center, corner_offset_vectors[2];
	mul3s(camera->vectors[2], camera->focal_length, focal_vector);
	add3v(focal_vector, camera->position, plane_center);
	mul3s(camera->vectors[0], camera->image.size[X] / camera->image.resolution[X], camera->image.vectors[0]);
	mul3s(camera->vectors[1], camera->image.size[Y] / camera->image.resolution[Y], camera->image.vectors[1]);
	mul3s(camera->image.vectors[X], .5f - camera->image.resolution[X] / 2.f, corner_offset_vectors[X]);
	mul3s(camera->image.vectors[Y], .5f - camera->image.resolution[Y] / 2.f, corner_offset_vectors[Y]);
	add3v3(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], camera->image.corner);

	return camera;
}

void camera_delete(Camera *camera)
{
	free(camera->image.pixels);
	free(camera);
}

Camera *camera_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 5, ERR_JSON_ARGC);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_vector_x = cJSON_GetObjectItemCaseSensitive(json, "vector_x"),
		*json_vector_y = cJSON_GetObjectItemCaseSensitive(json, "vector_y"),
		*json_fov = cJSON_GetObjectItemCaseSensitive(json, "fov"),
		*json_focal_length = cJSON_GetObjectItemCaseSensitive(json, "focal_length");

	err_assert(cJSON_IsArray(json_position)
		&& cJSON_IsArray(json_vector_x)
		&& cJSON_IsArray(json_vector_y), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3
		&& cJSON_GetArraySize(json_vector_x) == 3
		&& cJSON_GetArraySize(json_vector_y) == 3, ERR_JSON_ARRAY_SIZE);
	err_assert(cJSON_IsNumber(json_fov)
		&& cJSON_IsNumber(json_focal_length), ERR_JSON_VALUE_NOT_NUMBER);

	float fov = json_fov->valuedouble,
		focal_length = json_focal_length->valuedouble;
	Vec3 position, vectors[2];

	err_assert(fov > 0.f && fov < 180.f, ERR_JSON_CAMERA_FOV);

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_vector_x, vectors[0]);
	cJSON_parse_float_array(json_vector_y, vectors[1]);

	return camera_new(position, vectors, fov, focal_length, context);
}

void camera_scale(Camera *camera, const Vec3 neg_shift, const float scale)
{
	sub3v(camera->position, neg_shift, camera->position);
	mul3s(camera->position, scale, camera->position);
	camera->focal_length *= scale;
	Image *image = &camera->image;
	mul2s(image->size, scale, image->size);
	sub3v(image->corner, neg_shift, image->corner);
	mul3s(image->corner, scale, image->corner);
	mul3s(image->vectors[0], scale, image->vectors[0]);
	mul3s(image->vectors[1], scale, image->vectors[1]);
}

void save_image(FILE *file, const Image *image)
{
	err_assert(fprintf(file, "P6\n%d %d\n255\n", image->resolution[X], image->resolution[Y]) > 0, ERR_IO_WRITE_IMG);
	size_t num_pixels = image->resolution[X] * image->resolution[Y];
	err_assert(fwrite(image->pixels, sizeof(Color), num_pixels, file) == num_pixels, ERR_IO_WRITE_IMG);
}

/*******************************************************************************
*	Light
*******************************************************************************/

void light_init(Light *light, const Vec3 position, const Vec3 intensity)
{
	memcpy(light->position, position, sizeof(Vec3));
	memcpy(light->intensity, intensity, sizeof(Vec3));
}

void light_load(const cJSON *json, Light *light)
{
	err_assert(cJSON_GetArraySize(json) == 2, ERR_JSON_ARGC);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_intensity = cJSON_GetObjectItemCaseSensitive(json, "intensity");
	err_assert(cJSON_IsArray(json_position)
		&& cJSON_IsArray(json_intensity), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3
		&& cJSON_GetArraySize(json_intensity) == 3, ERR_JSON_ARRAY_SIZE);

	Vec3 position, intensity;

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_intensity, intensity);

	light_init(light, position, intensity);
}

void light_scale(Light *light, const Vec3 neg_shift, const float scale)
{
	sub3v(light->position, neg_shift, light->position);
	mul3s(light->position, scale, light->position);
}

/*******************************************************************************
*	BVH
*******************************************************************************/

//TODO: remove cuboid from new
BVH *bvh_new(const bool is_leaf, BoundingCuboid *bounding_cuboid)
{
	BVH *bvh = malloc(sizeof(BVH) + (is_leaf ? 1 : 2) * sizeof(BVHChild));
	bvh->is_leaf = is_leaf;
	bvh->bounding_cuboid = bounding_cuboid;
	return bvh;
}

void bvh_delete(BVH *bvh)
{
	bounding_cuboid_delete(bvh->bounding_cuboid);
	if (!bvh->is_leaf) {
		bvh_delete(bvh->children[0].bvh);
		bvh_delete(bvh->children[1].bvh);
	}
	free(bvh);
}

int bvh_morton_code_compare(const void *p1, const void *p2)
{
	return (int)((BVHWithMorton*)p1)->morton_code - (int)((BVHWithMorton*)p2)->morton_code;
}

BoundingCuboid *bvh_generate_bounding_cuboid_leaf(const BVHWithMorton *leaf_array, const size_t first, const size_t last)
{
	Vec3 corners[2] = {{FLT_MAX}, {FLT_MIN}};
	float epsilon = 0.f;
	size_t i, j;
	for (i = first; i <= last; i++) {
		BoundingCuboid *bounding_cuboid = leaf_array[i].bvh->bounding_cuboid;
		if (epsilon < bounding_cuboid->epsilon)
			epsilon = bounding_cuboid->epsilon;
		for (j = 0; j < 3; j++) {
			if (bounding_cuboid->corners[0][j] < corners[0][j])
				corners[0][j] = bounding_cuboid->corners[0][j];
			if (bounding_cuboid->corners[1][j] > corners[1][j])
				corners[1][j] = bounding_cuboid->corners[1][j];
		}
	}
	return bounding_cuboid_new(epsilon, corners);
}

BoundingCuboid *bvh_generate_bounding_cuboid_node(const BVH *bvh_left, const BVH *bvh_right)
{
	BoundingCuboid *left = bvh_left->bounding_cuboid, *right = bvh_right->bounding_cuboid;
	float epsilon = fmaxf(left->epsilon, right->epsilon);
	Vec3 corners[2] = {
		{
			fminf(left->corners[0][X], right->corners[0][X]),
			fminf(left->corners[0][Y], right->corners[0][Y]),
			fminf(left->corners[0][Z], right->corners[0][Z]),
		},
		{
			fmaxf(left->corners[1][X], right->corners[1][X]),
			fmaxf(left->corners[1][Y], right->corners[1][Y]),
			fmaxf(left->corners[1][Z], right->corners[1][Z]),
		},
	};
	return bounding_cuboid_new(epsilon, corners);
}

BVH *bvh_generate_node(const BVHWithMorton *leaf_array, const size_t first, const size_t last)
{
	if (first == last)
		return leaf_array[first].bvh;

	uint32_t first_code = leaf_array[first].morton_code;
	uint32_t last_code = leaf_array[last].morton_code;

	size_t split;
	if (first_code == last_code) {
		split = (first + last) / 2;
	} else {
		split = first;
		uint32_t common_prefix = clz(first_code ^ last_code);
		size_t step = last - first;

		do {
			step = (step + 1) >> 1; // exponential decrease
			size_t new_split = split + step; // proposed new position

			if (new_split < last) {
				uint32_t split_code = leaf_array[new_split].morton_code;
				if (first_code ^ split_code) {
					uint32_t split_prefix = clz(first_code ^ split_code);
					if (split_prefix > common_prefix)
						split = new_split; // accept proposal
				}
			}
		} while (step > 1);
	}

	BVH *bvh_left = bvh_generate_node(leaf_array, first, split);
	BVH *bvh_right = bvh_generate_node(leaf_array, split + 1, last);
	BVH *bvh = bvh_new(false, bvh_generate_bounding_cuboid_node(bvh_left, bvh_right));
	bvh->children[0].bvh = bvh_left;
	bvh->children[1].bvh = bvh_right;
	return bvh;
}

void bvh_generate(Context *context)
{
#ifdef UNBOUND_OBJECTS
	size_t num_leaves = context->num_objects - context->num_unbound_objects;
#else
	size_t num_leaves = context->num_objects;
#endif
	BVHWithMorton *leaf_array = malloc(sizeof(BVHWithMorton) * num_leaves);

	size_t i, j = 0;
	for (i = 0; i < context->num_objects; i++) {
		Object *object = context->objects[i];
#ifdef UNBOUND_OBJECTS
		if (object->object_data->is_bounded) {
#endif
			BVH *bvh = bvh_new(true, object->object_data->generate_bounding_cuboid(object));
			bvh->children[0].object = object;
			leaf_array[j++].bvh = bvh;
#ifdef UNBOUND_OBJECTS
		}
#endif
	}

	for (i = 0; i < num_leaves; i++) {
		BoundingCuboid *bounding_cuboid = leaf_array[i].bvh->bounding_cuboid;
		Vec3 norm_position;
		add3v(bounding_cuboid->corners[0], bounding_cuboid->corners[1], norm_position);
		mul3s(norm_position, .5f, norm_position);
		leaf_array[i].morton_code = morton_code(norm_position);
	}

	qsort(leaf_array, num_leaves, sizeof(BVHWithMorton), &bvh_morton_code_compare);

	context->bvh = bvh_generate_node(leaf_array, 0, num_leaves - 1);

	free(leaf_array);
}

void bvh_get_closest_intersection(const BVH *bvh, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
	if (bvh->is_leaf) {
		Vec3 normal;
		Object *object = bvh->children[0].object;
		float distance;
		if (object->object_data->get_intersection(object, ray, &distance, normal) && distance < *closest_distance) {
			*closest_distance = distance;
			*closest_object = object;
			memcpy(closest_normal, normal, sizeof(Vec3));
		}
		return;
	}

	bool intersect_l, intersect_r;
	float tmin_l, tmin_r, tmax;
	intersect_l = bounding_cuboid_intersects(bvh->children[0].bvh->bounding_cuboid, ray, &tmax, &tmin_l) && tmin_l < *closest_distance;
	intersect_r = bounding_cuboid_intersects(bvh->children[1].bvh->bounding_cuboid, ray, &tmax, &tmin_r) && tmin_r < *closest_distance;
	if (intersect_l && intersect_r) {
		if (tmin_l < tmin_r) {
			bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
			bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
		} else {
			bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
			bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
		}
	} else if (intersect_l) {
		bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
	} else if (intersect_r) {
		bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
	}
}

bool bvh_is_light_blocked(const BVH *bvh, const Line *ray, const float distance, Vec3 light_intensity)
{
	float tmin, tmax;

	if (bvh->is_leaf) {
		Vec3 normal;
		Object *object = bvh->children[0].object;
		if (object->object_data->get_intersection(object, ray, &tmin, normal) && tmin < distance) {
			if (object->material->transparent)
				mul3v(light_intensity, object->material->kt, light_intensity);
			else
				return true;
		}
		return false;
	}

	size_t i;
#pragma GCC unroll 2
	for (i = 0; i < 2; i++) {
		if (bounding_cuboid_intersects(bvh->children[i].bvh->bounding_cuboid, ray, &tmax, &tmin)
			&& tmin < distance
			&& bvh_is_light_blocked(bvh->children[i].bvh, ray, distance, light_intensity))
			return true;
	}
	return false;
}

#ifdef DEBUG
void bvh_print(const BVH *bvh, const uint32_t depth)
{
	uint32_t i;
	size_t j;
	for (i = 0; i < depth; i++)
		printf("\t");
	if (bvh->is_leaf) {
		printf("%s\n", bvh->children[0].object->object_data->name);
	} else {
		printf("NODE\n");
		for(j = 0; j < 2; j++)
			bvh_print(bvh->children[j].bvh, depth + 1);
	}
}
#endif

/*******************************************************************************
*	Material
*******************************************************************************/

void material_init(Material *material, const int32_t id, const Vec3 ks, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, const float shininess, const float refractive_index, Texture * const texture)
{
	material->id = id;
	memcpy(material->ks, ks, sizeof(Vec3));
	memcpy(material->ka, ka, sizeof(Vec3));
	memcpy(material->kr, kr, sizeof(Vec3));
	memcpy(material->kt, kt, sizeof(Vec3));
	memcpy(material->ke, ke, sizeof(Vec3));
	material->shininess = shininess;
	material->refractive_index = refractive_index;
	material->texture = texture;
	material->reflective = mag3(kr) > MATERIAL_THRESHOLD;
	material->transparent = mag3(kt) > MATERIAL_THRESHOLD;
}

void material_load(const cJSON *json, Material *material)
{
	cJSON  *json_id = cJSON_GetObjectItemCaseSensitive(json, "id"),
		*json_ks = cJSON_GetObjectItemCaseSensitive(json, "ks"),
		*json_ka = cJSON_GetObjectItemCaseSensitive(json, "ka"),
		*json_kr = cJSON_GetObjectItemCaseSensitive(json, "kr"),
		*json_kt = cJSON_GetObjectItemCaseSensitive(json, "kt"),
		*json_ke = cJSON_GetObjectItemCaseSensitive(json, "ke"),
		*json_shininess = cJSON_GetObjectItemCaseSensitive(json, "shininess"),
		*json_refractive_index = cJSON_GetObjectItemCaseSensitive(json, "refractive_index"),
		*json_texture = cJSON_GetObjectItemCaseSensitive(json, "texture");

	err_assert(cJSON_IsNumber(json_shininess)
		&& cJSON_IsNumber(json_refractive_index)
		&& cJSON_IsNumber(json_id), ERR_JSON_VALUE_NOT_NUMBER);
	err_assert(cJSON_IsObject(json_texture), ERR_JSON_VALUE_NOT_OBJECT);
	err_assert(cJSON_IsArray(json_ks)
		&& cJSON_IsArray(json_ka)
		&& cJSON_IsArray(json_kr)
		&& cJSON_IsArray(json_kt)
		&& cJSON_IsArray(json_ke), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_ks) == 3
		&& cJSON_GetArraySize(json_ka) == 3
		&& cJSON_GetArraySize(json_kr) == 3
		&& cJSON_GetArraySize(json_kt) == 3
		&& cJSON_GetArraySize(json_ke) == 3, ERR_JSON_ARRAY_SIZE);

	Vec3 ks, ka, kr, kt, ke;

	cJSON_parse_float_array(json_ks, ks);
	cJSON_parse_float_array(json_ka, ka);
	cJSON_parse_float_array(json_kr, kr);
	cJSON_parse_float_array(json_kt, kt);
	cJSON_parse_float_array(json_ke, ke);

	int32_t id = json_id->valueint;
	float shininess = json_shininess->valuedouble;
	float refractive_index = json_refractive_index->valuedouble;
	Texture *texture = texture_load(json_texture);

	material_init(material, id, ks, ka, kr, kt, ke, shininess, refractive_index, texture);
}

Material *get_material(const Context *context, const int32_t id)
{
	size_t i;
	for (i = 0; i < context->num_materials; i++)
		if (context->materials[i].id == id)
			return &context->materials[i];
	err(ERR_JSON_INVALID_MATERIAL);
	return NULL;
}

Texture *texture_load(const cJSON *json)
{
	cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json, "type");
	err_assert(cJSON_IsString(json_type), ERR_JSON_VALUE_NOT_STRING);

	switch (djb_hash(json_type->valuestring)) {
	case 3226203393: { //uniform
		cJSON *json_color = cJSON_GetObjectItemCaseSensitive(json, "color");
		err_assert(cJSON_IsArray(json_color), ERR_JSON_VALUE_NOT_ARRAY);
		err_assert(cJSON_GetArraySize(json_color) == 3, ERR_JSON_ARRAY_SIZE);

		TextureUniform *texture = malloc(sizeof(TextureUniform));
		texture->texture.get_color = &texture_get_color_uniform;
		cJSON_parse_float_array(json_color, texture->color);
		return (Texture*)texture;
		}
	case 2234799246: { //checkerboard
		cJSON *json_colors = cJSON_GetObjectItemCaseSensitive(json, "colors"),
			*json_scale = cJSON_GetObjectItemCaseSensitive(json, "scale");
		err_assert(cJSON_IsArray(json_colors), ERR_JSON_VALUE_NOT_ARRAY);
		err_assert(cJSON_GetArraySize(json_colors) == 2, ERR_JSON_ARRAY_SIZE);
		err_assert(cJSON_IsNumber(json_scale), ERR_JSON_VALUE_NOT_NUMBER);

		TextureCheckerboard *texture = malloc(sizeof(TextureCheckerboard));
		texture->texture.get_color = &texture_get_color_checkerboard;
		texture->scale = json_scale->valuedouble;

		cJSON *json_iter;
		size_t i = 0;
		cJSON_ArrayForEach (json_iter, json_colors) {
			err_assert(cJSON_IsArray(json_iter), ERR_JSON_VALUE_NOT_ARRAY);
			err_assert(cJSON_GetArraySize(json_iter) == 3, ERR_JSON_ARRAY_SIZE);
			cJSON_parse_float_array(json_iter, texture->colors[i++]);
		}
		return (Texture*)texture;
		}
	case 176032948: { //brick
		cJSON *json_colors = cJSON_GetObjectItemCaseSensitive(json, "colors"),
			*json_scale = cJSON_GetObjectItemCaseSensitive(json, "scale"),
			*json_mortar_width = cJSON_GetObjectItemCaseSensitive(json, "mortar width");
		err_assert(cJSON_IsArray(json_colors), ERR_JSON_VALUE_NOT_ARRAY);
		err_assert(cJSON_GetArraySize(json_colors) == 2, ERR_JSON_ARRAY_SIZE);
		err_assert(cJSON_IsNumber(json_scale)
			&& cJSON_IsNumber(json_mortar_width), ERR_JSON_VALUE_NOT_NUMBER);

		TextureBrick *texture = malloc(sizeof(TextureBrick));
		texture->texture.get_color = &texture_get_color_brick;
		texture->scale = json_scale->valuedouble;
		texture->mortar_width = json_mortar_width->valuedouble;

		cJSON *json_iter;
		size_t i = 0;
		cJSON_ArrayForEach (json_iter, json_colors) {
			err_assert(cJSON_IsArray(json_iter), ERR_JSON_VALUE_NOT_ARRAY);
			err_assert(cJSON_GetArraySize(json_iter) == 3, ERR_JSON_ARRAY_SIZE);
			cJSON_parse_float_array(json_iter, texture->colors[i++]);
		}
		return (Texture*)texture;
		}
	case 957354035: {
		cJSON *json_color = cJSON_GetObjectItemCaseSensitive(json, "color"),
			*json_color_gradient = cJSON_GetObjectItemCaseSensitive(json, "color gradient"),
			*json_noise_feature_scale = cJSON_GetObjectItemCaseSensitive(json, "noise feature scale"),
			*json_noise_scale = cJSON_GetObjectItemCaseSensitive(json, "noise scale"),
			*json_frequency_scale = cJSON_GetObjectItemCaseSensitive(json, "frequency scale");
		err_assert(cJSON_IsArray(json_color)
			&& cJSON_IsArray(json_color_gradient), ERR_JSON_VALUE_NOT_ARRAY);
		err_assert(cJSON_GetArraySize(json_color) == 3
			&& cJSON_GetArraySize(json_color_gradient) == 3, ERR_JSON_ARRAY_SIZE);
		err_assert(cJSON_IsNumber(json_noise_feature_scale)
			&& cJSON_IsNumber(json_noise_scale)
			&& cJSON_IsNumber(json_frequency_scale), ERR_JSON_VALUE_NOT_NUMBER);

		TextureNoisySin *texture = malloc(sizeof(TextureNoisySin));
		texture->texture.get_color = &texture_get_color_noisy_sin;
		texture->noise_feature_scale = json_noise_feature_scale->valuedouble;
		texture->noise_scale = json_noise_scale->valuedouble;
		texture->frequency_scale = json_frequency_scale->valuedouble;
		cJSON_parse_float_array(json_color, texture->color);
		cJSON_parse_float_array(json_color_gradient, texture->color_gradient);
		return (Texture*)texture;
		}
	default:
		err(ERR_JSON_UNRECOGNIZED);
	}
	return NULL;
}

void texture_get_color_uniform(const Texture *tex, const Vec3 point, Vec3 color)
{
	(void)point;
	TextureUniform *texture = (TextureUniform*)tex;
	memcpy(color, texture->color, sizeof(Vec3));
}

void texture_get_color_checkerboard(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureCheckerboard *texture = (TextureCheckerboard*)tex;
	Vec3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = ((uint32_t)scaled_point[X] + (uint32_t)scaled_point[Y] + (uint32_t)scaled_point[Z]) % 2u;
	memcpy(color, texture->colors[parity], sizeof(Vec3));
}

void texture_get_color_brick(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureBrick *texture = (TextureBrick*)tex;
	Vec3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = (uint32_t)scaled_point[X] % 2u;
	scaled_point[Y] -= parity * .5f;
	uint32_t is_mortar = (scaled_point[X] - floorf(scaled_point[X]) < texture->mortar_width)
	|| (scaled_point[Y] - floorf(scaled_point[Y]) < texture->mortar_width);
	memcpy(color, texture->colors[is_mortar], sizeof(Vec3));
}

void texture_get_color_noisy_sin(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureNoisySin *texture = (TextureNoisySin*)tex;
	Vec3 scaled_point;
	mul3s(point, texture->noise_feature_scale, scaled_point);
	float n = (1.f + sinf((point[X] + simplex_noise(scaled_point[X], scaled_point[Y], scaled_point[Z]) * texture->noise_scale) * texture->frequency_scale)) * .5f;
	mul3s(texture->color_gradient, n, color);
	add3v(color, texture->color, color);
}

/*******************************************************************************
*	Object
*******************************************************************************/

void object_add(Object * const object, Context *context)
{
	context->objects[context->num_objects++] = object;
}

void object_load(const cJSON *json, const Context *context, Object *object, const ObjectType object_type)
{
	cJSON *json_epsilon = cJSON_GetObjectItemCaseSensitive(json, "epsilon"),
		*json_material = cJSON_GetObjectItemCaseSensitive(json, "material");

	err_assert(cJSON_IsNumber(json_epsilon)
		&& cJSON_IsNumber(json_material), ERR_JSON_VALUE_NOT_NUMBER);

	object->object_data = &OBJECT_DATA[object_type];
	object->epsilon = json_epsilon->valuedouble;
	object->material = get_material(context, json_material->valueint);
}

#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const Context *context, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
	float distance;
	Vec3 normal;
	size_t i;
	for (i = 0; i < context->num_unbound_objects; i++) {
		Object *object = context->unbound_objects[i];
		if (object->object_data->get_intersection(object, ray, &distance, normal) && distance < *closest_distance) {
			*closest_distance = distance;
			*closest_object = object;
			memcpy(closest_normal, normal, sizeof(Vec3));

		}
	}
}

bool unbound_objects_is_light_blocked(const Context *context, const Line *ray, const float distance, Vec3 light_intensity)
{
	size_t i;
	for (i = 0; i < context->num_unbound_objects; i++) {
		Object *object = context->unbound_objects[i];
		if (object->object_data->intersects_in_range(context->unbound_objects[i], ray, distance)) {
			if (object->material->transparent)
				mul3v(light_intensity, object->material->kt, light_intensity);
			else
				return true;
			}
	}
	return false;
}
#endif /* UNBOUND_OBJECTS */

/*******************************************************************************
*	Sphere
*******************************************************************************/

Sphere *sphere_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 2 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_radius = cJSON_GetObjectItemCaseSensitive(json, "radius");

	err_assert(cJSON_IsNumber(json_radius), ERR_JSON_VALUE_NOT_NUMBER);
	err_assert(cJSON_IsArray(json_position), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3, ERR_JSON_ARRAY_SIZE);

	Sphere *sphere = malloc(sizeof(Sphere));
	object_load(json, context, (Object*)sphere, OBJECT_SPHERE);
	cJSON_parse_float_array(json_position, sphere->position);
	sphere->radius = json_radius->valuedouble;

	return sphere;
}

void sphere_delete(Object *object)
{
	free(object);
}

bool sphere_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Sphere *sphere = (Sphere*)object;
	if (line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->object.epsilon, distance)) {
			mul3s(ray->vector, *distance, normal);
			add3v(normal, ray->position, normal);
			sub3v(normal, sphere->position, normal);
			mul3s(normal, 1.f / sphere->radius, normal);
			return true;
		}
	return false;
}

bool sphere_intersects_in_range(const Object *object, const Line *ray, const float min_distance)
{
	Sphere *sphere = (Sphere*)object;
	float distance;
	bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->object.epsilon, &distance);
	if (intersects && distance < min_distance)
		return true;
	return false;
}

BoundingCuboid *sphere_generate_bounding_cuboid(const Object *object)
{
	Sphere *sphere = (Sphere*)object;
	Vec3 corners[2];
	sphere_get_corners(object, corners);
	return bounding_cuboid_new(sphere->object.epsilon, corners);
}

void sphere_get_corners(const Object *object, Vec3 corners[2])
{
	Sphere *sphere = (Sphere*)object;
	sub3s(sphere->position, sphere->radius, corners[0]);
	add3s(sphere->position, sphere->radius, corners[1]);
}

void sphere_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Sphere *sphere = (Sphere*)object;
	sphere->object.epsilon *= scale;
	sphere->radius *= scale;
	sub3v(sphere->position, neg_shift, sphere->position);
	mul3s(sphere->position, scale, sphere->position);
}

/*******************************************************************************
*	Triangle
*******************************************************************************/

void triangle_init(Triangle *triangle)
{
	sub3v(triangle->vertices[1], triangle->vertices[0], triangle->edges[0]);
	sub3v(triangle->vertices[2], triangle->vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
	norm3(triangle->normal);
}

void triangle_delete(Object *object)
{
	free(object);
}

Triangle *triangle_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 3 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	cJSON *json_vertex_1 = cJSON_GetObjectItemCaseSensitive(json, "vertex_1"),
		*json_vertex_2 = cJSON_GetObjectItemCaseSensitive(json, "vertex_2"),
		*json_vertex_3 = cJSON_GetObjectItemCaseSensitive(json, "vertex_3");

	err_assert(cJSON_IsArray(json_vertex_1)
		&& cJSON_IsArray(json_vertex_2)
		&& cJSON_IsArray(json_vertex_3), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_vertex_1) == 3
		&& cJSON_GetArraySize(json_vertex_2) == 3
		&& cJSON_GetArraySize(json_vertex_3) == 3, ERR_JSON_ARRAY_SIZE);

	Triangle *triangle = malloc(sizeof(Triangle));
	object_load(json, context, (Object*)triangle, OBJECT_TRIANGLE);
	cJSON_parse_float_array(json_vertex_1, triangle->vertices[0]);
	cJSON_parse_float_array(json_vertex_2, triangle->vertices[1]);
	cJSON_parse_float_array(json_vertex_3, triangle->vertices[2]);
	triangle_init(triangle);

	return triangle;
}

bool triangle_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Triangle *triangle = (Triangle*)object;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->object.epsilon, distance);
	if (intersects) {
		memcpy(normal, triangle->normal, sizeof(Vec3));
		return true;
	}
	return false;
}

bool triangle_intersects_in_range(const Object *object, const Line *ray, float min_distance)
{
	Triangle *triangle = (Triangle*)object;
	float distance;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->object.epsilon, &distance);
	return intersects && distance < min_distance;
}

BoundingCuboid *triangle_generate_bounding_cuboid(const Object *object)
{
	Triangle *triangle = (Triangle*)object;
	Vec3 corners[2];
	triangle_get_corners(object, corners);
	return bounding_cuboid_new(triangle->object.epsilon, corners);
}

void triangle_get_corners(const Object *object, Vec3 corners[2])
{
	Triangle *triangle = (Triangle*)object;
	memcpy(corners[0], triangle->vertices[2], sizeof(Vec3));
	memcpy(corners[1], triangle->vertices[2], sizeof(Vec3));
	size_t i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++) {
			if (corners[0][j] > triangle->vertices[i][j])
				corners[0][j] = triangle->vertices[i][j];
			else if (corners[1][j] < triangle->vertices[i][j])
				corners[1][j] = triangle->vertices[i][j];
		}
}

void triangle_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Triangle *triangle = (Triangle*)object;
	triangle->object.epsilon *= scale;
	size_t i;
	for (i = 0; i < 3; i++) {
		sub3v(triangle->vertices[i], neg_shift, triangle->vertices[i]);
		mul3s(triangle->vertices[i], scale, triangle->vertices[i]);
	}
	for (i = 0; i < 2; i++)
		mul3s(triangle->edges[i], scale, triangle->edges[i]);
}

/*******************************************************************************
*	Plane
*******************************************************************************/

#ifdef UNBOUND_OBJECTS
Plane *plane_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 2 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_normal = cJSON_GetObjectItemCaseSensitive(json, "normal");

	err_assert(cJSON_IsArray(json_position)
		&& cJSON_IsArray(json_normal), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3
		&& cJSON_GetArraySize(json_normal) == 3, ERR_JSON_ARRAY_SIZE);

	Plane *plane = malloc(sizeof(Plane));
	object_load(json, context, (Object*)plane, OBJECT_PLANE);
	cJSON_parse_float_array(json_normal, plane->normal);
	norm3(plane->normal);
	Vec3 position;
	cJSON_parse_float_array(json_position, position);
	plane->d = dot3(plane->normal, position);

	return plane;
}

void plane_delete(Object *object)
{
	free(object);
}

bool plane_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Plane *plane = (Plane*)object;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if (*distance > plane->object.epsilon) {
		if (signbit(a))
			memcpy(normal, plane->normal, sizeof(Vec3));
		else
			mul3s(plane->normal, -1.f, normal);
		return true;
	}
	return false;
}

bool plane_intersects_in_range(const Object *object, const Line *ray, float min_distance)
{
	Plane *plane = (Plane*)object;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	float distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	return distance > plane->object.epsilon && distance < min_distance;
}

void plane_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Plane *plane = (Plane*)object;
	Vec3 point = {1.f, 1.f, 1.f};
	size_t i;
	for (i = 0; i < 3; i++)
		if (fabsf(plane->normal[i]) > plane->object.epsilon)
			break;
	point[i] = 0.f;
	point[i] = (plane->d - dot3(point, plane->normal)) / plane->normal[i];
	sub3v(point, neg_shift, point);
	mul3s(point, scale, point);
	plane->d = dot3(plane->normal, point);
	plane->object.epsilon *= scale;
}
#endif /* UNBOUND_OBJECTS */

/*******************************************************************************
*	Mesh
*******************************************************************************/

void mesh_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 4 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	cJSON *json_filename = cJSON_GetObjectItemCaseSensitive(json, "filename"),
		*json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_rotation = cJSON_GetObjectItemCaseSensitive(json, "rotation"),
		*json_scale = cJSON_GetObjectItemCaseSensitive(json, "scale");

	err_assert(cJSON_IsString(json_filename), ERR_JSON_VALUE_NOT_STRING);
	err_assert(cJSON_IsNumber(json_scale), ERR_JSON_VALUE_NOT_NUMBER);
	err_assert(cJSON_IsArray(json_position)
		&& cJSON_IsArray(json_rotation), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3
		&& cJSON_GetArraySize(json_rotation) == 3, ERR_JSON_ARRAY_SIZE);

	float scale = json_scale->valuedouble;
	Vec3 position, rotation;

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_rotation, rotation);

	FILE *file = fopen(json_filename->valuestring, "rb");
	err_assert(file, ERR_JSON_IO_OPEN);

	Object *object = malloc(sizeof(Object));
	object_load(json, context, object, OBJECT_TRIANGLE);

	stl_load_objects(context, file, object, position, rotation, scale);
	fclose(file);
}

uint32_t stl_get_num_triangles(FILE *file)
{
	err_assert(fseek(file, sizeof(uint8_t[80]), SEEK_SET) == 0, ERR_STL_IO_FP);
	uint32_t num_triangles;
	err_assert(fread(&num_triangles, sizeof(uint32_t), 1, file) == 1, ERR_STL_IO_READ);
	return num_triangles;
}

//assumes that file is at SEEK_SET
void stl_load_objects(Context *context, FILE *file, const Object *object, const Vec3 position, const Vec3 rot, const float scale)
{
	//ensure that file is binary instead of ascii
	char header[5];
	err_assert(fread(header, sizeof(char), 5, file) == 5, ERR_STL_IO_READ);
	err_assert(strncmp("solid", header, 5), ERR_STL_ENCODING);

	float a = cosf(rot[Z]) * sinf(rot[Y]);
	float b = sinf(rot[Z]) * sinf(rot[Y]);
	Mat3 rotation_matrix = {
		{
			cosf(rot[Z]) * cosf(rot[Y]),
			a * sinf(rot[X]) - sinf(rot[Z]) * cosf(rot[X]),
			a * cosf(rot[X]) + sinf(rot[Z]) * sinf(rot[X])
		}, {
			sinf(rot[Z]) * cosf(rot[Y]),
			b * sinf(rot[X]) + cosf(rot[Z]) * cosf(rot[X]),
			b * cosf(rot[X]) - cosf(rot[Z]) * sinf(rot[X])
		}, {
			-sinf(rot[Y]),
			cosf(rot[Y]) * sinf(rot[X]),
			cosf(rot[Y]) * cosf(rot[X])
		}
	};

	uint32_t num_triangles = stl_get_num_triangles(file);
	context->objects_size += num_triangles - 1;
	context->objects = realloc(context->objects, sizeof(Object[context->objects_size]));
	err_assert(context->objects, ERR_MALLOC);

	uint32_t i;
	for (i = 0; i < num_triangles; i++) {
		STLTriangle stl_triangle;
		err_assert(fread(&stl_triangle, sizeof(STLTriangle), 1, file) == 1, ERR_STL_IO_READ);

		uint32_t j;
		for (j = 0; j < 3; j++) {
			Vec3 temp_vertices;
			mulm3(rotation_matrix, stl_triangle.vertices[j], temp_vertices);
			mul3s(temp_vertices, scale, stl_triangle.vertices[j]);
			add3v(stl_triangle.vertices[j], position, stl_triangle.vertices[j]);
		}
		Triangle *triangle = malloc(sizeof(Triangle));
		memcpy(triangle, object, sizeof(Object));
		memcpy(triangle->vertices, stl_triangle.vertices, sizeof(Vec3[3]));
		triangle_init(triangle);
		object_add((Object*)triangle, context);
	}
}

/*******************************************************************************
*	BoundingCuboid
*******************************************************************************/

BoundingCuboid *bounding_cuboid_new(const float epsilon, Vec3 corners[2])
{
	BoundingCuboid *bounding_cuboid = malloc(sizeof(BoundingCuboid));
	bounding_cuboid->epsilon = epsilon;
	memcpy(bounding_cuboid->corners, corners, sizeof(Vec3[2]));
	return bounding_cuboid;
}

void bounding_cuboid_delete(BoundingCuboid *bounding_cuboid)
{
	free(bounding_cuboid);
}

bool bounding_cuboid_intersects(const BoundingCuboid *cuboid, const Line *ray, float *tmax, float *tmin)
{
	float tymin, tymax;

	float divx = 1 / ray->vector[X];
	if (divx >= 0) {
		*tmin = (cuboid->corners[0][X] - ray->position[X]) * divx;
		*tmax = (cuboid->corners[1][X] - ray->position[X]) * divx;
	} else {
		*tmin = (cuboid->corners[1][X] - ray->position[X]) * divx;
		*tmax = (cuboid->corners[0][X] - ray->position[X]) * divx;
	}
	float divy = 1 / ray->vector[Y];
	if (divy >= 0) {
		tymin = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
	} else {
		tymin = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
	}

	if ((*tmin > tymax) || (tymin > *tmax))
		return false;
	if (tymin > *tmin)
		*tmin = tymin;
	if (tymax < *tmax)
		*tmax = tymax;

	float tzmin, tzmax;

	float divz = 1 / ray->vector[Z];
	if (divz >= 0) {
		tzmin = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
	} else {
		tzmin = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
	}

	if (*tmin > tzmax || tzmin > *tmax)
		return false;
	if (tzmin > *tmin)
		*tmin = tzmin;
	if (tzmax < *tmax)
		*tmax = tzmax;
	return *tmax > cuboid->epsilon;
}

/*******************************************************************************
*	JSON
*******************************************************************************/

void cJSON_parse_float_array(const cJSON *json, float *array)
{
	size_t i = 0;
	cJSON *json_iter;
	cJSON_ArrayForEach (json_iter, json) {
		err_assert(cJSON_IsNumber(json_iter), ERR_JSON_VALUE_NOT_NUMBER);
		array[i++] = json_iter->valuedouble;
	}
}

void scene_load(Context *context)
{
	fseek(context->scene_file, 0, SEEK_END);
  	size_t length = ftell(context->scene_file);
  	fseek(context->scene_file, 0, SEEK_SET);
  	char *buffer = malloc(length + 1);
	err_assert(buffer, ERR_MALLOC);
	err_assert(fread(buffer, 1, length, context->scene_file) == length, ERR_JSON_IO_READ);
	buffer[length] = '\0';
	cJSON *json = cJSON_Parse(buffer);
	free(buffer);
	err_assert(json, ERR_JSON_IO_READ);
	err_assert(cJSON_IsObject(json), ERR_JSON_FIRST_TOKEN);

	cJSON *json_materials = cJSON_GetObjectItemCaseSensitive(json, "Materials"),
		*json_objects = cJSON_GetObjectItemCaseSensitive(json, "Objects"),
		*json_lights = cJSON_GetObjectItemCaseSensitive(json, "Lights"),
		*json_camera = cJSON_GetObjectItemCaseSensitive(json, "Camera"),
		*json_ambient_light = cJSON_GetObjectItemCaseSensitive(json, "AmbientLight");

	err_assert(cJSON_IsObject(json_camera), ERR_JSON_NO_CAMERA);
	err_assert(cJSON_IsArray(json_lights)
		&& cJSON_IsArray(json_objects)
		&& cJSON_IsArray(json_materials), ERR_JSON_VALUE_NOT_ARRAY);

	context->camera = camera_load(json_camera, context);

	int num_objects = context->objects_size = cJSON_GetArraySize(json_objects),
		num_lights = cJSON_GetArraySize(json_lights),
		num_materials = cJSON_GetArraySize(json_materials);
	err_assert(num_lights > 0, ERR_JSON_NO_LIGHTS);
	err_assert(num_objects > 0, ERR_JSON_NO_OBJECTS);
	err_assert(num_materials > 0, ERR_JSON_NO_MATERIALS);
	context->objects = malloc(sizeof(Object*) * num_objects);
	context->lights = malloc(sizeof(Light[num_lights]));
	context->materials = malloc(sizeof(Material[num_materials]));
	err_assert(context->objects
		&& context->lights
		&& context->materials, ERR_MALLOC);

	if (cJSON_IsArray(json_ambient_light) && cJSON_GetArraySize(json_ambient_light) == 3)
		cJSON_parse_float_array(json_ambient_light, context->global_ambient_light_intensity);

	cJSON *json_iter;
#ifdef UNBOUND_OBJECTS
	size_t num_unbound_objects = 0;
	cJSON_ArrayForEach (json_iter, json_objects) {
		err_assert(cJSON_IsObject(json_iter), ERR_JSON_VALUE_NOT_OBJECT);
		cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json_iter, "type");
		err_assert(cJSON_IsString(json_type), ERR_JSON_VALUE_NOT_STRING);
		if (djb_hash(json_type->valuestring) == 232719795)
			num_unbound_objects++;
	}
	if (num_unbound_objects)
		context->unbound_objects = malloc(sizeof(Object*) * num_unbound_objects);
#endif /* UNBOUND_OBJECTS */

	cJSON_ArrayForEach (json_iter, json_materials) {
		err_assert(cJSON_IsObject(json_iter), ERR_JSON_VALUE_NOT_OBJECT);
		material_load(json_iter, &context->materials[context->num_materials++]);
	}

	cJSON_ArrayForEach (json_iter, json_objects) {
		cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json_iter, "type"),
			*json_parameters = cJSON_GetObjectItemCaseSensitive(json_iter, "parameters");
		err_assert(cJSON_IsObject(json_parameters), ERR_JSON_VALUE_NOT_OBJECT);
		Object *object;
		switch (djb_hash(json_type->valuestring)) {
		case 3324768284: /* Sphere */
			object = (Object*)sphere_load(json_parameters, context);
			break;
		case 103185867: /* Triangle */
			object = (Object*)triangle_load(json_parameters, context);
			break;
		case 232719795: /* Plane */
#ifdef UNBOUND_OBJECTS
			object = (Object*)plane_load(json_parameters, context);
			break;
#else
			continue;
#endif
		case 2088783990: /* Mesh */
			mesh_load(json_parameters, context);
			continue;
		}
#ifdef UNBOUND_OBJECTS
		if (!object->object_data->is_bounded)
			context->unbound_objects[context->num_unbound_objects++] = object;
#endif
		object_add(object, context);
	}

	cJSON_ArrayForEach (json_iter, json_lights) {
		err_assert(cJSON_IsObject(json_iter), ERR_JSON_VALUE_NOT_OBJECT);
		light_load(json_iter,&context->lights[context->num_lights++]);
	}

	cJSON_Delete(json);
}

/*******************************************************************************
*	MISC
*******************************************************************************/

void err(const ErrorCode error_code)
{
	printf(ERR_FORMAT_STRING, ERR_MESSAGES[error_code]);
	exit(error_code);
}

void get_closest_intersection(const Context *context, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
#ifdef UNBOUND_OBJECTS
	unbound_objects_get_closest_intersection(context, ray, closest_object, closest_normal, closest_distance);
#endif
	bvh_get_closest_intersection(context->bvh, ray, closest_object, closest_normal, closest_distance);
}

bool is_light_blocked(const Context *context, const Line *ray, const float distance, Vec3 light_intensity)
{
#ifdef UNBOUND_OBJECTS
	return unbound_objects_is_light_blocked(context, ray, distance, light_intensity)
		|| bvh_is_light_blocked(context->bvh, ray, distance, light_intensity);
#else
	return bvh_is_light_blocked(context->bvh, ray, distance, light_intensity);
#endif
}

//Places all objects in scene in cube of side length 1
void normalize_scene(Context *context)
{
	Vec3 min = {FLT_MAX}, max = {FLT_MIN};
	size_t i, j;
	for (i = 0; i < context->num_objects; i++) {
		Object *object = context->objects[i];
#ifdef UNBOUND_OBJECTS
		if (object->object_data->is_bounded) {
#endif
			Vec3 corners[2];
			object->object_data->get_corners(object, corners);
			for (j = 0; j < 3; j++) {
				if (corners[0][j] < min[j])
					min[j] = corners[0][j];
				if (corners[1][j] > max[j])
					max[j] = corners[1][j];
			}
#ifdef UNBOUND_OBJECTS
		}
#endif
	}

	Vec3 range;
	sub3v(max, min, range);
	float scale_factor = 1.f / max3(range);

	for (i = 0; i < context->num_objects; i++)
		context->objects[i]->object_data->scale(context->objects[i], min, scale_factor);

	for (i = 0; i < context->num_lights; i++)
		light_scale(&context->lights[i], min, scale_factor);

	camera_scale(context->camera, min, scale_factor);
}

void cast_ray(const Context *context, const Line *ray, const Vec3 kr, Vec3 color, const uint32_t remaining_bounces, Object *inside_object)
{
	Object *closest_object = NULL;
	Vec3 normal;
	float min_distance;
	Object *obj = inside_object;

	if (inside_object && inside_object->object_data->get_intersection(obj, ray, &min_distance, normal)) {
		closest_object = inside_object;
	} else {
		min_distance = FLT_MAX;
		get_closest_intersection(context, ray, &closest_object, normal, &min_distance);
	}
	Object *object = closest_object;
	if (! object)
		return;

	//LIGHTING MODEL
	Vec3 obj_color;

	Material *material = object->material;

	//emittance
	memcpy(obj_color, material->ke, sizeof(Vec3));

	//Line originating at point of intersection
	Line outgoing_ray;
	mul3s(ray->vector, min_distance, outgoing_ray.position);
	add3v(outgoing_ray.position, ray->position, outgoing_ray.position);

	float b = dot3(normal, ray->vector);
	bool is_outside = signbit(b);

	size_t i;
	for (i = 0; i < context->num_lights; i++) {
		Light *light = &context->lights[i];

		sub3v(light->position, outgoing_ray.position, outgoing_ray.vector);
		float light_distance = mag3(outgoing_ray.vector);
		mul3s(outgoing_ray.vector, 1.f / light_distance, outgoing_ray.vector);

		float a = dot3(outgoing_ray.vector, normal);

		Vec3 incoming_light_intensity;
		memcpy(incoming_light_intensity, light->intensity, sizeof(Vec3));
		if (is_outside
			&& !is_light_blocked(context, &outgoing_ray, light_distance, incoming_light_intensity)) {

			Vec3 distance;
			sub3v(light->position, outgoing_ray.position, distance);
			switch (context->light_attenuation) {
			case LIGHT_ATTENUATION_NONE:
				break;
			case LIGHT_ATTENUATION_LINEAR:
				mul3s(incoming_light_intensity, 1.f / (1.f + mag3(distance)), incoming_light_intensity);
				break;
			case LIGHT_ATTENUATION_SQUARE:
				mul3s(incoming_light_intensity, 1.f / (1.f + magsqr3(distance)), incoming_light_intensity);
				break;
			}

			Vec3 diffuse;
			material->texture->get_color(material->texture, outgoing_ray.position, diffuse);
			mul3v(diffuse, incoming_light_intensity, diffuse);
			mul3s(diffuse, fmaxf(0., a), diffuse);

			Vec3 reflected;
			float specular_mul;
			switch (context->reflection_model) {
			case REFLECTION_PHONG:
				mul3s(normal, 2 * a, reflected);
				sub3v(reflected, outgoing_ray.vector, reflected);
				specular_mul = - dot3(reflected, ray->vector);
				break;
			case REFLECTION_BLINN:
				mul3s(outgoing_ray.vector, -1.f, reflected);
				add3v(reflected, ray->vector, reflected);
				norm3(reflected);
				specular_mul = - dot3(normal, reflected);
				break;
			}
			Vec3 specular;
			mul3v(material->ks, incoming_light_intensity, specular);
			mul3s(specular, fmaxf(0., powf(specular_mul, material->shininess)), specular);

			add3v3(obj_color, diffuse, specular, obj_color);
		}
	}

	//global illumination
	switch (context->global_illumination_model) {
	case GLOBAL_ILLUMINATION_AMBIENT:
		mul3v(material->ka, context->global_ambient_light_intensity, obj_color);
		break;
	case GLOBAL_ILLUMINATION_PATH_TRACING:
		if (remaining_bounces && is_outside) {
			Mat3 rotation_matrix;
			if (normal[Y] - object->epsilon < -1.f) {
				Mat3 vx = {
					{1.f, 0.f, 0.f},
					{0.f, -1.f, 0.f},
					{0.f, 0.f, -1.f},
				};
				memcpy(rotation_matrix, vx, sizeof(Mat3));
			} else {
				float mul = 1.f / (1.f + dot3((Vec3){0.f, 1.f, 0.f}, normal));
				Mat3 vx = {
					{
						1 - sqr(normal[X]) * mul,
						normal[X],
						-normal[X] * normal[Z] * mul,
					},
					{
						-normal[X],
						1 - (sqr(normal[X]) + sqr(normal[Z])) * mul,
						-normal[Z],
					},
					{
						-normal[X] * normal[Z] * mul,
						normal[Z],
						1 - sqr(normal[Z]) * mul,
					},
				};
				memcpy(rotation_matrix, vx, sizeof(Mat3));
			}

			Vec3 delta = {1.f, 1.f, 1.f};
			size_t num_samples;
			if (remaining_bounces == context->max_bounces) {
				num_samples = context->samples_per_pixel;
				mul3s(delta, 1.f / (float)num_samples, delta);
			} else {
				num_samples = 1;
			}

			Vec3 light_mul;
			for (i = 0; i < num_samples; i++) {
				float inclination = acosf(rand_flt() * 2.f - 1.f);
				float azimuth = rand_flt() * PI;
				Vec3 apoint = {
					cosf(azimuth) * sinf(inclination),
					sinf(azimuth) * sinf(inclination),
					cosf(inclination),
				};
				mulm3(rotation_matrix, apoint, outgoing_ray.vector);
				mul3s(delta, dot3(normal, outgoing_ray.vector), light_mul);
				cast_ray(context, &outgoing_ray, light_mul, obj_color, 0, NULL);
			}
		}
		break;
	}

	mul3v(obj_color, kr, obj_color);
	switch (context->light_attenuation) {
	case LIGHT_ATTENUATION_NONE:
		break;
	case LIGHT_ATTENUATION_LINEAR:
		mul3s(obj_color, 1.f / (1.f + min_distance), obj_color);
		break;
	case LIGHT_ATTENUATION_SQUARE:
		mul3s(obj_color, 1.f / sqr(1.f + min_distance), obj_color);
		break;
	}
	add3v(color, obj_color, color);

	if(!remaining_bounces)
		return;

	//reflection
	if (inside_object != object
		&& material->reflective) {
		Vec3 reflected_kr;
		mul3v(kr, material->kr, reflected_kr);
		if (context->minimum_light_intensity_sqr < magsqr3(reflected_kr)) {
			mul3s(normal, 2 * b, outgoing_ray.vector);
			sub3v(ray->vector, outgoing_ray.vector, outgoing_ray.vector);
			cast_ray(context, &outgoing_ray, reflected_kr, color, remaining_bounces - 1, NULL);
		}
	}

	//transparency
	if (material->transparent) {
		Vec3 refracted_kt;
		mul3v(kr, material->kt, refracted_kt);
		if (context->minimum_light_intensity_sqr < magsqr3(refracted_kt)) {
			float incident_angle = acosf(fabs(b));
			float refractive_multiplier = is_outside ? 1.f / material->refractive_index : material->refractive_index;
			float refracted_angle = asinf(sinf(incident_angle) * refractive_multiplier);
			float delta_angle = refracted_angle - incident_angle;
			Vec3 c, f, g, h;
			cross(ray->vector, normal, c);
			norm3(c);
			if (!is_outside)
				mul3s(c, -1.f, c);
			cross(c, ray->vector, f);
			mul3s(ray->vector, cosf(delta_angle), g);
			mul3s(f, sinf(delta_angle), h);
			add3v(g, h, outgoing_ray.vector);
			norm3(outgoing_ray.vector);
			cast_ray(context, &outgoing_ray, refracted_kt, color, remaining_bounces - 1, object);
		}
	}
}

void create_image(const Context *context)
{
	Vec3 kr = {1.f, 1.f, 1.f};
	Camera *camera = context->camera;
	Vec3 *raw_pixels = calloc(context->resolution[X] * context->resolution[Y], sizeof(Vec3));
#ifdef MULTITHREADING
#pragma omp parallel for
#endif
	for (uint32_t row = 0; row < camera->image.resolution[Y]; row++) {
		Vec3 pixel_position;
		mul3s(camera->image.vectors[Y], row, pixel_position);
		add3v(pixel_position, camera->image.corner, pixel_position);
		Line ray;
		memcpy(ray.position, camera->position, sizeof(Vec3));
		uint32_t pixel_index = camera->image.resolution[X] * row;
		uint32_t col;
		for (col = 0; col < camera->image.resolution[X]; col++) {
			add3v(pixel_position, camera->image.vectors[X], pixel_position);
			sub3v(pixel_position, camera->position, ray.vector);
			norm3(ray.vector);
			cast_ray(context, &ray, kr, raw_pixels[pixel_index], context->max_bounces, NULL);
			pixel_index++;
		}
	}

	size_t image_size = camera->image.resolution[Y] * camera->image.resolution[X];
	size_t i;
	if (context->normalize_colors) {
		float min = FLT_MAX, max = FLT_MIN;
		for (i = 0; i < image_size; i++) {
			float *raw_pixel = raw_pixels[i];
			float pixel_min = min3(raw_pixel), pixel_max = max3(raw_pixel);
			if (pixel_min < min)
				min = pixel_min;
			if (pixel_max > max)
				max = pixel_max;
		}

		float slope = 1.f / (max - min);

		for (i = 0; i < image_size; i++) {
			sub3s(raw_pixels[i], min, raw_pixels[i]);
			mul3s(raw_pixels[i], slope, raw_pixels[i]);
		}
	}

	for (i = 0; i < image_size; i++) {
		uint8_t *pixel = camera->image.pixels[i];
		pixel[0] = (uint8_t)fmaxf(fminf(raw_pixels[i][0] * 255.f, 255.f), 0.f);
		pixel[1] = (uint8_t)fmaxf(fminf(raw_pixels[i][1] * 255.f, 255.f), 0.f);
		pixel[2] = (uint8_t)fmaxf(fminf(raw_pixels[i][2] * 255.f, 255.f), 0.f);
	}
}

void process_arguments(int argc, char *argv[], Context *context)
{
	if (argc < 5) {
		if (argc == 2) {
			if (! strcmp("--help", argv[1]) || ! strcmp("-h", argv[1])) {
				printf("%s", HELPTEXT);
				exit(0);
			}
		}
		err(ERR_ARGC);
	}

	context->scene_file = fopen(argv[1], "rb");
	err_assert(context->scene_file, ERR_ARGV_IO_OPEN_SCENE);

	err_assert(strstr(argv[2], ".ppm"), ERR_ARGV_FILE_EXT);
	context->output_file = fopen(argv[2], "wb");
	err_assert(context->output_file, ERR_ARGV_IO_OPEN_OUTPUT);

	context->resolution[X] = abs(atoi(argv[3]));
	context->resolution[Y] = abs(atoi(argv[4]));

	int32_t i;
	for (i = 5; i < argc; i += 2) {
		switch (djb_hash(argv[i])) {
			case 5859045://-m
				#ifdef MULTITHREADING
				if (djb_hash(argv[i + 1]) == 193414065) {//max
					NUM_THREADS = omp_get_max_threads();
				} else {
					NUM_THREADS = atoi(argv[i + 1]);
					err_assert(NUM_THREADS <= omp_get_max_threads(), ERR_ARGV_NUM_THREADS);
				}
				#else
				err(ERR_ARGV_MULTITHREADING);
				#endif /* MULTITHREADING */
				break;
			case 5859050://-b
				context->max_bounces = abs(atoi(argv[i + 1]));
				break;
			case 5859049://-a
				context->minimum_light_intensity_sqr = sqr(atof(argv[i + 1]));
				break;
			case 5859067://-s
				switch (djb_hash(argv[i + 1])) {
				case 187940251://phong
					context->reflection_model = REFLECTION_PHONG;
					break;
				case 175795714://blinn
					context->reflection_model = REFLECTION_BLINN;
					break;
				default:
					err(ERR_ARGV_REFLECTION);
				}
				break;
			case 5859055://-g
				switch (djb_hash(argv[i + 1])) {
				case 354625309://ambient
					context->global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT;
					break;
				case 2088095368://path
					context->global_illumination_model = GLOBAL_ILLUMINATION_PATH_TRACING;
					break;
				default:
					err(ERR_ARGV_GLOBAL_ILLUMINATION);
				}
				break;
			case 5859046://-n
				context->samples_per_pixel = abs(atoi(argv[i + 1]));
				break;
			case 5859051://-c
				context->normalize_colors = true;
				i--;
				break;
			case 5859044://-l
				switch (djb_hash(argv[i + 1])) {
				case 2087865487://none
					context->light_attenuation = LIGHT_ATTENUATION_NONE;
					break;
				case 193412846://lin
					context->light_attenuation = LIGHT_ATTENUATION_LINEAR;
					break;
				case 193433013://sqr
					context->light_attenuation = LIGHT_ATTENUATION_SQUARE;
					break;
				default:
					err(ERR_ARGV_LIGHT_ATTENUATION);
				}
				break;
			case 5859064://-p
				switch (djb_hash(argv[i + 1])) {
				case 2087865487://none
					context->log_option = LOG_NONE;
					break;
				case 2088303039://real
					context->log_option = LOG_REALTIME;
					break;
				case 193416643://cpu
					context->log_option = LOG_CPUTIME;
					break;
				}
				break;
			default:
				err(ERR_ARGV_UNRECOGNIZED);
		}
	}
}

void log_msg(const Context *context, const char *msg)
{
	double t;
	switch (context->log_option) {
	case LOG_NONE:
		return;
	case LOG_REALTIME: {
		struct timespec cur_t;
		timespec_get(&cur_t, TIME_UTC);
		t = (cur_t.tv_sec - context->start_timespec.tv_sec) + (cur_t.tv_nsec - context->start_timespec.tv_nsec) * 1e-9;
		}
		break;
	case LOG_CPUTIME: {
		clock_t cur_t = clock();
		t = (double)(cur_t - context->start_clock) / CLOCKS_PER_SEC;
		}
		break;
	}
	printf("[%07.3f] %s\n", t, msg);
}

int main(int argc, char *argv[])
{
	Context *context = context_new();

	srand((unsigned) context->start_timespec.tv_sec);

	process_arguments(argc, argv, context);

#ifdef MULTITHREADING
	omp_set_num_threads(NUM_THREADS);
#endif

	log_msg(context, "INITIALIZING SCENE.");

	scene_load(context);
	fclose(context->scene_file);
	context->scene_file = NULL;
	normalize_scene(context);
	bvh_generate(context);

	log_msg(context, "INITIALIZED SCENE. BEGAN RENDERING.");

	create_image(context);

	log_msg(context, "FINISHED RENDERING. SAVING IMAGE.");

	save_image(context->output_file, &context->camera->image);
	fclose(context->output_file);

	log_msg(context, "SAVED IMAGE.");

	context_delete(context);

	log_msg(context, "TERMINATED PROGRAM.");
}
