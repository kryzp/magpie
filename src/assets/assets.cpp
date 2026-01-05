#include "assets.h"

#include "texture_serializer.h"
#include "shader_serializer.h"
#include "model_serializer.h"

using namespace ast;

AssetImporter::AssetImporter()
	: serializers{}
{
	serializers[ASSET_TYPE_TEXTURE] = get_texture_serializer();
	serializers[ASSET_TYPE_SHADER] = get_shader_serializer();
	serializers[ASSET_TYPE_MODEL] = get_model_serializer();
}

AssetImporter::~AssetImporter()
{
}

Asset *AssetImporter::import(AssetManager &assets, AssetType type, const AssetMetaData &metadata)
{
	Asset *asset = serializers[type].try_load_data(assets, metadata);

	if (asset->has_flag(ASSET_FLAG_INVALID))
		debug_log("Failed to load asset: %s", metadata.file_path.c_str());
	else
		debug_log("Loaded asset: %s", metadata.file_path.c_str());
	
	return asset;
}

AssetManager::AssetManager()
	: importer()
	, assets{}
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::init(const Platform *platform, gfx::Device *device)
{
	this->platform = platform;
	this->device = device;
}

void AssetManager::destroy()
{
	assets.destroy_all();
}

void AssetManager::destroy_asset(const AssetHandle &handle)
{
	assets.remove(handle);
}

AssetHandle AssetManager::from_file_path(const String &path, AssetType type)
{
	if (path_to_handle.contains(path)) {
		return path_to_handle[path];
	} else {
		AssetMetaData metadata = {};
		metadata.file_path = path;

		Asset *asset = importer.import(*this, type, metadata);

		AssetHandle handle = assets.add(asset);

		path_to_handle[path] = handle;
		return handle;
	}
}

String AssetManager::get_system_file_path(const String &path) const
{
	return "res/" + path;
}

bool AssetManager::is_handle_valid(const AssetHandle &handle) const
{
	return assets.is_valid(handle);
}
