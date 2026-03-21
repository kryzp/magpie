module bindless;

[vk::binding(0, 0)] export SamplerState samplers[];
[vk::binding(0, 1)] export Texture2D images[];
[vk::binding(0, 1)] export TextureCube cubes[];
[vk::binding(0, 2)] export RWTexture2D rw_images[];

// TODO: Slang currently doesn't support RWTextureCube
//       --> Would RWTexture2DArray suffice here?
//[vk::binding(0, 4)] export RWTextureCube rw_cubes[];

export bool IsValid(uint handle)
{
	return handle != 0;
}
