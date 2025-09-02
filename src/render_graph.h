
typedef struct RenderingAttachment {
	VkRenderingAttachmentInfo info;
	Image *image;
	u32 width;
	u32 height;
} RenderingAttachment;

typedef enum RenderPassType {
	RenderPassType_Graphics,
	RenderPassType_Compute,
	RenderPassType_Mipmap
} RenderPassType;

typedef struct RenderState RenderState;

typedef struct RenderPass {
	RenderPassType type;

	// Generic data that can be set depending on whatever is required
	// by the pass. This is then given to the Record(...) function
	// as the "context".
	u8 context[128];

	union {
		struct {
			void (*Record)(RenderState *rs, RenderInfo *render_info, void *context);

			u32 view_mask;

			u32 attachment_count;
			RenderingAttachment attachments[8];

			u32 view_count;
			ImageView *views[16];
		} graphics;

		struct {
			void (*Record)(RenderState *rs, void *context);

			u32 view_count;
			ImageView *views[16];
		} compute;

		struct {
			Image *image;
		} mipmap;
	};
} RenderPass;

typedef struct RenderGraph {
	u32 pass_count;
	RenderPass passes[32];
} RenderGraph;
