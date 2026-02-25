#include "render_scene.h"

#include "assets/texture_serializer.h"
#include "math/calc.h"

#include "device.h"
#include "resource_cache.h"

using namespace gfx;

RenderScene::RenderScene()
	: device(nullptr)
	, cache(nullptr)
	, objects()
	, lights()
	, geometry_pages()
	, mesh_registry()
	, meshes()
	, materials()
	, mesh_buffer()
	, material_buffer()
{
}

RenderScene::~RenderScene()
{
}

void RenderScene::init(Device *device, ResourceCache *cache)
{
	assert(device && cache);

	this->device = device;
	this->cache = cache;
	
	mesh_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gpu_types::GpuRenderMesh) * MAX_MESHES
	);

	material_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gpu_types::GpuMaterial) * MAX_MATERIALS
	);

}

void RenderScene::destroy()
{
	device->destroy_buffer(mesh_buffer);
	device->destroy_buffer(material_buffer);

	for (auto &page : geometry_pages) {
		device->destroy_buffer(page.vertex_buffer);
		device->destroy_buffer(page.index_buffer);
	}
}

RenderSceneResources RenderScene::update_transient_resources(GpuRingBuffer &frame_arena)
{
	RenderSceneResources resources = {};

	update_page_buffer(frame_arena, resources);

	if (!objects.transforms.empty())
		update_object_buffer(frame_arena, resources);

	if (!lights.data.empty())
		update_light_buffer(frame_arena, resources);

	if (!materials.empty())
		update_material_buffer();

	if (!meshes.empty())
		update_mesh_buffer();

	return resources;
}

void RenderScene::update_object_buffer(GpuRingBuffer &frame_arena, RenderSceneResources &resources)
{
	resources.object_buffer = frame_arena.push<gpu_types::GpuObjectData>(objects.transforms.size());

	gpu_types::GpuObjectData *mapped_objects = resources.object_buffer.cpu;

	// Cache friendly - Yay!!!
	for (int i = 0; i < objects.transforms.size(); i++) {
		mapped_objects[i].model_matrix = objects.transforms[i];
		mapped_objects[i].normal_matrix = objects.transforms[i].remove_translation().inverse().transpose();
		mapped_objects[i].sphere_bounds = Vec4(0.f, 0.f, 0.f, 1.f);
		mapped_objects[i].material_index = objects.materials[i];
		mapped_objects[i].mesh_index = objects.meshes[i];
		mapped_objects[i].page_index = objects.page_indices[i];
	}
}

void RenderScene::update_light_buffer(GpuRingBuffer &frame_arena, RenderSceneResources &resources)
{
	resources.light_buffer = frame_arena.push<gpu_types::GpuLight>(lights.data.size());

	gpu_types::GpuLight *mapped_lights = resources.light_buffer.cpu;
	memory_copy(mapped_lights, lights.data.data(), lights.data.size() * sizeof(gpu_types::GpuLight));
}

void RenderScene::update_page_buffer(GpuRingBuffer &frame_arena, RenderSceneResources &resources)
{
	resources.page_table_buffer = frame_arena.push<gpu_types::GpuPagePointers>(geometry_pages.size());

	gpu_types::GpuPagePointers *mapped_ptrs = resources.page_table_buffer.cpu;

	for (int i = 0; i < geometry_pages.size(); i++) {
		GeometryPage &page = geometry_pages[i];
		
		/*
		// ==================================
		// OPAQUE PASS
		// ==================================

		// Indirect buffers like 256-byte alignment!
		GpuAlloc<gpu_types::GpuIndirectDraw> opaque_indirect_buffer = frame_arena.push<gpu_types::GpuIndirectDraw>(PAGE_MAX_OBJECTS);
		GpuAlloc<u32> opaque_counter_buffer = frame_arena.push<u32>(1);
		*opaque_counter_buffer.cpu = 0; // Reset the counter to zero.

		// Register handles for graph barriers.
		resources.opaque_pass.indirect_buffers.push_back(opaque_indirect_buffer);
		resources.opaque_pass.counter_buffers.push_back(opaque_counter_buffer);

		// Update page struct allocation.
		page.opaque_pass.indirect_offset = opaque_indirect_buffer.offset;
		page.opaque_pass.count_offset = opaque_counter_buffer.offset;
		*/

		// ==================================
		// FILL BUFFER TABLE ENTRY
		// ==================================

		mapped_ptrs[i].vertex_buffer = page.vertex_buffer->get_device_address();

		// Filled in dynamically during culling pass.
		mapped_ptrs[i].opaque_indirect_buffer = 0;
		mapped_ptrs[i].opaque_count_buffer = 0;
	}
}

void RenderScene::update_material_buffer()
{
	assert(materials.size() < MAX_MATERIALS);

	gpu_types::GpuMaterial *mapped_materials = (gpu_types::GpuMaterial *)material_buffer->map();
	memory_copy(mapped_materials, materials.data(), materials.size() * sizeof(gpu_types::GpuMaterial));
}

void RenderScene::update_mesh_buffer()
{
	assert(meshes.size() < MAX_MESHES);

	gpu_types::GpuRenderMesh *mapped_meshes = (gpu_types::GpuRenderMesh *)mesh_buffer->map();
	memory_copy(mapped_meshes, meshes.data(), meshes.size() * sizeof(gpu_types::GpuRenderMesh));
}

bool RenderScene::is_valid_object(RenderHandle handle) const
{
	return
		handle.index < objects.handles.size() && // Is it out of range.
		handle.generation == objects.handles[handle.index].generation; // Is it stale.
}

bool RenderScene::is_valid_light(RenderHandle handle) const
{
	return
		handle.index < lights.handles.size() && // Is it out of range.
		handle.generation == lights.handles[handle.index].generation; // Is it stale.
}

RenderHandle RenderScene::create_object(
	const Mat4 &transform,
	u32 mesh, u32 material
)
{
	RenderHandle handle = {};
	handle.index = alloc_handle_index(objects.handles, objects.free_indices);
	handle.generation = objects.handles[handle.index].generation;

	u32 dense_index = objects.transforms.size();
	
	const MeshMemoryLocation mesh_memory = mesh_registry[mesh];

	objects.back_references.push_back(handle);
	objects.transforms.push_back(transform);
	objects.meshes.push_back(mesh_memory.index);
	objects.materials.push_back(material);
	objects.sphere_bounds.push_back(Vec4(0.f, 0.f, 0.f, 1.f)); // <-- TODO: TEMP
	objects.page_indices.push_back(mesh_memory.page);

	objects.handles[handle.index].dense_index = dense_index;

	return handle;
}

void RenderScene::remove_object(RenderHandle handle)
{
	if (!is_valid_object(handle))
		return;

	HandleEntry &entry = objects.handles[handle.index];

	u32 curr_dense_index = entry.dense_index;
	u32 prev_dense_index = objects.transforms.size() - 1;

	if (curr_dense_index != prev_dense_index) {

		// Previous slot user is now the current slot.
		RenderHandle prev_handle = objects.back_references[prev_dense_index];

		objects.transforms      [curr_dense_index] = objects.transforms      [prev_dense_index];
		objects.sphere_bounds   [curr_dense_index] = objects.sphere_bounds   [prev_dense_index];
		objects.meshes          [curr_dense_index] = objects.meshes          [prev_dense_index];
		objects.materials       [curr_dense_index] = objects.materials       [prev_dense_index];
		objects.page_indices    [curr_dense_index] = objects.page_indices    [prev_dense_index];
		objects.back_references [curr_dense_index] = objects.back_references [prev_dense_index];

		objects.handles[prev_handle.index].dense_index = curr_dense_index;
	}

	objects.transforms.pop_back();
	objects.sphere_bounds.pop_back();
	objects.meshes.pop_back();
	objects.materials.pop_back();
	objects.page_indices.pop_back();
	objects.back_references.pop_back();

	objects.free_indices.push_back(handle.index);
}

void RenderScene::set_transform(RenderHandle handle, const Mat4 &transform)
{
	if (!is_valid_object(handle))
		return;

	u32 dense_index = objects.handles[handle.index].dense_index;

	objects.transforms[dense_index] = transform;
}

RenderHandle RenderScene::create_light(const Light &light)
{
	RenderHandle handle = {};
	handle.index = alloc_handle_index(lights.handles, lights.free_indices);
	handle.generation = lights.handles[handle.index].generation;

	u32 dense_index = lights.data.size();

	Vec3 light_colour = Vec3(1.f, 1.f, 1.f);

	const float epsilon_intensity = 0.1f;
	const float light_max = light_colour.max_value();
	const float heuristic_radius = CalcF::sqrt((light.intensity * light_max) / (light.falloff * epsilon_intensity));

	gpu_types::GpuLight gpu_light = {};
	gpu_light.position = light.position;
	gpu_light.colour = light_colour;
	gpu_light.intensity = light.intensity;
	gpu_light.attenuation = Vec3(light.falloff, 0.f, 0.f);
	gpu_light.radius = heuristic_radius;
	gpu_light.transform = Mat4::transform(light.position, Quat(), Vec3(heuristic_radius), Vec3::zero());

	lights.data.push_back(gpu_light);

	lights.handles[handle.index].dense_index = dense_index;

	return handle;
}

void RenderScene::remove_light(RenderHandle handle)
{
	if (!is_valid_light(handle))
		return;

	// TODO
}

void RenderScene::set_light_colour(RenderHandle handle, const Vec3 &colour)
{
	if (!is_valid_light(handle))
		return;

	// TODO
}

void RenderScene::set_light_intensity(RenderHandle handle, float intensity)
{
	if (!is_valid_light(handle))
		return;

	u32 dense_index = lights.handles[handle.index].dense_index;
	gpu_types::GpuLight &light = lights.data[dense_index];

	light.intensity = intensity;

	const float epsilon_intensity = 0.1f;
	const float light_max = light.colour.max_value();
	const float heuristic_radius = CalcF::sqrt((light.intensity * light_max) / (light.attenuation.x * epsilon_intensity));

	light.transform = Mat4::transform(light.position, Quat(), Vec3(heuristic_radius), Vec3::zero());
}

Mat4 RenderScene::get_light_view(RenderHandle handle) const
{
	return Mat4::identity();
}

Mat4 RenderScene::get_light_proj(RenderHandle handle) const
{
	return Mat4::identity();
}

u32 RenderScene::register_mesh(const Mesh &mesh)
{
	assert(meshes.size() < MAX_MESHES);

	u32 page_index = find_suitable_page(mesh.vertex_count, mesh.index_count);

	GeometryPage &page = geometry_pages[page_index];

	u32 index = meshes.size();

	device->submit_graphics_immediate([&](CommandBuffer &cmd) -> void {
		const u64 vertex_stride = sizeof(gpu_types::GpuModelVertex);
		const u64 index_stride = sizeof(u32);

		VkBufferCopy vertex_copy = {};
		vertex_copy.srcOffset = 0;
		vertex_copy.dstOffset = page.vertex_offset * vertex_stride;
		vertex_copy.size      = mesh.vertex_count * vertex_stride;
		
		VkBufferCopy index_copy = {};
		index_copy.srcOffset = 0;
		index_copy.dstOffset = page.index_offset * index_stride;
		index_copy.size      = mesh.index_count * index_stride;

		cmd.copy_buffer_to_buffer(
			mesh.vertex_buffer,
			page.vertex_buffer,
			{ vertex_copy }
		);

		cmd.copy_buffer_to_buffer(
			mesh.index_buffer,
			page.index_buffer,
			{ index_copy }
		);
	});

	gpu_types::GpuRenderMesh gpu_mesh = {};
	gpu_mesh.index_count = mesh.index_count;
	gpu_mesh.first_index = page.index_offset;
	gpu_mesh.vertex_buffer = page.vertex_buffer->get_device_address() + (page.vertex_offset * sizeof(gpu_types::GpuModelVertex));

	meshes.push_back(gpu_mesh);

	// Move forward on allocator.
	page.vertex_offset += mesh.vertex_count;
	page.index_offset += mesh.index_count;

	u32 handle = mesh_registry.size();

	MeshMemoryLocation location = {};
	location.page = page_index;
	location.index = index;

	mesh_registry.push_back(location);

	return handle;
}

u32 RenderScene::register_material(const Material &material, ast::AssetManager &assets)
{
	assert(materials.size() < MAX_MATERIALS);

	auto load_texture = [&](ast::AssetHandle handle) -> u32 {
		if (assets.is_valid(handle))
			return cache->fetch_texture_view_std(assets.get_asset<ast::TextureAsset>(handle)->texture)->get_bindless_handle();
		return 0;
	};

	gpu_types::GpuMaterial gpu_material = {};
	gpu_material.albedo_texture = load_texture(material.albedo);
	gpu_material.normal_texture = load_texture(material.normal);
	gpu_material.emissive_texture = load_texture(material.emissive);
	gpu_material.metallic_roughness_texture = load_texture(material.metallic_roughness);
	gpu_material.ambient_texture = load_texture(material.ambient);

	u32 index = materials.size();

	materials.push_back(gpu_material);

	return index;
}

u32 RenderScene::get_object_count() const
{
	return objects.handles.size();
}

u32 RenderScene::get_light_count() const
{
	return lights.handles.size();
}

const GpuBuffer *RenderScene::get_mesh_buffer() const
{
	return mesh_buffer;
}

const GpuBuffer *RenderScene::get_material_buffer() const
{
	return material_buffer;
}

const Vector<GeometryPage> &RenderScene::get_geometry_pages() const
{
	return geometry_pages;
}

u32 RenderScene::alloc_handle_index(Vector<HandleEntry> &map, Vector<u32> &free_list)
{
	u32 index;

	if (!free_list.empty()) {
		index = free_list.back();
		free_list.pop_back();
		map[index].generation++;
	} else {
		index = map.size();
		map.push_back({ 0, 1 });
	}

	return index;
}

u32 RenderScene::find_suitable_page(u32 vertex_count, u32 index_count)
{
	if (!geometry_pages.empty()) {
		auto &last = geometry_pages.back();

		bool large_enough =
			last.vertex_offset + vertex_count <= last.max_vertices &&
			last.index_offset + index_count <= last.max_indices;

		if (large_enough)
			return geometry_pages.size() - 1;
	}

	GeometryPage page = create_new_page();

	u32 index = geometry_pages.size();

	geometry_pages.push_back(page);

	return index;
}

GeometryPage RenderScene::create_new_page()
{
	GeometryPage page = {};
	
	// Doesn't need to be VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT
	// because we use vertex pulling.
	page.vertex_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		PAGE_VERTEX_BUFFER_SIZE
	);
	
	page.index_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		PAGE_INDEX_BUFFER_SIZE
	);

	page.vertex_offset = 0;
	page.index_offset = 0;

	page.max_vertices = PAGE_VERTEX_BUFFER_SIZE / sizeof(gpu_types::GpuModelVertex);
	page.max_indices = PAGE_INDEX_BUFFER_SIZE / sizeof(IndexType);

	return page;
}
