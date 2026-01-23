#pragma once

#include <volk/volk.h>

#include <functional>

#include "core/types.h"
#include "math/colour.h"
#include "container/deque.h"
#include "container/hash_map.h"

#include "device.h"
#include "sync.h"

// Name isn't necessary but I like symmetry.
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
	struct AttachmentInfo {
		enum SizeClass {
			SIZE_CLASS_ABSOLUTE,
			SIZE_CLASS_SWAPCHAIN_RELATIVE,
			SIZE_CLASS_MAX_ENUM
		};

		VkFormat format;
	
		SizeClass size_class;
		float size_x, size_y;
	
		u32 samples;
		u32 mips;
		u32 layers;

		bool is_cubemap;
		bool is_transient;
		bool is_storage;

		AttachmentInfo()
			: format(VK_FORMAT_UNDEFINED)
			, size_class(SIZE_CLASS_SWAPCHAIN_RELATIVE)
			, size_x(1.f), size_y(1.f)
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
			, size_x(1.f), size_y(1.f)
			, samples(1)
			, mips(1)
			, layers(1)
			, is_cubemap(false)
			, is_transient(false)
			, is_storage(false)
		{
		}

		void get_absolute_size(const Swapchain &swapchain, u32 *width, u32 *height) const
		{
			assert(width && height);

			switch (size_class) {
				case SIZE_CLASS_ABSOLUTE:
					*width = size_x;
					*height = size_y;
					break;
				case SIZE_CLASS_SWAPCHAIN_RELATIVE:
					*width = swapchain.get_width() * size_x;
					*height = swapchain.get_height() * size_y;
					break;
			}
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
	};

	constexpr static u32 RENDER_INVALID_INDEX = -1u;

	struct RenderResourceHandle {
		u32 index;
		RenderResourceHandle() : index(RENDER_INVALID_INDEX) { }
		RenderResourceHandle(u32 index) : index(index) { }
		bool is_valid() const { return index != RENDER_INVALID_INDEX; }
	};

	struct SubresourceAlias {
		RenderResourceHandle parent;
		u32 base_mip;
		u32 base_layer;
		VkImageViewType view_type;

		SubresourceAlias()
			: parent()
			, base_mip(0)
			, base_layer(0)
			, view_type()
		{
		}
	};

	struct RenderResource {
		enum Kind {
			KIND_UNDEFINED,
			KIND_TEXTURE,
			KIND_BUFFER,
			KIND_MAX_ENUM
		};

		RenderResource()
			: kind(KIND_UNDEFINED)
			, index(RENDER_INVALID_INDEX)
			, physical_index(RENDER_INVALID_INDEX)
			, alias_of()
			, texture_info()
			, buffer_info()
			, texture_accesses()
			, buffer_accesses()
		{
		}

		RenderResource(Kind kind, u32 index)
			: kind(kind)
			, index(index)
			, physical_index(RENDER_INVALID_INDEX)
			, alias_of()
			, texture_info()
			, buffer_info()
			, texture_accesses()
			, buffer_accesses()
		{
		}

		Kind kind;

		u32 index;
		u32 physical_index;

		SubresourceAlias alias_of;

		AttachmentInfo texture_info;
		GpuBufferInfo buffer_info;

		// TODO: drop this garbage.
		Deque<sync::TextureAccessType> texture_accesses;
		Deque<sync::GpuBufferAccessType> buffer_accesses;

		bool is_alias() const
		{
			return alias_of.parent.is_valid();
		}

		bool is_allocated() const
		{
			return physical_index != RENDER_INVALID_INDEX;
		}
	};

	struct ResourceAttributes {
		RenderResource::Kind kind;
		u32 resource_index;

		union {
			// TODO: Should be TextureAllocInfo.
			struct {
				VkFormat format;
				u32 width;
				u32 height;
				u32 depth;
				u32 samples;
				u32 mips;
				u32 layers;
				bool is_cubemap;
				bool is_transient;
				bool is_storage;
			} texture;

			// TODO: Should be BufferAllocInfo.
			struct {
				VkDeviceSize size;
				VmaAllocationCreateFlagBits flags;
				VkBufferUsageFlags2 usage;
			} buffer;
		};
	};

	struct RenderClear {
		RenderClear()
			: enabled(false)
		{
		}

		RenderClear(float r, float g, float b, float a)
			: enabled(true)
			, colour(r, g, b, a)
		{
		}

		RenderClear(float depth, u8 stencil)
			: enabled(true)
			, depth(depth)
			, stencil(stencil)
		{
		}

		bool enabled;

		union {
			DisplayColour colour;

			struct {
				float depth;
				u8 stencil;
			};
		};
	};

	class RenderScene;
	class Camera;

	struct SceneView {
		RenderScene *scene;
		const Camera *camera;
	};

	struct RenderContext {
		Device &device;
		CommandBuffer &cmd;
		const SceneView &view;
		float delta_time;
		float elapsed_time;
	};

	class RenderGraph;

	using RenderStageRecordFn = std::function<void(const RenderContext &ctx)>;

	class RenderStage {
	public:
		enum Type {
			TYPE_GRAPHICS,
			TYPE_COMPUTE,
			TYPE_TRANSFER,
			TYPE_PRESENT,
			TYPE_MAX_ENUM
		};

		struct OutputAttachment {
			RenderResourceHandle handle;
			RenderClear clear;
		};

		RenderStage(RenderGraph &graph, Type type);
		~RenderStage();

		const Type get_type() const;
		const SceneView &get_view() const;
		u32 get_multi_view_mask() const;
		const RenderStageRecordFn &get_record_fn() const;

		const Vector<OutputAttachment> &get_output_attachments() const;
		const Vector<RenderResourceHandle> &get_textures() const;
		const Vector<RenderResourceHandle> &get_buffers() const;

		void set_scene_view(const SceneView &view);
		void set_multi_view_mask(u32 mask);
		void set_record(const RenderStageRecordFn &fn);

		void add_colour_output(const RenderResourceHandle &handle, const RenderClear *clear = nullptr);
		
		void set_depth_stencil(const RenderResourceHandle &handle, const RenderClear *clear = nullptr);

		void add_buffer(const RenderResourceHandle &handle, sync::GpuBufferAccessType access);
		void add_texture(const RenderResourceHandle &handle, sync::TextureAccessType access);

	private:
		RenderGraph &graph;

		Type type;
		SceneView scene_view;
		u32 multi_view_mask;
		RenderStageRecordFn record;

		Vector<OutputAttachment> outputs;
		Vector<RenderResourceHandle> textures;
		Vector<RenderResourceHandle> buffers;
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

	class RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

		void init(Device *device);
		void destroy();

		void execute(
			CommandBuffer &cmd, Swapchain &swapchain,
			float delta_time, float elapsed_time
		);

		RenderStage &add_stage(RenderStage::Type type);

		void resize(u32 width, u32 height);

		void set_swapchain_source(const RenderResourceHandle &source);

		RenderResourceHandle move_subresource(const SubresourceAlias &alias);

		RenderResourceHandle create_texture_resource(const AttachmentInfo &info);
		RenderResourceHandle create_buffer_resource(const GpuBufferInfo &info);

		RenderResource &get_resource(const RenderResourceHandle &handle);

		Texture &get_physical_texture(const RenderResource &resource);
		const Texture &get_physical_texture(const RenderResource &resource) const;

		TextureView &get_physical_texture_view(const RenderResource &resource);
		const TextureView &get_physical_texture_view(const RenderResource &resource) const;

		GpuBuffer &get_physical_buffer(const RenderResource &resource);
		const GpuBuffer &get_physical_buffer(const RenderResource &resource) const;

		Device &get_device() const
		{
			assert(device);
			return *device;
		}

	private:
		void execute_stage(
			const RenderStage &stage,
			CommandBuffer &cmd, Swapchain &swapchain,
			float delta_time, float elapsed_time
		);

		ResourceAttributes get_resource_attributes(const RenderResource &resource, const Swapchain &swapchain);

		void build_physical_resources(const Swapchain &swapchain);

		void setup_physical_texture(u32 index);
		void setup_physical_buffer(u32 index);

		void setup_attachments();

		void setup_aliases();

		void transition_texture(Texture &texture, sync::TextureAccessType dst_access, Vector<VkImageMemoryBarrier2> &barriers);
		void transition_buffer(GpuBuffer &buffer, sync::GpuBufferAccessType dst_access, Vector<VkBufferMemoryBarrier2> &barriers);

		Device *device = nullptr;

		Vector<RenderStage> stages;

		Vector<RenderResource> resources;

		Vector<bool> physical_slot_used;
		Vector<ResourceAttributes> physical_attributes;
		Vector<TextureView> physical_texture_views;
		Vector<Texture> physical_textures;
		Vector<GpuBuffer> physical_buffers;

		RenderResource *swapchain_source;
	};
}
