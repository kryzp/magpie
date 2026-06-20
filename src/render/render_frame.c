
internal R_FrameParams
R_FrameParamsInterp(const R_FrameParams *prev,
					const R_FrameParams *curr,
					f32 alpha)
{
	R_FrameParams result = {0};
	result.arena = curr->arena;
	result.frame_number = curr->frame_number;
	result.dt = curr->dt;
	result.elapsed = curr->elapsed;
	result.scene_data = curr->scene_data; // todo: interpolate scene data (TO BE HONEST THIS WHOLE THING SEEMS A BIT MESSY AND OVERDUE FOR A REFACTOR / REWORK)
	result.camera = curr->camera; // todo: interpolate camera position
	
	return result;
}
