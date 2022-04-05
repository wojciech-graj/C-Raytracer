/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Loading camera, image, objects, materials from json scene file
 **/

#include "scene.h"
#include "type.h"
#include "argv.h"
#include "mem.h"
#include "camera.h"
#include "material.h"
#include "object.h"
#include "render.h"
#include "strhash.h"
#include "error.h"

#include <stdio.h>

#include "cJSON/cJSON.h"

#define SCENE_ERROR_MSG(msg) msg " in scene [%s]."

#define GET_JSON_TYPECHECK(var, parent, token, type)\
	do {\
		var = cJSON_GetObjectItemCaseSensitive(parent, token);\
		error_check(cJSON_Is ## type (var), SCENE_ERROR_MSG("Expected token [" token "] of type [" #type "]"), scene_filename);\
	} while (0)

#define GET_JSON_ARRAY(var, parent, token, len)\
	do {\
		GET_JSON_TYPECHECK(var, parent, token, Array);\
		error_check(cJSON_GetArraySize(var) == len, SCENE_ERROR_MSG("Expected token [" token "] of length [%d]"), len, scene_filename);\
	} while (0)

void cJSON_parse_float_array(const cJSON *json, float *array);
void camera_load(const cJSON *json);
void materials_load(const cJSON *json);
void material_load(const cJSON *json, size_t idx);
Texture *texture_load(const cJSON *json);
void objects_load(const cJSON *json);
void object_load(const cJSON *json, Object *object, ObjectType object_type);
Object *sphere_load(const cJSON *json);
Object *triangle_load(const cJSON *json);
#ifdef UNBOUND_OBJECTS
Object *plane_load(const cJSON *json);
#endif
void mesh_load(const cJSON *json, size_t *i_object);

static char *scene_filename;

void cJSON_parse_float_array(const cJSON *json, float *array)
{
	size_t i = 0;
	cJSON *json_iter;
	cJSON_ArrayForEach (json_iter, json) {
		error_check(cJSON_IsNumber(json_iter), SCENE_ERROR_MSG("Expected token in Array of type [Number]"), scene_filename);
		array[i++] = json_iter->valuedouble;
	}
}

void scene_load(void)
{
	printf_log("Loading scene.");

	scene_filename = myargv[ARG_INPUT_FILENAME];

	FILE *file_scene = fopen(scene_filename, "rb");
	error_check(file_scene, "Unable to open scene file [%s].", scene_filename);

	fseek(file_scene, 0, SEEK_END);
	size_t length = ftell(file_scene);
	fseek(file_scene, 0, SEEK_SET);
	char *buffer = safe_malloc(length + 1);
	size_t nmemb_read = fread(buffer, 1, length, file_scene);
	error_check(nmemb_read == length, "Failed to read scene file [%s].", scene_filename);
	fclose(file_scene);
	buffer[length] = '\0';
	cJSON *json = cJSON_Parse(buffer);
	free(buffer);

	error_check(json, "Failed to parse scene [%s].", scene_filename);
	error_check(cJSON_IsObject(json), "Expected parent token of type Object in scene [%s].", scene_filename);

	cJSON *json_materials = cJSON_GetObjectItemCaseSensitive(json, "Materials"),
		*json_objects = cJSON_GetObjectItemCaseSensitive(json, "Objects"),
		*json_camera = cJSON_GetObjectItemCaseSensitive(json, "Camera"),
		*json_ambient_light = cJSON_GetObjectItemCaseSensitive(json, "AmbientLight");

	GET_JSON_TYPECHECK(json_materials, json, "Materials", Array);
	GET_JSON_TYPECHECK(json_objects, json, "Objects", Array);
	GET_JSON_TYPECHECK(json_camera, json, "Camera", Object);

	camera_load(json_camera);
	materials_load(json_materials);
	objects_load(json_objects);

	if (cJSON_IsArray(json_ambient_light) && cJSON_GetArraySize(json_ambient_light) == 3)
		cJSON_parse_float_array(json_ambient_light, global_ambient_light_intensity);

	cJSON_Delete(json);
}

void camera_load(const cJSON *json)
{
	printf_log("Loading camera.");

	error_check(cJSON_GetArraySize(json) == 5, "Expected token [Camera] to contain 5 elements.");

	cJSON *json_position, *json_vector_x, *json_vector_y, *json_fov, *json_focal_length;

	GET_JSON_ARRAY(json_position, json, "position", 3);
	GET_JSON_ARRAY(json_vector_x, json, "vector_x", 3);
	GET_JSON_ARRAY(json_vector_y, json, "vector_y", 3);
	GET_JSON_TYPECHECK(json_fov, json, "fov", Number);
	GET_JSON_TYPECHECK(json_focal_length, json, "focal_length", Number);

	float fov = json_fov->valuedouble,
		focal_length = json_focal_length->valuedouble;
	Vec3 position, vectors[2];

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_vector_x, vectors[0]);
	cJSON_parse_float_array(json_vector_y, vectors[1]);

	camera_init(position, vectors, fov, focal_length);
}

void materials_load(const cJSON *json)
{
	num_materials = cJSON_GetArraySize(json);
	error_check(num_materials, SCENE_ERROR_MSG("Expected token [Materials] to contain nonzero element count"), scene_filename);
	printf_log("Loading %u materials.", num_materials);
	materials_init();

	size_t i = 0;
	cJSON *json_iter;
	cJSON_ArrayForEach (json_iter, json) {
		error_check(cJSON_IsObject(json_iter), SCENE_ERROR_MSG("Expected token in [Materials] of type Object"), scene_filename);
		material_load(json_iter, i);
		i++;
	}
}

void material_load(const cJSON *json, const size_t idx)
{
	cJSON  *json_id, *json_ks, *json_ka, *json_kr, *json_kt, *json_ke, *json_shininess, *json_refractive_index, *json_texture;

	GET_JSON_TYPECHECK(json_id, json, "id", Number);
	GET_JSON_TYPECHECK(json_shininess, json, "shininess", Number);
	GET_JSON_TYPECHECK(json_refractive_index, json, "refractive_index", Number);
	GET_JSON_TYPECHECK(json_texture, json, "texture", Object);
	GET_JSON_ARRAY(json_ks, json, "ks", 3);
	GET_JSON_ARRAY(json_ka, json, "ka", 3);
	GET_JSON_ARRAY(json_kr, json, "kr", 3);
	GET_JSON_ARRAY(json_kt, json, "kt", 3);
	GET_JSON_ARRAY(json_ke, json, "ke", 3);

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

	material_init(&materials[idx], id, ks, ka, kr, kt, ke, shininess, refractive_index, texture);
}

Texture *texture_load(const cJSON *json)
{
	cJSON *json_type;
	GET_JSON_TYPECHECK(json_type, json, "type", String);

	Texture *texture;
	switch (hash_djb(json_type->valuestring)) {
	case 3226203393: { //uniform
		cJSON *json_color;
		GET_JSON_ARRAY(json_color, json, "color", 3);

		Vec3 color;
		cJSON_parse_float_array(json_color, color);

		texture = texture_uniform_new(color);
		break;
		}
	case 2234799246: { //checkerboard
		cJSON *json_colors, *json_scale;
		GET_JSON_ARRAY(json_colors, json, "colors", 2);
		GET_JSON_TYPECHECK(json_scale, json, "scale", Number);

		float scale = json_scale->valuedouble;

		Vec3 colors[2];
		cJSON *json_iter;
		size_t i = 0;
		cJSON_ArrayForEach (json_iter, json_colors) {
			error_check(cJSON_IsArray(json_iter), SCENE_ERROR_MSG("Expected token in [colors] of type Array"), scene_filename);
			error_check(cJSON_GetArraySize(json_iter) == 3, SCENE_ERROR_MSG("Expected token in [colors] of length 3"), scene_filename);
			cJSON_parse_float_array(json_iter, colors[i]);
			i++;
		}

		texture = texture_checkerboard_new(colors, scale);
		break;
		}
	case 176032948: { //brick
		cJSON *json_colors, *json_scale, *json_mortar_width;
		GET_JSON_ARRAY(json_colors, json, "colors", 2);
		GET_JSON_TYPECHECK(json_scale, json, "scale", Number);
		GET_JSON_TYPECHECK(json_mortar_width, json, "mortar width", Number);

		float scale = json_scale->valuedouble;
		float mortar_width = json_mortar_width->valuedouble;

		Vec3 colors[2];
		cJSON *json_iter;
		size_t i = 0;
		cJSON_ArrayForEach (json_iter, json_colors) {
			error_check(cJSON_IsArray(json_iter), SCENE_ERROR_MSG("Expected token in [colors] of type Array"), scene_filename);
			error_check(cJSON_GetArraySize(json_iter) == 3, SCENE_ERROR_MSG("Expected token in [colors] of length 3"), scene_filename);
			cJSON_parse_float_array(json_iter, colors[i]);
			i++;
		}

		texture = texture_brick_new(colors, scale, mortar_width);
		return (Texture*)texture;
		}
	case 202158024: { //noisy periodic
		cJSON *json_color, *json_color_gradient, *json_noise_feature_scale, *json_noise_scale, *json_frequency_scale, *json_function;
		GET_JSON_ARRAY(json_color, json, "color", 3);
		GET_JSON_ARRAY(json_color_gradient, json, "color gradient", 3);
		GET_JSON_TYPECHECK(json_noise_feature_scale, json, "noise feature scale", Number);
		GET_JSON_TYPECHECK(json_noise_scale, json, "noise scale", Number);
		GET_JSON_TYPECHECK(json_frequency_scale, json, "frequency scale", Number);
		GET_JSON_TYPECHECK(json_function, json, "function", String);

		float noise_feature_scale = json_noise_feature_scale->valuedouble;
		float noise_scale = json_noise_scale->valuedouble;
		float frequency_scale = json_frequency_scale->valuedouble;

		Vec3 color, color_gradient;
		cJSON_parse_float_array(json_color, color);
		cJSON_parse_float_array(json_color_gradient, color_gradient);

		PeriodicFunction func;
		switch (hash_djb(json_function->valuestring)) {
		case 193433777: //sin
			func = PERIODIC_FUNC_SIN;
			break;
		case 193433504: //saw
			func = PERIODIC_FUNC_SAW;
			break;
		case 837065195: //triangle
			func = PERIODIC_FUNC_TRIANGLE;
			break;
		case 2144888260: //square
			func = PERIODIC_FUNC_SQUARE;
			break;
		default:
			error(SCENE_ERROR_MSG("Unexpected value [%s] of token [function]"), json_function->valuestring, scene_filename);
		}

		texture = texture_noisy_periodic_new(color, color_gradient, noise_feature_scale, noise_scale, frequency_scale, func);
		break;
		}
	default:
		error(SCENE_ERROR_MSG("Unrecognized token [%s] in texture"), json_type->valuestring, scene_filename);
	}

	return texture;
}

void objects_load(const cJSON *json)
{
	num_objects = cJSON_GetArraySize(json);
	error_check(num_objects, SCENE_ERROR_MSG("Expected token [Objects] to contain nonzero element count"), scene_filename);
	printf_log("Loading %u objects.", num_objects);

	num_emittant_objects = num_unbound_objects = 0;
	cJSON *json_iter;
	cJSON_ArrayForEach (json_iter, json) {
		error_check(cJSON_IsObject(json_iter), SCENE_ERROR_MSG("Expected token in [Objects] of type Object"), scene_filename);
		cJSON *json_type, *json_parameters, *json_material;

		GET_JSON_TYPECHECK(json_type, json_iter, "type", String);
		GET_JSON_TYPECHECK(json_parameters, json_iter, "parameters", Object);
		GET_JSON_TYPECHECK(json_material, json_parameters, "material", Number);

		Material *material = get_material(json_material->valueint);
		if (material->emittant)
			num_emittant_objects++;
#ifdef UNBOUND_OBJECTS
		if (hash_djb(json_type->valuestring) == 232719795 /* Plane */)
			num_unbound_objects++;
#endif
	}

	error_check(num_emittant_objects, SCENE_ERROR_MSG("Expected non-zero number of emittant objects"), scene_filename);
	objects_init();

	size_t i_object = 0;
	size_t i_emittant_object = 0;
#ifdef UNBOUND_OBJECTS
	size_t i_unbound_object = 0;
#endif
	cJSON_ArrayForEach (json_iter, json) {
		cJSON *json_type = cJSON_GetObjectItemCaseSensitive(json_iter, "type"),
			*json_parameters = cJSON_GetObjectItemCaseSensitive(json_iter, "parameters");
		Object *object;
		switch (hash_djb(json_type->valuestring)) {
		case 3324768284: /* Sphere */
			object = sphere_load(json_parameters);
			break;
		case 103185867: /* Triangle */
			object = triangle_load(json_parameters);
			break;
		case 232719795: /* Plane */
#ifdef UNBOUND_OBJECTS
			object = plane_load(json_parameters);
			break;
#else
			continue;
#endif
		case 2088783990: /* Mesh */
			mesh_load(json_parameters, &i_object);
			continue;
		}
#ifdef UNBOUND_OBJECTS
		if (!object->object_data->is_bounded)
			unbound_objects[i_unbound_object++] = object;
#endif
		if (object->material->emittant)
			emittant_objects[i_emittant_object++] = object;
		objects[i_object++] = object;
	}
}

void object_load(const cJSON *json, Object *object, const ObjectType object_type)
{
	cJSON *json_material,
		*json_epsilon = cJSON_GetObjectItemCaseSensitive(json, "epsilon"),
		*json_num_lights = cJSON_GetObjectItemCaseSensitive(json, "lights");

	GET_JSON_TYPECHECK(json_material, json, "material", Number);

	float epsilon = cJSON_IsNumber(json_epsilon) ? (float)json_epsilon->valuedouble : -1.f;
	Material *material = get_material(json_material->valueint);
	uint32_t num_lights = cJSON_IsNumber(json_num_lights) ? json_num_lights->valueint : 0;

	object_init(object, material, epsilon, num_lights, object_type);
}

Object *sphere_load(const cJSON *json)
{
	cJSON *json_position, *json_radius;

	GET_JSON_TYPECHECK(json_radius, json, "radius", Number);
	GET_JSON_ARRAY(json_position, json, "position", 3);

	float radius = json_radius->valuedouble;

	Vec3 position;
	cJSON_parse_float_array(json_position, position);

	Object *sphere = sphere_new(position, radius);
	object_load(json, sphere, OBJECT_SPHERE);
	sphere->object_data->postinit(sphere);

	return sphere;
}

Object *triangle_load(const cJSON *json)
{
	cJSON *json_vertex_1, *json_vertex_2, *json_vertex_3;

	GET_JSON_ARRAY(json_vertex_1, json, "vertex_1", 3);
	GET_JSON_ARRAY(json_vertex_2, json, "vertex_2", 3);
	GET_JSON_ARRAY(json_vertex_3, json, "vertex_3", 3);

	Vec3 vertices[3];
	cJSON_parse_float_array(json_vertex_1, vertices[0]);
	cJSON_parse_float_array(json_vertex_2, vertices[1]);
	cJSON_parse_float_array(json_vertex_3, vertices[2]);

	Object *triangle = triangle_new(vertices);
	object_load(json, triangle, OBJECT_TRIANGLE);
	triangle->object_data->postinit(triangle);

	return triangle;
}

#ifdef UNBOUND_OBJECTS
Object *plane_load(const cJSON *json)
{
	cJSON *json_position, *json_normal;

	GET_JSON_ARRAY(json_position, json, "position", 3);
	GET_JSON_ARRAY(json_normal, json, "normal", 3);

	Vec3 position, normal;
	cJSON_parse_float_array(json_normal, normal);
	cJSON_parse_float_array(json_position, position);

	Object *plane = plane_new(position, normal);
	object_load(json, plane, OBJECT_PLANE);
	plane->object_data->postinit(plane);

	return plane;
}
#endif

void mesh_load(const cJSON *json, size_t *i_object)
{
	cJSON *json_filename, *json_position, *json_rotation, *json_scale;

	GET_JSON_TYPECHECK(json_filename, json, "filename", String);
	GET_JSON_ARRAY(json_position, json, "position", 3);
	GET_JSON_ARRAY(json_rotation, json, "rotation", 3);
	GET_JSON_TYPECHECK(json_scale, json, "scale", Number);

	char *filename = json_filename->valuestring;
	float scale = json_scale->valuedouble;
	Vec3 position, rotation;

	cJSON_parse_float_array(json_position, position);
	cJSON_parse_float_array(json_rotation, rotation);

	Object object;
	object_load(json, &object, OBJECT_TRIANGLE);

	mesh_to_objects(filename, &object, position, rotation, scale, i_object);
}
