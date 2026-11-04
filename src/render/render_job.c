
internal void R_RenderJobInit(R_RenderJob *rj,
							  R_System *system,
							  R_Graph *graph,
							  G_Swapchain *swapchain,
							  LOG_Channel log_channel)
{
	rj->log_channel = log_channel;
	rj->system = system;
	rj->graph = graph;
	rj->swapchain = swapchain;

	for (u32 i = 0; i < R_FRAME_SLOT_COUNT; i++)
		rj->slots[i].arena = ArenaAlloc(R_RENDER_JOB_PACKET_ARENA_SIZE);

	rj->frame_arena = ArenaAlloc(R_RENDER_JOB_FRAME_ARENA_SIZE);
	rj->counter = osapi->JobCounterAlloc(0);
	rj->pending = NULL;
	rj->atomic_gpu_completed_frame = -1;

	DebugLogI(rj->log_channel, "Initialized.");
}

internal void R_RenderJobDestroy(R_RenderJob *rj)
{
	R_RenderJobWaitIdle(rj);

	osapi->JobCounterRelease(rj->counter);

	ArenaRelease(&rj->frame_arena);

	for (u32 i = 0; i < R_FRAME_SLOT_COUNT; i++)
		ArenaRelease(&rj->slots[i].arena);
}

internal R_FramePacket *R_RenderJobBeginPacket(R_RenderJob *rj, u64 frame_number, f32 dt, f32 elapsed)
{
	R_FramePacket *packet = &rj->slots[frame_number % R_FRAME_SLOT_COUNT];

	// The only job that could still be reading a slot is the one in flight,
	// and that one always holds the *other* slot.
	DebugLogAssert(rj->log_channel,
				   packet != rj->pending || osapi->JobCounterValue(rj->counter) == 0,
				   "Packet slot %u is still being rendered.",
				   (u32)(frame_number % R_FRAME_SLOT_COUNT));

	R_FramePacketBegin(packet, frame_number, dt, elapsed);

	return packet;
}

internal void R_RenderJobSubmit(R_RenderJob *rj, const R_FramePacket *packet)
{
	// wait for the job to finish.
	osapi->JobYield(rj->counter, 0);

	rj->pending = packet;

	J_Decl decl = {0};
	decl.EntryPoint = R_RenderJobEntry;
	decl.param = rj;
	decl.priority = J_Priority_High;
	decl.flags = J_Flag_None;

	osapi->JobKick(&decl, rj->counter);
}

internal void R_RenderJobWaitIdle(R_RenderJob *rj)
{
	osapi->JobYield(rj->counter, 0);
}

internal b32 R_RenderJobGpuCompletedFrame(R_RenderJob *rj, u64 *out_frame)
{
	i64 completed = osapi->AtomicLoadI64(&rj->atomic_gpu_completed_frame);

	if (completed < 0)
		return false;

	*out_frame = (u64)completed;
	return true;
}

internal J_ENTRY_POINT_DEF(R_RenderJobEntry)
{
	R_RenderJob *rj = param;

	const R_FramePacket *packet = rj->pending;
	const u64 frame_number = packet->frame_number;

	G_FrameWait(frame_number);

	if (frame_number >= G_FRAMES_IN_FLIGHT)
		osapi->AtomicStoreI64(&rj->atomic_gpu_completed_frame, (i64)(frame_number - G_FRAMES_IN_FLIGHT));

	G_RingBuffer *upload = R_SystemBeginFrameUploads(rj->system, G_FrameSlotIndex());

	R_FrameParams frame_params = R_FrameParamsBuild(&rj->frame_arena, rj->system, upload, packet);

	if (frame_params.shaders_ready)
	{
		R_SystemRender(rj->graph, &frame_params);
		
		R_GraphCompile(rj->graph, rj->swapchain);

		G_CmdBuffer cmd = G_BeginFrame(rj->swapchain);
		R_GraphExecute(rj->graph, rj->swapchain, &cmd);
		R_GraphPresentToSwapchain(rj->graph, rj->swapchain, &cmd);
		G_EndFrame(rj->swapchain, &cmd);

		R_GraphReset(rj->graph);
	}
	else
	{
		DebugLogW(rj->log_channel, "Skipping frame %llu: Shaders not ready (yet).", frame_number);
	}

	ArenaReset(&rj->frame_arena);
}
