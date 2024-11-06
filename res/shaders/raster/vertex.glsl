#version 450

layout(push_constant) uniform PushConstants {
    mat4 projMatrix;
    mat4 modelMatrix;
} pushConstants;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;

layout (location = 0) out vec3 o_colour;
layout (location = 1) out vec2 o_texCoord;

void main()
{
	gl_Position = pushConstants.projMatrix * vec4(a_position, 1.0);

	o_colour = a_colour;
	o_texCoord = a_uv;
}
