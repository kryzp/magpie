#pragma once

#include "core/types.h"

#include "container/vector.h"
#include "container/string.h"
#include "container/hash_map.h"
#include "container/stack.h"

#include "graphics/device.h"

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

#define AST_DEFINE_ASSET(type) \
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
		ASSET_FLAG_NONE    = 0,
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
			return (T *)this;
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

	// Asset serializers are just pairs of function pointers
	// for serializing and de-serializing an asset.
	struct AssetSerializer {
		void (*serialize)(AssetManager &assets, const AssetMetaData &metadata, const AssetHandle &handle, const FileStream &fs);
		Asset *(*try_load_data)(AssetManager &assets, const AssetMetaData &metadata);
	};

	class AssetImporter {
	public:
		AssetImporter();
		~AssetImporter();

		Asset *import(AssetManager &assets, AssetType type, const AssetMetaData &metadata);

	private:
		AssetSerializer serializers[ASSET_TYPE_MAX_ENUM];
	};

	class AssetManager {
	public:
		AssetManager();
		~AssetManager();

		void init(gfx::Device *device);
		void destroy();

		template <typename T, typename ...Args>
		T *create_new_asset(const String &name, const String &path, Args &&...args);

		template <typename T>
		T *get_asset(const AssetHandle &handle);

		void destroy_asset(const AssetHandle &handle);

		AssetHandle from_file_path(const String &path);

		String get_system_file_path(const String &path) const;
		bool is_handle_valid(const AssetHandle &handle) const;

		gfx::Device *get_device() const
		{
			return device;
		}

	private:
		gfx::Device *device = nullptr;

		class AssetList {
		public:
			constexpr static u32 INITIAL_CAPACITY = 16;

			AssetList()
				: list()
				, generations()
				, free_indices()
				, capacity()
				, curr_id()
			{
				list.resize(INITIAL_CAPACITY);
				generations.resize(INITIAL_CAPACITY);
			}

			~AssetList()
			{
			}

			void destroy_all()
			{
				for (auto &asset : list)
					delete asset.asset;

				list.clear();
			}

			AssetHandle add(Asset *asset, const String &path)
			{
				u32 index = 0;

				if (!free_indices.empty()) {
					index = free_indices.top();
					free_indices.pop();
				} else {
					index = curr_id++;
					assert(index < list.size());
				}

				AssetHandle handle = {};
				handle.index = index;
				handle.generation = generations[index];

				list[index].asset = asset;
				list[index].path = path;

				generations[index]++;

				return handle;
			}

			void remove(const AssetHandle &handle)
			{
				if (is_valid(handle)) {
					delete list[handle.index].asset;
					list[handle.index].asset = nullptr;
					free_indices.push(handle.index);
				}
			}

			Asset *get(const AssetHandle &handle) const
			{
				assert(is_valid(handle));

				if (!is_valid(handle))
					return nullptr;
				
				return list[handle.index].asset;
			}

			void set(const AssetHandle &handle, Asset *asset)
			{
				assert(is_valid(handle));

				if (is_valid(handle))
					list[handle.index].asset = asset;
			}

			bool is_valid(const AssetHandle &handle) const
			{
				return
					(handle.index < list.size()) &&
					(generations[handle.index] == (handle.generation + 1));
			}

			String get_path(const AssetHandle &handle) const
			{
				assert(is_valid(handle));

				if (!is_valid(handle))
					return "INVALID HANDLE";

				return list[handle.index].path;
			}

			struct AssetRecord {
				Asset *asset;
				String path;
			};

			Vector<AssetRecord> list;

			Vector<u32> generations;
			Stack<u32> free_indices;
			
			u64 capacity;
			u32 curr_id;
		};

		AssetImporter importer;
		AssetList assets;

		HashMap<String, AssetHandle> path_to_handle;
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
		assert(assets.is_valid(handle));

		if (!assets.is_valid(handle))
			return nullptr;

		Asset *here = assets.get(handle);

		if (here)
			return (T *)here;

		AssetMetaData metadata = {};
		metadata.file_path = assets.get_path(handle);

		Asset *asset = importer.import(*this, T::get_asset_type_static(), metadata);
		asset->handle = handle;

		assets.set(handle, asset);

		return (T *)asset;
	}
}
