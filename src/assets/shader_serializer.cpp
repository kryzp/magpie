#include "shader_serializer.h"

#include "core/hash.h"
#include "platform/platform.h"

using namespace ast;

static Vector<u8> load_file_bytes(const String &path)
{
	Vector<u8> bytes;

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
	gfx::ShaderKind kind;
	int stage_count;
	Vector<u8> stages[2];
};

static AssetLoadResult shader_load(const AssetLoadContext &ctx)
{
	String file_path = ctx.system_file_path();
	
	ShaderLoadData *load_data = new ShaderLoadData();
	load_data->kind = gfx::SHADER_KIND_UNKNOWN;
	load_data->stage_count = 0;
	
	AssetLoadResult result = {};
	result.data = load_data;
	result.stage_size = 0; // We don't need a staging buffer for shaders.
	result.failed = false;

	FILE *file = fopen(file_path.c_str(), "r");

	if (!file) {
		result.failed = true;
		return result;
	}

	char line[256] = {};
	Vector<String> spv_paths;

	while (fgets(line, sizeof(line), file)) {
		String line_str(line);

		line_str.erase(std::remove(line_str.begin(), line_str.end(), '\n'), line_str.end());
		line_str.erase(std::remove(line_str.begin(), line_str.end(), '\r'), line_str.end());

		if (line_str.empty() || line_str[0] == '#')
			continue;

		switch (hash::c_str(line_str.c_str())) {
			case hash::c_str("Graphics"):
				load_data->kind = gfx::SHADER_KIND_GRAPHICS;
				break;

			case hash::c_str("Compute"):
				load_data->kind = gfx::SHADER_KIND_COMPUTE;
				break;

			default:
				// Find delimeter ':'
				u64 colon_index = line_str.find(':');

				if (colon_index != String::npos) {
					String path = line_str.substr(colon_index + 1);

					// Get rid of the leading spaces.
					u64 first_char_idx = path.find_first_not_of(' ');
					if (first_char_idx != String::npos)
						path = path.substr(first_char_idx);

					// Resolve relative path.
					String absolute_spv_path = ctx.assets.get_system_file_path(path);
					spv_paths.push_back(absolute_spv_path);
				}

				break;
		}
	}

	fclose(file);

	load_data->stage_count = spv_paths.size();

	if (load_data->stage_count == 0)
		result.failed = true;
	
	for (int i = 0; i < load_data->stage_count; i++) {
		load_data->stages[i] = load_file_bytes(spv_paths[i]);
		if (load_data->stages[i].empty())
			result.failed = true;
	}

	return result;
}

static Asset *shader_asset_allocate(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::Device &device
)
{
	ShaderLoadData *load_data = (ShaderLoadData *)result.data;

	Vector<gfx::ShaderBytecode> stages;

	for (int i = 0; i < load_data->stage_count; i++) {
		Vector<u8> &stage = load_data->stages[i];

		gfx::ShaderBytecode bytecode = {};
		bytecode.bytes = stage.data();
		bytecode.size = stage.size();

		stages.push_back(bytecode);
	}

	gfx::ShaderProgram *gfx_shader = device.create_shader_program(stages);
	return new ShaderAsset(gfx_shader, device);
}

static void shader_upload(
	Asset *asset,
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::CommandBuffer &cmd,
	gfx::GpuBuffer *stage, u64 stage_base
)
{
	// We don't need to do any uploading for shaders.
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
	shader_serializer.allocate = shader_asset_allocate;
	shader_serializer.upload = shader_upload;
	shader_serializer.clean_up = shader_clean_up;

	return shader_serializer;
}
