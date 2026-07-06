import scene;
import shadows;

struct PushConstants
{
	scene::ObjectData *objects;
	scene::RenderMesh *meshes;
	shadows::ShadowCaster *shadow_casters;
	uint shadow_caster_index;
};

[vk::push_constant] PushConstants args;

struct VS_Output
{
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 world_position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint instance : SV_VulkanInstanceID, uint view_id : SV_ViewID)
{
	scene::ObjectData object_data = args.objects[instance];
	scene::RenderMesh mesh = args.meshes[object_data.mesh_index];
	scene::ModelVertex vertex = mesh.vertex_buffer[vertex_id];
    shadows::ShadowCaster caster = args.shadow_casters[args.shadow_caster_index];

    float3 local_position = vertex.position;

	if (scene::IsSkinned(object_data, mesh))
    {
        scene::SkinVertex skin_vertex = mesh.skin_buffer[vertex_id];
        float4x4 skin = scene::ComputeSkinningMatrix(skin_vertex, object_data.skinning_palette);

        local_position = mul(skin, float4(local_position, 1.0)).xyz;
    }

    float4 world_position = mul(object_data.model_matrix, float4(local_position, 1.0));

	VS_Output output;
	output.world_position = world_position.xyz;
	output.sv_position = mul(caster.face_matrices[view_id], world_position);
	return output;
}

/*
 * Output a linear depth rather than a hardware depth.
 * Better for lighting.
 */
[shader("fragment")]
float FragmentMain(VS_Output input) : SV_Depth
{
	shadows::ShadowCaster caster = args.shadow_casters[args.shadow_caster_index];

	float distance = length(input.world_position - caster.position);
	float depth = (distance - caster.near_plane) / (caster.far_plane - caster.near_plane);

	return saturate(depth);
}
