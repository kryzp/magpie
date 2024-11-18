#version 450

layout (location = 0) in VS_OUT
{
	vec3 colour;
	vec2 texCoord;
	vec3 position;
	vec3 normal;
	vec4 currScreenPos;
	vec4 prevScreenPos;
}
fs_in;

layout (location = 0) out vec4 o_fragColour;
layout (location = 1) out vec4 o_motion;
layout (location = 2) out vec4 o_normal;

layout (set = 0, binding = 1) uniform samplerCube sc_skyBox;

void main()
{
	vec3 dir = normalize(fs_in.position);

	o_fragColour = texture(sc_skyBox, dir) * vec4(fs_in.colour, 1.0);

//	o_motion = vec4(0.0, 0.1, 56.9, 1.0);
//	o_normals = vec4(0.0, 0.0, 0.0, 1.0);
}
