#ifndef RENDER_JOB_H
#define RENDER_JOB_H

#define R_RENDER_JOB_PACKET_ARENA_SIZE    Gigabytes(1)
#define R_RENDER_JOB_FRAME_ARENA_SIZE     Gigabytes(1)

typedef struct R_RenderJob R_RenderJob;
struct R_RenderJob
{
	LOG_Channel log_channel;

	R_System *system;
	R_Graph *graph;
	G_Swapchain *swapchain;

	R_FramePacket slots[R_FRAME_SLOT_COUNT];

	// 1 while a render job is in flight, 0 otherwise.
	OS_Handle counter;

	// only written while counter = 0
	const R_FramePacket *pending;

	Arena frame_arena;

	i64 atomic_gpu_completed_frame; // -1 if not finished yet
};

internal void R_RenderJobInit(R_RenderJob *rj,
							  R_System *system,
							  R_Graph *graph,
							  G_Swapchain *swapchain,
							  LOG_Channel log_channel);

internal void R_RenderJobDestroy(R_RenderJob *rj);

internal R_FramePacket *R_RenderJobBeginPacket(R_RenderJob *rj, u64 frame_number, f32 dt, f32 elapsed);
internal void R_RenderJobSubmit(R_RenderJob *rj, const R_FramePacket *packet);
internal void R_RenderJobWaitIdle(R_RenderJob *rj);
internal b32 R_RenderJobGpuCompletedFrame(R_RenderJob *rj, u64 *out_frame);

internal J_ENTRY_POINT_DEF(R_RenderJobEntry);

#endif // RENDER_JOB_H
