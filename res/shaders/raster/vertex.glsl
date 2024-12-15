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

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;

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
	mat4 mvMatrix = ubo.currViewMatrix * ubo.currModelMatrix;

	gl_Position = ubo.projMatrix * mvMatrix * vec4(a_position, 1.0);

	vs_out.colour = a_colour;
	vs_out.texCoord = a_uv;
	vs_out.position = a_position;
	vs_out.normal = mat3(transpose(inverse(mvMatrix))) * a_normal; // YES, I KNOW inverse() IS EXPENSIVE

	vs_out.currScreenPos = ubo.projMatrix * ubo.currViewMatrix * ubo.currModelMatrix * vec4(a_position, 1.0);
	vs_out.prevScreenPos = ubo.projMatrix * ubo.prevViewMatrix * ubo.prevModelMatrix * vec4(a_position, 1.0);
}
