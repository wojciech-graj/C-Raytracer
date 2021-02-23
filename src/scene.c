#include "scene.h"

#include <string.h>
#include <stdlib.h>

#include "jsmn.h"

enum scene_elems{CAMERA, SPHERE, TRIANGLE, PLANE, MESH, LIGHT, AMBIENTLIGHT, LIGHTAREA};
enum camera_elems{CAMERA_POSITION, CAMERA_VECTOR1, CAMERA_VECTOR2};
enum object_elems{OBJECT_KS, OBJECT_KD, OBJECT_KA, OBJECT_KR, OBJECT_KT, OBJECT_SHININESS, OBJECT_REFRACTIVE_INDEX, OBJECT_EPSILON};
enum sphere_elems{SPHERE_POSITION, SPHERE_RADIUS};
enum triangle_elems{TRIANGLE_VERTEX1, TRIANGLE_VERTEX2, TRIANGLE_VERTEX3};
enum plane_elems{PLANE_POSITION, PLANE_NORMAL};
enum mesh_elems{MESH_FILENAME, MESH_POSITION, MESH_ROTATION, MESH_SCALE};
enum light_elems{LIGHT_POSITION, LIGHT_INTENSITY};
enum lightarea_elems{LIGHTAREA_POSITION, LIGHTAREA_SIDE1, LIGHTAREA_SIDE2, LIGHTAREA_POINT_SPACING, LIGHTAREA_INTENSITY};

const char *scene_elem_tags[] = {"Camera", "Sphere", "Triangle", "Plane", "Mesh", "Light", "AmbientLight", "LightArea"};
const char *camera_elem_tags[] = {"position", "vector_x", "vector_y"};
const char *object_elem_tags[] = {"ks", "kd", "ka", "kr", "kt", "shininess", "refractive_index", "epsilon"};
const char *sphere_elem_tags[] = {"position", "radius"};
const char *triangle_elem_tags[] = {"vertex_1", "vertex_2", "vertex_3"};
const char *plane_elem_tags[] = {"position", "normal"};
const char *mesh_elem_tags[] = {"filename", "position", "rotation", "scale"};
const char *light_elem_tags[] = {"position", "intensity"};
const char *lightarea_elem_tags[] = {"position", "side_1", "side_2", "point_spacing", "intensity"};

/*******************************************************************************
*	JSON PARSING HELPER FUNCTIONS
*******************************************************************************/

int get_string_index(unsigned num_elems, const char **array, char *buffer, int string_start, unsigned string_length)
{
	unsigned i, j;
	for (i = 0; i < num_elems; i++) {
		unsigned length = strlen(array[i]);
		if (string_length == length) {
			for (j = 0; j < length; j++) {
				if (array[i][j] != buffer[string_start + j])
					goto NO_STRING_MATCH;
			}
			return i;
		}
		NO_STRING_MATCH: ;
	}
	return -1;
}

int get_string_token_index(char *buffer, jsmntok_t *tokens, int token_index, int num_elems, const char **elem_tags)
{
	err_assert(tokens[token_index].type == JSMN_STRING, ERR_JSON_KEY_NOT_STRING);
	jsmntok_t *string_token = &tokens[token_index];
	unsigned string_length = string_token->end - string_token->start;
	int string_index = get_string_index(num_elems, elem_tags, buffer, string_token->start, string_length);
	err_assert(string_index != -1, ERR_JSON_UNRECOGNIZED_KEY);
	return string_index;
}

float get_float(char *buffer, jsmntok_t *tokens, int token_index)
{
	err_assert(tokens[token_index].type == JSMN_PRIMITIVE, ERR_JSON_VALUE_NOT_FLOAT);
	jsmntok_t *float_token = &tokens[token_index];
	unsigned string_length = float_token->end - float_token->start;
	char *first_char = &buffer[float_token->start];
	char string[string_length + 1];
	strncpy(string, first_char, string_length);
	string[string_length] = '\0';
	return atof(string);
}

void get_float_array(int num_elems, float *array, char *buffer, jsmntok_t *tokens, int token_index)
{
	err_assert(tokens[token_index].type == JSMN_ARRAY, ERR_JSON_VALUE_NOT_ARRAY);
	err_assert(tokens[token_index].size == num_elems, ERR_JSON_ARRAY_SIZE);
	int i;
	for (i = 0; i < num_elems; i++) {
		token_index++;
		array[i] = get_float(buffer, tokens, token_index);
	}
}

FILE *get_file(char *buffer, jsmntok_t *tokens, int token_index)
{
	err_assert(tokens[token_index].type == JSMN_STRING, ERR_JSON_FILENAME_NOT_STRING);
	jsmntok_t *string_token = &tokens[token_index];
	unsigned filename_length = string_token->end - string_token->start;
	char *first_char = &buffer[string_token->start];
	char filename[filename_length + 1];
	strncpy(filename, first_char, filename_length);
	filename[filename_length] = '\0';
	FILE *file = fopen(filename, "rb");
	err_assert(file, ERR_JSON_IO_OPEN);
	return file;
}

/*******************************************************************************
*	SCENE LOADING
*******************************************************************************/

//NOTE: SOME OF THE FOLLOWING FUNCTIONS COULD BE MORE OPTIMIZED, BUT FOR THE SAKE OF EXTENSIBILITY AND READABILITY THIS HAS NOT BEEN DONE

Camera *scene_load_camera(char *buffer, jsmntok_t *tokens, int *token_index, unsigned resolution[2], Vec2 image_size, float focal_length)
{
	err_assert(tokens[*token_index].size == NUM_CAMERA_ELEMS, ERR_JSON_ARGC);

	Vec3 position, vectors[2];

	(*token_index)++;
	unsigned i;
	for (i = 0; i < NUM_CAMERA_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_CAMERA_ELEMS, camera_elem_tags);
		switch (string_index) {
			case CAMERA_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case CAMERA_VECTOR1:
				get_float_array(3, vectors[0], buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case CAMERA_VECTOR2:
				get_float_array(3, vectors[1], buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	Camera *camera = malloc(sizeof(Camera));
	init_camera(camera, position, vectors, focal_length, resolution, image_size);
	return camera;
}

//token_index is the index of the first string
void scene_load_object_params(char *buffer, jsmntok_t *tokens, int *token_index, Vec3 ks, Vec3 kd, Vec3 ka, Vec3 kr, Vec3 kt, float *shininess, float *refractive_index, float *epsilon)
{
	unsigned i;
	for (i = 0; i < NUM_OBJECT_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_OBJECT_ELEMS, object_elem_tags);
		switch (string_index) {
			case OBJECT_KS:
				get_float_array(3, ks, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case OBJECT_KD:
				get_float_array(3, kd, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case OBJECT_KA:
				get_float_array(3, ka, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case OBJECT_KR:
				get_float_array(3, kr, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case OBJECT_KT:
				get_float_array(3, kt, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case OBJECT_SHININESS:
				*shininess = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			case OBJECT_REFRACTIVE_INDEX:
				*refractive_index = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			case OBJECT_EPSILON:
				*epsilon = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
}

Sphere *scene_load_sphere(char *buffer, jsmntok_t *tokens, int *token_index)
{
	err_assert(tokens[*token_index].size == (NUM_OBJECT_ELEMS + NUM_SPHERE_ELEMS), ERR_JSON_ARGC);
	(*token_index)++;
	OBJECT_INIT_VARS_DECL;
	scene_load_object_params(SCENE_OBJECT_LOAD_VARS);
	Vec3 position;
	float radius;
	unsigned i;
	for (i = 0; i < NUM_SPHERE_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_SPHERE_ELEMS, sphere_elem_tags);
		switch (string_index) {
			case SPHERE_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case SPHERE_RADIUS:
				radius = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	return init_sphere(OBJECT_INIT_VARS, position, radius);
}

Triangle *scene_load_triangle(char *buffer, jsmntok_t *tokens, int *token_index)
{
	err_assert(tokens[*token_index].size == (NUM_OBJECT_ELEMS + NUM_TRIANGLE_ELEMS), ERR_JSON_ARGC);
	(*token_index)++;
	OBJECT_INIT_VARS_DECL;
	scene_load_object_params(SCENE_OBJECT_LOAD_VARS);
	Vec3 vertices[3];
	unsigned i;
	for (i = 0; i < NUM_TRIANGLE_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_TRIANGLE_ELEMS, triangle_elem_tags);
		switch (string_index) {
			case TRIANGLE_VERTEX1:
				get_float_array(3, vertices[0], buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case TRIANGLE_VERTEX2:
				get_float_array(3, vertices[1], buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case TRIANGLE_VERTEX3:
				get_float_array(3, vertices[2], buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	return init_triangle(OBJECT_INIT_VARS, vertices);
}

Plane *scene_load_plane(char *buffer, jsmntok_t *tokens, int *token_index)
{
	err_assert(tokens[*token_index].size == (NUM_OBJECT_ELEMS + NUM_PLANE_ELEMS), ERR_JSON_ARGC);
	(*token_index)++;
	OBJECT_INIT_VARS_DECL;
	scene_load_object_params(SCENE_OBJECT_LOAD_VARS);
	Vec3 position, normal;
	unsigned i;
	for (i = 0; i < NUM_PLANE_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_PLANE_ELEMS, plane_elem_tags);
		switch (string_index) {
			case PLANE_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case PLANE_NORMAL:
				get_float_array(3, normal, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	return init_plane(OBJECT_INIT_VARS, normal, position);
}

Mesh *scene_load_mesh(char *buffer, jsmntok_t *tokens, int *token_index)
{
	err_assert(tokens[*token_index].size == (NUM_OBJECT_ELEMS + NUM_MESH_ELEMS), ERR_JSON_ARGC);
	(*token_index)++;
	OBJECT_INIT_VARS_DECL;
	scene_load_object_params(SCENE_OBJECT_LOAD_VARS);
	FILE *file;
	Vec3 position, rotation;
	float scale;
	unsigned i;
	for (i = 0; i < NUM_MESH_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_MESH_ELEMS, mesh_elem_tags);
		switch (string_index) {
			case MESH_FILENAME:
				file = get_file(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			case MESH_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case MESH_ROTATION:
				get_float_array(3, rotation, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case MESH_SCALE:
				scale = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	Mesh *mesh = stl_load(OBJECT_INIT_VARS, file, position, rotation, scale);
	fclose(file);
	return mesh;
}

void scene_load_light(char *buffer, jsmntok_t *tokens, int *token_index, Light *light)
{
	err_assert(tokens[*token_index].size == NUM_LIGHT_ELEMS, ERR_JSON_ARGC);
	(*token_index)++;
	Vec3 position, intensity;
	unsigned i;
	for (i = 0; i < NUM_LIGHT_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_LIGHT_ELEMS, light_elem_tags);
		switch (string_index) {
			case LIGHT_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case LIGHT_INTENSITY:
				get_float_array(3, intensity, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	init_light(light, position, intensity);
}

void scene_load_lightarea(char *buffer, jsmntok_t *tokens, int *token_index, unsigned *num_lights, unsigned *light_array_size, Light **lights)
{
	err_assert(tokens[*token_index].size == NUM_LIGHTAREA_ELEMS, ERR_JSON_ARGC);
	(*token_index)++;
	Vec3 position, side_1, side_2, intensity;
	float point_spacing;
	unsigned i;
	for (i = 0; i < NUM_LIGHTAREA_ELEMS; i++) {
		int string_index = get_string_token_index(buffer, tokens, *token_index, NUM_LIGHTAREA_ELEMS, lightarea_elem_tags);
		switch (string_index) {
			case LIGHTAREA_POSITION:
				get_float_array(3, position, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case LIGHTAREA_SIDE1:
				get_float_array(3, side_1, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case LIGHTAREA_SIDE2:
				get_float_array(3, side_2, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			case LIGHTAREA_POINT_SPACING:
				point_spacing = get_float(buffer, tokens, *token_index + 1);
				*token_index += 2;
				break;
			case LIGHTAREA_INTENSITY:
				get_float_array(3, intensity, buffer, tokens, *token_index + 1);
				*token_index += 5;
				break;
			default:
				err_assert(false, ERR_JSON_UNRECOGNIZED);
		}
	}
	Vec2 light_size = {magnitude3(side_1), magnitude3(side_2)};
	int num_points[2] = {floor(light_size[0] / point_spacing) + 1, floor(light_size[1] / point_spacing) + 1};
	unsigned total_num_points = num_points[X] * num_points[Y];
	divide3(intensity, total_num_points, intensity);
	unsigned light_index = *num_lights;
	*num_lights += total_num_points;
	if (*num_lights > *light_array_size)
		*lights = realloc(*lights, (*num_lights + DYNAMIC_ARRAY_INCREMENT - (*num_lights % DYNAMIC_ARRAY_INCREMENT)) * sizeof(Light));
	Vec3 side_increment_1, side_increment_2, current_position;
	divide3(side_1, num_points[X], side_increment_1);
	divide3(side_2, num_points[Y], side_increment_2);
	int x, y;
	for (y = 0; y < num_points[Y]; y++) {
		multiply3(side_increment_2, y, current_position);
		add3(current_position, position, current_position);
		for (x = 0; x < num_points[X]; x++) {
			add3(current_position, side_increment_1, current_position);
			init_light(&(*lights)[light_index], current_position, intensity);
			light_index++;
		}
	}
}

void scene_load(FILE *scene_file, Camera **camera, unsigned *num_objects, Object **objects, unsigned *num_lights, Light **lights, Vec3 ambient_light_intensity, unsigned resolution[2], Vec2 image_size, float focal_length)
{
	fseek(scene_file, 0, SEEK_END);
	unsigned int length = ftell(scene_file);
	fseek(scene_file, 0, SEEK_SET);
	char *buffer = malloc(length + 1);
	err_assert(fread(buffer, 1, length, scene_file) == length, ERR_JSON_IO_READ);
	buffer[length] = '\0';

	jsmn_parser parser;
	jsmn_init(&parser);
	int num_tokens = jsmn_parse(&parser, buffer, strlen(buffer), NULL, MAX_JSON_TOKENS);
	err_assert(num_tokens <= MAX_JSON_TOKENS && num_tokens > 0, ERR_JSON_NUM_TOKENS);

	jsmntok_t tokens[num_tokens];
	jsmn_init(&parser);
	err_assert(jsmn_parse(&parser, buffer, strlen(buffer), tokens, num_tokens) == num_tokens, ERR_JSON_READ_TOKENS);
	err_assert(tokens[0].type == JSMN_OBJECT,ERR_JSON_FIRST_TOKEN);
	int token_index = tokens[0].start + 1;

	unsigned object_array_size = DYNAMIC_ARRAY_INCREMENT;
	unsigned light_array_size = DYNAMIC_ARRAY_INCREMENT;

	*camera = NULL;
	*objects = malloc(object_array_size * sizeof(Object));
	*lights = malloc(light_array_size * sizeof(Light));

	while (token_index < num_tokens) {
		int string_index = get_string_token_index(buffer, tokens, token_index, NUM_SCENE_ELEM_TYPES, scene_elem_tags);

		token_index++;
		switch (string_index) {
			case CAMERA:
				*camera = scene_load_camera(buffer, tokens, &token_index, resolution, image_size, focal_length);
				break;
			case SPHERE:
				(*objects)[*num_objects].sphere = scene_load_sphere(buffer, tokens, &token_index);
			break;
			case TRIANGLE:
				(*objects)[*num_objects].triangle = scene_load_triangle(buffer, tokens, &token_index);
				break;
			case PLANE:
				(*objects)[*num_objects].plane = scene_load_plane(buffer, tokens, &token_index);
				break;
			case MESH:
				(*objects)[*num_objects].mesh = scene_load_mesh(buffer, tokens, &token_index);
				break;
			case LIGHT:
				scene_load_light(buffer, tokens, &token_index, &(*lights)[*num_lights]);
				break;
			case AMBIENTLIGHT:
				get_float_array(3, ambient_light_intensity, buffer, tokens, token_index);
				token_index += 4;
				break;
			case LIGHTAREA:
				scene_load_lightarea(buffer, tokens, &token_index, num_lights, &light_array_size, lights);
				break;
			default:
				err_assert(false, ERR_JSON_ARGC_SCENE);
		}
		switch (string_index) {
			case SPHERE:
			case TRIANGLE:
			case PLANE:
			case MESH:
				(*num_objects)++;
				if (*num_objects == object_array_size) {
					object_array_size += DYNAMIC_ARRAY_INCREMENT;
					*objects = realloc(*objects, object_array_size * sizeof(Object));
					err_assert(*objects, ERR_MALLOC);
				}
				break;
			case LIGHT:
				(*num_lights)++;
				if (*num_lights == light_array_size) {
					light_array_size += DYNAMIC_ARRAY_INCREMENT;
					*lights = realloc(*lights, light_array_size * sizeof(Light));
					err_assert(*lights, ERR_MALLOC);
				}
				break;
		}
	}
	err_assert(camera, ERR_JSON_NO_CAMERA);
	err_assert(*num_objects > 0, ERR_JSON_NO_OBJECTS);
	err_assert(*num_lights > 0, ERR_JSON_NO_LIGHTS);
	*objects = realloc(*objects, *num_objects * sizeof(Object));
	err_assert(*objects, ERR_MALLOC);
	*lights = realloc(*lights, *num_lights * sizeof(Light));
	err_assert(*lights, ERR_MALLOC);
	free(buffer);
}
