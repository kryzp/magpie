
/*
 * Shaders are defined by using X-Macros.
 * I honestly don't know why more people don't use them.
 */

internal void ShadersInit(Shaders *s, MemoryArena *arena)
{
#define GraphicsShaderDef(__path_vert, __path_frag, __field)		\
	{								\
		String8 files[] = { str8(__path_vert), str8(__path_frag) }; \
		s->##__field = ShaderProgramInit(arena, 2, files);	\
	}
	
#define ComputeShaderDef(__path_comp, __field)			   \
	{							   \
		String8 file = str8(__path_comp);		   \
		s->##__field = ShaderProgramInit(arena, 1, &file); \
	}
	
#include "shaders.inc"

#undef GraphicsShaderDef
#undef ComputeShaderDef

	DebugLog("Loaded shaders.");
}

internal void ShadersDestroy(Shaders *s)
{
#define GraphicsShaderDef(__path_vert, __path_frag, __field) ShaderProgramDestroy(&s->##__field);
#define ComputeShaderDef(__path_comp, __field) ShaderProgramDestroy(&s->##__field);
#include "shaders.inc."
#undef GraphicsShaderDef
#undef ComputeShaderDef
}
