#version 450

layout (location = 0) in vec3 o_colour;
layout (location = 1) in vec2 o_texCoord;

layout (location = 0) out vec4 fragColour;
layout (location = 1) out vec4 uvColour;

void main()
{
	fragColour = vec4(o_colour, 1.0);
	uvColour = vec4(o_texCoord, 0.0, 1.0);
}
