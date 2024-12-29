#version 450

layout (push_constant) uniform PushConstants {
	float time;
} pc;

layout (binding = 0) uniform FrameUBO {
	mat4 projMatrix;
	mat4 currViewMatrix;
	mat4 prevViewMatrix;
	vec3 viewPos;
} frameData;

layout (binding = 1) uniform InstanceUBO {
	mat4 currModelMatrix;
	mat4 prevModelMatrix;
} instanceData;

layout (location = 0) out VS_OUT
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
vs_out;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;
layout (location = 4) in vec3 a_tangent;

void main()
{
	gl_Position = frameData.projMatrix * frameData.currViewMatrix * instanceData.currModelMatrix * vec4(a_position, 1.0);

	vs_out.colour = a_colour;
	vs_out.texCoord = a_uv;
	vs_out.position = vec3(instanceData.currModelMatrix * vec4(a_position, 1.0));

	mat3 normalMatrix = mat3(transpose(inverse(instanceData.currModelMatrix)));

	vec3 T = normalize(normalMatrix * a_tangent);
	vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = normalize(cross(N, T));

	mat3 TBN = transpose(mat3(T, B, N));
	vs_out.tangentViewPos = TBN * frameData.viewPos;
	vs_out.tangentLightPos = TBN * (vec3(0.0, 1000.0, 0.0) + 1000.0*vec3(cos(pc.time), 0.0, sin(pc.time)));
	vs_out.tangentFragPos = TBN * vs_out.position;

	vs_out.currScreenPos = frameData.projMatrix * frameData.currViewMatrix * instanceData.currModelMatrix * vec4(a_position, 1.0);
	vs_out.prevScreenPos = frameData.projMatrix * frameData.prevViewMatrix * instanceData.prevModelMatrix * vec4(a_position, 1.0);
}
