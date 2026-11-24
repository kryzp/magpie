#include "asset/asset_loader.h"
#include "asset/asset_system.h"
#include "graphics/device.h"
#include "graphics/shader_compiler.h"

struct ShaderLoadData : ast::LoadData {
	gfx::ShaderCompiledStages compiled;
};

const Vector<String> &ast::ShaderLoader::GetFileExtensions() const
{
	static const Vector<String> file_extensions = {
		"slang"
	};

	return file_extensions;
}

ast::LoadResult ast::ShaderLoader::Load(const LoaderContext &ctx)
{
	AssetSystem *assets = AssetSystem::Get();

	String file_path = assets->GetSystemFilePath(ctx.metadata.path);

	u64 shader_index = file_path.find("shaders");
	u64 prefix_length = 0;

	if (shader_index != String::npos)
		prefix_length = shader_index;

	Vector<String> search_paths;
	search_paths.push_back(file_path.substr(0, prefix_length) + "shaders/modules/");

	for (const String &path : assets->GetShaderSearchPaths())
		search_paths.push_back(path);

	ShaderLoadData *load_data = new ShaderLoadData();
	load_data->compiled = gfx::ShaderCompiler::Get()->Compile(file_path, search_paths);

	LoadResult result;
	result.user_data = load_data;
	result.stage_size = 0;
	result.failed = load_data->compiled.failed;

	return result;
}

void ast::ShaderLoader::Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
{
	ShaderLoadData *load_data = (ShaderLoadData *)result->user_data;
	gfx::ShaderCompiledStages *compiled = &load_data->compiled;

	asset->shader.key = gfx::Device::Get()->ShaderProgramCreate((unsigned)compiled->bytecodes.size(), compiled->bytecodes.data());
}

void ast::ShaderLoader::DestroyAsset(Asset *asset)
{
	gfx::Device::Get()->ShaderProgramDestroy(asset->shader.key);
}
