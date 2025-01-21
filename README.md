# Lilythorn

It's a renderer, I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

TODO (no particular order):
- shader buffers + ubos should be set individually in their own function, then data just gets sent to the models
- implement a render graph system!
- make texture transitioning nicer
- VK_FORMAT_R32G32B32_SFLOAT isnt supported??? wtf???
- drawindirect support (proper)
- stencil buffer support!
- I think queues arent currently assigned properly(?)
- some kind of compute queue id system, so graphics pipelines can specifically decide to wait on select compute pipelines to finish.

BACKBURNER:
- imgui
- pbr
- volumetrics
- particles (compute shaders!!)
- fluid sims
- refraction and stuff

other todo that isnt really important:
- use multiple compute queues
- mac support "works" but things like the file io have to be changed since SDL stream functions arent availabvle on mac for some reason??? so figure that out
