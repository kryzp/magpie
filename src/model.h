
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

typedef struct Material
{
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
}
Material;

typedef struct Mesh
{
	VertexFormat *vertex_format;
	
	GPUBuffer vertex_buffer;
	GPUBuffer index_buffer;
	
	u32 vertex_count;
	u32 index_count;
}
Mesh;

typedef struct Model Model;

typedef struct SubModel
{
	struct SubModel *next;
	
	Model *parent;
	Mesh mesh;
	Material material;
}
SubModel;

typedef struct Model
{
	MemoryArena *arena;
	
	u32 sub_model_count;
	SubModel *sub_models;
}
Model;
