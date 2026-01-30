#include "render_scene.h"

#include "assets/texture_serializer.h"
#include "math/calc.h"

#include "render_graph.h"

using namespace gfx;

void MeshPass::push_stage(RenderGraph &graph)
{
	struct MeshPassStageData {
		int foo;
	};

	graph.push_stage<MeshPassStageData>(
		"Mesh Pass",
		RenderStage::TYPE_TRANSFER,
		[&](RenderGraphBuilder &builder, MeshPassStageData &data) {
			GpuBufferInfo instance_info = {};
			instance_info.size = sizeof(gpu_types::GpuInstance) * RenderScene::MAX_OBJECTS;
			instance_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			instance_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;

			GpuBufferInfo compacted_instance_info = {};
			compacted_instance_info.size = sizeof(u32) * RenderScene::MAX_OBJECTS;
			compacted_instance_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			compacted_instance_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	
			GpuBufferInfo draw_indirect_info = {};
			draw_indirect_info.size = sizeof(gpu_types::GpuIndirect) * RenderScene::MAX_OBJECTS;
			draw_indirect_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			draw_indirect_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT;
	
//			GpuBufferInfo clear_indirect_info = {};
//			clear_indirect_info.size = sizeof(gpu_types::GpuIndirect) * RenderScene::MAX_OBJECTS;
//			clear_indirect_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
//			clear_indirect_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT;
			
			instance_buffer           = builder.create_buffer(instance_info);
			compacted_instance_buffer = builder.create_buffer(compacted_instance_info);
			draw_indirect_buffer      = builder.create_buffer(draw_indirect_info);
//			clear_indirect_buffer     = builder.create_buffer(clear_indirect_info);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const MeshPassStageData &data) {
		}
	);
}

void MeshPass::populate(RenderScene &scene)
{
	// RESET.
	multi_batches.clear();
	batches.clear();
	direct_batches.clear();

	// DIRECT BATCHES.
	for (auto &s : scene.get_objects()) {
		if (!render_scene_handle_valid(s.mesh_handle))
			continue;
		
		DirectBatch &direct_batch = direct_batches.emplace_back();
		direct_batch.object_id = s.id;
	}

	if (direct_batches.empty())
		return;

	// BATCHES.
	RenderSceneObject *object = &scene.get_object_from_handle(direct_batches[0].object_id);

	IndirectBatch *batch = &batches.emplace_back();
	batch->mesh_id = object->mesh_handle;
	batch->material_id = object->material_handle;
	batch->first = 0;
	batch->count = 1;

	for (int i = 1; i < direct_batches.size(); i++) {
		DirectBatch &curr = direct_batches[i];
		object = &scene.get_object_from_handle(curr.object_id);

		bool are_same_mesh = object->mesh_handle == batch->mesh_id;
		bool are_same_material = object->material_handle == batch->material_id;

		if (are_same_mesh && are_same_material) {
			batch->count++;
		} else {
			batch = &batches.emplace_back();
			batch->mesh_id = object->mesh_handle;
			batch->material_id = object->material_handle;
			batch->first = i;
			batch->count = 1;
		}
	}

	// MULTI BATCHES.
	MultiBatch *multi_batch = &multi_batches.emplace_back();
	multi_batch->count = 1;
	multi_batch->first = 0;

	for (int i = 1; i < batches.size(); i++) {
		IndirectBatch &curr_batch = batches[i];
		IndirectBatch &join_batch = batches[multi_batch->first];

		// As long as the materials are the same and the mesh
		// has been merged then we can combine the rendering
		// calls together.
		bool compatible_mesh = scene.get_mesh(join_batch.mesh_id)->is_merged;
		bool same_material = curr_batch.material_id == join_batch.material_id;

		if (compatible_mesh && same_material) {
			multi_batch->count++;
		} else {
			multi_batch = &multi_batches.emplace_back();
			multi_batch->count = 1;
			multi_batch->first = i;
		}
	}
}

void MeshPass::fill_instance_array(const RenderScene &scene, gpu_types::GpuInstance *instances)
{
	int index = 0;

	for (int i = 0; i < batches.size(); i++) {
		IndirectBatch &batch = batches[i];
		for (int k = 0; k < batch.count; k++) {
			DirectBatch &direct_batch = direct_batches[batch.first + k];
			instances[index].object_id = direct_batch.object_id;
			instances[index].batch_id = i;
			index++;
		}
	}
}

void MeshPass::fill_indirect_array(const RenderScene &scene, gpu_types::GpuIndirect *indirects)
{
	for (int i = 0; i < batches.size(); i++) {
		IndirectBatch &indirect_batch = batches[i];
		const RenderMesh *mesh = scene.get_mesh(indirect_batch.mesh_id);

		indirects[i].command.firstInstance = indirect_batch.first;
		indirects[i].command.instanceCount = 0; // This gets filled in the compute shader.
		indirects[i].command.vertexOffset = mesh->first_vertex;
		indirects[i].command.firstIndex = mesh->first_index;
		indirects[i].command.indexCount = mesh->index_count;
	}
}

void RenderScene::init(Device *device)
{
	this->device = device;

	object_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gpu_types::GpuObjectData) * MAX_OBJECTS
	);

	material_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gpu_types::GpuMaterial) * MAX_MATERIALS
	);

	light_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gpu_types::GpuLight) * MAX_OBJECTS
	);
}

void RenderScene::destroy()
{
	device->destroy_buffer(merged_vertex_buffer);
	device->destroy_buffer(merged_index_buffer);

	device->destroy_buffer(object_buffer);
	device->destroy_buffer(material_buffer);
	device->destroy_buffer(light_buffer);
}

void RenderScene::update()
{
}

void RenderScene::mesh_pass_stages(RenderGraph &graph)
{
	for (int i = 0; i < MeshPass::TYPE_MAX_ENUM; i++)
		passes[i].push_stage(graph);
}

void RenderScene::remove_object(u32 handle)
{
	pending_removals.push_back(handle);
}

void RenderScene::resolve_removing()
{
	Vector<u32> removals;
	
	for (int i = 0; i < pending_removals.size(); i++) {
		u32 to_remove_handle = pending_removals[i];
		RenderSceneObject &to_remove = get_object_from_handle(to_remove_handle);
		reusable_handles.push_back(to_remove_handle);
		removals.push_back(to_remove_handle);
	}

	// Remove in back->front order.
	for (int i = removals.size() - 1; i >= 0; i--) {
		objects.erase(objects.begin() + removals[i]);
	}

	pending_removals.clear();
}

u32 RenderScene::create_object(const Mat4 &transform)
{
	u32 handle = objects.size();

	if (reusable_handles.size() > 0) {
		handle = reusable_handles.back();
		reusable_handles.pop_back();
	}
	
	RenderSceneObject &object = objects.emplace_back();
	object.id = handle;
	object.transform = transform;

	return handle;
}

u32 RenderScene::upload_mesh(const Mesh &mesh)
{
	if (meshes.size() >= 1)
		return 0;

	u32 handle = meshes.size();

	/*
	for (int i = 0; i < meshes.size(); i++) {
		if (meshes[i] == mesh)
			return i;
	}
	*/

	RenderMesh &render_mesh = meshes.emplace_back();
	render_mesh.original = &mesh;
	render_mesh.is_merged = false;
	render_mesh.first_vertex = 0;
	render_mesh.first_index = 0;
	render_mesh.vertex_count = mesh.vertex_count;
	render_mesh.index_count = mesh.index_count;

	return handle;
}

u32 RenderScene::upload_material(const Material &material, ast::AssetManager &assets)
{
	if (materials.size() >= 1)
		return 0;

	/*
	for (int i = 0; i < scene->material_count; i++) {
		if (gfx_materials_equal(&scene->materials[i], material))
			return i;
	}
	*/

	u32 handle = materials.size();

	gpu_types::GpuMaterial gpu_material = {};
	gpu_material.diffuse_texture            = device->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(material.diffuse)             ->texture)->get_bindless_sampled();
	gpu_material.normal_texture             = device->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(material.normal)              ->texture)->get_bindless_sampled();
	gpu_material.emissive_texture           = device->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(material.emissive)            ->texture)->get_bindless_sampled();
	gpu_material.metallic_roughness_texture = device->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(material.metallic_roughness)  ->texture)->get_bindless_sampled();
	gpu_material.ambient_texture            = device->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(material.ambient)             ->texture)->get_bindless_sampled();

	u64 stride = sizeof(gpu_types::GpuMaterial);

	material_buffer->write(
		&gpu_material,
		stride, stride * handle
	);

	materials.push_back(material);

	return handle;
}

u32 RenderScene::upload_light(const Light &light)
{
	u32 handle = lights.size();

	lights.push_back(light);
	
	return handle;
}

RenderMesh *RenderScene::get_mesh(u32 handle)
{
	return &meshes[handle];
}

const RenderMesh *RenderScene::get_mesh(u32 handle) const
{
	return &meshes[handle];
}

Material *RenderScene::get_material(u32 handle)
{
	return &materials[handle];
}

const Material *RenderScene::get_material(u32 handle) const
{
	return &materials[handle];
}

Light *RenderScene::get_light(u32 handle)
{
	return &lights[handle];
}

const Light *RenderScene::get_light(u32 handle) const
{
	return &lights[handle];
}

void RenderScene::build_batches()
{
	for (auto &o : objects) {
		gpu_types::GpuObjectData o_data = {};
		o_data.model_matrix = o.transform;
		o_data.normal_matrix = o.transform.inverse().transpose();
		object_buffer->write(&o_data, sizeof(o_data), sizeof(o_data) * o.id);

		if (o.light_handle != RENDER_INVALID_HANDLE) {
			const Light &light = lights[o.light_handle];
			
			Vec3 light_colour = Vec3(1.f, 1.f, 1.f);

			const float epsilon_intensity = 0.1f;
			const float light_max = light_colour.max_value();
			const float heuristic_radius = CalcF::sqrt((light.intensity * light_max) / (light.falloff * epsilon_intensity));

			gpu_types::GpuLight gpu_light = {};
			gpu_light.position = o.transform.c[3].get_xyz();
			gpu_light.colour = light_colour;
			gpu_light.intensity = light.intensity;
			gpu_light.attenuation = Vec3(light.falloff, 0.f, 0.f);
			gpu_light.radius = heuristic_radius;
			gpu_light.transform = Mat4::transform(gpu_light.position, Quat(), Vec3(heuristic_radius), Vec3::zero());

			light_buffer->write(&gpu_light, sizeof(gpu_light), sizeof(gpu_light) * o.light_handle);
		}
	}

	for (int i = 0; i < MeshPass::TYPE_MAX_ENUM; i++)
		passes[i].populate(*this);
}

void RenderScene::merge_meshes()
{
	if (meshes.size() <= 0)
		return;

	// All meshes in the list *should* have the same vertex type.
	// If they don't we have a bit of a problem :/.
	const u64 vertex_size = meshes.front().original->vertex_size;
	const u64 index_size = sizeof(u16);

	u32 total_vertices = 0;
	u32 total_indices = 0;

	for (auto &mesh : meshes) {
		mesh.first_vertex = total_vertices;
		mesh.first_index = total_indices;

		total_vertices += mesh.vertex_count;
		total_indices += mesh.index_count;

		mesh.is_merged = true;
	}

	const u64 vb_size = total_vertices * vertex_size;
	const u64 ib_size = total_indices  * index_size;
	
	if (merged_vertex_buffer->get_size() < vb_size ||
	    merged_index_buffer->get_size() < ib_size) {
		
		device->destroy_buffer(merged_vertex_buffer);
		device->destroy_buffer(merged_index_buffer);

		merged_vertex_buffer = device->alloc_buffer(
			VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			vb_size
		);

		merged_vertex_buffer = device->alloc_buffer(
			VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			ib_size
		);
	}
	
	device->graphics().submit_immediate([&](CommandBuffer &cmd) -> void {
		for (auto &mesh : meshes) {
			VkBufferCopy vertex_copy = {};
			vertex_copy.srcOffset = 0;
			vertex_copy.dstOffset = mesh.first_vertex * vertex_size;
			vertex_copy.size      = mesh.vertex_count * vertex_size;
		
			VkBufferCopy index_copy = {};
			index_copy.srcOffset = 0;
			index_copy.dstOffset = mesh.first_index * index_size;
			index_copy.size      = mesh.index_count * index_size;

			cmd.copy_buffer_to_buffer(
				mesh.original->vertex_buffer,
				merged_vertex_buffer,
				{ vertex_copy }
			);

			cmd.copy_buffer_to_buffer(
				mesh.original->index_buffer,
				merged_index_buffer,
				{ index_copy }
			);
		}
	});
}

RenderSceneObject &RenderScene::get_object_from_handle(u32 handle)
{
	return objects[handle];
}

MeshPass &RenderScene::get_pass(MeshPass::Type type)
{
	return passes[type];
}

const MeshPass &RenderScene::get_pass(MeshPass::Type type) const
{
	return passes[type];
}

const Vector<RenderSceneObject> &RenderScene::get_objects() const
{
	return objects;
}

u32 RenderScene::get_light_count() const
{
	return lights.size();
}

const GpuBuffer *RenderScene::get_object_buffer() const
{
	return object_buffer;
}

const GpuBuffer *RenderScene::get_material_buffer() const
{
	return material_buffer;
}

const GpuBuffer *RenderScene::get_light_buffer() const
{
	return light_buffer;
}

const GpuBuffer *RenderScene::get_merged_vertex_buffer() const
{
	return merged_vertex_buffer;
}

const GpuBuffer *RenderScene::get_merged_index_buffer() const
{
	return merged_index_buffer;
}
