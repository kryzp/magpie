module bindless;

namespace bdl
{
    [vk::binding(0, 0)] public SamplerState samplers[];
    [vk::binding(1, 0)] public Texture2D textures[];
    [vk::binding(2, 0)] public TextureCube cubes[];
    [vk::binding(3, 0)] public RWTexture2D rw_textures[];

    // TODO: Slang currently doesn't support RWTextureCube
    //       --> Would RWTexture2DArray suffice here?
    //[vk::binding(0, 4)] public RWTextureCube rw_cubes[];

    public bool IsValid(uint handle)
    {
        return handle != 0;
    }

	/*
	// TODO: Something like this?
	public SamplerState GetSampler(uint handle)
	{
		return samples[handle];
	}
	*/
}
