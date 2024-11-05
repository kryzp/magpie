# Lilythorn

Basically, I'm making a renderer, but I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetrics :)

You'll notice it's very similar to my other Vulkan engine, because I basically ripped it out, removed all the garbage that was completely unnecessary and left only the Vulkan and SDL components, plus some maths and containers.

I recently bought Real Time Rendering, so it's time to make the investment worth it, I guess.

todo before i start on interesting stuff:
- integrate vulkan memory allocator
- optimise the vkmapmemory and vkunmapmemory in buffers to make it so they get called a lot less, very expensive operation!
- may automatically unbind all textures and push constants (even ubos and ssbos?) at the end of each frame, idk its fine for now

stuff im interested in:
- pbr
- volumetrics
- particles (compute shaders!!)
- fluid sims
- refraction and stuff
