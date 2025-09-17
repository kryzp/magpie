
typedef enum GBufferAttachment {
	GBufferAttachment_Position,
	GBufferAttachment_Albedo,
	GBufferAttachment_Normal,
	GBufferAttachment_MetallicRoughness,
	GBufferAttachment_Emissive,
	GBufferAttachment_MaxEnum
} GBufferAttachment;

// Geometry Buffer for deferred rendering.
typedef struct GBuffer {
	Image attachments[GBufferAttachment_MaxEnum];
	Image depth;
	ImageView *views[GBufferAttachment_MaxEnum];
	ImageView *depth_view;
} GBuffer;
