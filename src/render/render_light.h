#ifndef RENDER_LIGHT_H
#define RENDER_LIGHT_H

typedef enum R_LightType
{
	R_LightType_Point,
	R_LightType_COUNT
}
R_LightType;

typedef struct R_Light R_Light;
struct R_Light
{
	R_LightType type;

	v3 position;
	v3 direction;

	v3 colour;
	f32 intensity;
	f32 falloff;

	b32 casts_shadows;
	f32 shadow_near;
	f32 shadow_far;
};

static inline f32 R_LightHeuristicRadius(const R_Light *light, f32 epsilon_intensity)
{
	f32 maximum = V3Max(light->colour);
	return SquareRoot((light->intensity * maximum) / (light->falloff * epsilon_intensity));
}

#endif // RENDER_LIGHT_H
