#include "util.slang"

struct ModelVertex
{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 texcoord;
    [[vk::location(2)]] float3 colour;
    [[vk::location(3)]] float3 normal;
    [[vk::location(4)]] float3 tangent;
    [[vk::location(5)]] float3 bitangent;
};

struct Material
{
	uint diffuse;
	uint normal;
	uint emissive;
	uint mr;
	uint ambient;
};

struct Arguments
{
	FrameData *frame_data;
	ObjectData *object_transforms;
	Material *materials;
	uint material_id;
	uint sampler;
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]] SamplerState bindless_samplers[];
[[vk::binding(1)]] Texture2D bindless_images[];

struct VS_Output
{
	float4 sv_position : SV_Position;

	[[vk::location(0)]] float4 position;
	[[vk::location(1)]] float2 texcoord;
	[[vk::location(2)]] float3 colour;
	[[vk::location(3)]] float3x3 tbn;
};

// Can also use uint instance_id : SV_VulkanInstanceID
// instead of uint baseInstance : SV_StartInstanceLocation

[shader("vertex")]
VS_Output VertexMain(ModelVertex vertex, uint base_instance : SV_StartInstanceLocation)
{
    FrameData *frame_data = args.frame_data;
    ObjectData *transform = args.object_transforms + base_instance;

	float3x3 normal_matrix = (float3x3)transform->normal_matrix;

    float3 T = normalize(mul(normal_matrix, vertex.tangent));
    float3 B = normalize(mul(normal_matrix, vertex.bitangent));
    float3 N = normalize(mul(normal_matrix, vertex.normal));

    VS_Output output;

    output.position = mul(transform->model_matrix, float4(vertex.position, 1.));
    output.sv_position = mul(frame_data->projection, mul(frame_data->view, output.position));
    output.texcoord = vertex.texcoord;
    output.colour = vertex.colour;
    output.tbn = transpose(float3x3(T, B, N));

    return output;
}

struct FS_Output
{
	float4 position : SV_Target0;
	float4 albedo   : SV_Target1;
	float4 normal   : SV_Target2;
	float4 material : SV_Target3;
	float4 emissive : SV_Target4;
};

[shader("fragment")]
FS_Output FragmentMain(VS_Output input)
{
	Material material = args.materials[args.material_id];

	float2 uv = frac(input.texcoord);

	SamplerState sampler = bindless_samplers[args.sampler];

	float4 albedo              = bindless_images[material.diffuse]    .Sample(sampler, uv).rgba;
	float3 normal              = bindless_images[material.normal]     .Sample(sampler, uv).rgb;
   	float3 emissive            = bindless_images[material.emissive]   .Sample(sampler, uv).rgb;
	float3 metallic_roughness  = bindless_images[material.mr]         .Sample(sampler, uv).rgb;
	float  ambient_occlusion   = bindless_images[material.ambient]    .Sample(sampler, uv).r;

	// Temporary for to make lighting debugging easier.
	albedo = 1.;
	emissive = 0.;
	metallic_roughness = float3(0., .2, 1.);
	ambient_occlusion = 1.;

	if (albedo.a < 0.)
	        discard;

	normal = normalize(mul(input.tbn, 2.*normal - 1.));
	normal = normal*.5 + .5;

	metallic_roughness.r += ambient_occlusion;

	FS_Output output;

	output.position = float4(input.position.xyz, 1.);
	output.albedo   = float4(albedo.rgb, 1.);
	output.normal   = float4(normal, 1.);
	output.material = float4(metallic_roughness, 1.);
	output.emissive = float4(emissive, 1.);

	return output;
}
