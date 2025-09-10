
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

// Ready mesh data for rendering.
// TODO: These should be arrays instead of linked lists
//       given how many times they're indexed into.
typedef struct MeshPass {
	MultiBatch *multi_batches; // Instanced Draws.
	u32 batch_count;
	IndirectBatch *batches;    // Indirect Draws.
	u32 direct_batch_count;
	DirectBatch *direct_batches; // Direct Draws. TODO: This still needs to be actually sorted!
} MeshPass;
