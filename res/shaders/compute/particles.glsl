#version 450

#define PARTICLE_COUNT 64

layout (local_size_x = PARTICLE_COUNT, local_size_y = 1, local_size_z = 1) in;

layout (push_constant) uniform PushConstants {
	float deltaTime;
} pc;

layout (binding = 0) uniform ParameterUBO {
    mat4 viewProjMatrix;
} ubo;

struct Particle {
	vec3 position;
	float _padding0;
	vec3 velocity;
	float _padding1;
};

layout (binding = 1) buffer ParticleSSBO {
	Particle particles[PARTICLE_COUNT];
};

layout (binding = 2) uniform sampler2D u_motionTexture;
layout (binding = 3) uniform sampler2D u_normalTexture;
layout (binding = 4) uniform sampler2D u_depthTexture;

vec3 toScreenPosition(vec3 worldPosition)
{
	vec4 clipSpacePos = ubo.viewProjMatrix * vec4(worldPosition, 1.0);
	vec3 projCoords = clipSpacePos.xyz / clipSpacePos.w;
	return projCoords*0.5 + 0.5;
}

vec3 toWorldPosition(vec3 screenPosition)
{
	// yes i know inverse() is an expensive function...
	vec4 coord = inverse(ubo.viewProjMatrix) * vec4(2.0*screenPosition - 1.0, 1.0);
	return coord.xyz / coord.w;
}

void main()
{
	uint idx = gl_GlobalInvocationID.x;

	if (idx < 0 || idx >= PARTICLE_COUNT) {
		return;
	}
	
	vec3 gravity = vec3(0.0, -5.0, 0.0);

	vec3 position = particles[idx].position;
	vec3 screenPosition = toScreenPosition(position);

	float depth = texture(u_depthTexture, screenPosition.xy).x;

	vec3 projectedSurfacePosition = toWorldPosition(vec3(screenPosition.xy, depth));

	vec3 normal = 2.0*texture(u_normalTexture, screenPosition.xy).xyz - 1.0;

	float approxDistanceFromSurface = abs(dot(normal, position - projectedSurfacePosition));

	bool onSurface = false;//approxDistanceFromSurface <= COLLISION_DISTANCE;

	if (onSurface)
	{
		vec3 naiveNewPosition = position + particles[idx].velocity*pc.deltaTime;

		vec3 correctionMotion = texture(u_motionTexture, toScreenPosition(naiveNewPosition).xy).xyz;
		vec3 newScreenPosition = toScreenPosition(position) + correctionMotion;

		vec3 newPosition = toWorldPosition(newScreenPosition);
		vec3 curPosition = position;

		particles[idx].position = newPosition;
		particles[idx].velocity = (newPosition - curPosition) / pc.deltaTime;
	}
	else
	{
		particles[idx].position += particles[idx].velocity*pc.deltaTime + 0.5*gravity*pc.deltaTime*pc.deltaTime;
		particles[idx].velocity += gravity*pc.deltaTime;
	}
}
