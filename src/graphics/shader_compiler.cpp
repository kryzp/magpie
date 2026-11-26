#include "graphics/shader_compiler.h"

#if MAGPIE_HAS_SLANG
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#endif

#include <cstring>

static gfx::ShaderCompiler *selected_compiler = nullptr;

gfx::ShaderCompiler *gfx::ShaderCompiler::Get()
{
	return selected_compiler;
}

gfx::ShaderCompiler::ShaderCompiler(LogChannel log_channel_)
	: log_channel(log_channel_)
	, global_session(nullptr)
{
#if MAGPIE_HAS_SLANG
	slang::createGlobalSession(&global_session);
#endif

	selected_compiler = this;

	if (global_session)
		DebugLogI(log_channel, "Initialized.");
	else
		DebugLogE(log_channel, "Failed to initialize.");
}

gfx::ShaderCompiler::~ShaderCompiler()
{
	DebugLogI(log_channel, "Shutting down...");

#if MAGPIE_HAS_SLANG
	if (global_session)
		global_session->release();
#endif

	global_session = nullptr;

	if (selected_compiler == this)
		selected_compiler = nullptr;
}

void gfx::ShaderCompiler::LogDiagnostics(const char *context, const char *source, ISlangBlob *diag)
{
#if MAGPIE_HAS_SLANG
	if (!diag)
		return;

	const char *msg = static_cast<const char *>(diag->getBufferPointer());

	if (!msg || !msg[0])
		return;

	u64 len = strlen(msg);

	char fmt[1024] = {};
	u64 capped_len = (len > sizeof(fmt) - 1) ? sizeof(fmt) - 1 : len;
	memcpy(fmt, msg, capped_len);

	if (capped_len >= 2)
		fmt[capped_len - 2] = 0; // cut off the newline
	else
		fmt[0] = 0;

	DebugLogW(log_channel, "%s --> %s: %s", source, context, fmt);
#else
	(void)context;
	(void)source;
	(void)diag;
#endif
}

gfx::ShaderCompiledStages gfx::ShaderCompiler::Compile(
	const String &source_path,
	const Vector<String> &search_paths
)
{
	job::FiberMutexScope lock(mutex);

	ShaderCompiledStages compiled;
	compiled.failed = true;

	DebugLogAssert(log_channel, global_session, "Invalid global session.");

#if MAGPIE_HAS_SLANG
	const char *source = source_path.c_str();

	Vector<const char *> search_path_cstrs;

	for (const String &path : search_paths) {
		search_path_cstrs.push_back(path.c_str());
	}

	slang::TargetDesc target_desc = {};
	target_desc.format = SLANG_SPIRV;
	target_desc.profile = global_session->findProfile("spirv_1_5");

	slang::CompilerOptionEntry options[4] = {};
	{
		options[0].name = slang::CompilerOptionName::EmitSpirvDirectly;
		options[0].value.kind = slang::CompilerOptionValueKind::Int;
		options[0].value.intValue0 = 1;

		options[1].name = slang::CompilerOptionName::MatrixLayoutColumn;
		options[1].value.kind = slang::CompilerOptionValueKind::Int;
		options[1].value.intValue0 = 1;

		options[2].name = slang::CompilerOptionName::DisableWarning;
		options[2].value.kind = slang::CompilerOptionValueKind::String;
		options[2].value.stringValue0 = "41012"; // profile implicitly upgraded (there's a bunch of really basic functionality that everyone has but slang tells me it's implicitly upgrading because icba to list them all out)

		options[3].name = slang::CompilerOptionName::DisableWarning;
		options[3].value.kind = slang::CompilerOptionValueKind::String;
		options[3].value.stringValue0 = "39001"; // explicit binding overlaps (basically stop slang from throwing a hissy fit when I try to do aliasing in the bindless arrays)
	}

	slang::SessionDesc session_desc = {};
	session_desc.targets = &target_desc;
	session_desc.targetCount = 1;
	session_desc.searchPaths = search_path_cstrs.data();
	session_desc.searchPathCount = static_cast<SlangInt>(search_path_cstrs.size());
	session_desc.compilerOptionEntries = options;
	session_desc.compilerOptionEntryCount = ArraySize(options);

	Slang::ComPtr<slang::ISession> session;
	SlangResult sr = global_session->createSession(session_desc, session.writeRef());

	if (SLANG_FAILED(sr)) {
		DebugLogW(log_channel, "%s --> Session: Failed to create Slang session.", source);
		return compiled;
	}

	Slang::ComPtr<slang::IBlob> diagnostics;
	slang::IModule *module = session->loadModule(source, diagnostics.writeRef());

	LogDiagnostics("Loading Module", source, diagnostics);

	if (!module)
		return compiled;

	SlangInt32 entry_point_count = module->getDefinedEntryPointCount();

	if (entry_point_count <= 0) {
		DebugLogW(log_channel, "%s --> EntryPoints: No entry points found.", source);
		return compiled;
	}

	if (entry_point_count > GRAPHICS_SHADER_COMPILER_MAX_STAGES) {
		DebugLogW(log_channel, "%s --> EntryPoints: Too many entry points (max %u).", source, GRAPHICS_SHADER_COMPILER_MAX_STAGES);
		return compiled;
	}

	slang::IComponentType *components[1 + GRAPHICS_SHADER_COMPILER_MAX_STAGES];
	SlangInt component_count = 0;

	components[component_count++] = module;

	Slang::ComPtr<slang::IEntryPoint> entry_point_storage[GRAPHICS_SHADER_COMPILER_MAX_STAGES];

	for (SlangInt32 i = 0; i < entry_point_count; i++) {
		sr = module->getDefinedEntryPoint(i, entry_point_storage[i].writeRef());

		if (SLANG_FAILED(sr)) {
			DebugLogW(log_channel, "%s --> EntryPoint: Failed to get entry point.", source);
			return compiled;
		}

		components[component_count++] = entry_point_storage[i].get();
	}

	diagnostics = nullptr;
	Slang::ComPtr<slang::IComponentType> composed;

	sr = session->createCompositeComponentType(
		components,
		component_count,
		composed.writeRef(),
		diagnostics.writeRef()
	);

	LogDiagnostics("Composing", source, diagnostics);

	if (SLANG_FAILED(sr))
		return compiled;

	diagnostics = nullptr;
	Slang::ComPtr<slang::IComponentType> linked;

	sr = composed->link(linked.writeRef(), diagnostics.writeRef());

	LogDiagnostics("Linking", source, diagnostics);

	if (SLANG_FAILED(sr))
		return compiled;

	for (SlangInt32 i = 0; i < entry_point_count; i++) {
		diagnostics = nullptr;
		Slang::ComPtr<slang::IBlob> spirv_blob;

		sr = linked->getEntryPointCode(
			i, 0,
			spirv_blob.writeRef(),
			diagnostics.writeRef()
		);

		LogDiagnostics("Compiling", source, diagnostics);

		if (SLANG_FAILED(sr) || !spirv_blob) {
			compiled.bytecodes.clear();
			return compiled;
		}

		const u8 *code = static_cast<const u8 *>(spirv_blob->getBufferPointer());
		u64 code_size = spirv_blob->getBufferSize();

		ShaderBytecode bytecode;
		bytecode.bytes.assign(code, code + code_size);

		compiled.bytecodes.push_back(bytecode);
	}

	compiled.failed = false;
#else
	DebugLogE(log_channel, "Cannot compile \"%s\": built without Slang.", source_path.c_str());
#endif

	return compiled;
}
