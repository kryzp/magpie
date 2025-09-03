
typedef enum GBufferAttachment {
	GBufferAttachment_Position,
	GBufferAttachment_Albedo,
	GBufferAttachment_Normal,
	GBufferAttachment_Material,
	GBufferAttachment_Emissive,
	GBufferAttachment_MaxEnum
} GBufferAttachment;

// Geometry Buffer for deferred rendering.
typedef struct GBuffer {
	Image attachments[GBufferAttachment_MaxEnum];
	ImageView *views[GBufferAttachment_MaxEnum];
	Image depth;
	ImageView *depth_view;
} GBuffer;

typedef struct Renderer {
	GBuffer gbuffer;
} Renderer;
