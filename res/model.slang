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

struct Arguments
{
	FrameData *frame_data;
	TransformData *transforms;

	uint diffuse;
	uint normal;
	uint emissive;
	uint mr;
	uint ambient;

	uint sampler;

	uint _padding[2];
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

[shader("vertex")]
VS_Output VertexMain(ModelVertex vertex, uint instance_id : SV_InstanceID)
{
    FrameData *frame_data = args.frame_data;
    TransformData *transform = args.transforms + instance_id;

	float3x3 normal_matrix = (float3x3)transform->normal_matrix;

    float3 T = normalize(mul(normal_matrix, vertex.tangent));
    float3 B = normalize(mul(normal_matrix, vertex.bitangent));
    float3 N = normalize(mul(normal_matrix, vertex.normal));

    VS_Output output;

    output.position = mul(transform.model_matrix, float4(vertex.position, 1.0));
    output.sv_position = mul(frame_data->projection_matrix, mul(frame_data->view_matrix, output.position));
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
	float2 uv = frac(input.texcoord);

	SamplerState sampler = bindless_samplers[args.sampler];

	float4 albedo              = bindless_images[args.diffuse]    .Sample(sampler, uv).rgba;
	float3 normal              = bindless_images[args.normal]     .Sample(sampler, uv).rgb;
    float3 emissive            = bindless_images[args.emissive]   .Sample(sampler, uv).rgb;
	float3 material            = bindless_images[args.mr]         .Sample(sampler, uv).rgb;
	float  ambient_occlusion   = bindless_images[args.ambient]    .Sample(sampler, uv).r;

    if (albedo.a < 0.99)
	{
        discard;
	}

    normal = normalize(mul(input.tbn, 2.0*normal - 1.0));
	normal = normal*0.5 + 0.5;

	material.r += ambient_occlusion;

	FS_Output output;

	output.position = float4(input.position.xyz, 1.0);
	output.albedo   = float4(albedo.rgb, 1.0);
	output.normal   = float4(normal, 1.0);
	output.material = float4(material, 1.0);
	output.emissive = float4(emissive, 1.0);

	return output;
}
