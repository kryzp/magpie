#pragma once

#include "core/types.h"

#include "container/vector.h"
#include "container/string.h"
#include "container/hash_map.h"
#include "container/stack.h"

#include "graphics/device.h"

class FileStream;

namespace ast
{

//	ASSET_TYPE_SOUND,
//	ASSET_TYPE_MATERIAL,
//	ASSET_TYPE_MESH,
//	ASSET_TYPE_MAP,

// I <3 X-MACROS!!
#define ASSET_DEFINITIONS \
	ASSET_DEF(TEXTURE, Texture) \
	ASSET_DEF(SHADER, Shader) \
	ASSET_DEF(MODEL, Model)

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

struct AssetHandle {
	u32 index;
	u32 generation;
};

struct AssetMetaData {
	String file_path;
};

#define AST_DEFINE_ASSET(type) \
	static AssetType get_asset_type_static() { return type; } \
	virtual AssetType get_asset_type() const override { return type; }

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
	void (*serialize)(AssetManager &assets, const FileStream &fs, const AssetMetaData &metadata, const AssetHandle &handle);
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

	AssetHandle from_file_path(const String &path, AssetType type);

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
				delete asset;
		}

		AssetHandle add(Asset *asset)
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

			list[index] = asset;
			generations[index]++;

			asset->handle = handle;

			return handle;
		}

		void remove(const AssetHandle &handle)
		{
			if (is_valid(handle)) {
				delete list[handle.index];
				list[handle.index] = nullptr;
				free_indices.push(handle.index);
			}
		}

		Asset *get(const AssetHandle &handle) const
		{
			assert(is_valid(handle));
			if (!is_valid(handle))
				return nullptr;
			return list[handle.index];
		}

		bool is_valid(const AssetHandle &handle) const
		{
			return
				(handle.index < list.size()) &&
				(generations[handle.index] == (handle.generation + 1));
		}

		Vector<Asset *> list;
		Vector<u32> generations;
		Stack<u32> free_indices;
		u64 capacity;
		u32 curr_id;
	};

	AssetImporter importer;
	HashMap<String, AssetHandle> path_to_handle;
	AssetList assets;
};

template <typename T, typename ...Args>
T *AssetManager::create_new_asset(const String &name, const String &path, Args &&...args)
{
	T *asset = new T(std::forward<Args>(args)...);
	asset->handle = assets.add(asset);

	return asset;
}

template <typename T>
T *AssetManager::get_asset(const AssetHandle &handle)
{
	return assets.get(handle)->as<T>();
}

}
