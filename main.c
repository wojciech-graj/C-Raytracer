#include "main.h"

void add_3_vectors(Vec3 vec1, Vec3 vec2, Vec3 vec3, Vec3 result)
{
	result[X] = vec1[X] + vec2[X] + vec3[X];
	result[Y] = vec1[Y] + vec2[Y] + vec3[Y];
	result[Z] = vec1[Z] + vec2[Z] + vec3[Z];
}

void create_plane(Vec3 normal, Vec3 point, Plane *plane)
{
	memcpy(plane->normal, normal, sizeof(Vec3));
	plane->d = - dot3(normal, point);
}

void init_camera(Camera *camera,
	Vec3 position,
	Vec3 vec1,
	Vec3 vec2,
	double focal_length,
	int image_resolution[2],
	Vec2 image_size)
{
	memcpy(camera->position, position, sizeof(Vec3));
	memcpy(camera->vectors[0], vec1, sizeof(Vec3));
	memcpy(camera->vectors[1], vec2, sizeof(Vec3));
	normalize3(camera->vectors[0]);
	normalize3(camera->vectors[1]);
	cross(camera->vectors[0], camera->vectors[1], camera->vectors[2]);
	camera->focal_length = focal_length;
	memcpy(camera->image.resolution, image_resolution, 2 * sizeof(int));
	memcpy(camera->image.size, image_size, 2 * sizeof(double));
	camera->image.pixels = malloc(image_resolution[X] * image_resolution[Y] * sizeof(Color));

	Vec3 focal_vector;
	Vec3 plane_center;
	Vec3 corner_offset_vectors[2];
	multiply3(camera->vectors[2], camera->focal_length, focal_vector);
	add3(focal_vector, camera->position, plane_center);
	create_plane(camera->vectors[2], plane_center, &camera->projection_plane);
	multiply3(camera->vectors[0], camera->image.size[X] / camera->image.resolution[X], camera->image.vectors[0]);
	multiply3(camera->vectors[1], camera->image.size[Y] / camera->image.resolution[Y], camera->image.vectors[1]);
	multiply3(camera->image.vectors[X], .5 - camera->image.resolution[X] / 2., corner_offset_vectors[X]);
	multiply3(camera->image.vectors[Y], .5 - camera->image.resolution[Y] / 2., corner_offset_vectors[Y]);
	add_3_vectors(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], camera->image.corner);
}

bool intersects_sphere(Line *ray, Object object, double *distance)
{
	Sphere *sphere = object.sphere;
	Vec3 relative_position;
	subtract3(ray->position, sphere->position, relative_position);
	double a = dot3(ray->vector, ray->vector);
	double b = 2 * dot3(ray->vector, relative_position);
	double c = dot3(relative_position, relative_position) - sqr(sphere->radius);
	double determinant = sqr(b) - 4 * a * c;
	if(determinant < 0) {
		return false;
	} else {
		*distance = (sqrt(determinant) + b) / (-2 * a);
		if(*distance < 0) {
			return false;
		} else {
			return true;
		}
	}
}

void cast_ray(Camera *camera, int num_objects, Object objects[num_objects], Vec3 pixel_position, Color pixel)
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
		if(objects[i].common->intersects(&ray, objects[i], &distance)) {
			if(distance < min_distance) {
				min_distance = distance;
				closest_object = objects[i];
			}
		}
	}
	memcpy(pixel, closest_object.common ? closest_object.common->color : background_color, sizeof(Color));
}

void create_image(Camera *camera, int num_objects, Object objects[num_objects])
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
			cast_ray(camera, num_objects, objects, pixel_position, camera->image.pixels[pixel_index]);
			pixel_index++;
		}
	}
}

void init_sphere(Sphere *sphere, Vec3 position, double radius, Color color)
{
	sphere->intersects = &intersects_sphere;
	memcpy(sphere->color, color, sizeof(Color));

	memcpy(sphere->position, position, sizeof(Vec3));
	sphere->radius = radius;
}

void save_image(char *filename, Image *image)
{
	FILE *file = fopen("test.ppm","wb");
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
		(Vec3) {1., 0., 0.},
		(Vec3) {0., 1., 0.},
		1.,
		(int[2]) {640, 480},
		(Vec2) {4./3., 1.});

	Object objects[2];
	Sphere *sphere1 = malloc(sizeof(Sphere));
	init_sphere(sphere1,
		(Vec3) {0., 0., 10.},
		1.,
		(Color) {0, 255, 0});
	objects[0].sphere = sphere1;
	Sphere *sphere2 = malloc(sizeof(Sphere));
	init_sphere(sphere2,
		(Vec3) {0., 1., 9.},
		1.5,
		(Color) {255, 0, 0});
	objects[1].sphere = sphere2;

	create_image(&camera, 2, objects);

	save_image("text.ppm", &camera.image);

	free(camera.image.pixels);
}
