#include "model_vs.slang"

struct FS_Output
{
    float4 position			: SV_Target0;
	float4 albedo			: SV_Target1;
	float4 normal			: SV_Target2;
	float4 material			: SV_Target3;
	float4 emissive			: SV_Target4;
};

[shader("fragment")]
FS_Output fragmentMain(VS_Output input)
{
	float2 uv = frac(input.uv);

	SamplerState textureSampler		= g_bindlessSamplers[g_bindless.textureSampler_id];
	MaterialData *materialData		= g_bindless.buffers.materials + g_bindless.material_id;

	float4 albedo					= g_bindlessTexture2D[materialData.diffuse_id]		.Sample(textureSampler, uv).rgba;
	float  ambientOcclusion			= g_bindlessTexture2D[materialData.ambient_id]		.Sample(textureSampler, uv).r;
	float3 material					= g_bindlessTexture2D[materialData.material_id]		.Sample(textureSampler, uv).rgb;
	float3 normal					= g_bindlessTexture2D[materialData.normal_id]		.Sample(textureSampler, uv).rgb;
    float3 emissive					= g_bindlessTexture2D[materialData.emissive_id]		.Sample(textureSampler, uv).rgb;

    if (albedo.a < 0.99)
        discard;

    normal = normalize(mul(input.tbn, 2.0*normal - 1.0));
	normal = normal*0.5 + 0.5;

	material.r += ambientOcclusion;

	FS_Output output;
	output.position		= float4(input.position.xyz, 1.0);
	output.albedo		= float4(albedo.rgb, 1.0);
	output.normal		= float4(normal, 1.0);
	output.material		= float4(material, 1.0);
	output.emissive		= float4(emissive, 1.0);

	return output;
}
