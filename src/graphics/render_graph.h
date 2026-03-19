#pragma once

// Resources:
//  * FrameGraph: Extensible Rendering Architecture in Frostbite by Yuriy O'Donnell
//  * Render graphs and Vulkan � A Deep Dive by Hans-Kristian Arntzen

#include <volk/volk.h>

#include <functional>
#include <typeinfo>
#include <typeindex>

#include "core/types.h"
#include "math/colour.h"

#include "container/string.h"
#include "container/hash_map.h"

#include "device.h"
#include "resource_cache.h"
#include "sync.h"

/*
#define GFX_DECLARE_BLACKBOARD_DATA(_name)			\
public:												\
class Meta {										\
public:												\
	static inline u32 type_id() { return id; }		\
private:											\
	static u32 id;									\
}													\

#define GFX_BLACKBOARD_DATA(_name) \
u32 _name::Meta::id = RenderGraphBlackboard::assign_id()
*/

namespace gfx
{
	class RenderGraph;
	class RenderScene;
	class Camera;

	enum SizeClass {
		SIZE_CLASS_ABSOLUTE,
		SIZE_CLASS_SWAPCHAIN_RELATIVE,
		SIZE_CLASS_MAX_ENUM
	};

	struct AttachmentInfo {
		VkFormat format;
	
		SizeClass size_class;
		float size_x, size_y, size_z;
	
		u32 samples;
		u32 mips;
		u32 layers;

		bool is_cubemap;
		bool is_transient;
		bool is_storage;

		AttachmentInfo()
			: format(VK_FORMAT_UNDEFINED)
			, size_class(SIZE_CLASS_SWAPCHAIN_RELATIVE)
			, size_x(1.f), size_y(1.f), size_z(1.f)
			, samples(1)
			, mips(1)
			, layers(1)
			, is_cubemap(false)
			, is_transient(false)
			, is_storage(false)
		{
		}

		AttachmentInfo(VkFormat format)
			: format(format)
			, size_class(SIZE_CLASS_SWAPCHAIN_RELATIVE)
			, size_x(1.f), size_y(1.f), size_z(1.f)
			, samples(1)
			, mips(1)
			, layers(1)
			, is_cubemap(false)
			, is_transient(false)
			, is_storage(false)
		{
		}

		bool operator == (const AttachmentInfo &other)
		{
			return
				this->format == other.format &&
				this->size_class == other.size_class &&
				this->size_x == other.size_x &&
				this->size_y == other.size_y &&
				this->size_z == other.size_z &&
				this->samples == other.samples &&
				this->mips == other.mips &&
				this->layers == other.layers &&
				this->is_cubemap == other.is_cubemap &&
				this->is_transient == other.is_transient &&
				this->is_storage == other.is_storage;
		}

		bool operator != (const AttachmentInfo &other)
		{
			return !(*this == other);
		}
	};

	struct GpuBufferInfo {
		VkDeviceSize size;
		VmaAllocationCreateFlags flags;
		VkBufferUsageFlags2 usage;

		GpuBufferInfo()
			: size(0)
			, flags()
			, usage()
		{
		}

		GpuBufferInfo(
			VkDeviceSize size,
			VmaAllocationCreateFlags flags,
			VkBufferUsageFlags2 usage
		)
			: size(size)
			, flags(flags)
			, usage(usage)
		{
		}

		bool operator == (const GpuBufferInfo &other)
		{
			return
				this->size == other.size &&
				this->flags == other.flags &&
				this->usage == other.usage;
		}

		bool operator != (const GpuBufferInfo &other)
		{
			return !(*this == other);
		}
	};

	struct RenderClear {
		RenderClear()
			: colour(0.f, 0.f, 0.f, 0.f)
		{
		}

		RenderClear(float r, float g, float b, float a)
			: colour(r, g, b, a)
		{
		}

		RenderClear(float depth, u8 stencil)
			: depth(depth)
			, stencil(stencil)
		{
		}

		union {
			DisplayColour colour;

			struct {
				float depth;
				u8 stencil;
			};
		};
	};

	typedef u32 RenderResourceHandle;
	constexpr RenderResourceHandle RENDER_INVALID_HANDLE = -1u;

	struct RenderResourceEdge {
		RenderResourceHandle handle;
		AccessState access_state;
		SubresourceRange range;
		bool clear_enabled;
		RenderClear clear;
	};

	struct ResourceTrackingState {
		VkPipelineStageFlags2 pipeline_barrier_stage_flags = 0;
		VkAccessFlags2 to_flush_access = 0; // Access masks that need to be made available.
		VkAccessFlags2 invalidated_in_stage[64] = {}; // Bitmask of access flags that have been made visible to a specific pipeline.
		VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
	};

	struct RenderResource {
		enum Kind {
			KIND_TEXTURE,
			KIND_BUFFER,
			KIND_MAX_ENUM
		};

		Kind kind;

		bool is_imported;

		u32 first_stage_index;
		u32 last_stage_index;

		u32 ref_count;

		ResourceTrackingState tracking;

		const Texture *physical_texture;
		AttachmentInfo texture_info;

		const GpuBuffer *physical_buffer;
		GpuBufferInfo buffer_info;
		u64 buffer_offset;
	};

	struct RenderGraphBlackboardData {
		virtual ~RenderGraphBlackboardData() = default;
	};

	class RenderGraphBlackboard {
	public:
		RenderGraphBlackboard();
		~RenderGraphBlackboard();

		void clean();

		template <typename T, typename ...Args>
		T &add(Args &&...args);

		template <typename T>
		T &get();

	private:
		HashMap<std::type_index, Unique<RenderGraphBlackboardData>> items;
	};

	template <typename T, typename ...Args>
	T &RenderGraphBlackboard::add(Args &&...args)
	{
		Unique<T> data = create_unique<T>(std::forward<Args>(args)...);
		T &ref = *data;
		items[std::type_index(typeid(T))] = std::move(data);
		return ref;
	}

	template <typename T>
	T &RenderGraphBlackboard::get()
	{
		return *static_cast<T *>(items.at(std::type_index(typeid(T))).get());
	}

	struct RenderStage;

	class RenderStageResources {
	public:
		RenderStageResources(RenderGraph &graph, const RenderStage &stage);
		~RenderStageResources();

		RenderInfo build_rendering_info() const;
		
		const Texture *get_texture(RenderResourceHandle handle) const;
		const TextureView *get_texture_view(RenderResourceHandle handle, const SubresourceRange &range) const;

		const GpuBuffer *get_buffer(RenderResourceHandle handle) const;

		// The view is NOT the same as saying get_buffer(handle)->get_device_address()!!
		// This also applies the physical_offset to the output.
		// Required for situations where multiple resources lie on different sections of a physical buffer.
		GpuBufferRange get_buffer_range(RenderResourceHandle handle) const;

	private:
		RenderGraph &graph;
		const RenderStage &stage;
	};

	struct RenderContext {
		Device &device;
		ResourceCache &cache;
		CommandBuffer &cmd;
		RenderScene &scene;
		const Camera &camera;
		float delta_time;
		float elapsed_time;
	};

	struct RenderStage {
		friend class RenderGraph; // TODO: remove ts

	public:
		enum Type {
			TYPE_GRAPHICS,
			TYPE_COMPUTE,
			TYPE_TRANSFER,
			TYPE_MAX_ENUM
		};

		RenderStage();
		~RenderStage();

		void set_record(std::function<void(const RenderContext &ctx, const RenderStageResources &resources)> fn);
		void set_multi_view_mask(u32 mask);

		u32 get_multi_view_mask() const;
		
		const Vector<RenderResourceEdge> &get_inputs() const;
		const Vector<RenderResourceEdge> &get_outputs() const;

		RenderResourceHandle write_colour(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all_colour(), const RenderClear *clear = nullptr);
		RenderResourceHandle write_depth(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all_depth(), const RenderClear *clear = nullptr);
		
		RenderResourceHandle read_texture(RenderResourceHandle handle);

		RenderResourceHandle read_texture_compute(RenderResourceHandle handle);
		RenderResourceHandle write_texture_compute(RenderResourceHandle handle);

		RenderResourceHandle blit_texture_src(RenderResourceHandle handle);
		RenderResourceHandle blit_texture_dst(RenderResourceHandle handle);

		RenderResourceHandle write_buffer_graphics(RenderResourceHandle handle);
		RenderResourceHandle read_buffer_graphics(RenderResourceHandle handle);

		RenderResourceHandle write_buffer_compute(RenderResourceHandle handle);
		RenderResourceHandle read_buffer_compute(RenderResourceHandle handle);

		RenderResourceHandle indirect_buffer(RenderResourceHandle handle);

		RenderResourceHandle clear_buffer(RenderResourceHandle handle);

	private:
		RenderResourceHandle add_edge(
			RenderResourceHandle handle,
			const AccessState &state,
			const SubresourceRange &range,
			bool is_output, const RenderClear *clear
		);

		String name;
		Type type;
		u32 index;
		
		std::function<void(const RenderContext &ctx, const RenderStageResources &resources)> record;

		u32 multi_view_mask;

		Vector<RenderResourceEdge> inputs;
		Vector<RenderResourceEdge> outputs;

		Vector<VkMemoryBarrier2> memory_barriers;
		Vector<VkBufferMemoryBarrier2> buffer_barriers;
		Vector<VkImageMemoryBarrier2> texture_barriers;

		bool is_culled;
	};

	class RenderResourcePool {
	public:
		RenderResourcePool(RenderGraph &graph);
		~RenderResourcePool();

		void destroy();
		
		void flush();

		const Texture *acquire_texture(const AttachmentInfo &info, ResourceTrackingState *out_state);
//		void release_texture(const Texture *texture, const AttachmentInfo &info);

		const GpuBuffer *acquire_buffer(const GpuBufferInfo &info, ResourceTrackingState *out_state);
//		void release_buffer(const GpuBuffer *buffer, const GpuBufferInfo &info);

		void update_texture_state(const Texture *texture, const ResourceTrackingState &state);
		void update_buffer_state(const GpuBuffer *buffer, const ResourceTrackingState &state);

	private:
		RenderGraph &graph;

		u64 current_time;
		u64 gpu_completed_time;

		struct PooledTexture {
			const Texture *texture;
			AttachmentInfo info;
			bool in_use;
			u64 last_frame_used;
			ResourceTrackingState state;
		};

		struct PooledBuffer {
			const GpuBuffer *buffer;
			GpuBufferInfo info;
			bool in_use;
			u64 last_frame_used;
			ResourceTrackingState state;
		};

		Vector<PooledTexture> texture_pool;
		Vector<PooledBuffer> buffer_pool;
	};

	class RenderGraph {
		friend class RenderStageResources;

	public:
		RenderGraph();
		~RenderGraph();

		void init(Device *device, ResourceCache *cache);
		void destroy();

		void reset();

		RenderStage &push_stage(const String &name, RenderStage::Type type);

		void set_backbuffer_source(RenderResourceHandle handle);

		void compile(const Swapchain &swapchain);
		
		void execute(
			CommandBuffer &cmd,
			const Swapchain &swapchain,
			RenderScene &scene, const Camera &camera,
			float delta_time, float elapsed_time
		);

		RenderResourceHandle create_texture(const AttachmentInfo &info);
		RenderResourceHandle create_buffer(const GpuBufferInfo &info);

		RenderResourceHandle import_texture(const Texture *texture);
		RenderResourceHandle import_buffer(const GpuBuffer *buffer);

		Device &get_device()
		{
			assert(device);
			return *device;
		}

	private:
		Device *device;
		ResourceCache *cache;

		void propogate_dependencies();
		void backpropogate_dependencies();
		void allocate_resources(const Swapchain &swapchain);
		void generate_barriers();

		void process_invalidate(RenderStage &stage, const RenderResourceEdge &edge);
		void process_flush(RenderStage &stage, const RenderResourceEdge &edge);

		void present_to_swapchain(CommandBuffer &cmd, const Swapchain &swapchain);

		Vector<RenderResource> resources;
		Vector<RenderStage> stages;

		RenderResourcePool pool;

		HashMap<const void *, RenderResourceHandle> import_cache;

		RenderResourceHandle backbuffer_handle;

		HashMap<const Texture *, ResourceTrackingState> tracked_external_textures;
		HashMap<const GpuBuffer *, ResourceTrackingState> tracked_external_buffers;
	};
}
