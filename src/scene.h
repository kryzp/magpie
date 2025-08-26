
typedef struct GPU_Material
{
	u32 diffuse_texture;
	u32 normal_texture;
	u32 emissive_texture;
	u32 metallic_roughness_texture;
	u32 ambient_texture;
}
GPU_Material;

typedef enum CameraType
{
	CameraType_Perspective,
	CameraType_Orthographic
}
CameraType;

typedef struct Camera
{
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
}
Camera;

typedef enum LightType
{
	LightType_Point
}
LightType;

typedef struct Light
{
	LightType type;
	v3 position;
	v3 direction;
	v3 colour;
	f32 intensity;
	f32 falloff;
}
Light;

/*
typedef struct Bounds3D
{
	v3 position;
	v3 size;
}
Bounds3D;
*/

typedef enum SceneObjectFlags
{
	SceneObjectFlag_None            = 0 << 0,
	SceneObjectFlag_DrawForwardPass = 1 << 0
}
SceneObjectFlags;

typedef struct SceneObject
{
	u32 id;
	u32 mesh_id;
	u32 material_id;
	m4 transform;
	//Bounds3D bounds;
	u32 flags;
	//u64 custom_sort_key;
}
SceneObject;

typedef struct EnvironmentProbe
{
	Image irradiance, prefilter;
}
EnvironmentProbe;

#define SCENE_MAX_OBJECTS 64
#define SCENE_MAX_GPU_MATERIALS 32

typedef struct Scene
{
	// NOTE(kp): Unsorted object data.
	u32 object_count;
	SceneObject objects[SCENE_MAX_OBJECTS];
	
	// NOTE(kp): Objects pending addition.
	u32 pending_addition_count;
	SceneObject pending_addition[SCENE_MAX_OBJECTS];
	
	// NOTE(kp): Object handles pending removal.
	u32 pending_removal_count;
	u32 pending_removal[SCENE_MAX_OBJECTS];
	
	// NOTE(kp): Object handles available for reuse.
	u32 reusable_handle_count;
	u32 reusable_handles[SCENE_MAX_OBJECTS];
}
Scene;
