#version 450

#define PARTICLE_COUNT 1

layout (local_size_x = PARTICLE_COUNT, local_size_y = 1, local_size_z = 1) in;

layout (push_constant) uniform PushConstants {
	float deltaTime;
} pc;

layout (binding = 0) uniform ParameterUBO {
    mat4 projMatrix;
    mat4 inverseProjMatrix;
	mat4 inverseViewMatrix;
	mat4 inversePrevViewMatrix;
	mat4 prevViewMatrix;
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

layout (binding = 2) uniform sampler2D s_motionTexture;
layout (binding = 3) uniform sampler2D s_normalTexture;
layout (binding = 4) uniform sampler2D s_depthTexture;

const vec3 GRAVITY = vec3(0.0, -3.0, 0.0);
const float TOUCH_DISTANCE = 0.05;

vec2 toScreenPosition(vec3 worldPosition, mat4 vm)
{
	vec4 clipSpacePos = ubo.projMatrix * vm * vec4(worldPosition, 1.0);
	vec3 ndcPosition = clipSpacePos.xyz / clipSpacePos.w;

	return 0.5*ndcPosition.xy + 0.5;
}

vec3 toWorldPosition(vec2 screenPosition, mat4 ivm)
{
	float depth = texture(s_depthTexture, vec2(screenPosition.x, 1.0 - screenPosition.y)).x;
	vec4 coord = ivm * ubo.inverseProjMatrix * vec4(2.0*screenPosition - 1.0, depth, 1.0);
	vec3 worldPosition = coord.xyz / coord.w;

	return worldPosition;
}

float distanceFromSurface(vec3 position)
{
	vec2 screenPosition = toScreenPosition(position, ubo.prevViewMatrix);
	vec3 normal = normalize(2.0*texture(s_normalTexture, vec2(screenPosition.x, 1.0 - screenPosition.y)).xyz - 1.0);
	vec3 projectedSurfacePosition = toWorldPosition(screenPosition, ubo.inversePrevViewMatrix);

	return abs(dot(normal, position - projectedSurfacePosition));
}

void main()
{
	uint idx = gl_GlobalInvocationID.x;

	vec3 position = particles[idx].position;
	vec2 screenPosition = toScreenPosition(position, ubo.prevViewMatrix);

	bool onSurface = distanceFromSurface(position) <= TOUCH_DISTANCE;

	if (onSurface)
	{
		vec2 naiveMotion = texture(s_motionTexture, vec2(screenPosition.x, 1.0 - screenPosition.y)).xy;
		vec2 naiveScreenPosition = screenPosition + naiveMotion;

		vec2 correctionMotion = texture(s_motionTexture, vec2(naiveScreenPosition.x, 1.0 - naiveScreenPosition.y)).xy;
		vec2 newScreenPosition = screenPosition + correctionMotion;

		vec3 newPosition = toWorldPosition(newScreenPosition, ubo.inverseViewMatrix);
		vec3 curPosition = position;

		particles[idx].position = newPosition;
		particles[idx].velocity = (newPosition - curPosition) / pc.deltaTime;
	}
	else
	{
		particles[idx].position += particles[idx].velocity*pc.deltaTime + 0.5*GRAVITY*pc.deltaTime*pc.deltaTime;
		particles[idx].velocity += GRAVITY*pc.deltaTime;
	}
}
