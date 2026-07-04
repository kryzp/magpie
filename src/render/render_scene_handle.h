#ifndef RENDER_SCENE_HANDLE_H
#define RENDER_SCENE_HANDLE_H

typedef struct R_SceneHandle R_SceneHandle;
struct R_SceneHandle
{
	u32 index;
	u32 generation;
};

#endif // RENDER_SCENE_HANDLE_H
