#pragma once

#include <condition_variable>
#include <mutex>

#include "core/types.h"
#include "core/memory_arena.h"

#include "container/vector.h"
#include "container/string.h"
#include "container/hash_map.h"
#include "container/stack.h"

#include "graphics/device.h"

#include "job/job.h"

class FileStream;

//	ASSET_TYPE_SOUND,
//	ASSET_TYPE_MATERIAL,
//	ASSET_TYPE_MESH,
//	ASSET_TYPE_MAP,

// I <3 X-MACROS!!
#define ASSET_DEFINITIONS			\
	ASSET_DEF(TEXTURE, Texture)		\
	ASSET_DEF(SHADER, Shader)		\
	ASSET_DEF(MODEL, Model)

#define ASSET_DECLARE(type_)												\
	static AssetType get_asset_type_static() { return type_; }				\
	virtual AssetType get_asset_type() const override { return type_; }

namespace ast
{
	enum AssetType {
		ASSET_TYPE_UNKNOWN = 0,
	#define ASSET_DEF(capitals_, name_) ASSET_TYPE_##capitals_,
		ASSET_DEFINITIONS
	#undef ASSET_DEF
		ASSET_TYPE_MAX_ENUM
	};

	inline AssetType get_asset_type_from_string(const String &name)
	{
	#define ASSET_DEF(capitals_, name_) if (name == String(#name_)) return ASSET_TYPE_##capitals_;
		ASSET_DEFINITIONS
	#undef ASSET_DEF

		debug_log_crash("Unknown Asset Name: %s", name.c_str());

		return ASSET_TYPE_MAX_ENUM;
	}

	inline String get_string_from_asset_type(AssetType type)
	{
	#define ASSET_DEF(capitals_, name_) if (type == ASSET_TYPE_##capitals_) return #name_;
		ASSET_DEFINITIONS
	#undef ASSET_DEF

		debug_log_crash("Unknown Asset Type: %d", type);

		return "Unknown";
	}

	enum AssetState {
		ASSET_STATE_UNLOADED,
		ASSET_STATE_LOADING_DATA,
		ASSET_STATE_WAITING_FOR_DEPENDENCIES,
		ASSET_STATE_WAITING_FOR_GPU,
		ASSET_STATE_READY,
		ASSET_STATE_FAILED
	};

	inline bool asset_state_is_loading(AssetState state)
	{
		return
			state == ASSET_STATE_LOADING_DATA ||
			state == ASSET_STATE_WAITING_FOR_DEPENDENCIES ||
			state == ASSET_STATE_WAITING_FOR_GPU;
	}
	
	inline bool asset_state_needs_load(AssetState state)
	{
		return
			state == ASSET_STATE_UNLOADED ||
			state == ASSET_STATE_FAILED;
	}

	inline bool asset_state_is_loaded(AssetState state)
	{
		return
			state == ASSET_STATE_READY;
	}

	inline bool asset_state_is_finalized(AssetState state)
	{
		return
			state == ASSET_STATE_READY ||
			state == ASSET_STATE_FAILED;
	}

	struct AssetMetaData {
		String file_path;
	};

	struct AssetHandle {
		u32 index;
		u32 generation;

		bool is_null() const
		{
			return index == -1u && generation == 0;
		}

		static AssetHandle invalid()
		{
			return {
				.index = -1u,
				.generation = 0
			};
		}
	};

	struct Asset {
		friend class AssetManager;

		Asset() : handle() { }

		virtual void unload() = 0;

		virtual AssetType get_asset_type() const = 0;

		template <typename T>
		T *as()
		{
			return static_cast<T *>(this);
		}

		const AssetHandle &get_handle() const
		{
			return handle;
		}

	private:
		AssetHandle handle;
	};

	class AssetManager;

	struct AssetLoadContext {
		AssetManager &assets;
		const AssetMetaData &metadata;
		ArenaView &arena;
		String system_file_path() const;
	};

	struct AssetLoadResult {
		void *data;                       // For use by serializers.
		u64 stage_size;                   // Gpu buffer size required.
		bool failed;                      // Did we fail in loading?
		Vector<AssetHandle> dependencies; // Assets we depend on.
	};

	class IAssetSerializer {
	public:
		virtual ~IAssetSerializer() = default;
		virtual AssetLoadResult load(const AssetLoadContext &ctx) = 0;
		virtual Asset *finalize(const AssetLoadContext &ctx, const AssetLoadResult &result, gfx::Device &device) = 0;
		virtual void gpu_upload(Asset *asset, const AssetLoadContext &ctx, const AssetLoadResult &result, gfx::CommandBuffer &cmd, gfx::GpuBuffer *stage, u64 stage_base) { }
		virtual void dispose(const AssetLoadResult &result) { }
	};

	struct AssetUpload {
		VirtualArena *scratch; // Arena upon which the arena lies.
		ArenaView arena; // Used specifically for this upload.
		AssetMetaData metadata;
		AssetHandle handle;
		AssetType type;
		AssetLoadResult result;
	};

	class AssetManager {
		constexpr static u64 GPU_UPLOAD_CHUNK_SIZE = MEGABYTES(128);

	public:
		AssetManager();
		~AssetManager();

		void init(ArenaView &&arena, gfx::Device *device);
		void destroy();

		template <typename T, typename ...Args>
		T *create_new_asset(const String &name, const String &path, Args &&...args);

		void destroy_asset(const AssetHandle &handle);

		AssetHandle from_file_path(const String &path);

		bool is_loaded(const AssetHandle &handle) const;
		bool is_loading(const AssetHandle &handle);
		
		// Blocks until loaded.
		template <typename T>
		T *get_asset(const AssetHandle &handle);

		void load_now(const AssetHandle &handle, AssetType type);
		void load_async(const AssetHandle &handle, AssetType type);
		void reload_async(const AssetHandle &handle, AssetType type);

	private:
		void load_asset_internal(const AssetHandle &handle, AssetType type, job::JobCounter *counter);

	public:
		void poll_hot_reloads();

		void wait_for_async_uploads();
		
		void push_upload(AssetUpload &&upload);
		void push_for_dependency_resolution(AssetUpload &&upload);
		
	private:
		void resolve_pending_dependencies(job::JobCounter *counter);

	public:
		void flush_uploads();

		bool is_valid(const AssetHandle &handle) const;
		bool is_placeholder(const AssetHandle &handle) const;
		String get_path(const AssetHandle &handle) const;

		IAssetSerializer *get_serializer(AssetType type) const;
		
		void mount(const String &prefix, const String &physical_directory);
		String get_system_file_path(const String &path) const;

	private:
		gfx::Device *device = nullptr;

		void create_fallbacks();
		Asset *get_fallback_asset(AssetType type);
		
		AssetHandle allocate_asset(const String &path);

		void notify_dependents(const AssetHandle &handle, bool failed);
		void notify_dependents_no_lock(const AssetHandle &handle, bool failed);

		IAssetSerializer *serializers[ASSET_TYPE_MAX_ENUM];

		HashMap<String, AssetHandle> path_to_handle;
		
		job::JobCounter *async_upload_counter;

		Vector<AssetUpload> upload_queue;
		std::mutex upload_mutex;

		Vector<AssetUpload> dependency_queue;
		std::mutex dependency_mutex;
		
		std::condition_variable loading_cv;
		std::mutex loading_mutex;

		HashMap<String, String> mount_points;

		ArenaView asset_arena;
		std::mutex allocation_mutex;

		struct AssetRecord {
			Asset *asset = nullptr;
			String path;
			u32 generation = 0;
			u64 last_write_time = 0;
			AssetState state = ASSET_STATE_UNLOADED;
			u32 pending_dependencies = 0;
			Vector<AssetHandle> dependents; // Assets that depend on this asset.
			AssetUpload stashed_upload_data;
		};

		Vector<AssetRecord> asset_records;
		Stack<u32> free_asset_indices;
	};

	template <typename T, typename ...Args>
	T *AssetManager::create_new_asset(const String &name, const String &path, Args &&...args)
	{
		T *asset = new T(std::forward<Args>(args)...);
		asset->handle = asset_records.add(asset, path);

		path_to_handle[path] = asset->handle;

		return asset;
	}

	template <typename T>
	T *AssetManager::get_asset(const AssetHandle &handle)
	{
		assert(is_valid(handle));

		if (asset_records[handle.index].state == ASSET_STATE_READY)
			return asset_records[handle.index].asset->as<T>();

		if (asset_records[handle.index].state == ASSET_STATE_UNLOADED ||
			asset_records[handle.index].state == ASSET_STATE_FAILED)
			load_now(handle, T::get_asset_type_static());

		return asset_records[handle.index].asset->as<T>();
	}
}
