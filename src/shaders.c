
internal void ShadersInit(Shaders *s, MemoryArena *arena)
{
	struct {
		String8 vert;
		String8 frag;
		ShaderProgram *target;
	} graphics_shaders[] = {
		{ str8("res/brdf_lut_vertex.spv"),                   str8("res/brdf_lut_fragment.spv"),                   &s->brdf_lut_program },
		{ str8("res/hdr_to_environment_cubemap_vertex.spv"), str8("res/hdr_to_environment_cubemap_fragment.spv"), &s->hdr_to_environment_cubemap_program },
		{ str8("res/irradiance_convolution_vertex.spv"),     str8("res/irradiance_convolution_fragment.spv"),     &s->irradiance_map_program },
		{ str8("res/prefilter_convolution_vertex.spv"),      str8("res/prefilter_convolution_fragment.spv"),      &s->prefilter_map_program },
		{ str8("res/model_vertex.spv"),                      str8("res/model_fragment.spv"),                      &s->model_program },
		{ str8("res/skybox_vertex.spv"),                     str8("res/skybox_fragment.spv"),                     &s->skybox_program },
		{ str8("res/ambient_lighting_vertex.spv"),           str8("res/ambient_lighting_fragment.spv"),           &s->ambient_lighting_program },
		{ str8("res/direct_lighting_point_vertex.spv"),      str8("res/direct_lighting_point_fragment.spv"),      &s->direct_lighting_point_program },
	};

	for (i32 i = 0; i < ArraySize(graphics_shaders); i++) {
		String8 files[] = { graphics_shaders[i].vert, graphics_shaders[i].frag };
		*graphics_shaders[i].target = ShaderProgramInit(arena, ArraySize(files), files);
	}
}

internal void ShadersDestroy(Shaders *s)
{
	ShaderProgramDestroy(&s->ambient_lighting_program);
	ShaderProgramDestroy(&s->direct_lighting_point_program);
	ShaderProgramDestroy(&s->model_program);
	ShaderProgramDestroy(&s->hdr_to_environment_cubemap_program);
	ShaderProgramDestroy(&s->irradiance_map_program);
	ShaderProgramDestroy(&s->prefilter_map_program);
	ShaderProgramDestroy(&s->skybox_program);
	ShaderProgramDestroy(&s->brdf_lut_program);
}
