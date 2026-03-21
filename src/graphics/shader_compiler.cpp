#include "shader_compiler.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

using namespace gfx;

static SlangStage vk_to_slang_stage(VkShaderStageFlags stage)
{
	switch (stage) {
		case VK_SHADER_STAGE_VERTEX_BIT:    return SLANG_STAGE_VERTEX;
		case VK_SHADER_STAGE_FRAGMENT_BIT:  return SLANG_STAGE_FRAGMENT;
		case VK_SHADER_STAGE_COMPUTE_BIT:   return SLANG_STAGE_COMPUTE;
		default:                            return SLANG_STAGE_NONE;
	}
}

class SlangCompiler : public IShaderCompiler {
public:
	void start() override
	{
		/*
		slang::createGlobalSession(global_session.writeRef());

		slang::TargetDesc targets[1] = {};
		targets[0].format = SLANG_SPIRV;
		targets[1].profile = global_session->findProfile("glsl_460");

		const char *search_paths[] = { "shaders/" };

		slang::SessionDesc session_desc = {};
		session_desc.searchPaths = search_paths;
		session_desc.searchPathCount = array_size(search_paths);
		
		session_desc.targets = targets;
		session_desc.targetCount = array_size(targets);

		global_session->createSession(session_desc, &session);
		*/
	}

	void shutdown() override
	{
		/*
		global_session.setNull();
		*/
	}

	CompiledShaderStage compile(const char *path, const char *entry_point, VkShaderStageFlags stage_type) override
	{
		CompiledShaderStage compiled = {};
		
		/*
		compiled.failed = true;
		compiled.push_constant_size = 0;
		compiled.stage = stage_type;
		*/

		return compiled;
	}

private:
	Slang::ComPtr<slang::IGlobalSession> global_session;
};

IShaderCompiler *gfx::get_shader_compiler()
{
	static SlangCompiler slang_compiler;
	return &slang_compiler;
}
