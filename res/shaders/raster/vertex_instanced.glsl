#version 450

layout (push_constant) uniform PushConstants {
	float time;
} pushConstants;

layout (binding = 0) uniform ParameterUBO {
    mat4 projMatrix;
    mat4 currViewMatrix;
    mat4 currModelMatrix;
	mat4 prevModelMatrix;
	mat4 prevViewMatrix;
} ubo;

// regular vertex data
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;

// instanced vertex data
layout (location = 4) in vec3 instance_positionOffset;

layout (location = 0) out VS_OUT
{
	vec3 colour;
	vec2 texCoord;
	vec3 position;
	vec3 normal;
	vec4 currScreenPos;
	vec4 prevScreenPos;
}
vs_out;

void main()
{
//	finalPos += sin(pushConstants.time*2.0 + 0.2*length(instance_positionOffset)) * normalize(instance_positionOffset) * 0.8;

	gl_Position = ubo.projMatrix * ubo.currViewMatrix * (vec4(instance_positionOffset, 0.0) + ubo.currModelMatrix * vec4(a_position, 1.0));

	vs_out.colour = a_colour;
	vs_out.texCoord = a_uv;
	vs_out.position = a_position + instance_positionOffset;
	vs_out.normal = mat3(transpose(inverse(ubo.currViewMatrix * ubo.currModelMatrix))) * a_normal; // YES, I KNOW inverse() IS EXPENSIVE

	vs_out.currScreenPos = ubo.projMatrix * ubo.currViewMatrix * (vec4(instance_positionOffset, 0.0) + ubo.currModelMatrix * vec4(a_position, 1.0));
	vs_out.prevScreenPos = ubo.projMatrix * ubo.prevViewMatrix * (vec4(instance_positionOffset, 0.0) + ubo.prevModelMatrix * vec4(a_position, 1.0));
}
