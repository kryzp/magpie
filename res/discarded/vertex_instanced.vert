#version 450

layout (push_constant) uniform PushConstants {
	float time;
} pushConstants;

layout (binding = 0) uniform FrameUBO {
	mat4 projMatrix;
	mat4 currViewMatrix;
	mat4 prevViewMatrix;
} frameData;

layout (binding = 1) uniform InstanceUBO {
	mat4 currModelMatrix;
	mat4 prevModelMatrix;
} instanceData;

layout (binding = 2) uniform MaterialUBO {
	float temp;
} materialData;

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

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_colour;
layout (location = 3) in vec3 a_normal;

layout (location = 4) in vec3 instance_positionOffset;

void main()
{
	gl_Position = frameData.projMatrix * frameData.currViewMatrix * (vec4(instance_positionOffset, 0.0) + instanceData.currModelMatrix * vec4(a_position, 1.0));

	vs_out.colour = a_colour;
	vs_out.texCoord = a_uv;
	vs_out.position = a_position + instance_positionOffset;
	vs_out.normal = mat3(transpose(inverse(frameData.currViewMatrix * instanceData.currModelMatrix))) * a_normal; // YES, I KNOW inverse() IS EXPENSIVE

	vs_out.currScreenPos = frameData.projMatrix * frameData.currViewMatrix * (vec4(instance_positionOffset, 0.0) + instanceData.currModelMatrix * vec4(a_position, 1.0));
	vs_out.prevScreenPos = frameData.projMatrix * frameData.prevViewMatrix * (vec4(instance_positionOffset, 0.0) + instanceData.prevModelMatrix * vec4(a_position, 1.0));
}
