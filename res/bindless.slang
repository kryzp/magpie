
[vk::binding(0, 0)] SamplerState bindless_samplers[];
[vk::binding(0, 1)] Texture2D bindless_images[];
[vk::binding(0, 1)] TextureCube bindless_cubes[];
[vk::binding(0, 2)] RWTexture2D bindless_rw_images[];

// TODO: Slang currently doesn't support RWTextureCube
//       --> Would RWTexture2DArray suffice here?
//[vk::binding(0, 4)] RWTextureCube bindless_rw_cubes[];
