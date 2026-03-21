#include "shader_compiler.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

using namespace gfx;

class SlangCompiler : public IShaderCompiler {
public:
	SlangCompiler();
	~SlangCompiler() override;

	void init() override;
	void shutdown() override;

	CompiledShaderProgram compile(const char *source_path, const Vector<String> &search_paths) override;

private:
	slang::IGlobalSession *global_session;
};

static VkShaderStageFlags slang_stage_to_vk(SlangStage stage)
{
	switch (stage) {
		case SLANG_STAGE_VERTEX:    return VK_SHADER_STAGE_VERTEX_BIT;
		case SLANG_STAGE_FRAGMENT:  return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SLANG_STAGE_COMPUTE:   return VK_SHADER_STAGE_COMPUTE_BIT;
		case SLANG_STAGE_GEOMETRY:  return VK_SHADER_STAGE_GEOMETRY_BIT;

		case SLANG_STAGE_HULL:      return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case SLANG_STAGE_DOMAIN:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		default:                    return 0;
	}
}

SlangCompiler::SlangCompiler()
	: global_session(nullptr)
{
}

SlangCompiler::~SlangCompiler()
{
}

void SlangCompiler::init()
{
	slang::createGlobalSession(&global_session);
	debug_log("Slang shader compiler initialized.");
}

void SlangCompiler::shutdown()
{
	if (global_session) {
		global_session->release();
		global_session = nullptr;
	}

	debug_log("Slang shader compiler shut down.");
}

CompiledShaderProgram SlangCompiler::compile(
	const char *source_path,
	const Vector<String> &search_paths
)
{
	CompiledShaderProgram result = {};
	result.failed = true;

	if (!global_session) {
		debug_log("Shader compiler global session failed to initialize.");
		return result;
	}

	slang::TargetDesc target_desc = {};
	target_desc.format = SLANG_SPIRV;
	target_desc.profile = global_session->findProfile("glsl_460");
	target_desc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

	Vector<const char *> search_path_ptrs;

	for (const auto &sp : search_paths)
		search_path_ptrs.push_back(sp.c_str());

	slang::CompilerOptionEntry options[2] = {};
	options[0].name = slang::CompilerOptionName::EmitSpirvDirectly;
	options[0].value.intValue0 = 1;
	options[1].name = slang::CompilerOptionName::MatrixLayoutColumn;
	options[1].value.intValue0 = 1;

	slang::SessionDesc session_desc = {};
	session_desc.targets = &target_desc;
	session_desc.targetCount = 1;
	session_desc.searchPaths = search_path_ptrs.data();
	session_desc.searchPathCount = search_path_ptrs.size();
	session_desc.compilerOptionEntries = options;
	session_desc.compilerOptionEntryCount = array_size(options);

	Slang::ComPtr<slang::ISession> session;
	SlangResult sr = global_session->createSession(session_desc, session.writeRef());

	if (SLANG_FAILED(sr)) {
		debug_log("Failed to create Slang session for \"%s\".", source_path);
		return result;
	}

	// Load source module.
	Slang::ComPtr<slang::IBlob> diagnostics;
	slang::IModule *module = session->loadModule(source_path, diagnostics.writeRef());

	if (diagnostics) {
		const char *msg = (const char *)diagnostics->getBufferPointer();
		if (msg && msg[0])
			debug_log("Loading Shader Module [%s]: %s", source_path, msg);
	}

	if (!module) {
		debug_log("Failed to load shader module \"%s\".", source_path);
		return result;
	}

	// Search entry points.
	SlangInt entry_point_count = module->getDefinedEntryPointCount();

	if (entry_point_count <= 0) {
		debug_log("No entry points found in shader \"%s\".", source_path);
		return result;
	}

	// "Compose" the entire program (module + entry points).
	Vector<slang::IComponentType *> components;
	components.push_back(module);

	for (SlangInt i = 0; i < entry_point_count; i++) {
		Slang::ComPtr<slang::IEntryPoint> entry_point;
		sr = module->getDefinedEntryPoint(i, entry_point.writeRef());

		if (SLANG_FAILED(sr)) {
			debug_log("ShaderCompiler: Failed to get entry point %d from '%s'.", (int)i, source_path);
			return result;
		}

		components.push_back(entry_point.get());
	}

	Slang::ComPtr<slang::IComponentType> composed_program;
	sr = session->createCompositeComponentType(
		components.data(),
		components.size(),
		composed_program.writeRef(),
		diagnostics.writeRef()
	);

	if (diagnostics) {
		const char *msg = (const char *)diagnostics->getBufferPointer();

		if (msg && msg[0])
			debug_log("Composing Shader [%s]: %s", source_path, msg);
	}

	if (SLANG_FAILED(sr)) {
		debug_log("Failed to compose program for \"%s\".", source_path);
		return result;
	}

	// Link it!!!
	Slang::ComPtr<slang::IComponentType> linked_program;
	sr = composed_program->link(linked_program.writeRef(), diagnostics.writeRef());

	if (diagnostics) {
		const char *msg = (const char *)diagnostics->getBufferPointer();
		if (msg && msg[0])
			debug_log("Linking Shader [%s]: %s", source_path, msg);
	}

	if (SLANG_FAILED(sr)) {
		debug_log("Failed to link program for \"%s\".", source_path);
		return result;
	}

	// Extract the SPIR-V for each entry point of the programs layout.
	slang::ProgramLayout *layout = linked_program->getLayout();

	for (SlangInt i = 0; i < entry_point_count; i++) {
		Slang::ComPtr<slang::IBlob> spirv_code;
		sr = linked_program->getEntryPointCode(i, 0, spirv_code.writeRef(), diagnostics.writeRef());

		if (diagnostics) {
			const char *msg = (const char *)diagnostics->getBufferPointer();

			if (msg && msg[0])
				debug_log("Compiling Shader [%s] entry %d: %s", source_path, (int)i, msg);
		}

		if (SLANG_FAILED(sr) || !spirv_code) {
			debug_log("Failed to get SPIR-V code for entry point %d in \"%s\".", (int)i, source_path);
			return result;
		}

		// Get the entry point reflection to determine the stage.
		slang::EntryPointLayout *ep_layout = layout->getEntryPointByIndex(i);
		SlangStage slang_stage = ep_layout->getStage();

		VkShaderStageFlags vk_stage = slang_stage_to_vk(slang_stage);

		if (vk_stage == 0) {
			debug_log("Unsupported shader stage for entry point %d in \"%s\".", (int)i, source_path);
			return result;
		}

		// Determine push constant size from Slang reflection.
		// Slang encodes push constants as global-scope parameters with the [vk::push_constant] attribute.
		// The size of the global scope parameter block tells us the push constant size.
		u32 push_constant_size = 0;

		// Get global scope parameters size — this is the push constant block.
		slang::TypeLayoutReflection *global_params = layout->getGlobalParamsTypeLayout();
		if (global_params)
			push_constant_size = global_params->getSize();

		// Copy the SPIR-V into our own allocation
		u64 code_size = spirv_code->getBufferSize();
		u8 *code_copy = new u8[code_size];
		memory_copy(code_copy, spirv_code->getBufferPointer(), code_size);

		CompiledShaderStage stage = {};
		stage.bytecode.bytes = code_copy;
		stage.bytecode.size = code_size;
		stage.push_constant_size = push_constant_size;
		stage.stage = vk_stage;

		result.stages.push_back(stage);
	}

	result.failed = false;

//	debug_log("Compiled Shader \"%s\" - %d stage(s).", source_path, (int)entry_point_count);

	return result;
}

IShaderCompiler *gfx::get_shader_compiler()
{
	static SlangCompiler slang_compiler;
	return &slang_compiler;
}
