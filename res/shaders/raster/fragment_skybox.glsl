#version 450

layout (location = 0) in vec3 o_colour;
layout (location = 1) in vec2 o_texCoord;
layout (location = 2) in vec3 o_fragPosition;

layout (location = 0) out vec4 fragColour;

layout (set = 0, binding = 1) uniform samplerCube u_skyBox;

void main()
{
	vec3 dir = normalize(o_fragPosition);

	fragColour = texture(u_skyBox, dir) * vec4(o_colour, 1.0);
}
