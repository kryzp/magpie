#include "util.slang"
#include "bindless.slang"
#include "types.slang"

struct Arguments {
    FrameData *frame_data;
    ObjectData *objects;
    Material *materials;
	Mesh *meshes;
//	uint *instance_buffer;
	uint sampler;
};

[vk::push_constant]
Arguments args;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float4 position;
	[vk::location(1)] float2 texcoord;
	[vk::location(2)] float3 colour;
    [vk::location(3)] float3x3 tbn;
    [vk::location(6)] uint material_index; // TBN matrix takes 3 locations.
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id: SV_VertexID, uint instance: SV_VulkanInstanceID)
{
    uint index = instance;//args.instance_buffer[instance];

    FrameData *frame_data = args.frame_data;
    ObjectData object_data = args.objects[index];

    Mesh mesh = args.meshes[object_data.mesh_index];

    ModelVertex vertex = mesh.vertex_buffer[vertex_id];

    float3x3 normal_matrix = (float3x3)object_data.normal_matrix;

	float3 T = normalize(mul(normal_matrix, vertex.tangent));
	float3 B = normalize(mul(normal_matrix, vertex.bitangent));
	float3 N = normalize(mul(normal_matrix, vertex.normal));

    VS_Output output;
    output.position = mul(object_data.model_matrix, float4(vertex.position, 1.));
	output.sv_position = mul(frame_data->projection, mul(frame_data->view, output.position));
	output.texcoord = vertex.texcoord;
	output.colour = vertex.colour;
    output.tbn = transpose(float3x3(T, B, N));
    output.material_index = object_data.material_index;

	return output;
}

struct FS_Output
{
	float4 position : SV_Target0;
	float4 albedo   : SV_Target1;
	float4 normal   : SV_Target2;
	float4 pbr      : SV_Target3;
	float4 emissive : SV_Target4;
};

[shader("fragment")]
FS_Output FragmentMain(VS_Output input)
{
	Material material = args.materials[input.material_index];

	float2 uv = frac(input.texcoord);

	SamplerState sampler = bindless_samplers[args.sampler];

	float4 albedo              = bindless_images[material.albedo_texture]    .Sample(sampler, uv).rgba;
	float3 normal              = bindless_images[material.normal_texture]    .Sample(sampler, uv).rgb;
   	float3 emissive            = bindless_images[material.emissive_texture]  .Sample(sampler, uv).rgb;
	float3 metallic_roughness  = bindless_images[material.mr_texture]        .Sample(sampler, uv).rgb;
	float  ambient_occlusion   = bindless_images[material.ambient_texture]   .Sample(sampler, uv).r;

	if (albedo.a < 0.)
	        discard;

	normal = normalize(mul(input.tbn, 2.*normal - 1.));
	normal = normal*.5 + .5;

	metallic_roughness.r += ambient_occlusion;

	FS_Output output;

	output.position = float4(input.position.xyz, 1.);
	output.albedo   = float4(albedo.rgb, 1.);
	output.normal   = float4(normal, 1.);
	output.pbr      = float4(metallic_roughness, 1.);
	output.emissive = float4(emissive, 1.);

	return output;
}
