
typedef struct RenderingAttachment {
	VkRenderingAttachmentInfo info;
	Image *image;
	u32 width;
	u32 height;
} RenderingAttachment;

typedef enum RenderPassType {
	RenderPassType_Graphics,
	RenderPassType_Compute
} RenderPassType;

typedef struct RenderContext RenderContext;

typedef struct RenderPass {
	RenderPassType type;

	// NOTE(kp): Generic data that can be set depending on whatever is required
	//           by the pass. This is then given to the Record(...) function
	//           as the "context".
	u8 context[128];

	union {
		struct {
			void (*Record)(RenderContext *render_context,
				       RenderInfo *render_info, void *context);

			u32 view_mask;

			u32 attachment_count;
			RenderingAttachment attachments[8];

			u32 view_count;
			ImageView *views[16];
		} graphics;

		struct ComputePassDef {
			void (*Record)(RenderContext *render_context,
				       void *context);

			u32 view_count;
			ImageView *views[16];
		} compute;
	};
} RenderPass;

typedef struct RenderGraph {
	u32 pass_count;
	RenderPass passes[32];
} RenderGraph;
