#include "shader_serializer.h"

#include "graphics/shader_compiler.h"

using namespace ast;

struct ShaderLoadData {
	gfx::CompiledShaderProgram compiled;
};

class ShaderSerializer : public IAssetSerializer {
public:
	AssetLoadResult load(const AssetLoadContext &ctx) override;
	Asset *finalize(const AssetLoadContext &ctx, const AssetLoadResult &result, Asset *existing_asset, gfx::Device &device) override;
};

AssetLoadResult ShaderSerializer::load(const AssetLoadContext &ctx)
{
	String file_path = ctx.system_file_path();

	ShaderLoadData *load_data = ctx.arena.push<ShaderLoadData>();

	AssetLoadResult result = {};
	result.data = load_data;
	result.stage_size = 0; // We don't need a staging buffer for shaders.
	result.failed = false;

	// Determine the module search path.
	// Convention: modules live in "shaders/modules/" relative to the shader file's root.
	// We find the "shaders/" part of the path and add "shaders/modules/" as a search path.
	String search_path;

	u64 shaders_index = file_path.find("shaders");
	if (shaders_index != String::npos) {
		search_path = file_path.substr(0, shaders_index) + "shaders/modules/";
	} else {
		// Fallback: use same directory as the shader file.
		u64 last_slash = file_path.find_last_of("/\\");
		if (last_slash != String::npos)
			search_path = file_path.substr(0, last_slash + 1);
		else
			search_path = "./";
	}

	Vector<String> search_paths;
	search_paths.push_back(search_path);

	// Also add the passes directory for potential cross-imports.
	if (shaders_index != String::npos) {
		String passes_path = file_path.substr(0, shaders_index) + "shaders/passes/";
		search_paths.push_back(passes_path);
	}

	// Compile the .slang source file.
	gfx::IShaderCompiler *compiler = gfx::get_shader_compiler();
	load_data->compiled = compiler->compile(file_path.c_str(), search_paths);

	if (load_data->compiled.failed) {
		result.failed = true;
		return result;
	}

	result.watch_paths.push_back(file_path); // Watch the slang file.
	result.watch_paths.push_back(search_path); // Watch the modules directory (dependencies).

	return result;
}

Asset *ShaderSerializer::finalize(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	Asset *existing_asset,
	gfx::Device &device
)
{
	ShaderLoadData *load_data = (ShaderLoadData *)result.data;

	Vector<gfx::ShaderBytecode> bytecodes;

	for (auto &stage : load_data->compiled.stages)
		bytecodes.push_back(stage.bytecode);

	// Use the max push constant size from all stages.
	// TODO: Could this be done better?
	u32 push_constant_size = 0;
	for (auto &stage : load_data->compiled.stages) {
		if (stage.push_constant_size > push_constant_size)
			push_constant_size = stage.push_constant_size;
	}

	gfx::ShaderProgram *new_shader = device.create_shader_program(bytecodes);

	// Override with the Slang-reflected push constant size if it's larger.
	if (push_constant_size > new_shader->get_push_constant_size())
		debug_log_crash("fuck");

	if (existing_asset) {
		ShaderAsset *shader_asset = existing_asset->as<ShaderAsset>();
		device.destroy_shader_program(shader_asset->shader);
		shader_asset->shader = new_shader;
		return shader_asset;
	}

	return ctx.arena.push<ShaderAsset>(new_shader, device);
}

IAssetSerializer *ast::get_shader_serializer()
{
	static ShaderSerializer shader_serializer;
	return &shader_serializer;
}
