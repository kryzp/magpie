
typedef struct ModelVertex {
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
} ModelVertex;

typedef struct VertexFormats {
	VertexFormat vec3;
	VertexFormat model;
} VertexFormats;
