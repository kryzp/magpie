# Lilythorn

Basically, I'm making a renderer, but I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetrics :)

You'll notice it's very similar to my other Vulkan engine, because I basically ripped it out, removed all the garbage that was completely unnecessary and left only the Vulkan and SDL components, plus some maths and containers.

I recently bought Real Time Rendering, so it's time to make the investment worth it, I guess.

todo:
- ALLOW MSAA SAMPLING ON RENDER TARGETS!!!
- drawindirect support
- somehow seperate depth texture from render target to allow for creating a render target without a depth buffer? idk.
- model loading
- stencil buffer support! currently i just discard all the data
- imgui?

stuff im interested in:
- pbr
- volumetrics
- particles (compute shaders!!)
- fluid sims
- refraction and stuff

other todo that isnt really important:
- use multiple compute queues
- mac support "works" but things like the file io have to be changed since SDL stream functions arent availabvle on mac for some reason??? so figure that out
