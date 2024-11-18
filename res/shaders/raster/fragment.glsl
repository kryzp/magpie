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

layout (location = 0) out vec4 o_albedo;
layout (location = 1) out vec4 o_motion;
layout (location = 2) out vec4 o_normal;

layout (set = 0, binding = 1) uniform sampler2D s_texture;

void main()
{
	vec3 col = texture(s_texture, fs_in.texCoord).rgb;
	col *= fs_in.colour;
	o_albedo = vec4(col, 1.0);

	// ---

	o_normal = vec4(normalize(0.5 + 0.5*fs_in.normal), 1.0);

	// ---

	vec3 currScreenPos = 0.5 + 0.5*(fs_in.currScreenPos.xyz / fs_in.currScreenPos.w);
	vec3 prevScreenPos = 0.5 + 0.5*(fs_in.prevScreenPos.xyz / fs_in.prevScreenPos.w);

	vec3 ds = currScreenPos.xyz - prevScreenPos.xyz;

	o_motion = vec4(ds, 1.0);
}
