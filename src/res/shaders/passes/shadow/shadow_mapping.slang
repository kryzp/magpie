import scene;
import shadows;

// yes i know materials and material_sampler isn't used
// it's just cause I'm lazy and using the same push constants
// for both the opaque and masked depth passes.
struct PushConstants {
	scene::ObjectData *objects;
	scene::RenderMesh *meshes;
	scene::Material *materials;
	shadows::ShadowCaster *shadow_casters;
	uint32_t shadow_caster_index;
	uint32_t material_sampler;
};

[vk::push_constant]
PushConstants args;

struct VS_Output {
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint instance : SV_VulkanInstanceID, uint view_id : SV_ViewID)
{
	scene::ObjectData object_data = args.objects[instance];
	scene::RenderMesh mesh = args.meshes[object_data.mesh_index];
	scene::ModelVertex vertex = mesh.vertex_buffer[vertex_id];
	shadows::ShadowCaster caster = args.shadow_casters[args.shadow_caster_index];

	float3 local_position = vertex.position;

	if (scene::IsSkinned(object_data, mesh)) {
		scene::SkinVertex skin_vertex = mesh.skin_buffer[vertex_id];
		float4x4 skin = scene::ComputeSkinningMatrix(skin_vertex, object_data.skinning_palette);
		local_position = mul(skin, float4(local_position, 1.0)).xyz;
	}

	float4 world_position = mul(object_data.model_matrix, float4(local_position, 1.0));

	VS_Output output;
	output.sv_position = mul(caster.face_matrices[view_id], world_position);
	return output;
}
