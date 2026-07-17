#include "slang_compiler.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

#include <stdlib.h>
#include <string.h>

static void
SLANG_LogDiagnostics(SLANG_LogFn log_fn,
					 const char *context,
					 const char *source,
					 slang::IBlob *diag,
					 void *user_data)
{
	if (!diag || !log_fn)
		return;

	const char *msg = (const char *)diag->getBufferPointer();

	if (!msg)
		return;

	if (!msg[0])
		return;

	size_t len = strlen(msg);

	char fmt[1024] = {0};
	size_t capped_len = (len > sizeof(fmt)) ? sizeof(fmt) : len;
	memcpy(fmt, msg, capped_len);
	fmt[capped_len-2] = 0; // cut off the newline
		
	log_fn(context, source, fmt, user_data);
}

extern "C" void
SLANG_Init(void **out_global_session)
{
	slang::IGlobalSession *gs = nullptr;
	slang::createGlobalSession(&gs);
	*out_global_session = (void *)gs;
}

extern "C" void
SLANG_Shutdown(void *global_session)
{
	if (global_session)
	{
		slang::IGlobalSession *gs = (slang::IGlobalSession *)global_session;
		gs->release();
	}
}

extern "C" SLANG_CompileResult
SLANG_Compile(void *global_session,
			  const char *source_path,
			  uint32_t search_path_count,
			  const char *const *search_paths,
			  SLANG_LogFn *log_fn, void *user_data)
{
	SLANG_CompileResult result = {};
	result.failed = 1;

	slang::IGlobalSession *gs = (slang::IGlobalSession *)global_session;

	if (!gs)
		return result;

	slang::TargetDesc target_desc = {};
	target_desc.format = SLANG_SPIRV;
	target_desc.profile = gs->findProfile("spirv_1_5");
	target_desc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

	slang::CompilerOptionEntry options[2] = {};
	options[0].name = slang::CompilerOptionName::EmitSpirvDirectly;
	options[0].value.intValue0 = 1;
	options[1].name = slang::CompilerOptionName::MatrixLayoutColumn;
	options[1].value.intValue0 = 1;

	slang::SessionDesc session_desc = {};
	session_desc.targets = &target_desc;
	session_desc.targetCount = 1;
	session_desc.searchPaths = search_paths;
	session_desc.searchPathCount = (SlangInt)search_path_count;
	session_desc.compilerOptionEntries = options;
	session_desc.compilerOptionEntryCount = 2;

	Slang::ComPtr<slang::ISession> session;
	SlangResult sr = gs->createSession(session_desc, session.writeRef());

	if (SLANG_FAILED(sr))
	{
		if (log_fn)
			log_fn("Session", source_path, "Failed to create Slang session.", user_data);

		return result;
	}

	Slang::ComPtr<slang::IBlob> diagnostics;
	slang::IModule *module = session->loadModule(source_path, diagnostics.writeRef());

	SLANG_LogDiagnostics(log_fn, "Loading Module", source_path, diagnostics, user_data);

	if (!module)
		return result;

	SlangInt32 entry_point_count = module->getDefinedEntryPointCount();

	if (entry_point_count <= 0)
	{
		if (log_fn)
			log_fn("EntryPoints", source_path, "No entry points found.", user_data);

		return result;
	}

	if (entry_point_count > SLANG_MAX_STAGES)
	{
		if (log_fn)
		{
			char fmt[512];
			snprintf(fmt, sizeof(fmt), "Too many entry points (max %u).", SLANG_MAX_STAGES);
			log_fn("EntryPoints", source_path, fmt, user_data);
		}

		return result;
	}

	slang::IComponentType *components[1 + SLANG_MAX_STAGES];
	SlangInt component_count = 0;

	components[component_count++] = module;

	Slang::ComPtr<slang::IEntryPoint> entry_point_storage[SLANG_MAX_STAGES];

	for (SlangInt32 i = 0; i < entry_point_count; i++)
	{
		sr = module->getDefinedEntryPoint(i, entry_point_storage[i].writeRef());

		if (SLANG_FAILED(sr))
		{
			if (log_fn)
				log_fn("EntryPoint", source_path, "Failed to get entry point.", user_data);

			return result;
		}

		components[component_count++] = entry_point_storage[i].get();
	}

	diagnostics = nullptr;
	Slang::ComPtr<slang::IComponentType> composed;

	sr = session->createCompositeComponentType(components,
											   component_count,
											   composed.writeRef(),
											   diagnostics.writeRef());

	SLANG_LogDiagnostics(log_fn, "Composing", source_path, diagnostics, user_data);

	if (SLANG_FAILED(sr))
		return result;

	diagnostics = nullptr;
	Slang::ComPtr<slang::IComponentType> linked;

	sr = composed->link(linked.writeRef(), diagnostics.writeRef());

	SLANG_LogDiagnostics(log_fn, "Linking", source_path, diagnostics, user_data);

	if (SLANG_FAILED(sr))
		return result;

	for (SlangInt32 i = 0; i < entry_point_count; i++)
	{
		diagnostics = nullptr;
		Slang::ComPtr<slang::IBlob> spirv_blob;

		sr = linked->getEntryPointCode((SlangInt)i, 0,
									   spirv_blob.writeRef(),
									   diagnostics.writeRef());

		SLANG_LogDiagnostics(log_fn, "Compiling", source_path, diagnostics, user_data);

		if (SLANG_FAILED(sr) || !spirv_blob)
			return result;

		uint64_t code_size = spirv_blob->getBufferSize();
		void *code_copy = malloc(code_size);

		if (!code_copy)
			return result;

		memcpy(code_copy, spirv_blob->getBufferPointer(), code_size);

		result.stages[i].bytes = code_copy;
		result.stages[i].size = code_size;
	}

	result.stage_count = entry_point_count;
	result.failed = 0;

	return result;
}

extern "C" void
SLANG_FreeResult(SLANG_CompileResult *result)
{
	if (!result)
		return;

	for (uint32_t i = 0; i < result->stage_count; i++)
	{
		free(result->stages[i].bytes);
		result->stages[i].bytes = NULL;
	}
}
