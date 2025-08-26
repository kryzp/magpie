
typedef struct GPU_FrameData
{
	m4 view;
	m4 projection;
	m4 view_projection;
	m4 view_projection_no_translation;
	
	m4 inv_view;
	m4 inv_projection;
	
	v4 camera_position;
	
	v4 window_resolution;
	
	f32 time;
	f32 _padding[3];
}
GPU_FrameData;

typedef struct GPU_ObjectData
{
	m4 model_matrix;
	m4 normal_matrix;
}
GPU_ObjectData;

typedef struct GPU_Light
{
	v4 position;    // x,y,z: position,    w: n/a
	v4 colour;      // x,y,z: colour,      w: intensity
	v4 attenuation; // x,y,z: attenuation, w: n/a
}
GPU_Light;

// ---

typedef enum GBufferAttachment
{
	GBufferAttachment_Position,
	GBufferAttachment_Albedo,
	GBufferAttachment_Normal,
	GBufferAttachment_Material,
	GBufferAttachment_Emissive,
	GBufferAttachment_MaxEnum
}
GBufferAttachment;

// NOTE(kp): Geometry Buffer for deferred rendering.
typedef struct GBuffer
{
	Image attachments[GBufferAttachment_MaxEnum];
	Image depth;
}
GBuffer;

typedef struct Renderer
{
	GBuffer gbuffer;
}
Renderer;
