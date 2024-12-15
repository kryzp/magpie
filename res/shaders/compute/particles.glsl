#version 450

#define PARTICLE_COUNT 1

layout (local_size_x = PARTICLE_COUNT, local_size_y = 1, local_size_z = 1) in;

layout (push_constant) uniform PushConstants {
	float deltaTime;
} pc;

layout (binding = 0) uniform ParameterUBO {
    mat4 viewProjMatrix;
    mat4 inverseViewProjMatrix;
} ubo;

struct Particle {
	vec3 position;
	float stuck;
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
	vec3 ndcPosition = clipSpacePos.xyz / clipSpacePos.w;

	vec3 screenPosition = vec3(
		ndcPosition.x*0.5 + 0.5,
		1.0 - (ndcPosition.y*0.5 + 0.5),
		worldPosition.z
	);
	
	return screenPosition;
}

vec3 toWorldPosition(vec3 screenPosition)
{
	vec3 ndcPosition = vec3(
		2.0*screenPosition.x - 1.0,
		1.0 - 2.0*screenPosition.y,
		screenPosition.z
	);
	
	vec4 coord = ubo.inverseViewProjMatrix * vec4(ndcPosition, 1.0);

	vec3 worldPosition = coord.xyz / coord.w;

	return worldPosition;
}

void main()
{
	uint idx = gl_GlobalInvocationID.x;

	if (idx < 0 || idx >= PARTICLE_COUNT) {
		return;
	}
	
//	vec3 gravity = vec3(0.0, -9.81, 0.0);
//
//	vec3 position = particles[idx].position;
//	vec3 screenPosition = toScreenPosition(position);
//	
//	vec3 normal = 2.0*texture(u_normalTexture, screenPosition.xy).xyz - 1.0;
//	float depth = texture(u_depthTexture, screenPosition.xy).x;
//
//	vec3 projectedSurfacePosition = toWorldPosition(vec3(screenPosition.x, screenPosition.y, depth));
//
//	float approxDistanceFromSurface = abs(dot(normal, position - projectedSurfacePosition));
//
//	bool onSurface = approxDistanceFromSurface <= 0.1;

	particles[idx].position = toWorldPosition(toScreenPosition(particles[idx].position));

	/*
	if (depth <= 0.99 && onSurface) // THIS IS VERY BROKEN RIGHT NOW
	{
		vec3 naiveMotion = texture(u_motionTexture, screenPosition.xy).xyz;
		vec3 naiveScreenPosition = screenPosition + vec3(naiveMotion.x, -naiveMotion.y, naiveMotion.z);

		vec3 correctionMotion = texture(u_motionTexture, naiveScreenPosition.xy).xyz;
		vec3 newScreenPosition = screenPosition + vec3(correctionMotion.x, -correctionMotion.y, correctionMotion.z);

		vec3 newPosition = toWorldPosition(newScreenPosition);
		vec3 curPosition = position;

		particles[idx].position = newPosition;
		particles[idx].velocity = (newPosition - curPosition) / pc.deltaTime;

		particles[idx].stuck = depth;
	}
	else
	{
		particles[idx].position += particles[idx].velocity*pc.deltaTime + 0.5*gravity*pc.deltaTime*pc.deltaTime;
		particles[idx].velocity += gravity*pc.deltaTime;

		particles[idx].stuck =10.0+ depth;
	}
	*/
}
