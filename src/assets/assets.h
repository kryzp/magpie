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
#define ASSET_DEFINITIONS \
	ASSET_DEF(TEXTURE, Texture) \
	ASSET_DEF(SHADER, Shader) \
	ASSET_DEF(MODEL, Model)

#define ASSET_DECLARE(type) \
	static AssetType get_asset_type_static() { return type; } \
	virtual AssetType get_asset_type() const override { return type; }

namespace ast
{
	enum AssetType {
		ASSET_TYPE_UNKNOWN = 0,
	#define ASSET_DEF(capitals_, name_) ASSET_TYPE_##capitals_,
		ASSET_DEFINITIONS
	#undef ASSET_DEF
		ASSET_TYPE_MAX_ENUM
	};

	enum AssetFlag {
		ASSET_FLAG_NONE    = 0 << 0,
		ASSET_FLAG_INVALID = 1 << 0
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

	struct AssetMetaData {
		String file_path;
	};

	struct AssetHandle {
		u32 index;
		u32 generation;

		bool is_null() const
		{
			return index == 0 && generation == 0;
		}

		static AssetHandle invalid()
		{
			return {
				.index = 0,
				.generation = 0
			};
		}
	};

	struct Asset {
		friend class AssetManager;

		Asset()
			: handle()
			, flags(0)
		{
		}

		virtual ~Asset() = default;

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

		bool has_flag(AssetFlag flag) const
		{
			return flags & flag;
		}

		void set_flag(AssetFlag flag, bool enabled)
		{
			if (enabled)
				flags |= flag;
			else
				flags &= ~flag;
		}

	private:
		AssetHandle handle;
		u32 flags;
	};

	class AssetManager;

	struct AssetLoadContext {
		AssetManager &assets;
		const AssetMetaData &metadata;
		ArenaView &arena;
		String system_file_path() const;
	};

	struct AssetLoadResult {
		void *data;        // For use by serializers.
		u64 stage_size;    // Gpu buffer size required.
		bool failed;       // Did we fail in loading?
	};

	struct AssetSerializer {
		// Phase 1 - Thread-Safe, Load in file into a blob.
		// Phase 2 - Not Thread-Safe a. Allocate the actual asset.
		//                           b. Upload the asset onto the GPU.

		// Phase 1
		AssetLoadResult (*load)(const AssetLoadContext &ctx);

		// Phase 2a
		Asset *(*allocate)(
			const AssetLoadContext &ctx, const AssetLoadResult &result,
			gfx::Device &device
		);

		// Phase 2b
		void (*upload)(
			Asset *asset,
			const AssetLoadContext &ctx, const AssetLoadResult &result,
			gfx::CommandBuffer &cmd,
			gfx::GpuBuffer *stage, u64 stage_base
		);
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
	public:
		// What sized chunks of data are sent to the GPU at a time.
		constexpr static u64 GPU_UPLOAD_CHUNK_SIZE = MEGABYTES(128);

		/*
		// Maximum amount of memory on the CPU before we stop
		// to give time to upload it to the GPU.
		constexpr static u64 MAX_MEMORY_PRESSURE = MEGABYTES(512);
		
		std::atomic<u64> memory_pressure;
		*/

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
		
		template <typename T>
		T *get_asset(const AssetHandle &handle);

		void load_now(const AssetHandle &handle, AssetType type);
		void load_async(const AssetHandle &handle, AssetType type);
		
		void reload_async(const AssetHandle &handle, AssetType type);

	private:
		void load_asset_internal(const AssetHandle &handle, AssetType type);

	public:
		void poll_hot_reloads();

		void wait_for_async_uploads();
		void push_upload(AssetUpload &&upload);
		void flush_uploads();

		bool is_valid(const AssetHandle &handle);
		bool is_placeholder(const AssetHandle &handle) const;

		const AssetSerializer &get_serializer(AssetType type) const;
		
		void mount(const String &prefix, const String &physical_directory);
		String get_system_file_path(const String &path) const;

	private:
		gfx::Device *device = nullptr;

		void create_fallbacks();
		Asset *get_fallback_asset(AssetType type);
		
		class AssetList {
		public:
			constexpr static u32 INITIAL_CAPACITY = 16;

			AssetList()
				: list()
				, free_indices()
				, capacity()
				, curr_index(1) // index = 0 is an invalid handle.
			{
				list.resize(INITIAL_CAPACITY);
			}

			~AssetList()
			{
			}

			AssetHandle add(Asset *asset, const String &path)
			{
				u32 index;

				if (!free_indices.empty()) {
					index = free_indices.top();
					free_indices.pop();
				} else {
					index = curr_index++;
				}

				if (index >= list.size())
					list.resize(list.size() * 2);

				AssetHandle handle = {};
				handle.index = index;
				handle.generation = list[index].generation;

				list[index].asset = asset;
				list[index].path = path;
				list[index].generation++;
				list[index].last_write_time = 0;

				return handle;
			}

			void remove(const AssetHandle &handle)
			{
				assert(is_valid(handle));

				delete list[handle.index].asset;
				list[handle.index].asset = nullptr;

				free_indices.push(handle.index);
			}

			Asset *get(const AssetHandle &handle) const
			{
				return is_valid(handle)
					? list[handle.index].asset
					: nullptr;
			}

			void set(const AssetHandle &handle, Asset *asset)
			{
				assert(is_valid(handle));
				list[handle.index].asset = asset;
			}

			String get_path(const AssetHandle &handle) const
			{
				assert(is_valid(handle));
				return list[handle.index].path;
			}

			bool is_valid(const AssetHandle &handle) const
			{
				return
					(handle.index > 0) &&
					(handle.index < list.size()) &&
					(list[handle.index].generation == (handle.generation + 1));
			}

			struct AssetRecord {
				Asset *asset;
				String path;
				u32 generation;
				u64 last_write_time;
			};

			Vector<AssetRecord> list;
			Stack<u32> free_indices;
			u64 capacity;
			u32 curr_index;
		};

		AssetList assets;

		AssetSerializer serializers[ASSET_TYPE_MAX_ENUM];

		HashMap<String, AssetHandle> path_to_handle;

		Vector<AssetUpload> upload_queue;
		job::JobCounter *upload_counter;
		std::mutex upload_mutex;

		HashMap<u32, bool> loading_assets;
		std::condition_variable loading_cv;
		std::mutex loading_mutex;

		HashMap<String, String> mount_points;

		ArenaView asset_arena;
	};

	template <typename T, typename ...Args>
	T *AssetManager::create_new_asset(const String &name, const String &path, Args &&...args)
	{
		T *asset = new T(std::forward<Args>(args)...);
		asset->handle = assets.add(asset, path);

		path_to_handle[path] = asset->handle;

		return asset;
	}

	template <typename T>
	T *AssetManager::get_asset(const AssetHandle &handle)
	{
		T *here = assets.get(handle)->as<T>();

		if (here)
			return here;

		load_now(handle, T::get_asset_type_static());
		return assets.get(handle)->as<T>();
	}
}
