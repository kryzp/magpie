
static G_Profiler *g_selected_profiler = NULL;

internal void G_ProfilerInitAndSelect(G_Profiler *profiler)
{
	profiler->period = G_GetSelected()->vk_physical_device_properties.properties.limits.timestampPeriod;
	
	static VkQueryType query_types[] = {
		[G_ProfileType_Timestamp] = VK_QUERY_TYPE_TIMESTAMP,
		[G_ProfileType_PipelineStatistics] = VK_QUERY_TYPE_PIPELINE_STATISTICS
	};

	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		for (u32 j = 0; j < G_ProfileType_COUNT; j++)
		{
			VkQueryPipelineStatisticFlags pipeline_stat_flags = 0;

			if (query_types[j] == VK_QUERY_TYPE_PIPELINE_STATISTICS)
				pipeline_stat_flags = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;

			g_selected_profiler->frames[i].pools[j].vk_pool = G_QueryPoolCreate(G_PROFILER_MAX_QUERIES_PER_FRAME,
																					  query_types[j],
																					  pipeline_stat_flags);
		}
	}
	
	G_ProfilerSelectContext(profiler);
}

internal void G_ProfilerDestroy(void)
{
	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		for (u32 j = 0; j < G_ProfileType_COUNT; j++)
		{
			G_QueryPoolDestroy(g_selected_profiler->frames[i].pools[j].vk_pool);
		}
	}
	
	g_selected_profiler = NULL;
}

internal void G_ProfilerSelectContext(G_Profiler *profiler)
{
	g_selected_profiler = profiler;
}

internal void G_ProfilerGrabQueries(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	G_ProfilerFrame *current_frame = &g_selected_profiler->frames[G_GetFrameInFlightIndex()];

	for (u32 i = 0; i < G_ProfileType_COUNT; i++)
	{
		G_ProfilePool *pool = &current_frame->pools[i];
		G_ProfileQueryList *query_list = &g_selected_profiler->query_lists[i];

		if (pool->count <= 0)
			continue;

		u64 *queries = ArenaPushArray(scratch.arena, u64, pool->count);

		/*
		vkGetQueryPoolResults(device->get_context().get_device(),
							  pool.vk_pool,
							  0,
							  pool.count,
							  pool.count * sizeof(u64),
							  queries,
							  sizeof(u64),
							  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
		*/
		
		for (u32 j = 0; j < pool->count; j++)
		{
			G_ProfileEvent ev = pool->events[j];
			
			u64 stat = 0;

			switch (ev.type)
			{
				case G_ProfileType_Timestamp:
					stat = queries[ev.ts.end] - queries[ev.ts.start];
					break;

				case G_ProfileType_PipelineStatistics:
					stat = queries[ev.query];
					break;
			}
			
			AssertTrue(query_list->count < ArraySize(query_list->queries));
			
			query_list->queries[query_list->count].name_hash = HashStr8(ev.name);
			query_list->queries[query_list->count].stat = stat;
			
			query_list->count++;
		}

		pool->count = 0;
	}
	
	ScratchRelease(&scratch);
}

internal void G_ProfilerAddEvent(const G_ProfileEvent *event)
{
	G_ProfilerFrame *current_frame = &g_selected_profiler->frames[G_GetFrameInFlightIndex()];
	G_ProfilePool *current_pool = &current_frame->pools[event->type];
	
	AssertTrue(current_pool->count < ArraySize(current_pool->events));
	
	current_pool->events[current_pool->count++] = *event;
}

internal VkQueryPool G_ProfilerGetVkPool(G_ProfileType type)
{
	G_ProfilerFrame *current_frame = &g_selected_profiler->frames[G_GetFrameInFlightIndex()];
	return current_frame->pools[type].vk_pool;
}

internal u64 G_ProfilerGetNewID(G_ProfileType type)
{
	G_ProfilerFrame *current_frame = &g_selected_profiler->frames[G_GetFrameInFlightIndex()];
	return current_frame->pools[type].count++;
}

internal u64 G_ProfilerGetRawStat(G_ProfileType type, String8 name)
{
	G_ProfileQueryList *list = &g_selected_profiler->query_lists[type];

	u64 name_hash = HashStr8(name);

	for (u32 i = 0; i < list->count; i++)
	{
		G_ProfileQuery *query = &list->queries[i];
		
		if (query->name_hash == name_hash)
			return query->stat;
	}

	return 0;
}

internal f64 G_ProfilerGetTimer(String8 name)
{
	f64 dur = (f64)G_ProfilerGetRawStat(G_ProfileType_Timestamp, name);
	return dur * (f64)g_selected_profiler->period / 1000000.0;
}

internal i32 G_ProfilerGetPipelineStatistics(String8 name)
{
	i32 pipeline_stat = G_ProfilerGetRawStat(G_ProfileType_PipelineStatistics, name);
	return pipeline_stat;
}

internal G_ProfileScope G_ProfileBegin(const G_CmdBuffer *cmd, String8 name)
{
	G_ProfileScope scope = {0};
	scope.cmd = cmd;
	scope.name = name;
	scope.id = G_ProfilerGetNewID(G_ProfileType_Timestamp);

	G_CmdWriteTimestamp(cmd,
						VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
						G_ProfilerGetVkPool(G_ProfileType_Timestamp),
						scope.id);

	return scope;
}

internal void G_ProfileEnd(const G_ProfileScope *scope)
{
	u64 end_id = G_ProfilerGetNewID(G_ProfileType_Timestamp);
	
	G_CmdWriteTimestamp(scope->cmd,
						VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
						G_ProfilerGetVkPool(G_ProfileType_Timestamp),
						end_id);
	
	G_ProfileEvent event = {0};
	event.name = scope->name;
	event.type = G_ProfileType_Timestamp;
	event.ts.start = scope->id;
	event.ts.end = end_id;

	G_ProfilerAddEvent(&event);
}

internal G_ProfileScope G_ProfileStatsBegin(const G_CmdBuffer *cmd, String8 name)
{
	G_ProfileScope scope = {0};
	scope.cmd = cmd;
	scope.name = name;
	scope.id = G_ProfilerGetNewID(G_ProfileType_PipelineStatistics);

	G_CmdBeginQuery(cmd,
					G_ProfilerGetVkPool(G_ProfileType_PipelineStatistics),
					scope.id, 0);

	return scope;
}

internal void G_ProfileStatsEnd(const G_ProfileScope *scope)
{
	G_CmdEndQuery(scope->cmd,
				  G_ProfilerGetVkPool(G_ProfileType_PipelineStatistics),
				  scope->id);

	G_ProfileEvent event = {0};
	event.name = scope->name;
	event.type = G_ProfileType_PipelineStatistics;
	event.query = scope->id;

	G_ProfilerAddEvent(&event);
}
