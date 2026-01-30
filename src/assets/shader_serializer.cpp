#include "shader_serializer.h"

#include "platform/platform.h"
#include "io/filesystem.h"

using namespace ast;

static void serialize(AssetManager &assets, const AssetMetaData &metadata, const AssetHandle &handle, const FileStream &fs)
{
}

static Asset *try_load_data(AssetManager &assets, const AssetMetaData &metadata)
{
	const String &file_path = assets.get_system_file_path(metadata.file_path);

	bool failed_to_load = false;

	Vector<String> paths;

	// TODO: This is just a temporary solution.
	//       In the future, create an intermediary
	//       shader file listing the type and names.
	//       E.g:
	//       -----
	//       kind: Graphics
	//       paths: model.vert.spv, model.frag.spv
	if (platform::file_exists((file_path + ".comp.spv").c_str())) {
		paths.push_back(file_path + ".comp.spv");
	} else if (platform::file_exists((file_path + ".frag.spv").c_str())) {
		// TODO: Shader stage order should not matter in the paths ffs.
		paths.push_back(file_path + ".vert.spv");
		paths.push_back(file_path + ".frag.spv");
	} else {
		failed_to_load = true;
	}

	gfx::Device &device = assets.get_device();
	gfx::ShaderProgram *gfx_shader = device.create_shader_program(paths);
	ShaderAsset *asset = new ShaderAsset(gfx_shader, device);

	if (failed_to_load)
		asset->set_flag(ASSET_FLAG_INVALID, true);

	return asset;
}

AssetSerializer ast::get_shader_serializer()
{
	AssetSerializer shader_serializer = {};
	shader_serializer.serialize = serialize;
	shader_serializer.try_load_data = try_load_data;

	return shader_serializer;
}
