#ifndef RENDER_RENDER_JOB_H
#define RENDER_RENDER_JOB_H

#include "core/types.h"
#include "core/log.h"
#include "os/job.h"
#include "os/platform.h"
#include "graphics/swapchain.h"
#include "render/frame.h"

#include <atomic>

namespace rnd {

class System;
class Graph;

class RenderJob {
public:
	RenderJob(
		System *system,
		Graph *graph,
		gfx::Swapchain *swapchain,
		os::Window *window,
		LogChannel log_channel
	);
	~RenderJob();

	RenderJob(const RenderJob &) = delete;
	RenderJob &operator=(const RenderJob &) = delete;

	FramePacket *BeginPacket(u64 frame_number, float dt, float elapsed);
	void Submit(const FramePacket *packet);
	void WaitIdle();
	bool GpuCompletedFrame(u64 *out_frame);

private:
	static void Entry(void *param);

	void Run();

	LogChannel log_channel;

	System *system;
	Graph *graph;
	gfx::Swapchain *swapchain;
	os::Window *window;

	FramePacket slots[RENDER_FRAME_SLOT_COUNT];

	job::Counter counter;

	// only written while counter = 0
	const FramePacket *pending;

	FrameParams frame_params;

	std::atomic<i64> atomic_gpu_completed_frame; // -1 if not finished yet
};
}

#endif // RENDER_RENDER_JOB_H
