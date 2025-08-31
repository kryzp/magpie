
/*
typedef enum TransparencyMode
{
	TransparencyMode_Opaque,
	//TransparencyMode_Transparent,
	TransparencyMode_MaxEnum
}
TransparencyMode;

typedef struct MaterialShader
{
	ShaderProgram passes[MeshPassType_MaxEnum];
	TransparencyMode transparency;
}
MaterialShader;
*/

typedef struct Material {
	//MaterialShader *shader;

	u32 diffuse_texture_handle;
	//v3 diffuse_factor;

	u32 normal_texture_handle;
	//f32 normal_factor;

	u32 emissive_texture_handle;
	//v3 emissive_factor;

	u32 metallic_roughness_texture_handle;
	//f32 metallic_factor;
	//f32 roughness_factor;

	u32 ambient_texture_handle;
	//f32 ambient_factor;
} Material;

internal b32 MaterialsEqual(Material *a, Material *b)
{
	return (a->diffuse_texture_handle == b->diffuse_texture_handle &&
		a->normal_texture_handle == b->normal_texture_handle &&
		a->emissive_texture_handle == b->emissive_texture_handle &&
		a->metallic_roughness_texture_handle ==
			b->metallic_roughness_texture_handle &&
		a->ambient_texture_handle == b->ambient_texture_handle);
}

typedef struct Mesh {
	VertexFormat *vertex_format;

	GPUBuffer vertex_buffer;
	GPUBuffer index_buffer;

	u32 vertex_count;
	u32 index_count;
} Mesh;

internal b32 MeshesEqual(Mesh *a, Mesh *b)
{
	return (a->vertex_format == b->vertex_format &&
		a->vertex_buffer.handle == b->vertex_buffer.handle &&
		a->index_buffer.handle == b->index_buffer.handle &&
		a->vertex_count == b->vertex_count &&
		a->index_count == b->index_count);
}

typedef struct Model Model;

typedef struct SubModel {
	struct SubModel *next;

	Model *parent;
	Mesh mesh;
	Material material;
} SubModel;

typedef struct Model {
	MemoryArena *arena;

	u32 sub_model_count;
	SubModel *sub_models;
} Model;
