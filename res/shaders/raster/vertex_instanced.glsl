#version 450

layout (push_constant) uniform PushConstants {
	float deltaTime;
} pushConstants;

layout (binding = 0) uniform ParameterUBO {
    mat4 projMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
} ubo;

// regular vertex data
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;

// instanced vertex data
layout (location = 4) in vec2 instance_positionOffset;

layout (location = 0) out vec3 o_colour;
layout (location = 1) out vec2 o_texCoord;
layout (location = 2) out vec3 o_fragPosition;

void main()
{
	gl_Position = ubo.projMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(a_position + vec3(instance_positionOffset, 0.0), 1.0);

	o_colour = vec3(a_colour.xy, instance_positionOffset.x * 0.2);
	o_texCoord = a_uv;
	o_fragPosition = a_position + vec3(instance_positionOffset, 0.0);
}
