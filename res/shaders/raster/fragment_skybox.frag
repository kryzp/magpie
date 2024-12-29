#version 450

layout (location = 0) in VS_OUT
{
	vec3 colour;
	vec2 texCoord;
	vec3 position;
	vec4 currScreenPos;
	vec4 prevScreenPos;
	vec3 tangentViewPos;
	vec3 tangentLightPos;
	vec3 tangentFragPos;
}
fs_in;

layout (location = 0) out vec4 o_fragColour;
//layout (location = 1) out vec4 o_motion;
//layout (location = 2) out vec4 o_normal;

layout (set = 0, binding = 3) uniform samplerCube sc_skyBox;

void main()
{
	vec3 dir = normalize(fs_in.position);
	o_fragColour = texture(sc_skyBox, dir) * vec4(fs_in.colour, 1.0);
}
