
typedef struct Shaders {
	ShaderProgram ambient_lighting_program;
	ShaderProgram direct_lighting_point_program;
	ShaderProgram model_program;
	ShaderProgram hdr_to_environment_cubemap_program;
	ShaderProgram irradiance_map_program;
	ShaderProgram prefilter_map_program;
	ShaderProgram skybox_program;
	ShaderProgram brdf_lut_program;
} Shaders;
