
typedef struct Mesh
{
	VertexFormat *vertex_format;
	
	GPUBuffer vertex_buffer;
	GPUBuffer index_buffer;
	
	u32 vertex_count;
	u32 index_count;
}
Mesh;

typedef struct Material
{
	u32 diffuse;
	u32 normal;
	u32 emissive;
	u32 mr;
	u32 ambient;
}
Material;

typedef struct Model Model;

typedef struct SubModel
{
	struct SubModel *next;
	
	Model *parent;
	Mesh mesh;
	Material material;
}
SubModel;

typedef struct ModelVertex
{
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
}
ModelVertex;

typedef struct Model
{
	MemoryArena *arena;
	
	String8 directory;
	
	u32 sub_model_count;
	SubModel *sub_models;
}
Model;
