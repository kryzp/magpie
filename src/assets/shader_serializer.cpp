#include "shader_serializer.h"

#include "platform/platform.h"

using namespace ast;

static Vector<b8> load_file_bytes(const String &path)
{
	Vector<b8> bytes;

	FILE *file = fopen(path.c_str(), "rb");
	u64 file_size = 0;

	if (file) {
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes.resize(file_size);
		fread(bytes.data(), file_size, 1, file);

		fclose(file);
	}

	return bytes;
}

struct ShaderLoadData {
	Vector<b8> stages[2];
};

static AssetLoadResult shader_load(const AssetLoadContext &ctx)
{
	String file_path = ctx.system_file_path();
	
	Vector<String> paths;

	bool failed_to_load = false;

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

	ShaderLoadData *load_data = new ShaderLoadData();
	
	for (int i = 0; i < paths.size(); i++)
		load_data->stages[i] = load_file_bytes(paths[i]);
	
	AssetLoadResult result = {};
	result.data = load_data;
	result.stage_size = 0; // We don't need a gpu staging buffer for loading shaders.
	result.failed = failed_to_load;

	return result;
}

static Asset *shader_finalize(
	const AssetLoadContext &ctx, const AssetLoadResult &load,
	gfx::Device &device, gfx::CommandBuffer &cmd,
	gfx::GpuBuffer *stage, u64 stage_base
)
{
	ShaderLoadData *load_data = (ShaderLoadData *)load.data;

	Vector<gfx::ShaderBytecode> stages;

	for (auto &stage : load_data->stages) {
		gfx::ShaderBytecode bytecode = {};
		bytecode.bytes = stage.data();
		bytecode.size = stage.size();

		stages.push_back(bytecode);
	}

	gfx::ShaderProgram *gfx_shader = device.create_shader_program(stages);

	return new ShaderAsset(gfx_shader, device);
}

static void shader_clean_up(void *data)
{
	ShaderLoadData *load_data = (ShaderLoadData *)data;
	delete load_data;
}

AssetSerializer ast::get_shader_serializer()
{
	AssetSerializer shader_serializer = {};
	shader_serializer.load = shader_load;
	shader_serializer.finalize = shader_finalize;
	shader_serializer.clean_up = shader_clean_up;

	return shader_serializer;
}
