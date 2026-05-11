import bindless;
import frame;
import scene;
import irradiance;
import lighting;
import material;
import shadows;
import material;

struct PushConstants
{
	frame::FrameData *frame_data;
	scene::ObjectData *objects;
	scene::Material *materials;
	scene::RenderMesh *meshes;

	lighting::Light *lights;
	shadows::ShadowCaster *shadow_casters;

	irradiance::ProbeSH *irr_sh;
	irradiance::ProbeGridInfo *irr_grid_info;

	uint32_t irradiance_fallback_cubemap;
	uint32_t prefilter_cubemap;
	uint32_t brdf_lut;

	uint32_t linear_sampler;
	uint32_t shadow_sampler;

	uint32_t light_count;
};

[vk::push_constant]
PushConstants args;

struct VS_Output
{
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 world_position;
	[vk::location(1)] float2 texcoord;
	[vk::location(2)] float3 colour;
	[vk::location(3)] float3x3 tbn;
	[vk::location(6)] uint32_t material_index; // TBN matrix takes 3 locations	
};

[shader("vertex")]
VS_Output
VertexMain(uint vertex_id : SV_VertexID, uint instance : SV_VulkanInstanceID)
{
	uint index = instance;//args.instance_buffer[instance];

	frame::FrameData frame_data = *args.frame_data;
	scene::ObjectData object_data = args.objects[index];
	scene::RenderMesh mesh = args.meshes[object_data.mesh_index];
	scene::ModelVertex vertex = mesh.vertex_buffer[vertex_id];

	float3 local_position  = vertex.position;
	float3 local_normal    = vertex.normal;
	float3 local_tangent   = vertex.tangent;
	float3 local_bitangent = vertex.bitangent;

	if (scene::IsSkinned(object_data, mesh))
	{
		scene::SkinVertex skin_vertex = mesh.skin_buffer[vertex_id];
		float4x4 skin = scene::ComputeSkinningMatrix(skin_vertex, object_data.skinning_palette);

		local_position = mul(skin, float4(local_position, 1.0)).xyz;

		float3x3 skin_tbn = (float3x3)skin;
		
		local_tangent   = mul(skin_tbn, local_tangent);
		local_bitangent = mul(skin_tbn, local_bitangent);
		local_normal    = mul(skin_tbn, local_normal);
	}
	
	float3x3 normal_matrix = (float3x3)object_data.normal_matrix;

	float3 T = normalize(mul(normal_matrix, local_tangent));
	float3 B = normalize(mul(normal_matrix, local_bitangent));
	float3 N = normalize(mul(normal_matrix, local_normal));

	T = normalize(T - dot(T, N) * N);
	B = normalize(B - dot(B, N) * N);

	float4 world_pos = mul(object_data.model_matrix, float4(local_position, 1.0));

	VS_Output output;
	output.sv_position = mul(frame_data.view_projection, world_pos);
	output.world_position = world_pos.xyz;
	output.texcoord = vertex.texcoord;
	output.colour = vertex.colour;
	output.tbn = transpose(float3x3(T, B, N));
	output.material_index = object_data.material_index;

	return output;
}

[shader("fragment")]
float4
FragmentMain(VS_Output input) : SV_Target
{
	frame::FrameData frame_data = *args.frame_data;
	scene::Material mat = args.materials[input.material_index];

	material::SurfaceInput surface_input;
	surface_input.world_position = input.world_position;
	surface_input.texcoord = input.texcoord;
	surface_input.vertex_colour = input.colour;
	surface_input.tbn = input.tbn;

	lighting::Surface surface = material::SampleSurface(surface_input, mat, frame_data, args.linear_sampler);

	if (material::ShouldDiscard(surface.alpha, mat.alpha_mode, mat.alpha_cutoff))
		discard;

	if (mat.unlit != 0)
		return float4(surface.albedo, surface.alpha);

	lighting::IBL ibl;
	ibl.irradiance_fallback_cubemap = args.irradiance_fallback_cubemap;
	ibl.prefilter_cubemap = args.prefilter_cubemap;
	ibl.brdf_lut = args.brdf_lut;
	ibl.irr_sh = args.irr_sh;
	ibl.irr_grid_info = args.irr_grid_info;
	ibl.sampler = args.linear_sampler;

	float3 Lo = lighting::EvaluateLight_Ambient(surface, ibl);

	for (int i = 0; i < args.light_count; i++)
	{
		lighting::Light light = args.lights[i];
		Lo += lighting::EvaluateLight_Point(surface, light, args.shadow_casters, args.shadow_sampler, frame_data);
	}

	Lo += surface.emissive;

	return float4(Lo, surface.alpha);
}
