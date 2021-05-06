#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/cJSON.h"

/* TODO:
	Treat emittant objects as lights
	Path tracing
*/

/*******************************************************************************
*	MACRO
*******************************************************************************/

#ifdef DISPLAY_TIME
#include <time.h>
#define PRINT_TIME(msg)\
	timespec_get(&current_t, TIME_UTC);\
	printf("[%07.3f] %s\n", (double)((current_t.tv_sec - start_t.tv_sec) + (current_t.tv_nsec - start_t.tv_nsec) * 1e-9f), msg);
#else
#define PRINT_TIME(format)
#endif

#ifdef MULTITHREADING
#include <omp.h>
uint32_t NUM_THREADS = 1;
#endif

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
#define OBJECT_MEMBERS\
	ObjectFunctions *functions;\
	float epsilon;\
	Material *material;\
	BoundingCuboid *bounding_cuboid
#define OBJECT_INIT_PARAMS\
	const float epsilon,\
	Material *material
#define OBJECT_INIT_VARS\
	epsilon,\
	material
#define OBJECT_INIT_VARS_DECL\
	Material *material;\
	float epsilon
#define OBJECT_NEW(var_type, name, enum_type)\
	var_type *name = malloc(sizeof(var_type));\
	name->functions = &OBJECT_FUNCTIONS[enum_type];\
	name->epsilon = epsilon;\
	name->material = material
#define OBJECT_FUNCTIONS_INIT(name) {\
	.get_intersection = &name##_get_intersection,\
	.intersects_in_range = &name##_intersects_in_range,\
	.delete = &name##_delete,\
	}
#define SCENE_OBJECT_LOAD_VARS\
	json,\
	context,\
	&epsilon,\
	&material
#define OBJECT_LOAD_PARAMS\
	const cJSON *json,\
	const Context *context,\
	float *epsilon,\
	Material **material

/*******************************************************************************
*	TYPE DEFINITION
*******************************************************************************/

typedef uint8_t Color[3];
typedef float Vec2[2];
typedef float Vec3[3];

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

/* Object */
typedef union Object Object;
typedef struct CommonObject CommonObject;
typedef struct Plane Plane;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct ObjectFunctions ObjectFunctions;

/*******************************************************************************
*	GLOBAL
*******************************************************************************/

enum object_type{OBJECT_PLANE, OBJECT_SPHERE, OBJECT_TRIANGLE};
enum directions{X, Y, Z};
typedef enum {REFLECTION_PHONG, REFLECTION_BLINN} ReflectionModel;

/* ERROR */
typedef enum {
	ERR_ARGC,
	ERR_ARGV_FILE_EXT,
	ERR_ARGV_NUM_THREADS,
	ERR_ARGV_MULTITHREADING,
	ERR_ARGV_REFLECTION,
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
-a (float)     : DEFAULT = 0.01  : specifies the minimum light intensity for which a ray is cast\n\
-s (string)    : DEFAULT = phong : specifies the reflection model\n\
	Accepted values:\n\
	phong      : phong reflection model\n\
	blinn      : blinn-phong reflection model\n";

/*******************************************************************************
*	STRUCTURE DEFINITION
*******************************************************************************/

typedef struct Context {
	FILE *scene_file;
	FILE *output_file;

	Object *objects;
	size_t num_objects;
	size_t objects_size;
	Object *objects_without_bounding; //Since planes do not have a bounding cuboid and cannot be included in the AABB Tree, they must be looked at separately
	size_t num_objects_without_bounding;
	Light *lights;
	size_t num_lights;
	Material *materials;
	size_t num_materials;
	Camera *camera;

	Vec3 global_ambient_light_intensity;
	uint32_t max_bounces;
	float minimum_light_intensity_sqr;
	ReflectionModel reflection_model;
	uint32_t resolution[2];
} Context;

typedef struct Line {
	float vector[3];
	float position[3];
} Line;

typedef struct  STLTriangle {
	float normal[3];//normal is unreliable so it is not used.
	float vertices[3][3];
	uint16_t attribute_bytes;//attribute bytes is unreliable so it is not used.
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
	Vec3 kd; /*diffuse reflection constant*/
	Vec3 ka; /*ambient reflection constant*/
	Vec3 kr; /*specular interreflection constant*/
	Vec3 kt; /*transparency constant*/
	Vec3 ke; /*emittance constant*/
	float shininess; /*shininess constant*/
	float refractive_index;
	bool reflective;
	bool transparent;
} Material;

/* Object */
typedef union Object {
	CommonObject *common;
	Sphere *sphere;
	Triangle *triangle;
	Plane *plane;
} Object;

typedef struct CommonObject {
	OBJECT_MEMBERS;
} CommonObject;

typedef struct Sphere {
	OBJECT_MEMBERS;
	Vec3 position;
	float radius;
} Sphere;

typedef struct Triangle {//triangle ABC
	OBJECT_MEMBERS;
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
	Vec3 normal;
} Triangle;

typedef struct Plane {//normal = {a,b,c}, ax + by + cz = d
	OBJECT_MEMBERS;
	Vec3 normal;
	float d;
} Plane;

typedef struct ObjectFunctions {
	bool (*get_intersection)(const Object, const Line*, float*, Vec3);
	bool (*intersects_in_range)(const Object, const Line*, float);
	void (*delete)(Object);
} ObjectFunctions;

/* TODO:
typedef struct AABBTree {} */

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
void mul2(const Vec2 vec, const float mul, Vec2 result);
void mul3(const Vec3 vec, const float mul, Vec3 result);
void mul3v(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void div2(const Vec2 vec, const float div, Vec2 result);
void div3(const Vec3 vec, const float div, Vec3 result);
void add2(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void add2s(const Vec2 vec1, const float summand, Vec2 result);
void add3(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void add3s(const Vec3 vec1, const float summand, Vec3 result);
void add3_3(const Vec3 vec1, const Vec3 vec2, const Vec3 vec3, Vec3 result);
void sub2(const Vec2 vec1, const Vec2 vec2, Vec2 result);
void sub2s(const Vec2 vec1, const float subtrahend, Vec2 result);
void sub3(const Vec3 vec1, const Vec3 vec2, Vec3 result);
void sub3s(const Vec3 vec1, const float subtrahend, Vec3 result);
void norm2(Vec2 vec);
void norm3(Vec3 vec);
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

/* Light */
void light_init(Light *light, const Vec3 position, const Vec3 intensity);
void light_load(const cJSON *json, Light *light);

/* Material */
void material_init(Material *material, const int32_t id, const Vec3 ks, const Vec3 kd, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, const float shininess, const float refractive_index);
void material_load(const cJSON *json, Material *material);
Material *get_material(const Context *context, const int32_t id);

/* Object */
void object_add(const Object object, Context *context);
void object_params_load(OBJECT_LOAD_PARAMS);

/* Sphere */
Sphere *sphere_new(OBJECT_INIT_PARAMS, const Vec3 position, const float radius);
void sphere_delete(Object object);
Sphere *sphere_load(const cJSON *json, Context *context);
bool sphere_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal);
bool sphere_intersects_in_range(const Object object, const Line *ray, const float min_distance);

/* Triangle */
Triangle *triangle_new(OBJECT_INIT_PARAMS, Vec3 vertices[3]);
void triangle_delete(Object object);
Triangle *triangle_load(const cJSON *json, Context *context);
bool triangle_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal);
bool triangle_intersects_in_range(const Object object, const Line *ray, float min_distance);

/* Plane */
Plane *plane_new(OBJECT_INIT_PARAMS, const Vec3 normal, const Vec3 point);
void plane_delete(Object object);
Plane *plane_load(const cJSON *json, Context *context);
bool plane_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal);
bool plane_intersects_in_range(const Object object, const Line *ray, float min_distance);

/* Mesh */
void mesh_load(const cJSON *json, Context *context);
uint32_t stl_get_num_triangles(FILE *file);
void stl_load_objects(OBJECT_INIT_PARAMS, Context *context, FILE *file, const Vec3 position, const Vec3 rot, const float scale);

/* BoundingCuboid */
BoundingCuboid *bounding_cuboid_new(const float epsilon, Vec3 corners[2]);
void bounding_cuboid_delete(BoundingCuboid *bounding_cuboid);
bool bounding_cuboid_intersects(const BoundingCuboid *bounding_cuboid, const Line *ray);

/* SCENE */
void cJSON_parse_float_array(const cJSON *json, float *array);
void scene_load(Context *context);

/* MISC */
void err(const ErrorCode error_code);
float get_closest_intersection(const Context *context, const Line *ray, Object *closest_object, Vec3 closest_normal);
bool is_intersection_in_distance(const Context *context, const Line *ray, const float min_distance, const CommonObject *excl_object, Vec3 light_intensity);
void cast_ray(const Context *context, const Line *ray, const Vec3 kr, Vec3 color, const uint32_t bounce_count, CommonObject *inside_object);
void create_image(const Context *context);
void process_arguments(int argc, char *argv[], Context *context);
int main(int argc, char *argv[]);

/* ObjectFunctions */
ObjectFunctions OBJECT_FUNCTIONS[] = {
	[OBJECT_PLANE] = OBJECT_FUNCTIONS_INIT(plane),
	[OBJECT_SPHERE] = OBJECT_FUNCTIONS_INIT(sphere),
	[OBJECT_TRIANGLE] = OBJECT_FUNCTIONS_INIT(triangle),
};

/*******************************************************************************
*	ALGORITHM
*******************************************************************************/

float sqr(const float val)
{
	return val * val;
}

float mag2(const Vec2 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]));
}

float mag3(const Vec3 vec)
{
	return sqrtf(sqr(vec[X]) + sqr(vec[Y]) + sqr(vec[Z]));
}

float dot2(const Vec2 vec1, const Vec2 vec2)
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y];
}

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

void mul2(const Vec2 vec, const float mul, Vec2 result)
{
	result[X] = vec[X] * mul;
	result[Y] = vec[Y] * mul;
}

void mul3(const Vec3 vec, const float mul, Vec3 result)
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

void div2(const Vec2 vec, const float div, Vec2 result)
{
	result[X] = vec[X] / div;
	result[Y] = vec[Y] / div;
}

void div3(const Vec3 vec, const float div, Vec3 result)
{
	result[X] = vec[X] / div;
	result[Y] = vec[Y] / div;
	result[Z] = vec[Z] / div;
}

void add2(const Vec2 vec1, const Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] + vec2[X];
	result[Y] = vec1[Y] + vec2[Y];
}

void add2s(const Vec2 vec1, const float summand, Vec2 result)
{
	result[X] = vec1[X] + summand;
	result[Y] = vec1[Y] + summand;
}

void add3(const Vec3 vec1, const Vec3 vec2, Vec3 result)
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

void add3_3(const Vec3 vec1, const Vec3 vec2, const Vec3 vec3, Vec3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void sub2(const Vec2 vec1, const Vec2 vec2, Vec2 result)
{
	result[X] = vec1[X] - vec2[X];
	result[Y] = vec1[Y] - vec2[Y];
}

void sub2s(const Vec2 vec1, const float subtrahend, Vec2 result)
{
	result[X] = vec1[X] - subtrahend;
	result[Y] = vec1[Y] - subtrahend;
}

void sub3(const Vec3 vec1, const Vec3 vec2, Vec3 result)
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
	div2(vec, mag2(vec), vec);
}

void norm3(Vec3 vec)
{
	div3(vec, mag3(vec), vec);
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
	sub3(line_position, vertex, s);
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
	sub3(line_position, sphere_position, relative_position);
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

/*******************************************************************************
*	Context
*******************************************************************************/

Context *context_new(void)
{
	Context *context = malloc(sizeof(Context));
	*context = (Context) {
			.max_bounces = 10,
			.minimum_light_intensity_sqr = .01f * .01f,
			.reflection_model = REFLECTION_PHONG,
			.resolution = {600, 400},
	};
	return context;
}

void context_delete(Context *context)
{
	size_t i;
	for (i = 0; i < context->num_objects; i++)
		context->objects[i].common->functions->delete(context->objects[i]);
	camera_delete(context->camera);
	free(context->objects);
	free(context->objects_without_bounding);
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
	mul3(camera->vectors[2], camera->focal_length, focal_vector);
	add3(focal_vector, camera->position, plane_center);
	mul3(camera->vectors[0], camera->image.size[X] / camera->image.resolution[X], camera->image.vectors[0]);
	mul3(camera->vectors[1], camera->image.size[Y] / camera->image.resolution[Y], camera->image.vectors[1]);
	mul3(camera->image.vectors[X], .5 - camera->image.resolution[X] / 2., corner_offset_vectors[X]);
	mul3(camera->image.vectors[Y], .5 - camera->image.resolution[Y] / 2., corner_offset_vectors[Y]);
	add3_3(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], camera->image.corner);

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

/*******************************************************************************
*	Material
*******************************************************************************/

void material_init(Material *material, const int32_t id, const Vec3 ks, const Vec3 kd, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, const float shininess, const float refractive_index)
{
	material->id = id;
	memcpy(material->ks, ks, sizeof(Vec3));
	memcpy(material->kd, kd, sizeof(Vec3));
	memcpy(material->ka, ka, sizeof(Vec3));
	memcpy(material->kr, kr, sizeof(Vec3));
	memcpy(material->kt, kt, sizeof(Vec3));
	memcpy(material->ke, ke, sizeof(Vec3));
	material->shininess = shininess;
	material->refractive_index = refractive_index;
	material->reflective = mag3(kr) > MATERIAL_THRESHOLD;
	material->transparent = mag3(kt) > MATERIAL_THRESHOLD;
}

void material_load(const cJSON *json, Material *material)
{
	cJSON  *json_id = cJSON_GetObjectItemCaseSensitive(json, "id"),
		*json_ks = cJSON_GetObjectItemCaseSensitive(json, "ks"),
		*json_kd = cJSON_GetObjectItemCaseSensitive(json, "kd"),
		*json_ka = cJSON_GetObjectItemCaseSensitive(json, "ka"),
		*json_kr = cJSON_GetObjectItemCaseSensitive(json, "kr"),
		*json_kt = cJSON_GetObjectItemCaseSensitive(json, "kt"),
		*json_ke = cJSON_GetObjectItemCaseSensitive(json, "ke"),
		*json_shininess = cJSON_GetObjectItemCaseSensitive(json, "shininess"),
		*json_refractive_index = cJSON_GetObjectItemCaseSensitive(json, "refractive_index");

	err_assert(cJSON_IsNumber(json_shininess)
		&& cJSON_IsNumber(json_refractive_index)
		&& cJSON_IsNumber(json_id), ERR_JSON_VALUE_NOT_NUMBER);
	err_assert(cJSON_IsArray(json_ks)
		&& cJSON_IsArray(json_kd)
		&& cJSON_IsArray(json_ka)
		&& cJSON_IsArray(json_kr)
		&& cJSON_IsArray(json_kt)
		&& cJSON_IsArray(json_ke), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_ks) == 3
		&& cJSON_GetArraySize(json_kd) == 3
		&& cJSON_GetArraySize(json_ka) == 3
		&& cJSON_GetArraySize(json_kr) == 3
		&& cJSON_GetArraySize(json_kt) == 3
		&& cJSON_GetArraySize(json_ke) == 3, ERR_JSON_ARRAY_SIZE);

	int32_t id;
	Vec3 ks, kd, ka, kr, kt, ke;
	float shininess, refractive_index;

	cJSON_parse_float_array(json_ks, ks);
	cJSON_parse_float_array(json_kd, kd);
	cJSON_parse_float_array(json_ka, ka);
	cJSON_parse_float_array(json_kr, kr);
	cJSON_parse_float_array(json_kt, kt);
	cJSON_parse_float_array(json_ke, ke);

	id = json_id->valueint;
	shininess = json_shininess->valuedouble;
	refractive_index = json_refractive_index->valuedouble;

	material_init(material, id, ks, kd, ka, kr, kt, ke, shininess, refractive_index);
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

/*******************************************************************************
*	Object
*******************************************************************************/

void object_add(const Object object, Context *context)
{
	context->objects[context->num_objects++] = object;
}

void object_params_load(OBJECT_LOAD_PARAMS)
{
	cJSON *json_epsilon = cJSON_GetObjectItemCaseSensitive(json, "epsilon"),
		*json_material = cJSON_GetObjectItemCaseSensitive(json, "material");

	err_assert(cJSON_IsNumber(json_epsilon)
		&& cJSON_IsNumber(json_material), ERR_JSON_VALUE_NOT_NUMBER);

	*epsilon = json_epsilon->valuedouble;
	*material = get_material(context, json_material->valueint);
}

/*******************************************************************************
*	Sphere
*******************************************************************************/

Sphere *sphere_new(OBJECT_INIT_PARAMS, const Vec3 position, const float radius)
{
	OBJECT_NEW(Sphere, sphere, OBJECT_SPHERE);
	memcpy(sphere->position, position, sizeof(Vec3));
	sphere->radius = radius;

	// BoundingCuboid
	Vec3 corners[2];
	sub3s(position, radius, corners[0]);
	add3s(position, radius, corners[1]);
	sphere->bounding_cuboid = bounding_cuboid_new(sphere->epsilon, corners);

	return sphere;
}


void sphere_delete(Object object)
{
	bounding_cuboid_delete(object.common->bounding_cuboid);
	free(object.common);
}

Sphere *sphere_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 2 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	OBJECT_INIT_VARS_DECL;
	object_params_load(SCENE_OBJECT_LOAD_VARS);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_radius = cJSON_GetObjectItemCaseSensitive(json, "radius");

	err_assert(cJSON_IsNumber(json_radius), ERR_JSON_VALUE_NOT_NUMBER);
	err_assert(cJSON_IsArray(json_position), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3, ERR_JSON_ARRAY_SIZE);

	Vec3 position;

	cJSON_parse_float_array(json_position, position);

	float radius = json_radius->valuedouble;

	return sphere_new(OBJECT_INIT_VARS, position, radius);
}

bool sphere_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal)
{
	Sphere *sphere = object.sphere;
	if (bounding_cuboid_intersects(sphere->bounding_cuboid, ray)
		&& line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->epsilon, distance)) {
			Vec3 position;
			mul3(ray->vector, *distance, position);
			add3(position, ray->position, position);
			sub3(position, sphere->position, normal);
			return true;
		}
	return false;
}

bool sphere_intersects_in_range(const Object object, const Line *ray, const float min_distance)
{
	Sphere *sphere = object.sphere;
	if (bounding_cuboid_intersects(sphere->bounding_cuboid, ray)) {
		float distance;
		bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->epsilon, &distance);
		if (intersects && distance < min_distance)
			return true;
	}
	return false;
}

/*******************************************************************************
*	Triangle
*******************************************************************************/

Triangle *triangle_new(OBJECT_INIT_PARAMS, Vec3 vertices[3])
{
	OBJECT_NEW(Triangle, triangle, OBJECT_TRIANGLE);
	memcpy(triangle->vertices, vertices, sizeof(Vec3[3]));
	sub3(vertices[1], vertices[0], triangle->edges[0]);
	sub3(vertices[2], vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);

	//BoundingCuboid
	Vec3 corners[2];
	memcpy(corners[0], vertices[2], sizeof(Vec3));
	memcpy(corners[1], vertices[2], sizeof(Vec3));
	size_t i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++) {
			if (corners[0][j] > vertices[i][j])
				corners[0][j] = vertices[i][j];
			else if (corners[1][j] < vertices[i][j])
				corners[1][j] = vertices[i][j];
		}
	triangle->bounding_cuboid = bounding_cuboid_new(triangle->epsilon, corners);

	return triangle;
}

void triangle_delete(Object object)
{
	bounding_cuboid_delete(object.common->bounding_cuboid);
	free(object.common);
}

Triangle *triangle_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 3 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	OBJECT_INIT_VARS_DECL;
	object_params_load(SCENE_OBJECT_LOAD_VARS);

	cJSON *json_vertex_1 = cJSON_GetObjectItemCaseSensitive(json, "vertex_1"),
		*json_vertex_2 = cJSON_GetObjectItemCaseSensitive(json, "vertex_2"),
		*json_vertex_3 = cJSON_GetObjectItemCaseSensitive(json, "vertex_3");

	err_assert(cJSON_IsArray(json_vertex_1)
		&& cJSON_IsArray(json_vertex_2)
		&& cJSON_IsArray(json_vertex_3), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_vertex_1) == 3
		&& cJSON_GetArraySize(json_vertex_2) == 3
		&& cJSON_GetArraySize(json_vertex_3) == 3, ERR_JSON_ARRAY_SIZE);

	Vec3 vertices[3];

	cJSON_parse_float_array(json_vertex_1, vertices[0]);
	cJSON_parse_float_array(json_vertex_2, vertices[1]);
	cJSON_parse_float_array(json_vertex_3, vertices[2]);

	return triangle_new(OBJECT_INIT_VARS, vertices);
}

bool triangle_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal)
{
	Triangle *triangle = object.triangle;
	if (bounding_cuboid_intersects(triangle->bounding_cuboid, ray)) {
		bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->epsilon, distance);
		if (intersects) {
			memcpy(normal, triangle->normal, sizeof(Vec3));
			return true;
		}
	}
	return false;
}

bool triangle_intersects_in_range(const Object object, const Line *ray, float min_distance)
{
	Triangle *triangle = object.triangle;
	if (bounding_cuboid_intersects(triangle->bounding_cuboid, ray)) {
		float distance;
		bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->epsilon, &distance);
		return intersects && distance < min_distance;
	}
	return false;
}

/*******************************************************************************
*	Plane
*******************************************************************************/

Plane *plane_new(OBJECT_INIT_PARAMS, const Vec3 normal, const Vec3 point)
{
	OBJECT_NEW(Plane, plane, OBJECT_PLANE);
	memcpy(plane->normal, normal, sizeof(Vec3));
	plane->d = dot3(normal, point);
	plane->bounding_cuboid = NULL;

	return plane;
}

void plane_delete(Object object)
{
	free(object.common);
}

Plane *plane_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 2 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	OBJECT_INIT_VARS_DECL;
	object_params_load(SCENE_OBJECT_LOAD_VARS);

	cJSON *json_position = cJSON_GetObjectItemCaseSensitive(json, "position"),
		*json_normal = cJSON_GetObjectItemCaseSensitive(json, "normal");

	err_assert(cJSON_IsArray(json_position)
		&& cJSON_IsArray(json_normal), ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(cJSON_GetArraySize(json_position) == 3
		&& cJSON_GetArraySize(json_normal) == 3, ERR_JSON_ARRAY_SIZE);

	Vec3 position, normal;

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_normal, normal);

	return plane_new(OBJECT_INIT_VARS, normal, position);
}

bool plane_get_intersection(const Object object, const Line *ray, float *distance, Vec3 normal)
{
	Plane *plane = object.plane;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->epsilon) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if (*distance > plane->epsilon) {
		memcpy(normal, plane->normal, sizeof(Vec3));
		return true;
	}
	return false;
}

bool plane_intersects_in_range(const Object object, const Line *ray, float min_distance)
{
	Plane *plane = object.plane;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->epsilon) //ray is parallel to line
		return false;
	float distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	return distance > plane->epsilon && distance < min_distance;
}

/*******************************************************************************
*	Mesh
*******************************************************************************/

void mesh_load(const cJSON *json, Context *context)
{
	err_assert(cJSON_GetArraySize(json) == 4 + NUM_OBJECT_MEMBERS, ERR_JSON_ARGC);

	OBJECT_INIT_VARS_DECL;
	object_params_load(SCENE_OBJECT_LOAD_VARS);

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

	stl_load_objects(OBJECT_INIT_VARS, context, file, position, rotation, scale);
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
void stl_load_objects(OBJECT_INIT_PARAMS, Context *context, FILE *file, const Vec3 position, const Vec3 rot, const float scale)
{
	//ensure that file is binary instead of ascii
	char header[5];
	err_assert(fread(header, sizeof(char), 5, file) == 5, ERR_STL_IO_READ);
	err_assert(strncmp("solid", header, 5), ERR_STL_ENCODING);

	float a = cosf(rot[Z]) * sinf(rot[Y]);
	float b = sinf(rot[Z]) * sinf(rot[Y]);
	Vec3 rotation_matrix[3] = {
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
			Vec3 rotated_vertex = {
				dot3(rotation_matrix[X], stl_triangle.vertices[j]),
				dot3(rotation_matrix[Y], stl_triangle.vertices[j]),
				dot3(rotation_matrix[Z], stl_triangle.vertices[j])};
			mul3(rotated_vertex, scale, stl_triangle.vertices[j]);
			add3(stl_triangle.vertices[j], position, stl_triangle.vertices[j]);
		}
		Object object;
		object.triangle = triangle_new(OBJECT_INIT_VARS, stl_triangle.vertices);
		object_add(object, context);
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

bool bounding_cuboid_intersects(const BoundingCuboid *cuboid, const Line *ray)
{
	float tmin, tmax, tymin, tymax;

	float divx = 1 / ray->vector[X];
	if (divx >= 0) {
		tmin = (cuboid->corners[0][X] - ray->position[X]) * divx;
		tmax = (cuboid->corners[1][X] - ray->position[X]) * divx;
	} else {
		tmin = (cuboid->corners[1][X] - ray->position[X]) * divx;
		tmax = (cuboid->corners[0][X] - ray->position[X]) * divx;
	}
	float divy = 1 / ray->vector[Y];
	if (divy >= 0) {
		tymin = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
	} else {
		tymin = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
	}

	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	float tzmin, tzmax;

	float divz = 1 / ray->vector[Z];
	if (divz >= 0) {
		tzmin = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
	} else {
		tzmin = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
	}

	if (tmin > tzmax || tzmin > tmax)
		return false;
	/* The following code snippet can be used for bounds checking
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	return(tmin < t1 && tmax > t0);*/
	return true;
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
	context->objects = malloc(sizeof(Object[num_objects]));
	context->lights = malloc(sizeof(Light[num_lights]));
	context->materials = malloc(sizeof(Material[num_materials]));
	err_assert(context->objects
		&& context->lights
		&& context->materials, ERR_MALLOC);

	if (cJSON_IsArray(json_ambient_light) && cJSON_GetArraySize(json_ambient_light) == 3)
		cJSON_parse_float_array(json_ambient_light, context->global_ambient_light_intensity);

	cJSON *json_iter;
	size_t num_objects_without_bounding = 0;
	cJSON_ArrayForEach (json_iter, json_objects) {
		err_assert(cJSON_IsObject(json_iter), ERR_JSON_VALUE_NOT_OBJECT);
		cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json_iter, "type");
		err_assert(cJSON_IsString(json_type), ERR_JSON_VALUE_NOT_STRING);
		if (djb_hash(json_type->valuestring) == 232719795)
			num_objects_without_bounding++;
	}
	if (num_objects_without_bounding)
		context->objects_without_bounding = malloc(sizeof(Object[num_objects_without_bounding]));

	cJSON_ArrayForEach (json_iter, json_materials) {
		err_assert(cJSON_IsObject(json_iter), ERR_JSON_VALUE_NOT_OBJECT);
		material_load(json_iter, &context->materials[context->num_materials++]);
	}

	cJSON_ArrayForEach (json_iter, json_objects) {
		cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json_iter, "type"),
			*json_parameters = cJSON_GetObjectItemCaseSensitive(json_iter, "parameters");
		err_assert(cJSON_IsObject(json_parameters), ERR_JSON_VALUE_NOT_OBJECT);
		Object object;
		switch (djb_hash(json_type->valuestring)) {
		case 3324768284: /* Sphere */
			object.sphere = sphere_load(json_parameters, context);
			break;
		case 103185867: /* Triangle */
			object.triangle = triangle_load(json_parameters, context);
			break;
		case 232719795: /* Plane */
			object.plane = plane_load(json_parameters, context);
			context->objects_without_bounding[context->num_objects_without_bounding++] = object;
			break;
		case 2088783990: /* Mesh */
			mesh_load(json_parameters, context);
			continue;
		}

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

float get_closest_intersection(const Context *context, const Line *ray, Object *closest_object, Vec3 closest_normal)
{
	Vec3 normal;
	float min_distance = FLT_MAX;
	size_t i;
	for (i = 0; i < context->num_objects; i++) {
		float distance;
		if (context->objects[i].common->functions->get_intersection(context->objects[i], ray, &distance, normal)
			&& distance < min_distance) {
			min_distance = distance;
			*closest_object = context->objects[i];
			memcpy(closest_normal, normal, sizeof(Vec3));
		}
	}
	return min_distance;
}

bool is_intersection_in_distance(const Context *context, const Line *ray, const float min_distance, const CommonObject *excl_object, Vec3 light_intensity)
{
	size_t i;
	for (i = 0; i < context->num_objects; i++) {
		CommonObject *object = context->objects[i].common;
		if (object != excl_object
			&& object->functions->intersects_in_range(context->objects[i], ray, min_distance)) {
			if (object->material->transparent)
				mul3v(light_intensity, object->material->kt, light_intensity);
			else
				return true;
			}
	}
	return false;
}

void cast_ray(const Context *context, const Line *ray, const Vec3 kr, Vec3 color, const uint32_t bounce_count, CommonObject *inside_object)
{
	Object closest_object;
	closest_object.common = NULL;
	Vec3 normal;
	float min_distance;

	if (inside_object) {
		Object obj;
		obj.common = inside_object;
		if (inside_object->functions->get_intersection(obj, ray, &min_distance, normal))
			closest_object.common = inside_object;
		else
			min_distance = get_closest_intersection(context, ray, &closest_object, normal);
	} else {
		min_distance = get_closest_intersection(context, ray, &closest_object, normal);
	}
	CommonObject *object = closest_object.common;
	if (! object)
		return;

	norm3(normal);
	float b = dot3(normal, ray->vector);
	if (b > 0.f)
		mul3(normal, -1.f, normal);

	//LIGHTING MODEL
	Vec3 obj_color;

	Vec3 point;
	mul3(ray->vector, min_distance, point);
	add3(point, ray->position, point);

	Material *material = object->material;

	//ambient
	mul3v(material->ka, context->global_ambient_light_intensity, obj_color);

	//emittance
	add3(material->ke, obj_color, obj_color);

	//Line from point to light
	Line light_ray;
	memcpy(light_ray.position, point, sizeof(Vec3));

	size_t i;
	for (i = 0; i < context->num_lights; i++) {
		Light *light = &context->lights[i];

		sub3(light->position, point, light_ray.vector);
		float light_distance = mag3(light_ray.vector);
		norm3(light_ray.vector);

		float a = dot3(light_ray.vector, normal);

		Vec3 incoming_light_intensity = {1., 1., 1.};
		if (object != inside_object && ! is_intersection_in_distance(context, &light_ray, light_distance, (inside_object == object) ? inside_object : NULL, incoming_light_intensity)) {
			Vec3 intensity;
			mul3v(incoming_light_intensity, light->intensity, intensity);
			Vec3 diffuse;
			mul3v(material->kd, intensity, diffuse);
			mul3(diffuse, fmaxf(0., a), diffuse);

			Vec3 reflected;
			float specular_mul;
			switch (context->reflection_model) {
			case REFLECTION_PHONG:
				mul3(normal, 2 * a, reflected);
				sub3(reflected, light_ray.vector, reflected);
				specular_mul = - dot3(reflected, ray->vector);
				break;
			case REFLECTION_BLINN:
				mul3(light_ray.vector, -1.f, reflected);
				add3(reflected, ray->vector, reflected);
				norm3(reflected);
				specular_mul = - dot3(normal, reflected);
				break;
			}
			Vec3 specular;
			mul3v(material->ks, intensity, specular);
			mul3(specular, fmaxf(0., powf(specular_mul, material->shininess)), specular);

			add3_3(obj_color, diffuse, specular, obj_color);
		}
	}
	mul3v(obj_color, kr, obj_color);
	add3(color, obj_color, color);

	if(bounce_count >= context->max_bounces)
		return;

	//reflection
	if (inside_object != object
		&& material->reflective) {
		Vec3 reflected_kr;
		mul3v(kr, material->kr, reflected_kr);
		if (context->minimum_light_intensity_sqr < sqr(reflected_kr[X]) + sqr(reflected_kr[Y]) + sqr(reflected_kr[Z])) {
			Line reflection_ray;
			memcpy(reflection_ray.position, point, sizeof(Vec3));
			mul3(normal, 2 * dot3(ray->vector, normal), reflection_ray.vector);
			sub3(ray->vector, reflection_ray.vector, reflection_ray.vector);
			cast_ray(context, &reflection_ray, reflected_kr, color, bounce_count + 1, NULL);
		}
	}

	//transparency
	if (material->transparent) {
		Vec3 refracted_kt;
		mul3v(kr, material->kt, refracted_kt);
		if (context->minimum_light_intensity_sqr < sqr(refracted_kt[X]) + sqr(refracted_kt[Y]) + sqr(refracted_kt[Z])) {
			Line refraction_ray;
			memcpy(refraction_ray.position, point, sizeof(Vec3));
			float incident_angle = acosf(fabs(b));
			float refractive_multiplier = inside_object ? (material->refractive_index) : (1.f / material->refractive_index);
			float refracted_angle = asinf(sinf(incident_angle) * refractive_multiplier);
			float delta_angle = refracted_angle - incident_angle;
			Vec3 c, f, g, h;
			cross(ray->vector, normal, c);
			norm3(c);
			cross(c, ray->vector, f);
			mul3(ray->vector, cosf(delta_angle), g);
			mul3(f, sinf(delta_angle), h);
			add3(g, h, refraction_ray.vector);
			norm3(refraction_ray.vector);
			cast_ray(context, &refraction_ray, refracted_kt, color, bounce_count + 1, object);
		}
	}
}

void create_image(const Context *context)
{
	Vec3 kr = {1.f, 1.f, 1.f};
	Camera *camera = context->camera;
	#ifdef MULTITHREADING
	#pragma omp parallel for
	#endif
	for (uint32_t row = 0; row < camera->image.resolution[Y]; row++) {
		Vec3 pixel_position;
		mul3(camera->image.vectors[Y], row, pixel_position);
		add3(pixel_position, camera->image.corner, pixel_position);
		Line ray;
		memcpy(ray.position, camera->position, sizeof(Vec3));
		uint32_t pixel_index = camera->image.resolution[X] * row;
		uint32_t col;
		for (col = 0; col < camera->image.resolution[X]; col++) {
			add3(pixel_position, camera->image.vectors[X], pixel_position);
			Vec3 color = {0., 0., 0.};
			sub3(pixel_position, camera->position, ray.vector);
			norm3(ray.vector);
			cast_ray(context, &ray, kr, color, 0, NULL);
			mul3(color, 255., color);
			uint8_t *pixel = camera->image.pixels[pixel_index];
			pixel[0] = fmaxf(fminf(color[0], 255.), 0.);
			pixel[1] = fmaxf(fminf(color[1], 255.), 0.);
			pixel[2] = fmaxf(fminf(color[2], 255.), 0.);
			pixel_index++;
		}
	}
}

void process_arguments(int argc, char *argv[], Context *context)
{
	if (argc <= 7) {
		if (argc == 2) {
			if (! strcmp("--help", argv[1]) || ! strcmp("-h", argv[1])) {
				printf("%s", HELPTEXT);
				exit(0);
			}
		}
		err_assert(false, ERR_ARGC);
	}

	int32_t i;
	for (i = 1; i < argc; i += 2) {
		switch (djb_hash(argv[i])) {
			case 5859054://-f
			case 3325380195://--file
				context->scene_file = fopen(argv[i + 1], "rb");
				break;
			case 5859047://-o
			case 739088698://--output
				err_assert(strstr(argv[i + 1], ".ppm"), ERR_ARGV_FILE_EXT);
				context->output_file = fopen(argv[i + 1], "wb");
				break;
			case 5859066: //-r
			case 2395108907://--resolution
				context->resolution[X] = abs(atoi(argv[i + 1]));
				context->resolution[Y] = abs(atoi(argv[i++ + 2]));
				break;
			case 5859045://-m
				#ifdef MULTITHREADING
				if (djb_hash(argv[i + 1]) == 193414065) {//if "max"
					NUM_THREADS = omp_get_max_threads();
				} else {
					NUM_THREADS = atoi(argv[i + 1]);
					err_assert(NUM_THREADS <= omp_get_max_threads(), ERR_ARGV_NUM_THREADS);
				}
				#else
				err_assert(false, ERR_ARGV_MULTITHREADING);
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
					err_assert(false, ERR_ARGV_REFLECTION);
				}
				break;
			default:
				err_assert(false, ERR_ARGV_UNRECOGNIZED);
		}
	}
	err_assert(context->scene_file, ERR_ARGV_IO_OPEN_SCENE);
	err_assert(context->output_file, ERR_ARGV_IO_OPEN_OUTPUT);
}

int main(int argc, char *argv[])
{
#ifdef DISPLAY_TIME
	struct timespec start_t, current_t;
	timespec_get(&start_t, TIME_UTC);
#endif

	Context *context = context_new();
	process_arguments(argc, argv, context);

#ifdef MULTITHREADING
	omp_set_num_threads(NUM_THREADS);
#endif

	PRINT_TIME("INITIALIZING SCENE.");

	scene_load(context);
	fclose(context->scene_file);
	context->scene_file = NULL;

	PRINT_TIME("INITIALIZED SCENE. BEGAN RENDERING.");

	create_image(context);

	PRINT_TIME("FINISHED RENDERING. SAVING IMAGE.");

	save_image(context->output_file, &context->camera->image);
	fclose(context->output_file);

	PRINT_TIME("SAVED IMAGE.");

	context_delete(context);

	PRINT_TIME("TERMINATED PROGRAM.");
}
