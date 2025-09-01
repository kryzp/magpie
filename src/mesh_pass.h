
typedef struct MultiBatch {
	struct MultiBatch *next;

	u32 first;
	u32 count;
} MultiBatch;

typedef struct IndirectBatch {
	struct IndirectBatch *next;

	u32 mesh_id;
	u32 material_id;
	u32 first;
	u32 count;
} IndirectBatch;

typedef struct DirectBatch {
	struct DirectBatch *next;

	u32 object_id;
	//u64 sort_key;
} DirectBatch;

/*
typedef enum MeshPassType
{
	MeshPassType_Deferred,
	//MeshPassType_Shadow,
	//MeshPassType_Transparency,
	MeshPassType_MaxEnum
}
MeshPassType;
*/

// Ready mesh data for rendering.
typedef struct MeshPass {
	// Instanced Draws.
	MultiBatch *multi_batches;

	// Indirect Draws.
	IndirectBatch *batches;

	// Direct draws.
	// TODO: This still needs to be actually sorted!
	DirectBatch *direct_batches;
} MeshPass;
