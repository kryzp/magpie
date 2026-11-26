#include "asset/asset_loader.h"
#include "asset/asset_system.h"
#include "os/file_system.h"
#include "script/system.h"

struct ScriptLoadData : ast::LoadData {
	scr::Ref ref;
};

const Vector<String> &ast::ScriptLoader::GetFileExtensions() const
{
	static const Vector<String> file_extensions = {
		"lua"
	};

	return file_extensions;
}

ast::LoadResult ast::ScriptLoader::Load(const LoaderContext &ctx)
{
	String file_path = AssetSystem::Get()->GetSystemFilePath(ctx.metadata.path);

	ScriptLoadData *script = new ScriptLoadData();

	scr::System *scripting = scr::System::Get();

	Vector<u8> source_bytes;

	if (fs::ReadEntireFile(file_path, &source_bytes)) {
		scr::Ref chunk_ref = scripting->Compile(source_bytes.data(), source_bytes.size(), file_path);

		if (!chunk_ref.IsNull()) {
			script->ref = scripting->ExecuteModule(chunk_ref);
			scripting->Release(chunk_ref);
		}
	}

	LoadResult result;
	result.user_data = script;
	result.stage_size = 0;
	result.failed = false;

	return result;
}

void ast::ScriptLoader::Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
{
	ScriptLoadData *script = static_cast<ScriptLoadData *>(result->user_data);
	asset->script.ref = script->ref;
}
