
// TODO: Move elsewhere.
internal String8 LoadFileBytes(MemoryArena *dst, String8 path)
{
	b8 *bytes = NULL;

	FILE *file = fopen((char *)path.str, "rb");
	u64 file_size = 0;

	if (file) {
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes = MemoryArenaPush(dst, file_size);
		fread(bytes, file_size, 1, file);

		fclose(file);
	}

	return String8Init(bytes, file_size);
}

internal u32 ShaderStageAlignUp(u32 value, u32 alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

internal ShaderStage ShaderStageLoadFromBytecode(MemoryArena *arena,
						 String8 path,
						 u32 *push_constant_size)
{
	ScratchArena scratch = GetScratch(arena, 1);

	String8 source = LoadFileBytes(scratch.arena, path);

	SpvReflectShaderModule reflect_module = {0};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(source.len, source.str, &reflect_module);

	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
		DebugLogCrash("Failed to reflect SPIR-V module: %d\n", reflect_result);

	ShaderStage stage = {0};

	if (reflect_module.entry_point_count >= 1) {
		stage.stage = (VkShaderStageFlags)reflect_module.entry_points[0].shader_stage;
	} else {
		spvReflectDestroyShaderModule(&reflect_module);
		ReleaseScratch(&scratch);
		DebugLogCrash("No entry points found in SPIR-V.\n");
		return stage;
	}

	if (push_constant_size) {
		u32 push_constant_count = 0;
		reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, 0);

		if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0) {
			SpvReflectBlockVariable **pcs = MemoryArenaPushC(scratch.arena,
									 push_constant_count,
									 sizeof(SpvReflectBlockVariable *));

			spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs);

			for (u32 i = 0; i < push_constant_count; i++) {
				SpvReflectBlockVariable *pc = pcs[i];

				u32 alignment = 4;
				
				for (u32 j = 0; j < pc->member_count; j++)
					alignment = MaxValue(alignment, pc->members[j].size);

				u32 padded = ShaderStageAlignUp(pc->size, alignment);
				*push_constant_size = MaxValue(*push_constant_size, padded);
			}
		}
	}

	VkShaderModuleCreateInfo module_create_info = {0};
	module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = source.len;
	module_create_info.pCode = (const u32 *)source.str;

	VK_CHECK(vkCreateShaderModule(graphics_device->device,
				      &module_create_info, 0, &stage.module),
		 "Failed to create shader module.");

	spvReflectDestroyShaderModule(&reflect_module);
	ReleaseScratch(&scratch);

	return stage;
}

internal void ShaderStageDestroy(ShaderStage *stage)
{
	vkDestroyShaderModule(graphics_device->device, stage->module, NULL);
	stage->module = VK_NULL_HANDLE;
}

internal ShaderProgram ShaderProgramInit(MemoryArena *arena, u32 stage_count, String8 *stage_paths)
{
	ShaderProgram program = {0};
	program.stage_count = stage_count;

	for (i32 i = 0; i < stage_count; i++) {
		printf("Loading shader stage: %.*s\n", (u32)stage_paths[i].len, stage_paths[i].str);
		program.stages[i] = ShaderStageLoadFromBytecode(arena, stage_paths[i], &program.push_constant_size);
	}
	
	return program;
}

internal void ShaderProgramDestroy(ShaderProgram *program)
{
	for (i32 i = 0; i < program->stage_count; i++)
		ShaderStageDestroy(program->stages + i);
}

internal b32 ShaderProgramIsCompute(ShaderProgram *program)
{
	return program->stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT;
}
