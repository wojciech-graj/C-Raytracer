#include "main.h"

void add_3_vectors(Vec3 vec1, Vec3 vec2, Vec3 vec3, Vec3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void init_camera(Camera *camera,
	Vec3 position,
	Vec3 vectors[2],
	double focal_length,
	int image_resolution[2],
	Vec2 image_size)
{
	memcpy(camera->position, position, sizeof(Vec3));
	memcpy(camera->vectors, vectors, sizeof(Vec3[2]));
	normalize3(camera->vectors[0]);
	normalize3(camera->vectors[1]);
	cross(camera->vectors[0], camera->vectors[1], camera->vectors[2]);
	camera->focal_length = focal_length;
	memcpy(camera->image.resolution, image_resolution, 2 * sizeof(int));
	memcpy(camera->image.size, image_size, 2 * sizeof(double));
	camera->image.pixels = malloc(image_resolution[X] * image_resolution[Y] * sizeof(Color));

	Vec3 focal_vector, plane_center, corner_offset_vectors[2];
	multiply3(camera->vectors[2], camera->focal_length, focal_vector);
	add3(focal_vector, camera->position, plane_center);
	multiply3(camera->vectors[0], camera->image.size[X] / camera->image.resolution[X], camera->image.vectors[0]);
	multiply3(camera->vectors[1], camera->image.size[Y] / camera->image.resolution[Y], camera->image.vectors[1]);
	multiply3(camera->image.vectors[X], .5 - camera->image.resolution[X] / 2., corner_offset_vectors[X]);
	multiply3(camera->image.vectors[Y], .5 - camera->image.resolution[Y] / 2., corner_offset_vectors[Y]);
	add_3_vectors(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], camera->image.corner);
}

bool intersects_sphere(Object object, Line *ray, double *distance)
{
	Sphere *sphere = object.sphere;
	Vec3 relative_position;
	subtract3(ray->position, sphere->position, relative_position);
	double a = dot3(ray->vector, ray->vector);
	double b = 2 * dot3(ray->vector, relative_position);
	double c = dot3(relative_position, relative_position) - sqr(sphere->radius);
	double determinant = sqr(b) - 4 * a * c;
	if(determinant < 0) //no collision
		return false;
	else {
		*distance = (sqrt(determinant) + b) / (-2 * a);
		if(*distance > 0)//if in front of origin of ray
			return true;
		else
			return false;
	}
}

//Möller–Trumbore intersection algorithm
bool intersects_triangle(Object object, Line *ray, double *distance)
{
	Triangle *triangle = object.triangle;
	Vec3 h, s, q;
	cross(ray->vector, triangle->edges[1], h);
	double a = dot3(triangle->edges[0], h);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	double f = 1. / a;
	subtract3(ray->position, triangle->vertices[0], s);
	double u = f * dot3(s, h);
	if(u < 0. || u > 1.)
		return false;
	cross(s, triangle->edges[0], q);
	double v = f * dot3(ray->vector, q);
	if(v < 0. || u + v > 1.)
		return false;
	*distance = f * dot3(triangle->edges[1], q);
	if(*distance > EPSILON)
		return true;
	else
		return false;
}

bool intersects_plane(Object object, Line *ray, double *distance)
{
	Plane *plane = object.plane;
	double a = dot3(plane->normal, ray->vector);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if(*distance > EPSILON)
		return true;
	else
		return false;
}

void get_normal_plane(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)position;
	Plane *plane = object.plane;
	if(dot3(plane->normal, ray->vector) < 0)
		memcpy(normal, plane->normal, sizeof(Vec3));
	else
		multiply3(plane->normal, -1, normal);
}

void get_normal_sphere(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)ray;
	Sphere *sphere = object.sphere;
	subtract3(position, sphere->position, normal);
	if(dot3(normal, ray->vector) > 0) //if collision from within sphere
		multiply3(normal, -1, normal);
}

void get_normal_triangle(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)position;
	Triangle *triangle = object.triangle;
	if(dot3(triangle->normal, ray->vector) < 0)
		memcpy(normal, triangle->normal, sizeof(Vec3));
	else
		multiply3(triangle->normal, -1, normal);
}

void cast_ray(Camera *camera, int num_objects, Object objects[num_objects], int num_lights, Light *lights[num_lights], Vec3 pixel_position, Color pixel)
{
	Line ray;
	memcpy(ray.position, camera->position, sizeof(Vec3));
	subtract3(pixel_position, camera->position, ray.vector);
	normalize3(ray.vector);

	Object closest_object;
	closest_object.common = NULL;
	double min_distance = DBL_MAX;
	double distance;
	int i;
	for(i = 0; i < num_objects; i++)
	{
		if(objects[i].common->intersects(objects[i], &ray, &distance)) {
			if(distance < min_distance) {
				min_distance = distance;
				closest_object = objects[i];
			}
		}
	}
	if(closest_object.common) {
		CommonObject *object = closest_object.common;
		//Phong reflection model
		Vec3 color;

		Vec3 point;
		multiply3(ray.vector, min_distance, point);
		add3(point, ray.position, point);

		Vec3 normal;
		object->get_normal(closest_object, &ray, point, normal);
		normalize3(normal);

		//ambient
		multiply3v(object->ka, ambient_light_intensity, color);

		for(i = 0; i < num_lights; i++)
		{
			Light *light = lights[i];

			//direction from point to light
			Vec3 inv_incedent;
			subtract3(light->position, point, inv_incedent);
			normalize3(inv_incedent);

			double a = dot3(inv_incedent, normal);

			//direction of a reflected ray of light from point
			Vec3 reflected;
			multiply3(normal, 2 * a, reflected);
			subtract3(reflected, inv_incedent, reflected);

			Vec3 diffuse;
			multiply3v(object->kd, light->intensity, diffuse);
			multiply3(diffuse, fmax(0., a), diffuse);

			Vec3 specular;
			multiply3v(object->ks, light->intensity, specular);
			multiply3(specular, fmax(0., pow(- dot3(reflected, ray.vector), object->alpha)), specular);

			add_3_vectors(color, diffuse, specular, color);
		}

		multiply3(color, 255., color);
		pixel[0] = fmax(fmin(color[0], 255.), 0.);
		pixel[1] = fmax(fmin(color[1], 255.), 0.);
		pixel[2] = fmax(fmin(color[2], 255.), 0.);
	} else
		memcpy(pixel, background_color, sizeof(Color));
}

void create_image(Camera *camera, int num_objects, Object objects[num_objects], int num_lights, Light *lights[num_lights])
{
	int pixel_index = 0;
	int pixel[2];
	Vec3 pixel_position;
	for(pixel[Y] = 0; pixel[Y] < camera->image.resolution[Y]; pixel[Y]++)
	{
		multiply3(camera->image.vectors[Y], pixel[Y], pixel_position);
		add3(pixel_position, camera->image.corner, pixel_position);
		for(pixel[X] = 0; pixel[X] < camera->image.resolution[X]; pixel[X]++)
		{
			add3(pixel_position, camera->image.vectors[X], pixel_position);
			cast_ray(camera, num_objects, objects, num_lights, lights, pixel_position, camera->image.pixels[pixel_index]);
			pixel_index++;
		}
	}
}

void init_triangle(Triangle *triangle, OBJECT_INIT_PARAMS, Vec3 vertices[3])
{
	OBJECT_INIT(triangle);
	memcpy(triangle->vertices, vertices, sizeof(Vec3[3]));
	subtract3(vertices[1], vertices[0], triangle->edges[0]);
	subtract3(vertices[2], vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
	multiply3(triangle->normal, -1., triangle->normal);//TODO: remove
}

void init_sphere(Sphere *sphere, OBJECT_INIT_PARAMS, Vec3 position, double radius)
{
	OBJECT_INIT(sphere);
	memcpy(sphere->position, position, sizeof(Vec3));
	sphere->radius = radius;
}

void init_plane(Plane *plane, OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point)
{
	OBJECT_INIT(plane);
	memcpy(plane->normal, normal, sizeof(Vec3));
	plane->d = dot3(normal, point);
}

void init_light(Light *light, Vec3 position, Vec3 intensity)
{
	memcpy(light->position, position, sizeof(Vec3));
	memcpy(light->intensity, intensity, sizeof(Vec3));
}

void save_image(char *filename, Image *image)
{
	FILE *file = fopen(filename, "wb");
	fprintf(file, "P6\n%d %d\n255\n", image->resolution[X], image->resolution[Y]);
    fwrite(image->pixels, image->resolution[X] * image->resolution[Y], sizeof(Color), file);
    fclose(file);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	Camera camera;
	init_camera(&camera,
		(Vec3) {0., 0., 0.},
		(Vec3[2]) {
			{1., 0., 0.},
			{0., 1., 0.}},
		1.,
		(int[2]) {640, 480},
		(Vec2) {4./3., 1.});

	const int num_objects = 4;

	Object objects[num_objects];
	Sphere *sphere1 = malloc(sizeof(Sphere));
	init_sphere(sphere1,
		(Vec3) {0., 0., 0.},
		(Vec3) {0., 1., 0.},
		(Vec3) {0., .5, 0.},
		0.,
		(Vec3) {-3., 0., 8.},
		1.);
	objects[0].sphere = sphere1;

	Sphere *sphere2 = malloc(sizeof(Sphere));
	init_sphere(sphere2,
		(Vec3) {.9, .9, .9},
		(Vec3) {1., 0., 0.},
		(Vec3) {.5, 0., 0.},
		5.,
		(Vec3) {0., 1., 7.},
		1.5);
	objects[1].sphere = sphere2;

	Triangle *triangle1 = malloc(sizeof(Triangle));
	init_triangle(triangle1,
		(Vec3) {0., 0., 0.},
		(Vec3) {0., 0., 1.},
		(Vec3) {0., 0., .5},
		0.,
		(Vec3[3]) {
			{3., -1., 7.3},
			{2., 1., 7.3},
			{1., -1., 7.3}});
	objects[2].triangle = triangle1;

	Plane *plane1 = malloc(sizeof(Plane));
	init_plane(plane1,
		(Vec3) {0., 0., 0.},
		(Vec3) {.5, .5, .5},
		(Vec3) {.5, .5, .5},
		0.,
		(Vec3) {0., 0., 1.},
		(Vec3) {0., 0., 10.});
	objects[3].plane = plane1;

	const int num_lights = 1;

	Light *lights[num_lights];

	lights[0] = malloc(sizeof(Light));
	init_light(lights[0],
		(Vec3) {5., -5., 5.},
		(Vec3) {1., 1., 1.});

	create_image(&camera, num_objects, objects, num_lights, lights);

	save_image("test.ppm", &camera.image);

	free(camera.image.pixels);
	int i;
	for(i = 0; i < num_objects; i++)
	{
		free(objects[i].common);
	}
}
