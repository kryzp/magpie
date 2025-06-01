#ifndef BINDLESS_SLANG_
#define BINDLESS_SLANG_

[[vk::binding(0)]] SamplerState g_bindlessSamplers[];
[[vk::binding(1)]] Texture2D g_bindlessTexture2D[];
[[vk::binding(2)]] TextureCube g_bindlessTextureCube[];

#endif // BINDLESS_SLANG_
