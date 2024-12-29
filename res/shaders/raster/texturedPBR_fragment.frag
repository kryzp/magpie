#version 450

layout (push_constant) uniform PushConstants {
	float time;
} pushConstants;

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

layout (binding = 2) uniform MaterialUBO {
	float temp;
} materialData;

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

layout (location = 0) out vec4 o_albedo;

layout (set = 0, binding = 3) uniform sampler2D s_diffuseTexture;
layout (set = 0, binding = 4) uniform sampler2D s_specularTexture;
layout (set = 0, binding = 5) uniform sampler2D s_normalTexture;

void main()
{
	vec3 diffuseColour = texture(s_diffuseTexture, fs_in.texCoord).rgb;
	vec3 specularColour = texture(s_specularTexture, fs_in.texCoord).rgb;
	vec3 normal = texture(s_normalTexture, fs_in.texCoord).rgb;

	normal = normalize(2.0*normal - 1.0);

	vec3 lightDir = normalize(fs_in.tangentLightPos - fs_in.tangentFragPos);
	vec3 viewDir = normalize(fs_in.tangentViewPos - fs_in.tangentFragPos);

	// ambient
	vec3 ambient = 0.15 * diffuseColour;

	// diffuse
	float diffuseValue = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diffuseValue * diffuseColour;

	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specularValue = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	vec3 specular = specularValue * specularColour;

	// final result
	vec3 finalColour = ambient + diffuse + specular;
	o_albedo = vec4(finalColour * fs_in.colour, 1.0);
}

/*
layout (location = 0) out vec4 o_albedo;
//layout (location = 1) out vec4 o_motion;
//layout (location = 2) out vec4 o_normal;

layout (set = 0, binding = 3) uniform sampler2D s_texture;

void main()
{
//	o_normal = vec4(0.5 + 0.5*normalize(fs_in.normal), 1.0);
//
//	// ---
//
//	vec3 currScreenPos = 0.5 + 0.5*(fs_in.currScreenPos.xyz / fs_in.currScreenPos.w);
//	vec3 prevScreenPos = 0.5 + 0.5*(fs_in.prevScreenPos.xyz / fs_in.prevScreenPos.w);
//
//	vec2 ds = currScreenPos.xy - prevScreenPos.xy;
//
//	o_motion = vec4(ds, 0.0, 1.0);
//
//	// ---

	vec3 col = texture(s_texture, fs_in.texCoord).rgb;
	col *= fs_in.colour;
	o_albedo = vec4(col, 1.0);
}
*/