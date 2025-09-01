
typedef enum CameraType {
	CameraType_Perspective,
	CameraType_Orthographic
} CameraType;

typedef struct Camera {
	CameraType type;

	v3 position;
	v3 forward;
	v3 up;

	f32 fov;
	f32 aspect;
	f32 near_plane;
	f32 far_plane;

	m4 view;
	m4 projection;
} Camera;

typedef enum LightType {
	LightType_Point
} LightType;

typedef struct Light {
	LightType type;
	v3 direction;
	v3 colour;
	f32 intensity;
	f32 falloff;
} Light;

/*
typedef struct Bounds3D
{
	v3 position;
	v3 size;
}
Bounds3D;
*/

typedef enum SceneObjectFlags {
	SceneObjectFlag_None = 0 << 0,
	SceneObjectFlag_DrawDeferredPass = 1 << 0
} SceneObjectFlags;

typedef struct SceneObject {
	struct SceneObject *next;
	
	u32 id;
	m4 transform;
	
	u32 mesh_id;
	u32 material_id;
	//Bounds3D bounds;
	u32 flags;
	//u64 custom_sort_key;
	
	u32 light_id;
} SceneObject;

typedef struct EnvironmentProbe {
	Image irradiance, prefilter;
} EnvironmentProbe;

#define SCENE_INVALID_HANDLE ((u32)(-1))
#define SCENE_MAX_OBJECTS 64
#define SCENE_MAX_MATERIALS 32

typedef struct Scene {
	MemoryArena *arena;

	// Linked list of objects.
	u32 object_count;
	SceneObject *first_free_object;
	SceneObject *objects;

	// Object handles pending removal.
	u32 pending_removal_count;
	u32 pending_removal[SCENE_MAX_OBJECTS];

	// Object handles available for reuse.
	u32 reusable_handle_count;
	u32 reusable_handles[SCENE_MAX_OBJECTS];
} Scene;
