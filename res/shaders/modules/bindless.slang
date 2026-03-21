module bindless;

namespace bdl
{
    [vk::binding(0, 0)] public SamplerState samplers[];
    [vk::binding(0, 1)] public Texture2D images[];
    [vk::binding(0, 1)] public TextureCube cubes[];
    [vk::binding(0, 2)] public RWTexture2D rw_images[];

    // TODO: Slang currently doesn't support RWTextureCube
    //       --> Would RWTexture2DArray suffice here?
    //[vk::binding(0, 4)] public RWTextureCube rw_cubes[];

    public bool IsValid(uint handle)
    {
        return handle != 0;
    }
}
