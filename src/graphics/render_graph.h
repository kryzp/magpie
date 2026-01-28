#pragma once

// Resources:
//  * FrameGraph: Extensible Rendering Architecture in Frostbite by Yuriy O'Donnell
//  * Render graphs and Vulkan — A Deep Dive by Hans-Kristian Arntzen

#include <volk/volk.h>

#include <functional>

#include "core/types.h"
#include "math/colour.h"
#include "container/deque.h"
#include "container/hash_map.h"

#include "device.h"
#include "sync.h"

#define GFX_DEFINE_BLACKBOARD_DATA(_name)			\
public:												\
class Meta {										\
public:												\
	static inline u32 type_id() { return id; }		\
private:											\
	static u32 id;									\
}													\

#define GFX_IMPLEMENT_BLACKBOARD_DATA(_name) \
u32 _name::Meta::id = RenderGraphBlackboard::assign_id()

namespace gfx
{
	class RenderGraph;
	class RenderScene;
	class Camera;

	struct SceneView {
		RenderScene *scene;
		const Camera *camera;
	};

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
		VmaAllocationCreateFlagBits flags;
		VkBufferUsageFlags2 usage;

		GpuBufferInfo()
			: size(0)
			, flags()
			, usage()
		{
		}

		GpuBufferInfo(
			VkDeviceSize size,
			VmaAllocationCreateFlagBits flags,
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

	struct SubresourceRange {
		u32 base_mip;
		u32 mips;
		u32 base_layer;
		u32 layers;

		static SubresourceRange all()
		{
			return {
				0, VK_REMAINING_MIP_LEVELS,
				0, VK_REMAINING_ARRAY_LAYERS
			};
		}
	};

	typedef u32 RenderResourceHandle;
	constexpr RenderResourceHandle RENDER_INVALID_HANDLE = -1u;

	struct RenderResourceEdge {
		RenderResourceHandle handle;

		union {
			struct {
				TextureAccessType access;
				SubresourceRange range;
				bool clear_enabled;
				RenderClear clear;
			} texture;

			struct {
				GpuBufferAccessType access;
			} buffer;
		};

		RenderResourceEdge()
			: handle()
			, texture()
		{
		}
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

		union {
			struct {
				const Texture *physical_resource;
				AttachmentInfo info;
				TextureAccessType initial_access;
				TextureAccessType final_access;
			} texture;

			struct {
				const GpuBuffer *physical_resource;
				GpuBufferInfo info;
				GpuBufferAccessType initial_access;
				GpuBufferAccessType final_access;
			} buffer;
		};
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

		static inline int assign_id()
		{
			return id_counter++;
		}

	private:
		static inline int id_counter = 0;
		HashMap<u32, RenderGraphBlackboardData *> items;
	};

	template <typename T, typename ...Args>
	T &RenderGraphBlackboard::add(Args &&...args)
	{
		u32 id = T::Meta::type_id();
		T *data = new T(std::forward<Args>(args)...);
		items[id] = data;
		return *data;
	}

	template <typename T>
	T &RenderGraphBlackboard::get()
	{
		u32 id = T::Meta::type_id();
		return *static_cast<T *>(items[id]);
	}

	struct RenderStage;

	class RenderStageResources {
	public:
		RenderStageResources(RenderGraph &graph, const RenderStage &stage);
		~RenderStageResources();

		RenderInfo build_rendering_info() const;
		
		const Texture *get_texture(RenderResourceHandle handle) const;
		TextureView get_texture_view(RenderResourceHandle handle) const;
		const GpuBuffer *get_buffer(RenderResourceHandle handle) const;

	private:
		RenderGraph &graph;
		const RenderStage &stage;
	};

	struct RenderContext {
		Device &device;
		CommandBuffer &cmd;
		SceneView scene_view;
		float delta_time;
		float elapsed_time;
	};

	struct RenderStage {
		enum Type {
			TYPE_GRAPHICS,
			TYPE_COMPUTE,
			TYPE_TRANSFER,
			TYPE_MAX_ENUM
		};

		const char *name;
		Type type;
		u32 index;
		
		std::function<void(const RenderContext &ctx, const RenderStageResources &resources)> record;

		u32 multi_view_mask;

		Vector<RenderResourceEdge> inputs;
		Vector<RenderResourceEdge> outputs;

		Vector<VkImageMemoryBarrier2> image_barriers;
		Vector<VkBufferMemoryBarrier2> buffer_barriers;

		bool is_culled;
	};

	class RenderGraphBuilder {
	public:
		RenderGraphBuilder(RenderGraph &graph, RenderStage &stage);
		~RenderGraphBuilder();

		VkFormat get_depth_format() const;

		void set_multi_view_mask(u32 mask);

		// Create a virtual resource without GPU memory backing.
		RenderResourceHandle create_texture(const AttachmentInfo &info) const;
		RenderResourceHandle create_buffer(const GpuBufferInfo &info) const;

		RenderResourceHandle write_colour(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all(), const RenderClear *clear = nullptr) const;
		RenderResourceHandle write_depth(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all(), const RenderClear *clear = nullptr) const;
		
		RenderResourceHandle read_texture(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all()) const;

		RenderResourceHandle blit_texture_src(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all()) const;
		RenderResourceHandle blit_texture_dst(RenderResourceHandle handle, const SubresourceRange &range = SubresourceRange::all()) const;

		RenderResourceHandle write_buffer(RenderResourceHandle handle, GpuBufferAccessType usage);
		RenderResourceHandle read_buffer(RenderResourceHandle handle, GpuBufferAccessType usage);

	private:
		RenderGraph &graph;
		RenderStage &current_stage;
	};

	class RenderResourcePool {
	public:
		RenderResourcePool(RenderGraph &graph);
		~RenderResourcePool();
		
		void flush();
		void destroy();

		const Texture *acquire_texture(const AttachmentInfo &info);
//		void release_texture(const Texture *texture, const AttachmentInfo &info);

		const GpuBuffer *acquire_buffer(const GpuBufferInfo &info);
//		void release_buffer(const GpuBuffer *buffer, const GpuBufferInfo &info);

	private:
		RenderGraph &graph;

		struct PooledTexture {
			const Texture *texture;
			AttachmentInfo info;
			bool in_use;
		};

		struct PooledBuffer {
			const GpuBuffer *buffer;
			GpuBufferInfo info;
			bool in_use;
		};

		Vector<PooledTexture> texture_pool;
		Vector<PooledBuffer> buffer_pool;
	};

	class RenderGraph {
		friend class RenderGraphBuilder;
		friend class RenderStageResources;

	public:
		RenderGraph();
		~RenderGraph();

		void init(Device *device);
		void destroy();

		void reset();

		template <typename T>
		void push_stage(
			const char *name,
			RenderStage::Type type,
			const std::function<void(RenderGraphBuilder &builder, T &data)> &setup,
			const std::function<void(const RenderContext &ctx, const RenderStageResources &resources, const T &data)> &execute
		);

		void set_backbuffer_source(RenderResourceHandle handle);

		void compile(const Swapchain &swapchain);
		
		void execute(
			CommandBuffer &cmd,
			const Swapchain &swapchain,
			const SceneView &scene_view,
			float delta_time, float elapsed_time
		);

		RenderResourceHandle import_texture(const Texture *texture);
		RenderResourceHandle import_buffer(const GpuBuffer *buffer);

		Device &get_device()
		{
			assert(device);
			return *device;
		}

	private:
		Device *device;

		void backpropogate_dependencies();
		void allocate_resources(const Swapchain &swapchain);
		void generate_barriers();

		void process_edge(RenderStage &stage, const RenderResourceEdge &edge);

		void present_to_swapchain(CommandBuffer &cmd, const Swapchain &swapchain);

		Vector<RenderResource> resources;
		Vector<RenderStage> stages;

		RenderResourcePool pool;

		HashMap<const void *, RenderResourceHandle> import_cache;
		HashMap<const void *, u32> resource_access_cache;

		RenderResourceHandle backbuffer_handle;
	};
	
	template <typename T>
	void RenderGraph::push_stage(
		const char *name,
		RenderStage::Type type,
		const std::function<void(RenderGraphBuilder &builder, T &data)> &setup,
		const std::function<void(const RenderContext &ctx, const RenderStageResources &resources, const T &data)> &execute
	)
	{
		RenderStage stage = {};
		stage.name = name;
		stage.type = type;
		stage.index = stages.size();

		RenderGraphBuilder builder(*this, stage);

		T data = {};
		setup(builder, data);
		
		stage.record = [=](const RenderContext &ctx, const RenderStageResources &resources) {
			execute(ctx, resources, data);
		};

		stages.push_back(stage);
	}
}
