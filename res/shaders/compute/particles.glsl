#version 450

// https://vulkan-tutorial.com/Compute_Shader

layout (binding = 0) uniform ParameterUBO {
	float deltaTime;
} ubo;

struct Particle {
	vec2 position;
	vec2 velocity;
};

layout (std140, binding = 1) buffer ParticleSSBO {
	Particle particles[];
};

layout (local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

void main()
{
	uint index = gl_GlobalInvocationID.x;

	if (index < 0 || index >= 8) {
		return;
	}

	Particle particleIn = particles[index];

	particles[index].position = particleIn.position + particleIn.velocity * ubo.deltaTime;
}
