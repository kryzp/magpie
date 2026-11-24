import bindless;
import frame;
import scene;

struct PushConstants {
	frame::FrameData *frame_data;
	scene::ObjectData *objects;
	scene::Material *materials;
	scene::RenderMesh *meshes;
	uint32_t material_sampler;
	uint32_t _pad;
};

[vk::push_constant]
PushConstants args;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float2 texcoord;
	[vk::location(1)] uint32_t material_index;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint instance : SV_VulkanInstanceID)
{
	scene::ObjectData object_data = args.objects[instance];
	scene::RenderMesh mesh = args.meshes[object_data.mesh_index];
	scene::ModelVertex vertex = mesh.vertex_buffer[vertex_id];

	float3 local_position = vertex.position;

	if (scene::IsSkinned(object_data, mesh)) {
		scene::SkinVertex skin_vertex = mesh.skin_buffer[vertex_id];
		float4x4 skin = scene::ComputeSkinningMatrix(skin_vertex, object_data.skinning_palette);
		local_position = mul(skin, float4(local_position, 1.0)).xyz;
	}

	float4 world_pos = mul(object_data.model_matrix, float4(local_position, 1.0));

	VS_Output o;
	o.sv_position = mul(args.frame_data->view_projection, world_pos);
	o.texcoord = vertex.texcoord;
	o.material_index = object_data.material_index;
	return o;
}

[shader("fragment")]
void FragmentMain(VS_Output input)
{
	scene::Material mat = args.materials[input.material_index];

	if (mat.alpha_mode != scene::ALPHA_MODE_MASK)
		return;

	float alpha = mat.albedo_factor.a;

	if (BindlessIsValid(mat.albedo_texture))
		alpha *= SampleBindless(mat.albedo_texture, args.material_sampler, input.texcoord).a;

	if (alpha < mat.alpha_cutoff)
		discard;
}
