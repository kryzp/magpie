#include "render/render_job.h"
#include "render/system.h"
#include "render/graph/graph.h"
#include "graphics/device.h"

rnd::RenderJob::RenderJob(
	System *system_,
	Graph *graph_,
	gfx::Swapchain *swapchain_,
	os::Window *window_,
	LogChannel log_channel_
)
	: log_channel(log_channel_)
	, system(system_)
	, graph(graph_)
	, swapchain(swapchain_)
	, window(window_)
	, counter(0)
	, pending(nullptr)
	, atomic_gpu_completed_frame(-1)
{
	DebugLogI(log_channel, "Initialized.");
}

rnd::RenderJob::~RenderJob()
{
	WaitIdle();
}

rnd::FramePacket *rnd::RenderJob::BeginPacket(u64 frame_number, float dt, float elapsed)
{
	FramePacket *packet = &slots[frame_number % RENDER_FRAME_SLOT_COUNT];

	DebugLogAssert(
		log_channel,
		packet != pending || counter.Value() == 0,
		"Packet slot %u is still being rendered.",
		(unsigned)(frame_number % RENDER_FRAME_SLOT_COUNT)
	);

	unsigned window_width = 0;
	unsigned window_height = 0;
	window->GetSize(&window_width, &window_height);

	packet->Begin(frame_number, dt, elapsed, window_width, window_height);

	return packet;
}

void rnd::RenderJob::Submit(const FramePacket *packet)
{
	// wait for the last frame to finish executing.
	counter.Yield(0);

	pending = packet;

	job::Decl decl;
	decl.entry_point = Entry;
	decl.param = this;
	decl.priority = job::Priority_High;
	decl.flags = job::Flag_None;

	job::Kick(decl, &counter);
}

void rnd::RenderJob::WaitIdle()
{
	counter.Yield(0);
}

bool rnd::RenderJob::GpuCompletedFrame(u64 *out_frame)
{
	i64 completed = atomic_gpu_completed_frame.load();

	if (completed < 0)
		return false;

	*out_frame = (u64)completed;
	return true;
}

void rnd::RenderJob::Entry(void *param)
{
	((RenderJob *)param)->Run();
}

void rnd::RenderJob::Run()
{
	gfx::Device *device = gfx::Device::Get();

	const FramePacket *packet = pending;
	const u64 frame_number = packet->frame_number;

	device->FrameWait(frame_number);

	if (frame_number >= GRAPHICS_FRAMES_IN_FLIGHT)
		atomic_gpu_completed_frame.store((i64)(frame_number - GRAPHICS_FRAMES_IN_FLIGHT));

	gfx::RingBuffer *upload = system->BeginFrameUploads(device->FrameSlotIndex());

	frame_params.Build(system, upload, packet);

	if (frame_params.shaders_ready) {
		system->Render(graph, &frame_params);

		graph->Compile(swapchain);

		gfx::CmdBuffer cmd = device->BeginFrame(swapchain);
		graph->Execute(cmd);
		graph->PresentToSwapchain(swapchain, cmd);
		device->EndFrame(swapchain, cmd);

		graph->Reset();
	} else {
		DebugLogW(log_channel, "Skipping frame %llu: Shaders not ready (yet).", frame_number);
	}
}
