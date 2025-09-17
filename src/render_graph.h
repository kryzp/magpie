
typedef struct RenderingAttachment {
	VkRenderingAttachmentInfo info;
	ImageView *view;
	u32 width;
	u32 height;
} RenderingAttachment;

typedef enum RenderPassType {
	RenderPassType_Graphics,
	RenderPassType_Compute,
	RenderPassType_Transfer,
	RenderPassType_Mipmap,
	RenderPassType_Present,
	RenderPassType_MaxEnum
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

			// TODO: In the future when adding vertex buffers just make a seperate list
			u32 buffer_count;
			GPUBuffer *buffers[16];
		} graphics;

		struct {
			void (*Record)(RenderState *rs, void *context);

			u32 read_only_view_count;
			ImageView *read_only_views[16];

			u32 rw_view_count;
			ImageView *rw_views[16];
			
			u32 buffer_count;
			GPUBuffer *buffers[16];
		} compute;

		struct {
			void (*Record)(RenderState *rs, void *context);

			u32 src_count;
			ImageView *src[16];

			u32 dst_count;
			ImageView *dst[16];
		} transfer;

		struct {
			Image *image;
		} mipmap;
		
		struct {
			Image *swapchain;
		} present;
	};
} RenderPass;

typedef struct RenderGraph {
	u32 pass_count;
	RenderPass passes[32];
} RenderGraph;
